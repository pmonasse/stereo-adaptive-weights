/**
 * @file image.cpp
 * @brief image class with shallow copy
 * @author Pascal Monasse <monasse@imagine.enpc.fr>
 *
 * Copyright (c) 2012-2014, Pascal Monasse
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

#include "image.h"
#include "nan.h"
#include "io_png.h"
#include "io_tiff.h"
#include <algorithm>
#include <cassert>

/// Constructor
///
/// The main interest of this one is to allow arrays of Image.
Image::Image()
: count(0), tab(0), w(0), h(0), c(0) {}

/// Constructor
Image::Image(int width, int height, int channels)
: count(new int(1)), tab(new float[width*height*channels]),
  w(width), h(height), c(channels) {}

/// Constructor with array of pixels.
///
/// Make sure it is not deleted during the lifetime of the image.
Image::Image(float* pix, int width, int height, int channels)
: count(0), tab(pix), w(width), h(height), c(channels) {}

/// Copy constructor (shallow copy)
Image::Image(const Image& I)
: count(I.count), tab(I.tab), w(I.w), h(I.h), c(I.c) {
    if(count)
        ++*count;
}

/// Assignment operator (shallow copy)
Image& Image::operator=(const Image& I) {
    if(count != I.count) {
        kill();
        if(I.count)
            ++*I.count;
    }
    count=I.count; tab=I.tab; w=I.w; h=I.h; c=I.c;
    return *this;
}

/// Deep copy
Image Image::clone() const {
    Image I(w,h,c);
    std::copy(tab, tab+w*h*c, I.tab);
    return I;
}

/// Free memory
void Image::kill() {
    if(count && --*count == 0) {
        delete count;
        delete [] tab;
    }
}

/// Convert image to gray level
Image Image::gray() const {
    if(channels() == 1)
        return *this;
    assert(channels() == 3);
    Image out(w,h);
    const float* in = tab;
    for(int y=0; y<h; y++)
        for(int x=0; x<w; x++, in+=3)
            out(x,y) = rgb_to_gray(in[0], in[1], in[2]);
    return out;
}

/// Save \a disp map in float TIFF image.
bool save_disparity(const char* fileName, const Image& disp,
                    int dMin, int dMax) {
    const int w=disp.width(), h=disp.height();
    float *out = new float[w*h], *o=out;
    for(int y=0; y<h; y++)
        for(int x=0; x<w; x++) {
            float v = disp(x,y);
            if(! (is_number(v) &&
                  static_cast<float>(dMin)<=v && v<=static_cast<float>(dMax)))
                v = NaN;
            *o++ = v;
        }
    bool ok = (io_tiff_write_f32(fileName, out, w, h, 1) == 0);
    delete [] out;
    return ok;
}
