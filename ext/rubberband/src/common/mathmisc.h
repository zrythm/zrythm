/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Rubber Band Library
    An audio time-stretching and pitch-shifting library.
    Copyright 2007-2024 Particular Programs Ltd.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.

    Alternatively, if you have a valid commercial licence for the
    Rubber Band Library obtained by agreement with the copyright
    holders, you may redistribute and/or modify it under the terms
    described in that licence.

    If you wish to distribute code using the Rubber Band Library
    under terms other than those of the GNU General Public License,
    you must obtain a valid commercial licence before doing so.
*/

#ifndef RUBBERBAND_MATHMISC_H
#define RUBBERBAND_MATHMISC_H

#include "sysutils.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif // M_PI

namespace RubberBand {

inline double mod(double x, double y) {
    return x - (y * floor(x / y));
}
inline float modf(float x, float y) {
    return x - (y * float(floor(x / y)));
}

inline double princarg(double a) {
    return mod(a + M_PI, -2.0 * M_PI) + M_PI;
}
inline float princargf(float a) {
    return modf(a + (float)M_PI, -2.f * (float)M_PI) + (float)M_PI;
}

inline int binForFrequency(double f, int fftSize, double sampleRate) {
    return int(round(f * double(fftSize) / sampleRate));
}
inline double frequencyForBin(int b, int fftSize, double sampleRate) {
    return (double(b) * sampleRate) / double(fftSize);
}

void pickNearestRational(double ratio, int maxDenom, int &num, int &denom);

size_t roundUp(size_t value); // to nearest power of two

size_t roundUpDiv(double divisionOf, size_t divisor);

}

#endif
