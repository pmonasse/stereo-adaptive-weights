/**
 * @file disparity.cpp
 * @brief Disparity Map computation
 * @author Laura F. Julià <fernandl@imagine.enpc.fr>
 *         Pascal Monasse <monasse@imagine.enpc.fr>
 */

#include "disparity.h"
#include "image.h"
#include "io_png.h"
#include <algorithm>
#include <limits>
#include <iostream>
#include <vector>
#include <cmath>

/// Combination of weights, defined at compile time. Usage of a function pointer
/// is significantly slower, unfortunately.
#if   defined(COMB_LEFT)
    static float COMB_WEIGHTS(float w1, float)    { return w1; }
#elif defined(COMB_MAX)
    static float COMB_WEIGHTS(float w1, float w2) { return std::max(w1,w2); }
#elif defined(COMB_MIN)
    static float COMB_WEIGHTS(float w1, float w2) { return std::min(w1,w2); }
#elif defined(COMB_MULT)
    static float COMB_WEIGHTS(float w1, float w2) { return w1*w2; }
#elif defined(COMB_PLUS)
    static float COMB_WEIGHTS(float w1, float w2) { return w1+w2; }
#else
#error "Unknown combination of weights"
#endif

/// Computes image of raw matching costs e at disparity d.
///
/// At each pixel, a linear combination of colors L1 distance (with max
/// threshold) and x-derivatives absolute difference (with max threshold).
static void e_cost(Image im1R, Image im1G, Image im1B,      // color channels reference image
                   Image im2R, Image im2G, Image im2B,  // color channels target image
                   Image gradient1, Image gradient2,    // gradients of the two images
                   int d,                               // disparity value
                   const ParamDisparity& param,         // parameters
                   Image& cost)                          // output cost image
{
    const int width=im1R.width(), height=im1R.height();
    for(int y=0; y<height; y++)
        for(int x=0; x<width; x++) {

            // If there is no corresponding disparity point, we assign maximal color difference.
            float costColor = param.color_threshold;
            float costGradient = param.gradient_threshold;

            // Color L1 distance
            if(0<=x+d && x+d<width) {
                // L1 color distance.
                float col1[3] = {im1R(x,y), im1G(x,y), im1B(x,y)};
                float col2[3] = {im2R(x+d,y), im2G(x+d,y), im2B(x+d,y)};
                costColor = 0;
                for(int i=0; i<3; i++) {
                    costColor += abs(col1[i]-col2[i]);
                }
                costColor *= 1.0/3;
                // Color threshold
                if(costColor > param.color_threshold)
                    costColor = param.color_threshold;

                // x-derivatives absolute difference and threshold
                costGradient = gradient1(x,y)-gradient2(x+d,y);
                if(costGradient < 0)
                    costGradient = -costGradient;
                if(costGradient > param.gradient_threshold)
                    costGradient = param.gradient_threshold;
            }
            // Linear combination of the two penalties
            cost(x,y) = (1-param.alpha)*costColor + param.alpha*costGradient;
        }
}

