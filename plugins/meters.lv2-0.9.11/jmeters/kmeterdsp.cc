/*
    Copyright (C) 2008-2011 Fons Adriaensen <fons@linuxaudio.org>
    Copyright (C) 2013 by Robin Gareus <robin@gareus.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <math.h>
#include "kmeterdsp.h"

namespace LV2M {

float  Kmeterdsp::_omega;
int    Kmeterdsp::_hold;
float  Kmeterdsp::_fsamp;


Kmeterdsp::Kmeterdsp (void) :
    _z1 (0),
    _z2 (0),
    _rms (0),
    _peak (0),
    _cnt (0),
    _fpp (0),
    _fall (0),
    _flag (false)
{
}


Kmeterdsp::~Kmeterdsp (void)
{
}

void Kmeterdsp::init (float fsamp)
{
    const float hold = 0.5f;
    _fsamp = fsamp;

    _hold = (int)(hold * fsamp + 0.5f); // number of samples to hold peak
    _omega = 9.72f / fsamp; // ballistic filter coefficient
}

void Kmeterdsp::process (float *p, int n)
{
    // Called by JACK's process callback.
    //
    // p : pointer to sample buffer
    // n : number of samples to process

    float  s, t, z1, z2;

    if (_fpp != n) {
	const float fall = 15.0f;
	const float tme = (float) n / _fsamp; // period time in seconds
	_fall = powf (10.0f, -0.05f * fall * tme); // per period fallback multiplier
	_fpp = n;
    }

    t = 0;
    // Get filter state.
    z1 = _z1 > 50 ? 50 : (_z1 < 0 ? 0 : _z1);
    z2 = _z2 > 50 ? 50 : (_z2 < 0 ? 0 : _z2);

    // Perform filtering. The second filter is evaluated
    // only every 4th sample - this is just an optimisation.
    n /= 4;  // Loop is unrolled by 4.
    while (n--)
    {
	s = *p++;
	s *= s;
	if (t < s) t = s;             // Update digital peak.
	z1 += _omega * (s - z1);      // Update first filter.
	s = *p++;
	s *= s;
	if (t < s) t = s;             // Update digital peak.
	z1 += _omega * (s - z1);      // Update first filter.
	s = *p++;
	s *= s;
	if (t < s) t = s;             // Update digital peak.
	z1 += _omega * (s - z1);      // Update first filter.
	s = *p++;
	s *= s;
	if (t < s) t = s;             // Update digital peak.
	z1 += _omega * (s - z1);      // Update first filter.
        z2 += 4 * _omega * (z1 - z2); // Update second filter.
    }

    if (isnan(z1)) z1 = 0;
    if (isnan(z2)) z2 = 0;
    if (!finite(t)) t = 0;

    // Save filter state. The added constants avoid denormals.
    _z1 = z1 + 1e-20f;
    _z2 = z2 + 1e-20f;

    s = sqrtf (2.0f * z2);
    t = sqrtf (t);

    if (_flag) // Display thread has read the rms value.
    {
	_rms  = s;
	_flag = false;
    }
    else
    {
        // Adjust RMS value and update maximum since last read().
        if (s > _rms) _rms = s;
    }

    // Digital peak hold and fallback.
    if (t >= _peak)
    {
	// If higher than current value, update and set hold counter.
	_peak = t;
	_cnt = _hold;
    }
    else if (_cnt > 0)
    {
	// else decrement counter if not zero,
	_cnt-=_fpp;
    }
    else
    {
        _peak *= _fall;     // else let the peak value fall back,
	_peak += 1e-10f;    // and avoid denormals.
    }
}

/* Returns highest _rms value since last call */
float Kmeterdsp::read ()
{
    float rv= _rms;
    _flag = true; // Resets _rms in next process().
    return rv;
}

void Kmeterdsp::read (float &rms, float &peak)
{
    rms  = _rms;
    peak = _peak;
    _flag = true; // Resets _rms in next process().
}

void Kmeterdsp::reset ()
{
    _z1 = _z2 = _rms = _peak = .0f;
    _cnt = 0;
    _flag = false;
}

} /* end namespace */
/* vi:set ts=8 sts=8 sw=4: */
