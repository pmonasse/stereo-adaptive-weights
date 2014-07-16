/**
 * @file disparity.h
 * @brief Disparity Map computation
 * @author Laura F. Julià <fernandl@imagine.enpc.fr>
 *         Pascal Monasse <monasse@imagine.enpc.fr>
 */

#ifndef DISPARITY_H
#define DISPARITY_H

#include <algorithm>

class Image;

/// Parameters specific to the disparity computation with adaptive weights
struct ParamDisparity {
    float color_threshold; ///< Max cost for color difference term
    float gradient_threshold; ///< Max cost for gradient difference term
    float alpha; ///< Balance between color/gradient difference
	float gamma_c; ///< Weight of color similarity (higher means less weight)
	float gamma_p; ///< Weight for position (higher means less weight)
    int window_radius; ///< Radius r of window, size is (2*r+1)x(2*r+1)

    // Constructor with default parameters
    ParamDisparity()
    : color_threshold(30),
      gradient_threshold(2),
      alpha(0.9),
	  gamma_c(12),
	  gamma_p(17.5),
      window_radius(17) {}
};

void disparityAW(Image im1, Image im2, int dispMin, int dispMax,
                 const ParamDisparity& param, Image& disp1, Image& disp2);

#endif
