Bilaterally Weighted Patches for Disparity Map Computation.

Laura F. Julià, <fernandl@imagine.enpc.fr>, Imagine, ENPC, France
Pascal Monasse, <monasse@imagine.enpc.fr>, Imagine, ENPC, France

- Build
mkdir Build && cd Build
cmake -D CMAKE_BUILD_TYPE:string=Release ..
make

- Run
Usage: ./stereoAdaptiveWeights [options] im1.png im2.png dmin dmax

Adaptive weights parameters:
    --gcol gamma_col: gamma for color difference (12)
    --gpos gamma_pos: gamma for spatial distance (17.5)
    -R radius: radius of patch window (17)
    -A alpha: value of alpha for matching cost (0.9)
    -t T: threshold for color difference in matching cost (30)
    -g G: threshold for gradient difference in matching cost (2)

Disparity map image:
    -f factor: the disparity values will be scaled by this factor
               to obtain a gray level(4)

Occlusion detection:
    -o tolDiffDisp: tolerance for left-right disp. diff. (0)

Densification:
    -O sense: camera sense='0':right, '1':left (0)
    -r radius: radius of the weighted median filter (9)
    -c sigmac: value of sigma_color (25.5)
    -s sigmas: value of sigma_space (9)

The parameter 'sense' used in densification is the direction of camera motion:
    - from left to right (value '0'), common for Middlebury pairs
    - from right to left (value '1')

- Output image files
disparity.png: disparity map after Yoon-Kweon's adaptive weights method.
disparity_postprocessed.png: final densification with median filter

- Test
./stereoAdaptiveWeights -f 16 ../data/tsukuba_l.png ../data/tsukuba_r.png -15 0
Compare resulting image files with those in folder data.
