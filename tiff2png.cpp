/**
 * @file tiff2png.cpp
 * @brief Float TIFF to 8-bit color PNG conversion
 * @author Laura F. Julià <fernandl@imagine.enpc.fr>
 *         Pascal Monasse <monasse@imagine.enpc.fr>
 */

#include "nan.h"
#include "cmdLine.h"
#include "io_png.h"
#include "io_tiff.h"
#include <iostream>
#include <cstdlib>

/// Float TIFF to 8-bit color PNG conversion.
///
/// The value->gray function is affine: gray=a*value+b.
/// Values outside [vMin,vMax] are assumed invalid and written in cyan color.
int main(int argc, char *argv[]) {
    int grayMin=255, grayMax=0;
    CmdLine cmd;
    cmd.add( make_option('m',grayMin,"min") );
    cmd.add( make_option('M',grayMax,"max") );

    try {
        cmd.process(argc, argv);
    } catch(std::string str) {
        std::cerr << "Error: " << str << std::endl<<std::endl;
        argc=1; // To display usage
    }
    if(argc!=5) {
        std::cerr << "Usage: "<< argv[0]
                  << " [options] in.tif vMin vMax out.png\n"
                  << "Options:\n"
                  << "    -m,--min grayMin: gray level for vMin (255)\n"
                  << "    -M,--max grayMax: gray level for vMax (0)"
                  << std::endl;
        return 1;
    }

    float vMin, vMax;
    if(! ((std::istringstream(argv[2])>>vMin).eof() &&
          (std::istringstream(argv[3])>>vMax).eof())) {
        std::cerr << "Error reading vMin or vMax" << std::endl;
        return 1;
    }
    if(vMax < vMin) {
        std::cerr << "Error: vMax("<<vMax<< ") < vMin("<<vMin<< ')'<< std::endl;
        return 1;
    }

    size_t w, h;
    float* pix = io_tiff_read_f32_gray(argv[1], &w, &h);
    if(! pix) {
        std::cerr << "Unable to read file " << argv[1] << " as PNG" <<std::endl;
        return 1;
    }

    const float a=(grayMax-grayMin)/float(vMax-vMin);
    const float b=(grayMin*vMax-grayMax*vMin)/float(vMax-vMin);

    const float* in=pix;
    unsigned char *out=new unsigned char[3*w*h];
    unsigned char *red=out, *green=out+w*h, *blue=out+2*w*h;
    for(size_t i=w*h; i>0; i--, in++, red++) {
        if(is_number(*in) && vMin<=*in && *in<=vMax) {
            float v = a * *in + b +0.5f;
            if(v<0) v=0;
            if(v>255) v=255;
            *red = static_cast<unsigned char>(v);
            *green++ = *red;
            *blue++  = *red;
        } else { // Cyan for disparities out of range
            *red=0;
            *green++ = *blue++ = 255;
        }
    }
    if(io_png_write_u8(argv[4], out, w, h, 3) != 0) {
        std::cerr << "Unable to write file " <<argv[4]<< " as TIFF" <<std::endl;
        return 1;
    }
    delete [] out;
    std::free(pix);
    return 0;
}
