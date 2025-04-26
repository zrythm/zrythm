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

#include "mathmisc.h"

namespace RubberBand {

void pickNearestRational(double ratio, int max_denom, int &num, int &denom)
{
    // Farey algorithm, see
    // https://www.johndcook.com/blog/2010/10/20/best-rational-approximation/
    double a = 0.0, b = 1.0, c = 1.0, d = 0.0;
    double pa = a, pb = b, pc = c, pd = d;
    double eps = 1e-9;
    while (b <= max_denom && d <= max_denom) {
        double mediant = (a + c) / (b + d);
        if (fabs(ratio - mediant) < eps) {
            if (b + d <= max_denom) {
                num = a + c;
                denom = b + d;
                return;
            } else if (d > b) {
                num = c;
                denom = d;
                return;
            } else {
                num = a;
                denom = b;
                return;
            }
        }
        if (ratio > mediant) {
            pa = a; pb = b;
            a += c; b += d;
        } else {
            pc = c; pd = d;
            c += a; d += b;
        }
    }
    if (fabs(ratio - (pc / pd)) < fabs(ratio - (pa / pb))) {
        num = pc;
        denom = pd;
    } else {
        num = pa;
        denom = pb;
    }
}

size_t roundUp(size_t value)
{
    if (!(value & (value - 1))) return value;
    size_t bits = 0;
    while (value) { ++bits; value >>= 1; }
    value = size_t(1) << bits;
    return value;
}

size_t roundUpDiv(double divisionOf, size_t divisor)
{
    if (divisionOf < 0.0) return 0;
    return roundUp(size_t(ceil(divisionOf / double(divisor))));
}

}
