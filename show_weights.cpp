/**
 * @file show_weights.cpp
 * @brief Computation of adaptive  (bilateral) weights in a square window
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
#include "cmdLine.h"
#include "io_png.h"
#include <algorithm>
#include <iostream>
#include <cmath>

/// Type for weights combination
typedef float (*Comb)(float,float);

/// The std::max and std::min functions take arguments float&, not float.
/// We need to encapsulate them for parameter compatibility.
static float max(float a, float b) {
    return std::max(a,b);
}
static float min(float a, float b) {
    return std::min(a,b);
}
static float mult(float a, float b) {
    return a*b;
}
static float plus(float a, float b) {
    return a+b;
}

/// Usage Description
static void usage(const char* name) {
    ParamDisparity p; // Parameters for adaptive weights
	std::cerr <<"Show weights\n"
              << "Usage: " << name
              << " [options] im1.png x y out.png [im2.png disp]\n"
              << "Options (default values in parentheses)\n"
              << "Adaptive weights parameters:\n"
              << "    -R radius: radius of the window patch ("
              <<p.window_radius << ")\n"
              << "    --gcol gamma_c: gamma for color similarity ("
              <<p.gamma_c << ")\n"
              << "    --gpos gamma_p: gamma for distance ("
              <<p.gamma_p << ")\n"
              << "    -c: weights combination (mult)\n\n"
              << "Weights combination choice (relevant only with im2.png):\n"
              << "    - 'max': max(w1,w2)\n"
              << "    - 'min': min(w1,w2)\n"
              << "    - 'mult': w1*w2\n"
              << "    - 'plus': w1+w2"
              << std::endl;
}

/// Load color image
Image loadImage(const char* name) {
    size_t width, height;
    float* pix = io_png_read_f32_rgb(name, &width, &height);
    if(! pix) {
        std::cerr << "Unable to read file " << name << " as PNG" << std::endl;
        std::exit(1);
    }
    const int w=static_cast<int>(width), h=static_cast<int>(height);
    Image im(w, h, 3);
    const float *r=pix, *g=r+w*h, *b=g+w*h;
    for(int y=0; y<h; y++)
        for(int x=0; x<w; x++) {
            im(x,y,0) = *r++;
            im(x,y,1) = *g++;
            im(x,y,2) = *b++;
        }
    std::free(pix);
    return im;
}

/// Relative weight between pixels (x,y) and (x+dx,y+dy).
float weight(const Image& im, int x, int y, int dx, int dy,
             float gamma_c, float gamma_p) {
    float d=0; // L1 color distance
    for(int i=0; i<im.channels(); i++)
        d += std::abs(im(x+dx,y+dy,i)-im(x,y,i));
    return
        std::exp(-d/(im.channels()*gamma_c)) *
        std::exp(-std::sqrt(float(dx*dx+dy*dy))/gamma_p);
}

/// Compute the window of weights around pixel (xp,yp) in \a im1.
Image show_weights(const Image& im1, const Image& im2, int xp, int yp, int xq,
                   Comb* comb, int r, float gamma_c, float gamma_p) {
    Image W(2*r+1,2*r+1);
    std::fill_n(&W(0,0), W.width()*W.height(), 0);
    int w1=im1.width(), h1=im1.height();
    int w2=im2.width(), h2=im2.height();
    for(int y=-r; y<=r; y++)
        if(0<=yp+y && yp+y<h1 && (!comb || yp+y<h2))
            for(int x=-r; x<=r; x++)
                if(0<=xp+x && xp+x<w1 &&
                   (!comb || (0<=xq+x && xq+x<w2))) {
                    float w = weight(im1, xp,yp, x,y, gamma_c, gamma_p);
                    if(comb)
                        w = (*comb)(w,
                                    weight(im2, xq,yp, x,y, gamma_c, gamma_p));
                    W(x+r,y+r) = w;
                }
    return W;
}

/// Rescale weights to interval [0,255]
void rescale(Image& w) {
    float f = 255.0f / w(w.width()/2,w.height()/2); // Max value at the middle
    for(int y=0; y<w.height(); y++)
        for(int x=0; x<w.width(); x++) {
            float v = f*w(x,y);
            if(v<0) v = 0;
            if(v>255.0f) v = 255.0f;
            w(x,y) = v;
        }
}

/// Main Program
int main(int argc, char *argv[])
{
	CmdLine cmd;

    std::string combine;
    ParamDisparity p; // Parameters for adaptive weights
    cmd.add( make_option('R',p.window_radius) );
    cmd.add( make_option(0,p.gamma_c,"gcol") );
    cmd.add( make_option(0,p.gamma_p,"gpos") );
	cmd.add( make_option('c', combine) );

	try {
		cmd.process(argc, argv);
	} catch(std::string str) {
		std::cerr << "Error: " << str << std::endl<<std::endl;
        argc = 1; // To display usage
	}
	if(argc!=5 && argc!=7) {
		usage(argv[0]);
		return 1;
	}

    // Load images
    Image im1 = loadImage(argv[1]);
    Image im2;
    if(argc>5)
        im2 = loadImage(argv[5]);

	int x,y;
	if(! ((std::istringstream(argv[2])>>x).eof() &&
		  (std::istringstream(argv[3])>>y).eof())) {
        std::cerr << "Error reading x or y" << std::endl;
        return 1;
	}

	int disp=0;
	if(argc>6 && !((std::istringstream(argv[6])>>disp).eof()) ) {
        std::cerr << "Error reading disparity" << std::endl;
        return 1;
	}

    Comb* comb=0;
    if(cmd.used('c') && im2.channels()!=0) {
        if(combine == "max")
            comb = new Comb(max);
        else if(combine == "min")
            comb = new Comb(min);
        else if(combine == "mult")
            comb = new Comb(mult);
        else if(combine == "plus")
            comb = new Comb(plus);
        else {
            std::cerr << "Unrecognized option for weights combination "
                      << "(should be max,min,mult or plus)" << std::endl;
            return 1;
        }
    }

    Image w = show_weights(im1, im2, x, y, x+disp, comb,
                           p.window_radius, p.gamma_c, p.gamma_p);
    rescale(w);
    if(io_png_write_f32(argv[4], &w(0,0), w.width(), w.height(), 1) != 0) {
        std::cerr << "Unable to write file " << argv[4] << std::endl;
        return 1;
    }

	return 0;
}
