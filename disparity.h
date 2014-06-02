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
    float color_threshold;
    float gradient_threshold;
    float alpha;
	float gamma_s;
	float gamma_p;
    int window_radius;

    // Constructor with default parameters
    ParamDisparity()
    : color_threshold(30),
      gradient_threshold(2),
      alpha(0.9),
	  gamma_s(12),
	  gamma_p(17.5),
      window_radius(17) {}
};

void disparityAW(Image im1Color, Image im2Color, int dispMin, int dispMax,
                    const ParamDisparity& param, Image& disparity, Image& disparity2);

#endif