/// Adaptive Weights disparity computation
///
/// The dissimilarity is computed putting adaptative
/// weights on the raw matching cost
void disparityAW(Image im1Color, Image im2Color,              // the two color images
                 int dispMin, int dispMax,            // disparity bounds
                 const ParamDisparity& param,         // Parameters
                 Image& disparity, Image& disparity2)  // Output disparity images
{
    // Getting the three color channels (between 0 and 255)
    Image im1R=im1Color.r(), im1G=im1Color.g(), im1B=im1Color.b();
    Image im2R=im2Color.r(), im2G=im2Color.g(), im2B=im2Color.b();

    const int width=im1R.width(), height=im1R.height();  // Images dimensions
    const int r = param.window_radius;                   // window's radius
    const int nd = dispMax-dispMin+1;                    // Disparity range
    std::cout << "Range of disparities: " << nd << " disparities, "<<std::endl;

    // Sample of color similarity strengths depending on the color distance
    std::vector<float> distS;
    float e2=exp(-1/(3*param.gamma_s));
    for(int x=0; x<=3*255; x++)
        distS.push_back(pow(e2,x));

    // Sample of proximity strengths depending on x and y positions in the square neighborhood
    std::vector<float> distP;
    float e1=exp(-1/param.gamma_p);
    for(int y=-r; y<=r; y++)
    for(int x=-r; x<=r; x++)
        distP.push_back(pow(e1,sqrt((float)(x*x+y*y))));

    // Compute x-derivatives of both images
    Image im1Gray(width,height);
    Image im2Gray(width,height);
    rgb_to_gray(&im1R(0,0),&im1G(0,0),&im1B(0,0), width,height, &im1Gray(0,0));
    rgb_to_gray(&im2R(0,0),&im2G(0,0),&im2B(0,0), width,height, &im2Gray(0,0));
    Image gradient1 = im1Gray.gradX();
    Image gradient2 = im2Gray.gradX();

    // Compute raw matching cost for all possible disparities.
    Image** cost = new Image*[nd];
    for(int d=dispMin; d<=dispMax; d++) {
        cost[d-dispMin] = new Image(width,height);
        e_cost(im1R,im1G,im1B, im2R,im2G,im2B, gradient1, gradient2,
               d, param, *cost[d-dispMin]);
    }

    // Dissimilarity computation

    // Images for the two parts of the dissimilarity
    Image E1(width,height), E2(width,height);
    std::fill_n(&E1(0,0), width*height, std::numeric_limits<float>::max());
    std::fill_n(&E2(0,0), width*height, std::numeric_limits<float>::max());

    const int dim=2*r+1; // window dimension

#pragma omp parallel for
    for(int yp=0; yp<height; yp++)
    for(int xp=0; xp<width; xp++)
        for(int d=dispMin; d<=dispMax; d++) {
            // raw matching cost for disparity d
            const Image& dCost = *cost[d-dispMin];
            if(xp+d>=0 && xp+d<width) {
                float nom=0, den=0;
                for(int y=-r; y<=r; y++)
                if(yp+y>=0 && yp+y<height)
                for(int x=-r; x<=r; x++)
                if(xp+x>=0 && xp+x<width && xp+x+d>=0 && xp+x+d<width) {
                    // Weight p in the left image
                    int dist_s1=0;
                    dist_s1 += abs(im1R(xp+x,yp+y)-im1R(xp,yp));
                    dist_s1 += abs(im1G(xp+x,yp+y)-im1G(xp,yp));
                    dist_s1 += abs(im1B(xp+x,yp+y)-im1B(xp,yp));
                    float w1=distS[dist_s1]*distP[(y+r)*dim+(x+r)];
                    // Weight q in the right image
#ifdef COMB_LEFT
                    float w2=1;
#else
                    int dist_s2=0;
                    dist_s2 += abs(im2R(xp+x+d,yp+y)-im2R(xp+d,yp));
                    dist_s2 += abs(im2G(xp+x+d,yp+y)-im2G(xp+d,yp));
                    dist_s2 += abs(im2B(xp+x+d,yp+y)-im2B(xp+d,yp));
                    float w2=distS[dist_s2]*distP[(y+r)*dim+(x+r)];
#endif

                    // Combination of weights and raw cost
                    nom+=COMB_WEIGHTS(w1,w2)*dCost(xp+x,yp+y);
                    // normalization term
                    den+=COMB_WEIGHTS(w1,w2);
                }
                // Dissimilarity for this disparity
                float E=nom/den;

                // Winner takes all label selection
                if(E1(xp,yp) > E) {
                    E1(xp,yp) = E;
                    disparity(xp,yp) = d;
                }
                if(E2(xp+d,yp) > E) {
                    E2(xp+d,yp) = E;
                    disparity2(xp+d,yp)= -d;
                }
            }
        }
    // Delete cost images
    for(int d=dispMin; d<=dispMax; d++)
        delete cost[d-dispMin];
    delete [] cost;
}

