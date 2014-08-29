/**
 * @file disparity.cpp
 * @brief Disparity map estimation using bilaterally weighted patches
 * @author Laura F. Julia <fernandl@imagine.enpc.fr>
 *         Pascal Monasse <monasse@imagine.enpc.fr>
 *
 * Copyright (c) 2014, Laura F. Julia, Pascal Monasse
 * All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * You should have received a copy of the GNU General Pulic License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "disparity.h"
#include "image.h"
#include <algorithm>
#include <limits>
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
    assert(im1.channels() == im2.channels());
    const int width=im1.width(), height=im1.height(), channels=im1.channels();
    Image cost(width,height);
    for(int y=0; y<height; y++)
        for(int x=0; x<width; x++) {
            // Max distance if disparity moves outside image
            float costColor = param.tauCol;
            float costGradient = param.tauGrad;

            // Color L1 distance
            if(0<=x+d && x+d<width) {
                // L1 color distance.
                costColor = 0;
                for(int i=0; i<channels; i++)
                    costColor += std::abs(im1(x,y,i)-im2(x+d,y,i));
                costColor /= (float)channels;
                // Color threshold
                if(costColor > param.tauCol)
                    costColor = param.tauCol;

                // x-derivatives absolute difference and threshold
                costGradient=std::min(std::abs(gradient1(x,y)-gradient2(x+d,y)),
                                      param.tauGrad);
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
    Image grad1 = im1.gray().gradX();
    Image grad2 = im2.gray().gradX();

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
/// \param w The output support window
static void support(const Image& im, int xp, int yp, int r,
                    float* distC, Image& w) {
    const int width=im.width(), height=im.height(), c=im.channels();
    for(int y=-r; y<=r; y++)
        if(0<=yp+y && yp+y<height)
            for(int x=-r; x<=r; x++)
                if(0<=xp+x && xp+x<width) {
                    float d=0;
                    for(int i=0; i<c; i++)
                        d += std::abs(im(xp+x,yp+y,i)-im(xp,yp,i));
                    w(x+r,y+r)=distC[static_cast<int>(d)];
                }
}

/// Combined cost of matching points (xp,yp) to (xq,yp).
///
/// The support weights of p and q are \a wp and \a wq. The elementary pixel
/// cost is in image \a e.
float costCombined(int xp, int xq, int yp, int r,
                   const Image& wp, const Image& wq,
                   const float* distP, const Image& e) {
    const int width = e.width();
    float num=0, den=0; // Numerator and denominator in the fraction
    for(int y=-r; y<=r; y++)
        if(0<=yp+y && yp+y<e.height())
            for(int x=-r; x<=r; x++)
                if(0<=xp+x && xp+x<width && 0<=xq+x && xq+x<width) {
                    float w1=wp(x+r,y+r); // Weight p
                    float w2=wq(x+r,y+r); // Weight q
                    float comb = distP[(y+r)*(2*r+1)+(x+r)]*COMB_WEIGHTS(w1,w2);
                    num+=comb*e(xp+x,yp+y);
                    den+=comb;
                }
    return num/den;
}

/// Adaptive Weights disparity computation.
///
/// The dissimilarity is computed putting adaptive weights on the raw cost.
/// \param im1,im2 the two color images
/// \param dMin,dMax disparity range
/// \param param raw cost computation parameters
/// \param disp1 output disparity map from image 1 to image 2
/// \param disp2 output disparity map from image 2 to image 1
void disparityAW(Image im1, Image im2,
                 int dMin, int dMax, const ParamDisparity& param,
                 Image& disp1, Image& disp2) {
    const int width=im1.width(), height=im1.height();
    const int r = param.radius;
#ifdef COMB_LEFT // Disparity range
    const int nd = 1; // Do not compute useless weights in target image
#else
    const int nd = dMax-dMin+1;
#endif

    // Tabulated proximity weights (color distance)
    const int maxL1 = im1.channels()*255; // Maximum L1 distance between colors
    float* distC = new float[maxL1+1];
    float e2=exp(-1/(im1.channels()*param.gammaCol));
    distC[0]=1.0f;
    for(int x=1; x<=maxL1; x++)
        distC[x] = e2*distC[x-1]; // distC[x] = exp(-x/(c*gamma))

    // Tabulated proximity weights (spatial distance)
    const int dim=2*r+1; // window dimension
    float *distP = new float[dim*dim], *d=distP;
    for(int y=-r; y<=r; y++)
    for(int x=-r; x<=r; x++)
        *d++ = exp(-2.0f*sqrt((float)(x*x+y*y))/param.gammaPos);

    Image* cost = costVolume(im1, im2, dMin, dMax, param);

    // Images of dissimilarity 1->2 and 2->1
    Image E1(width,height), E2(width,height);
    std::fill_n(&E1(0,0), width*height, std::numeric_limits<float>::max());
    std::fill_n(&E2(0,0), width*height, std::numeric_limits<float>::max());

#ifdef _OPENMP
#pragma omp parallel for
#endif
    for(int y=0; y<height; y++) {
        // Weight window in reference image
        Image W1(dim,dim);
        // Weight windows in target image for each disparity (useless for
        // COMB_LEFT, but better to have readable code than multiplying #ifdef)
        Image* weights2 = new Image[nd];
        for(int d=0; d<nd; d++) {
            weights2[d] = Image(dim,dim);
            if(d+1<nd) // Support for dMax computed later
                support(im2, d,y, r, distC, weights2[d]);
        }

        for(int x=0; x<width; x++) {
            // Reference window weights
            support(im1, x,y, r, distC, W1);
#ifndef COMB_LEFT // Weight window at disparity dMax in target image
            support(im2, x+dMax,y, r, distC, weights2[(x+dMax-dMin)%nd]);
#endif
            for(int d=dMin; d<=dMax; d++) {
                if(0<=x+d && x+d<width) {
                    const Image& e = cost[d-dMin]; // Raw cost for disp. d
                    const Image& W2 = weights2[(x+d-dMin)%nd];
                    float E = costCombined(x, x+d, y, r, W1, W2, distP, e);
                    if(E1(x,y) > E) {
                        E1(x,y) = E;
                        disp1(x,y) = static_cast<float>(d);
                    }
                    if(E2(x+d,y) > E) {
                        E2(x+d,y) = E;
                        disp2(x+d,y)= -static_cast<float>(d);
                    }
                }
            }
        }
        delete [] weights2;
    }
    delete [] cost;
    delete [] distC;
    delete [] distP;
}
