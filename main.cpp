/**
 * @file main.cpp
 * @brief Disparity map estimation using Yoon and Kweon's adaptive weights
 * @author Laura F. Julià <fernandl@imagine.enpc.fr>
 *         Pascal Monasse <monasse@imagine.enpc.fr>
 */


#include "disparity.h"
#include "occlusion.h"
#include "image.h"
#include "cmdLine.h"
#include "io_png.h"
#include <iostream>

/// Names of output image files
static const char* OUTFILE1="disparity.png";
static const char* OUTFILE2="disparity_postprocessed.png";

/// Usage description
static void usage(const char* name) {
    ParamDisparity p;
    ParamOcclusion q;
    std::cerr <<"Yoon-Kweon disparity map estimation with adaptive weights.\n"
              << "Usage: " << name << " [options] im1.png im2.png dmin dmax\n\n"
              << "Options (default values in parentheses)\n"
              << "Adaptive weights parameters:\n"
              << "    --gcol gamma_col: gamma for color difference ("
              <<p.gamma_s<<")\n"
              << "    --gpos gamma_pos: gamma for spatial distance ("
              <<p.gamma_p<<")\n"
              << "    -R radius: radius of patch window ("
              <<p.window_radius << ")\n"
              << "    -A alpha: value of alpha for matching cost ("
              <<p.alpha<<")\n"
              << "    -t T: threshold for color difference in matching cost ("
              <<p.color_threshold<<")\n"
              << "    -g G: threshold for gradient difference in matching cost ("
              <<p.gradient_threshold << ")\n\n"
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
              <<q.sigma_space << ")\n\n"
              << "Display:\n"
              << "    -a grayMin: value of gray for min disparity (255)\n"
              << "    -b grayMax: value of gray for max disparity (0)"
              << std::endl;
}


/// Main program
int main(int argc, char *argv[])
{
    int grayMin=255, grayMax=0;
    int sense=0; // Camera motion direction: '0'=to-right, '1'=to-left
    CmdLine cmd;

    ParamDisparity paramD; // Parameters for adaptive weights
    cmd.add( make_option('R',paramD.window_radius) );
    cmd.add( make_option('A',paramD.alpha) );
    cmd.add( make_option('t',paramD.color_threshold) );
    cmd.add( make_option('g',paramD.gradient_threshold) );
    cmd.add( make_option(0,paramD.gamma_s,"gcol") );
    cmd.add( make_option(0,paramD.gamma_p,"gpos") );

    ParamOcclusion paramOcc; // Parameters for filling occlusions
    cmd.add( make_option('o',paramOcc.tol_disp) ); // Detect occlusion
    cmd.add( make_option('O',sense) ); // Fill occlusion
    cmd.add( make_option('r',paramOcc.median_radius) );
    cmd.add( make_option('c',paramOcc.sigma_color) );
    cmd.add( make_option('s',paramOcc.sigma_space) );

    // Display parameters
    cmd.add( make_option('a',grayMin) );
    cmd.add( make_option('b',grayMax) );

    try {
        cmd.process(argc, argv);
    } catch(std::string str) {
        std::cerr << "Error: " << str << std::endl<<std::endl;
        usage(argv[0]);
        return 1;
    }

    if( argc!=5 ) {
        usage(argv[0]);
        return 1;
    }

    if(sense != 0 && sense != 1) {
        std::cerr << "Error: invalid camera motion direction " << sense
            << " (must be 0 or 1)" << std::endl;
        return 1;
    }

    // Load images
    size_t width, height, width2, height2;
    float* pix1 = io_png_read_f32_rgb(argv[1], &width, &height);
    float* pix2 = io_png_read_f32_rgb(argv[2], &width2, &height2);
    if(width != width2 || height != height2) {
        std::cerr << "The images must have the same size!" << std::endl;
        return 1;
    }
    Image im1(pix1, width, height);
    Image im2(pix2, width, height);

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
    Image disparity(width,height);
    std::fill_n(&disparity(0,0), width*height, dMin-1);
    Image disparity2(width,height);
    std::fill_n(&disparity2(0,0), width*height, dMin-1);

    // Compute disparity using adaptive weights.
    disparityAW(im1, im2, dMin, dMax, paramD,disparity,disparity2);

    // Save disparity image
    if(! save_disparity(OUTFILE1, disparity, dMin,dMax, grayMin,grayMax)) {
        std::cerr << "Error writing file " << OUTFILE1 << std::endl;
        return 1;
    }

    // Detecting occlusions
    std::cout << "Detect occlusions...";
    detect_occlusion(disparity, disparity2, dMin-1, paramOcc.tol_disp);

    // Save disparity image with occlusions filled and smoothed image
    std::cout << "Post-processing: fill occlusions" << std::endl;
    Image dispDense = disparity.clone();
    if(sense == 0)
        dispDense.fillMaxX(dMin);
    else
        dispDense.fillMinX(dMin);

    std::cout << "Post-processing: smooth the disparity map" << std::endl;
    fill_occlusion(dispDense, im1.medianColor(1),disparity, dMin, dMax, paramOcc);
    if(! save_disparity(OUTFILE2, disparity, dMin,dMax, grayMin,grayMax)) {
        std::cerr << "Error writing file " << OUTFILE2 << std::endl;
        return 1;
    }

    // free space
    free(pix1);
    free(pix2);

    return 0;
}
