/* Copyright (C) 2013 Robin Gareus <robin@gareus.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */


#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include "truepeakdsp.h"

namespace LV2M {

TruePeakdsp::TruePeakdsp (void)
	: _m (0)
	, _p (0)
	, _res (true)
	, _buf (NULL)
{
}


TruePeakdsp::~TruePeakdsp (void)
{
	free(_buf);
}


void TruePeakdsp::process (float *data, int n)
{
	assert (n > 0);
	assert (n <= 8192);
	_src.inp_count = n;
	_src.inp_data = data;
	_src.out_count = n * 4;
	_src.out_data = _buf;
	_src.process ();

	float v;
	float m = _res ? 0: _m;
	float p = _res ? 0: _p;
	float z1 = _z1 > 20 ? 20 : (_z1 < 0 ? 0 : _z1);
	float z2 = _z2 > 20 ? 20 : (_z2 < 0 ? 0 : _z2);
	float *b = _buf;

	while (n--) {
		z1 *= _w3;
		z2 *= _w3;

		v = fabsf(*b++);
		if (v > z1) z1 += _w1 * (v - z1);
		if (v > z2) z2 += _w2 * (v - z2);
		if (v > p) p = v;

		v = fabsf(*b++);
		if (v > z1) z1 += _w1 * (v - z1);
		if (v > z2) z2 += _w2 * (v - z2);
		if (v > p) p = v;

		v = fabsf(*b++);
		if (v > z1) z1 += _w1 * (v - z1);
		if (v > z2) z2 += _w2 * (v - z2);
		if (v > p) p = v;

		v = fabsf(*b++);
		if (v > z1) z1 += _w1 * (v - z1);
		if (v > z2) z2 += _w2 * (v - z2);
		if (v > p) p = v;

		v = z1 + z2;
		if (v > m) m = v;
	}

	_z1 = z1 + 1e-20f;
	_z2 = z2 + 1e-20f;

	m *= _g;

	if (_res) {
		_m = m;
		_p = p;
		_res = false;
	} else {
		if (m > _m) { _m = m; }
		if (p > _p) { _p = p; }
	}
}

void TruePeakdsp::process_max (float *p, int n)
{
	assert (n <= 8192);
	_src.inp_count = n;
	_src.inp_data = p;
	_src.out_count = n * 4;
	_src.out_data = _buf;
	_src.process ();

	float m = _res ? 0 : _m;
	float v;
	float *b = _buf;
	while (n--) {
		v = fabsf(*b++);
		if (v > m) m = v;
		v = fabsf(*b++);
		if (v > m) m = v;
		v = fabsf(*b++);
		if (v > m) m = v;
		v = fabsf(*b++);
		if (v > m) m = v;
	}
	_m = m;
}


float TruePeakdsp::read (void)
{
	_res = true;
	return _m;
}

void TruePeakdsp::read (float &m, float &p)
{
	_res = true;
	m = _m;
	p = _p;
}

void TruePeakdsp::reset ()
{
	_res = true;
	_m = 0;
	_p = 0;
}


void TruePeakdsp::init (float fsamp)
{
	_src.setup(fsamp, fsamp * 4.0, 1, 24, 1.0);
	_buf = (float*) malloc(32768 * sizeof(float));

	_z1 = _z2 = .0f;
	_w1 = 4000.0f / fsamp / 4.0;
	_w2 = 17200.0f / fsamp / 4.0;
	_w3 = 1.0f - 7.0f / fsamp / 4.0;
	_g = 0.502f;

	/* q/d initialize */
	float zero[8192];
	for (int i = 0; i < 8192; ++i) {
		zero[i]= 0.0;
	}
	_src.inp_count = 8192;
	_src.inp_data = zero;
	_src.out_count = 32768;
	_src.out_data = _buf;
	_src.process ();
}

};
