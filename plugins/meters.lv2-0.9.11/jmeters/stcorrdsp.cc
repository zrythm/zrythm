// ------------------------------------------------------------------------
//
//  Copyright (C) 2008-2012 Fons Adriaensen <fons@linuxaudio.org>
//    
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// ------------------------------------------------------------------------


#include <math.h>
#include "stcorrdsp.h"

namespace LV2M {

float  Stcorrdsp::_w1;
float  Stcorrdsp::_w2; 



Stcorrdsp::Stcorrdsp (void) :
    _zl (0),
    _zr (0),
    _zlr (0),
    _zll (0),
    _zrr (0)
{
}


Stcorrdsp::~Stcorrdsp (void)
{
}


void Stcorrdsp::process (float *pl, float *pr, int n)
{
    float zl, zr, zlr, zll, zrr;

    zl = _zl;
    zr = _zr;
    zlr = _zlr;
    zll = _zll;
    zrr = _zrr;
    while (n--)
    {
	zl += _w1 * (*pl++ - zl) + 1e-20f;
	zr += _w1 * (*pr++ - zr) + 1e-20f;
	zlr += _w2 * (zl * zr - zlr);
	zll += _w2 * (zl * zl - zll);
	zrr += _w2 * (zr * zr - zrr);
    }

    if (!finite(zl)) zl = 0;
    if (!finite(zr)) zr = 0;
    if (!finite(zlr)) zlr = 0;
    if (!finite(zll)) zll = 0;
    if (!finite(zrr)) zrr = 0;

    _zl = zl;
    _zr = zr;
    _zlr = zlr + 1e-10f;
    _zll = zll + 1e-10f;
    _zrr = zrr + 1e-10f;
}


float Stcorrdsp::read (void)
{
    return _zlr / sqrtf (_zll * _zrr + 1e-10f);
}


void Stcorrdsp::init (int fsamp, float flp, float tcf)
{
    // fsamp = sample frequency
    // flp   = lowpass frequency
    // tcf   = correlation filter time constant

    _w1 = 6.28f * flp / fsamp;
    _w2 = 1 / (tcf * fsamp);
}

}
/* vi:set ts=8 sts=8 sw=4: */
