// ------------------------------------------------------------------------
//
//  Copyright (C) 2012 Fons Adriaensen <fons@linuxaudio.org>
//  Copyright (C) 2015 Robin Gareus <robin@areus.org>
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
#include "msppmdsp.h"

namespace LV2M {

float  Msppmdsp::_w1 = 0;
float  Msppmdsp::_w2 = 0;
float  Msppmdsp::_w3 = 0;
float  Msppmdsp::_g = 0; 


Msppmdsp::Msppmdsp (float mdb) :
    _z1 (0),
    _z2 (0),
    _m (0),
    _res (true),
    _mv (0)
{
    _mv = powf(10, .05 * mdb);
}


Msppmdsp::~Msppmdsp (void)
{
}


void Msppmdsp::processM (float *pl, float *pr, int n)
{
    float z1, z2, m, t;

    z1 = _z1 > 20 ? 20 : (_z1 < 0 ? 0 : _z1);
    z2 = _z2 > 20 ? 20 : (_z2 < 0 ? 0 : _z2);
    m = _res ? 0: _m;
    _res = false;

    n /= 4;
    while (n--)
    {
	z1 *= _w3;
	z2 *= _w3;
	t = _mv * fabsf (*pl++ + *pr++);
	if (t > z1) z1 += _w1 * (t - z1);
	if (t > z2) z2 += _w2 * (t - z2);
	t = _mv * fabsf (*pl++ + *pr++);
	if (t > z1) z1 += _w1 * (t - z1);
	if (t > z2) z2 += _w2 * (t - z2);
	t = _mv * fabsf (*pl++ + *pr++);
	if (t > z1) z1 += _w1 * (t - z1);
	if (t > z2) z2 += _w2 * (t - z2);
	t = _mv * fabsf (*pl++ + *pr++);
	if (t > z1) z1 += _w1 * (t - z1);
	if (t > z2) z2 += _w2 * (t - z2);
	t = z1 + z2;
	if (t > m) m = t;
    }

    _z1 = z1 + 1e-10f;
    _z2 = z2 + 1e-10f;
    _m = m;
}

void Msppmdsp::processS (float *pl, float *pr, int n)
{
    float z1, z2, m, t;

    z1 = _z1 > 20 ? 20 : (_z1 < 0 ? 0 : _z1);
    z2 = _z2 > 20 ? 20 : (_z2 < 0 ? 0 : _z2);
    m = _res ? 0: _m;
    _res = false;

    n /= 4;
    while (n--)
    {
	z1 *= _w3;
	z2 *= _w3;
	t = _mv * fabsf (*pl++ - *pr++);
	if (t > z1) z1 += _w1 * (t - z1);
	if (t > z2) z2 += _w2 * (t - z2);
	t = _mv * fabsf (*pl++ - *pr++);
	if (t > z1) z1 += _w1 * (t - z1);
	if (t > z2) z2 += _w2 * (t - z2);
	t = _mv * fabsf (*pl++ - *pr++);
	if (t > z1) z1 += _w1 * (t - z1);
	if (t > z2) z2 += _w2 * (t - z2);
	t = _mv * fabsf (*pl++ - *pr++);
	if (t > z1) z1 += _w1 * (t - z1);
	if (t > z2) z2 += _w2 * (t - z2);
	t = z1 + z2;
	if (t > m) m = t;
    }

    _z1 = z1 + 1e-10f;
    _z2 = z2 + 1e-10f;
    _m = m;
}


float Msppmdsp::read (void)
{
    _res = true;
    return _g * _m;
}


void Msppmdsp::init (float fsamp)
{
    _w1 = 200.0f / fsamp;
    _w2 = 860.0f / fsamp;
    _w3 = 1.0f - 4.0f / fsamp;
    _g = 0.5141f;
}

}
/* vi:set ts=8 sts=8 sw=4: */
