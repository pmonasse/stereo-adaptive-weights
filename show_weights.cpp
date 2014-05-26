/**
 * @file show_weights.cpp
 * @brief Computation of adaptive weights in a square window
 * @author Laura F. Julià <fernandl@imagine.enpc.fr>
 *         Pascal Monasse <monasse@imagine.enpc.fr>
 */


#include "image.h"
#include "io_png.h"
#include <iostream>
#include <algorithm>
#include <cmath>


/// Compute weights of a window.
///
/// Computes the Yoon-Kweon adaptive weights for a window of the
/// input image and stores them in a grey image.
static void show_weights(char* input,       // input image name
						 int xp, int yp,    // window's center coordinates
                         int radius,        // window's radius
                         float gamma_s,     // color similarity parameter
                         float gamma_p,     // distance parameter
						 char* output)      // output image name
{

    // load the input image
    size_t w, h;
    float* pix = io_png_read_f32_rgb(input, &w, &h);
    Image im(pix, w, h);
    Image imR=im.r(), imG=im.g(), imB=im.b();

    float inr,ing,inb;
    float distS,distP;

    // create weight images
	Image weights(2*radius+1,2*radius+1);
	std::fill_n(&weights(0,0), (2*radius+1)*(2*radius+1), 0);

    // Compute weights
    for(int y=0; y<(int)h; y++)
        if(yp-radius<=y && y<=yp+radius)
            for (int x=0; x<(int)w; x++)
                if(xp-radius<=x && x<=xp+radius) {

                    // color difference (L1)
                    inr=abs(imR(x,y)-imR(xp,yp));
                    ing=abs(imG(x,y)-imG(xp,yp));
                    inb=abs(imB(x,y)-imB(xp,yp));
                    distS=inr+ing+inb;
					distS/=3;

                    // space difference (L2)
                    distP=(x-xp)*(x-xp)+(y-yp)*(y-yp);
                    distP=sqrt(distP);

                    // total weight
                    weights(x-xp+radius,y-yp+radius)=exp(-distS/gamma_s)*exp(-distP/gamma_p);
                }

    w=2*radius+1;
	h=w;
    const float* in=&(const_cast<Image&>(weights))(0,0);
    unsigned char *out = new unsigned char[3*w*h];
    unsigned char *red=out, *green=out+w*h, *blue=out+2*w*h;
    float maxim = weights(radius,radius);
    for(size_t i=w*h; i>0; i--, in++, red++) {
        float v = (255./maxim)* *in ;
        if(v<0) v=0;
        if(v>255) v=255;
        *red = static_cast<unsigned char>(v);
        *green++ = *red;
        *blue++  = *red;
    }
    io_png_write_u8(output, out, w, h, 3);
}


/// Compute combined weights of two windows.
///
/// Computes the Yoon-Kweon adaptive weights for a window in the reference image
/// and the window corresponding to disparity \a disp in the target window
/// combines them and stores them in a grey image.
static void show_weights2(char* input,   // input image name
                         char* input2,    // 2nd input image name
						 int xp, int yp,  // window's center coordinates in the ref. image
                         int radius,      // window's radius
                         float gamma_s,   // color similarity parameter
                         float gamma_p,   // distance parameter
						 char* output,    // output image name
						 int disp)		  // disparity value to combine the two windows
{
    // load the input images
    size_t w, h, w2, h2;
    float* pix=io_png_read_f32_rgb(input,&w,&h);
    float* pix2=io_png_read_f32_rgb(input2,&w2,&h2);
    Image im(pix, w, h), im2(pix2, w2, h2);
    Image imR=im.r(), imG=im.g(), imB=im.b(), imR2=im2.r(), imG2=im2.g(), imB2=im2.b();

    float inr,ing,inb,inr2,ing2,inb2;
    float distS,distP,distS2;

    // create weight images
	Image weights(2*radius+1,2*radius+1);
	std::fill_n(&weights(0,0), (2*radius+1)*(2*radius+1), 0);

    // Compute weights
    for(int y=0; y<(int)h; y++)
        if(yp-radius<=y && y<=yp+radius)
            for (int x=0; x<(int)w; x++)
                if(xp-radius<=x && x<=xp+radius) {

                    // color difference (L1)
                    inr=abs(imR(x,y)-imR(xp,yp));
                    ing=abs(imG(x,y)-imG(xp,yp));
                    inb=abs(imB(x,y)-imB(xp,yp));
                    distS=inr+ing+inb;
					distS/=3;

					inr2=abs(imR2(x+disp,y)-imR2(xp+disp,yp));
                    ing2=abs(imG2(x+disp,y)-imG2(xp+disp,yp));
                    inb2=abs(imB2(x+disp,y)-imB2(xp+disp,yp));
                    distS2=inr2+ing2+inb2;
					distS2/=3;

                    // space difference (L2)
                    distP=(x-xp)*(x-xp)+(y-yp)*(y-yp);
                    distP=sqrt(distP);

                    // total weights combining the two windows
                    weights(x-xp+radius,y-yp+radius)=exp(-(distS+distS2)/gamma_s)*exp(-2*distP/gamma_p);
                }

    w=2*radius+1;
	h=w;
    const float* in=&(const_cast<Image&>(weights))(0,0);
    unsigned char *out = new unsigned char[3*w*h];
    unsigned char *red=out, *green=out+w*h, *blue=out+2*w*h;
    float maxim = weights(radius,radius);
    for(size_t i=w*h; i>0; i--, in++, red++) {
        float v = (255./maxim)* *in ;
        if(v<0) v=0;
        if(v>255) v=255;
        *red = static_cast<unsigned char>(v);
        *green++ = *red;
        *blue++  = *red;
    }
    io_png_write_u8(output, out, w, h, 3);
}
