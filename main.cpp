/**
 * @file main.cpp
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
#include "occlusion.h"
#include "image.h"
#include "cmdLine.h"
#include "io_png.h"
#include <iostream>

/// Default prefix of output image files
static const char* PREFIX="disparity";
static const char* SUFFIX1=".tif";     ///< Suffix for output 1: dense disparity
static const char* SUFFIX2="_occ.tif"; ///< Suffix for output 2: LR filtered
static const char* SUFFIX3="_pp.tif";  ///< Suffix for output 3: post_processed

#ifndef COMB
#error "The macro COMB must be set to one of the allowed values at compilation"
#endif

/// Usage description
static void usage(const char* name) {
    ParamDisparity p;
    ParamOcclusion q;
    std::cerr <<"Bilaterally weighted patches for disparity map computation.\n"
              << "Usage: " << name
              << " [options] im1.png im2.png dmin dmax [out_prefix]\n\n"
              << "Options (default values in parentheses)\n"
              << "Adaptive weights parameters:\n"
              << "    --gcol gamma_col: gamma for color difference ("
              <<p.gammaCol<<")\n"
              << "    --gpos gamma_pos: gamma for spatial distance ("
              <<p.gammaPos<<")\n"
              << "    -R radius: radius of patch window ("
              <<p.radius << ")\n"
              << "    -A alpha: value of alpha for matching cost ("
              <<p.alpha<<")\n"
              << "    -t T: threshold of color difference in matching cost ("
              <<p.tauCol<<")\n"
              << "    -g G: threshold of gradient difference in matching cost ("
              <<p.tauGrad << ")\n"
              << "    Combination of weights is '" << COMB
              << "' (recompile to change it)\n\n"
              << "Occlusion detection:\n"
              << "    -o tolDiffDisp: tolerance for left-right disp. diff. ("
              <<q.tol_disp << ")\n\n"
              << "Densification:\n"
              << "    -O sense: camera sense='0':right, '1':left (0)\n"
              << "    -r radius: radius of the weighted median filter ("
              <<q.median_radius << ")\n"
              << "    -c sigmac: value of sigma_color ("
              <<q.sigma_color << ")\n"
              << "    -s sigmas: value of sigma_space ("
              <<q.sigma_space << ")"
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

/// Main program
int main(int argc, char *argv[]) {
    int sense=0; // Camera motion direction: '0'=to-right, '1'=to-left
    CmdLine cmd;

    ParamDisparity paramD; // Parameters for adaptive weights
    cmd.add( make_option('R',paramD.radius) );
    cmd.add( make_option('A',paramD.alpha) );
    cmd.add( make_option('t',paramD.tauCol) );
    cmd.add( make_option('g',paramD.tauGrad) );
    cmd.add( make_option(0,paramD.gammaCol,"gcol") );
    cmd.add( make_option(0,paramD.gammaPos,"gpos") );

    ParamOcclusion paramOcc; // Parameters for filling occlusions
    cmd.add( make_option('o',paramOcc.tol_disp) ); // Detect occlusion
    cmd.add( make_option('O',sense) ); // Fill occlusion
    cmd.add( make_option('r',paramOcc.median_radius) );
    cmd.add( make_option('c',paramOcc.sigma_color) );
    cmd.add( make_option('s',paramOcc.sigma_space) );

    try {
        cmd.process(argc, argv);
    } catch(std::string str) {
        std::cerr << "Error: " << str << std::endl<<std::endl;
        argc=1; // To display usage
    }
    if(argc!=5 && argc!=6) {
        usage(argv[0]);
        return 1;
    }

    if(sense != 0 && sense != 1) {
        std::cerr << "Error: invalid camera motion direction " << sense
                  << " (must be 0 or 1)" << std::endl;
        return 1;
    }

    // Load images
    Image im1 = loadImage(argv[1]);
    Image im2 = loadImage(argv[2]);
    const int width=im1.width(), height=im1.height();
    if(width!=im2.width() || height!=im2.height()) {
        std::cerr << "The images must have the same size!" << std::endl;
        return 1;
    }

    // Set disparity range
    int dMin, dMax;
    if(! ((std::istringstream(argv[3])>>dMin).eof() &&
          (std::istringstream(argv[4])>>dMax).eof())) {
            std::cerr << "Error reading dMin or dMax" << std::endl;
            return 1;
    }
    if(dMin>dMax) {
        std::cerr << "Wrong disparity range! (dMin > dMax)" << std::endl;
        return 1;
    }

    // Create disparity images
    Image disp1(width,height);
    std::fill_n(&disp1(0,0), width*height, static_cast<float>(dMin-1));
    Image disp2(width,height);
    std::fill_n(&disp2(0,0), width*height, static_cast<float>(dMin-1));

    // Compute disparity using adaptive weights.
    disparityAW(im1, im2, dMin, dMax, paramD,disp1,disp2);

    // Prepare output file names
    std::string prefix((argc>5)? argv[5]: PREFIX);
    std::string outFile1 = prefix + SUFFIX1; // initial disparity map
    std::string outFile2 = prefix + SUFFIX2; // with occlusions
    std::string outFile3 = prefix + SUFFIX3; // filled occlusions

    // Save disparity image
    if(! save_disparity(outFile1.c_str(), disp1, dMin,dMax)) {
        std::cerr << "Error writing file " << outFile1 << std::endl;
        return 1;
    }

    // Detecting occlusions
    detect_occlusion(disp1,disp2,static_cast<float>(dMin-1),paramOcc.tol_disp);
    if(! save_disparity(outFile2.c_str(), disp1, dMin,dMax)) {
        std::cerr << "Error writing file " << outFile2 << std::endl;
        return 1;
    }

    // Fill occlusions (post-processing)
    Image dispDense = disp1.clone();
    if(sense == 0)
        dispDense.fillMaxX(static_cast<float>(dMin));
    else
        dispDense.fillMinX(static_cast<float>(dMin));
    fill_occlusion(dispDense, im1.median(1), disp1, dMin, dMax, paramOcc);
    if(! save_disparity(outFile3.c_str(), disp1, dMin,dMax)) {
        std::cerr << "Error writing file " << outFile3 << std::endl;
        return 1;
    }

    return 0;
}
