Bilaterally Weighted Patches for Disparity Map Computation.

Laura F. Julia, <fernandl@imagine.enpc.fr>, Univ. Paris Est, LIGM, ENPC, France
Pascal Monasse, <monasse@imagine.enpc.fr>, Univ. Paris Est, LIGM, ENPC, France

Version 1.2 released on 2017/07/05

Future releases and updates:
https://github.com/pmonasse/stereo-adaptive-weights.git

- Build
Prerequisites: CMake version 2.6 or later
$ mkdir Build && cd Build
$ cmake -D CMAKE_BUILD_TYPE:string=Release ..
$ make

The software uses the libraries libPNG and libTIFF, and their dependencies zlib and libJPEG, of the system if found, otherwise it compiles the missing ones from the sources included in the folder third_party.

- Run
Usage: ./stereoAdaptiveWeights [options] im1.png im2.png dmin dmax [out_prefix]

Adaptive weights parameters:
    --gcol gamma_col: gamma for color difference (12)
    --gpos gamma_pos: gamma for spatial distance (17.5)
    -R radius: radius of patch window (17)
    -A alpha: value of alpha for matching cost (0.9)
    -t T: threshold for color difference in matching cost (30)
    -g G: threshold for gradient difference in matching cost (2)

Occlusion detection:
    -o tolDiffDisp: tolerance for left-right disp. diff. (0)

Densification:
    -O sense: camera sense='0':right, '1':left (0)
    -r radius: radius of the weighted median filter (9)
    -c sigmac: value of sigma_color (25.5)
    -s sigmas: value of sigma_space (9)

The parameter 'sense' used in densification is the direction of camera motion:
    - from left to right (value '0', default), common for Middlebury pairs
    - from right to left (value '1')
    - other value: no densification.

- Output image files
The optional string 'out_prefix' (default: 'disparity') is the prefix used for the output images. The 3 output images are in float TIFF format:
'out_prefix'.tif: initial disparity map (dense)
'out_prefix'_occ.tif: after occlusion detection (occluded pixels take value NaN)
'out_prefix'_pp.tif: final disparity map after post-processing (dense)
No widespread image viewer is able to display these images. An included utility program, tiff2png, applies an affine transform to pixel values to output a color 8-bit PNG image.

- Test
$ ./stereoAdaptiveWeights ../data/tsukuba_l.png ../data/tsukuba_r.png -15 0
$ ./tiff2png disparity.tif -15 0 disparity.png
$ ./tiff2png disparity_occ.tif -15 0 disparity_occ.png
$ ./tiff2png disparity_pp.tif -15 0 disparity_pp.png
Compare resulting image files with those in folder data. They must be identical.

- Combination of weights
The weight combination function of left and right images must be chosen at compile time with the CMake variable COMBINE_WEIGHTS. To change it:
$ mkdir Build && cd Build
$ cmake -D CMAKE_BUILD_TYPE:string=Release -D COMBINE_WEIGHTS:string=left ..
$ make
The options are mult (default), plus, min, max, left. They should have comparable running time, except 'left' is faster since it does not use the weights in the right image.

- Other utilities
Usage: ./show_weights [options] im1.png x y out.png [im2.png disp]
Options (default values in parentheses)
Adaptive weights parameters:
    -R radius: radius of the window patch (17)
    --gcol gamma_c: gamma for color similarity (12)
    --gpos gamma_p: gamma for distance (17.5)
    -c: weights combination (mult)

Weights combination choice (relevant only with im2.png):
    - 'max': max(w1,w2)
    - 'min': min(w1,w2)
    - 'mult': w1*w2
    - 'plus': w1+w2
(the 'left' combination is obtained by simply not using im2.png and disp)
disp is the integer disparity, in other words the weight window in im2.png is centered at (x+d,y).

Usage: ./tiff2png [options] in.tif vMin vMax out.png
Options:
    -m,--min grayMin: gray level for vMin (255)
    -M,--max grayMax: gray level for vMax (0)
This outputs a color 8-bit PNG image, applying an affine map between points (vMin,grayMin) and (vMax,grayMax). Pixels are gray except NaN input values or values outside the range [vMin,vMax] are in cyan color.

- Note
There is a known bug (#58800) in g++ 4.8.2 in function std::nth_element in Image::median (filters.cpp). This is fixed in Ubuntu 14.04, but other systems may be vulnerable (for example MinGW-4.8.2). This provokes a memory error and is a security hazard. The adopted solution is to use std::sort, with a tiny bit longer computation time.

- Files (Only those with * are reviewed)
disparity.cpp (*)
disparity.h (*)
image.cpp (*)
image.h (*)
main.cpp (*)
show_weights.cpp (*)
tiff2png.cpp (*)
cmdLine.h
io_png.c
io_png.h
io_tiff.c
io_tiff.h
nan.h
filters.cpp
occlusion.cpp
occlusion.h
data/... (test data)
third_party/... (standard support libraries)
CMakeLists.txt
LICENSE.txt
README.txt (this document)

- Changes
*1.0: initial release
*1.1: fix out of bounds read
*1.2: upgrade third party libraries
