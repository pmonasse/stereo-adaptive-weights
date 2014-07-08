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
#include <cassert>

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

/// Convert image \a in to gray scale.
Image gray(const Image& in) {
    if(in.channels() == 1)
        return in.clone();
    assert(in.channels() == 3);
    const int w=in.width(), h=in.height();
    Image out(w,h);
    for(int y=0; y<h; y++)
        for(int x=0; x<w; x++)
            out(x,y) = rgb_to_gray(in(x,y,0), in(x,y,1), in(x,y,2));
    return out;
}

/// Computes image of raw matching costs e at disparity d.
///
/// At each pixel, a linear combination of colors L1 distance (with max
/// threshold) and x-derivatives absolute difference (with max threshold).
/// \param im1 first image
/// \param im2 second image
/// \param gradient1,gradient2 the gradient images
/// \param d the disparity (layer of the cost volume)
/// \param param parameters for cost computation
static Image costLayer(Image im1, Image im2,
                       Image gradient1, Image gradient2,
                       int d, const ParamDisparity& param) {
    const int width=im1.width(), height=im1.height();
    Image cost(width,height);
    for(int y=0; y<height; y++)
        for(int x=0; x<width; x++) {
            // Max distance if disparity moves outside image
            float costColor = param.color_threshold;
            float costGradient = param.gradient_threshold;

            // Color L1 distance
            if(0<=x+d && x+d<width) {
                // L1 color distance.
                costColor = 0;
                for(int i=0; i<3; i++)
                    costColor += abs(im1(x,y,i)-im2(x+d,y,i));
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

/// Compute the cost volume.
static Image* costVolume(const Image& im1, const Image& im2,
                         int dMin, int dMax, const ParamDisparity& param) {
    // Compute x-derivatives of both images
    Image grad1 = gray(im1).gradX();
    Image grad2 = gray(im2).gradX();

    // Compute raw matching cost for all disparities.
    Image* cost = new Image[dMax-dMin+1];
    for(int d=dMin; d<=dMax; d++)
        cost[d-dMin] = costLayer(im1, im2, grad1, grad2, d, param);
    return cost;
}

/// Fill support weights.
///
/// \param im The image
/// \param xp,yp Center point
/// \param r Window radius
/// \param distC Tabulated color distances
/// \param distP Tabulated position distances
/// \param w The output support window
static void support(const Image& im,
                    int xp, int yp, int r,
                    float* distC, float* distP,
                    Image& w) {
    const int width=im.width(), height=im.height(), c=im.channels();
    for(int y=-r; y<=r; y++)
        if(0<=yp+y && yp+y<height)
            for(int x=-r; x<=r; x++)
                if(0<=xp+x && xp+x<width) {
                    int d=0;
                    for(int i=0; i<c; i++)
                        d += std::abs(im(xp+x,yp+y,i)-im(xp,yp,i));
                    w(x+r,y+r)=distC[d]*distP[(y+r)*w.width()+(x+r)];
                }
}

/// Combined cost of matching points (xp,yp) to (xq,yp).
///
/// The support weights of p and q are \a wp and \a wq. The elementary pixel
/// cost is in image \a cost.
float costCombined(int xp, int xq, int yp, int r, const Image& wp, const Image&
#ifndef COMB_LEFT // Unname to avoid warning since unused, if using 'left'
                   wq
#endif
                   , const Image& cost) {
    const int width = cost.width();
    float num=0, den=0;
    for(int y=-r; y<=r; y++)
        if(0<=yp+y && yp+y<cost.height())
            for(int x=-r; x<=r; x++)
                if(0<=xp+x && xp+x<width && 0<=xq+x && xq+x<width) {
                    float w1=wp(x+r,y+r); // Weight p
#ifdef COMB_LEFT
                    float w2=1;
#else
                    float w2=wq(x+r,y+r); // Weight q
#endif
                    // Combination of weights and raw cost
                    num+=COMB_WEIGHTS(w1,w2)*cost(xp+x,yp+y);
                    // normalization term
                    den+=COMB_WEIGHTS(w1,w2);
                }
    return num/den;
}

/// Adaptive Weights disparity computation.
///
/// The dissimilarity is computed putting adaptive weights on the raw cost.
/// \param im1,im2 the two color images
/// \param dispMin,dispMax disparity range
/// \param param raw cost computation parameters
/// \param disparity1 output disparity map from image 1 to image 2
/// \param disparity2 output disparity map from image 2 to image 1
void disparityAW(Image im1, Image im2,
                 int dispMin, int dispMax,
                 const ParamDisparity& param,
                 Image& disparity1, Image& disparity2) {
    const int width=im1.width(), height=im1.height();  // Images dimensions
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

    Image* cost = costVolume(im1, im2, dispMin, dispMax, param);

    // Images of dissimilarity 1->2 and 2->1
    Image E1(width,height), E2(width,height);
    std::fill_n(&E1(0,0), width*height, std::numeric_limits<float>::max());
    std::fill_n(&E2(0,0), width*height, std::numeric_limits<float>::max());

#ifdef _OPENMP
#pragma omp parallel for
#endif
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
            support(im2, 0+d,yp, r, distC, distP, weights[(0+d-dispMin)%nd]);
#endif
        for(int xp=0; xp<width; xp++) {
            // Reference window weights
            support(im1, xp,yp, r, distC, distP, weights1);
#ifndef COMB_LEFT
            // Target window weights at disparity dispMax
            support(im2, xp+dispMax,yp, r, distC, distP,
                    weights[(xp+dispMax-dispMin)%nd]);
#endif
            // Compute dissimilarity for all possible disparities
            for(int d=dispMin; d<=dispMax; d++) {
                if(0<=xp+d && xp+d<width) {
                    const Image& c = cost[d-dispMin]; // raw cost for disp. d
#ifdef COMB_LEFT // Weights of target window

                    const Image& weights2 = weights1; // Unused
#else
                    const Image& weights2 = weights[(xp+d-dispMin)%nd];
#endif
                    float E = costCombined(xp, xp+d, yp, r,
                                           weights1, weights2, c);
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
