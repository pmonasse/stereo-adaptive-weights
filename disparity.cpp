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
/// \param im1R,im1G,im1B the color channels of image 1
/// \param im2R,im2G,im2B the color channels of image 2
/// \param gradient1,gradient2 the gradient images
/// \param d the disparity (layer of the cost volume)
/// \param param parameters for cost computation
static Image costLayer(Image im1R, Image im1G, Image im1B,
                       Image im2R, Image im2G, Image im2B,
                       Image gradient1, Image gradient2,
                       int d, const ParamDisparity& param) {
    const int width=im1R.width(), height=im1R.height();
    Image cost(width,height);
    for(int y=0; y<height; y++)
        for(int x=0; x<width; x++) {
            // Max distance if disparity moves outside image
            float costColor = param.color_threshold;
            float costGradient = param.gradient_threshold;

            // Color L1 distance
            if(0<=x+d && x+d<width) {
                // L1 color distance.
                float col1[3] = {im1R(x,y), im1G(x,y), im1B(x,y)};
                float col2[3] = {im2R(x+d,y), im2G(x+d,y), im2B(x+d,y)};
                costColor = 0;
                for(int i=0; i<3; i++)
                    costColor += abs(col1[i]-col2[i]);
                costColor *= 1.0f/3;
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
    return cost;
}

/// Fill support weights.
///
/// \param im1R,im1G,im1B The image channels
/// \param xp,yp Center point
/// \param r Window radius
/// \param distC Tabulated color distances
/// \param distP Tabulated position distances
/// \param w The output support window
static void support(const Image& im1R, const Image& im1G, const Image& im1B,
                    int xp, int yp, int r,
                    float* distC, float* distP,
                    Image& w) {
    const int width=im1R.width(), height=im1R.height();
    for(int y=-r; y<=r; y++)
        if(0<=yp+y && yp+y<height)
            for(int x=-r; x<=r; x++)
                if(0<=xp+x && xp+x<width) {
                    int d=0;
                    d += std::abs(im1R(xp+x,yp+y)-im1R(xp,yp));
                    d += std::abs(im1G(xp+x,yp+y)-im1G(xp,yp));
                    d += std::abs(im1B(xp+x,yp+y)-im1B(xp,yp));
                    w(x+r,y+r)=distC[d]*distP[(y+r)*w.width()+(x+r)];
                }
}

/// Adaptive Weights disparity computation
///
/// The dissimilarity is computed putting adaptative
/// weights on the raw matching cost
/// \param im1Color,im2Color the two color images
/// \param dispMin,dispMax disparity range
/// \param param cost parameters
/// \param disparity1 output disparity map from image 1 to image 2
/// \param disparity2 output disparity map from image 2 to image 1
void disparityAW(Image im1Color, Image im2Color,
                 int dispMin, int dispMax,
                 const ParamDisparity& param,
                 Image& disparity1, Image& disparity2) {
    // Getting the three color channels (between 0 and 255)
    Image im1R=im1Color.r(), im1G=im1Color.g(), im1B=im1Color.b();
    Image im2R=im2Color.r(), im2G=im2Color.g(), im2B=im2Color.b();

    const int width=im1R.width(), height=im1R.height();  // Images dimensions
    const int r = param.window_radius;                   // window's radius
    const int nd = dispMax-dispMin+1;                    // Disparity range
    std::cout << "Range of disparities: " << nd << " disparities, "<<std::endl;

    // Tabulated proximity weights (color distance)
    float* distC = new float[3*255+1];
    float e2=exp(-1/(3*param.gamma_s));
    for(int x=0; x<=3*255; x++)
        distC[x] = pow(e2,x);

    // Tabulated proximity weights (spatial distance)
    const int dim=2*r+1; // window dimension
    float *distP = new float[dim*dim], *d=distP;
    float e1=exp(-1/param.gamma_p);
    for(int y=-r; y<=r; y++)
    for(int x=-r; x<=r; x++)
        *d++ = pow(e1,sqrt((float)(x*x+y*y)));

    // Compute x-derivatives of both images
    Image im1Gray(width,height);
    Image im2Gray(width,height);
    rgb_to_gray(&im1R(0,0),&im1G(0,0),&im1B(0,0), width,height, &im1Gray(0,0));
    rgb_to_gray(&im2R(0,0),&im2G(0,0),&im2B(0,0), width,height, &im2Gray(0,0));
    Image gradient1 = im1Gray.gradX();
    Image gradient2 = im2Gray.gradX();

    // Compute raw matching cost for all disparities.
    Image* cost = new Image[nd];
    for(int d=dispMin; d<=dispMax; d++) {
        cost[d-dispMin] = 
            costLayer(im1R,im1G,im1B, im2R,im2G,im2B, gradient1, gradient2,
                      d, param);
    }

    // Images of dissimilarity 1->2 and 2->1
    Image E1(width,height), E2(width,height);
    std::fill_n(&E1(0,0), width*height, std::numeric_limits<float>::max());
    std::fill_n(&E2(0,0), width*height, std::numeric_limits<float>::max());

#pragma omp parallel for
    for(int yp=0; yp<height; yp++) {
        // Image window for the weights in the reference image
        Image weights1(dim,dim);
        // Vector of weights windows on the target image
        Image* weights = new Image[nd];
        for(int d=dispMin; d<=dispMax; d++)
            weights[d-dispMin] = Image(dim,dim);

#ifndef COMB_LEFT
        // Target window weights for all disparities except dispMax
        for(int d=dispMin; d<dispMax; d++)
            support(im2R, im2G, im2B, 0+d,yp, r, distC, distP,
                    weights[(0+d-dispMin)%nd]);
#endif
        for(int xp=0; xp<width; xp++) {
            // Reference window weights
            support(im1R, im1G, im1B, xp,yp, r, distC, distP, weights1);
#ifndef COMB_LEFT
            // Target window weights at disparity dispMax
            support(im2R, im2G, im2B, xp+dispMax,yp, r, distC, distP,
                    weights[(xp+dispMax-dispMin)%nd]);
#endif
            // Compute dissimilarity for all possible disparities
            for(int d=dispMin; d<=dispMax; d++) {
                // raw matching cost for disparity d
                const Image& dCost = cost[d-dispMin];
#ifndef COMB_LEFT
                // Weights image of target window
                const Image& weights2 = weights[(xp+d-dispMin)%nd];
#endif
                if(0<=xp+d && xp+d<width) {
                    float num=0, den=0;
                    for(int y=-r; y<=r; y++)
                    if(0<=yp+y && yp+y<height)
                    for(int x=-r; x<=r; x++)
                    if(0<=xp+x && xp+x<width && 0<=xp+x+d && xp+x+d<width) {
                        // Weight p in the left image
                        float w1=weights1(x+r,y+r);
                        // Weight q in the right image
#ifdef COMB_LEFT
                        float w2=1;
#else
                        float w2=weights2(x+r,y+r);
#endif
                        // Combination of weights and raw cost
                        num+=COMB_WEIGHTS(w1,w2)*dCost(xp+x,yp+y);
                        // normalization term
                        den+=COMB_WEIGHTS(w1,w2);
                    }
                    // Dissimilarity for this disparity
                    float E=num/den;

                    // Winner takes all label selection
                    if(E1(xp,yp) > E) {
                        E1(xp,yp) = E;
                        disparity1(xp,yp) = d;
                    }
                    if(E2(xp+d,yp) > E) {
                        E2(xp+d,yp) = E;
                        disparity2(xp+d,yp)= -d;
                    }
                }
            }
        }
        delete [] weights;
    }
    delete [] cost;
    delete [] distC;
    delete [] distP;
}
