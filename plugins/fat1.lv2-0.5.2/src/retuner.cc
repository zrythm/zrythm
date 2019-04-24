// -----------------------------------------------------------------------
//
//  Copyright (C) 2009-2011 Fons Adriaensen <fons@linuxaudio.org>
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
// -----------------------------------------------------------------------


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "retuner.h"

using namespace LV2AT;

Retuner::Retuner (int fsamp) :
    _fsamp (fsamp),
    _refpitch (440.0f),
    _notebias (0.0f),
    _corrfilt (1.0f),
    _corrgain (1.0f),
    _corroffs (0.0f),
    _notemask (0xFFF)
{
    int   i, h;
    float t, x, y;

    if (_fsamp < 64000)
    {
        // At 44.1 and 48 kHz resample to double rate.
        _upsamp = true;
        _ipsize = 4096;
        _fftlen = 2048;
        _frsize = 128;
        _resampler.setup (1, 2, 1, 32); // 32 is medium quality.
        // Prefeed some input samples to remove delay.
        _resampler.inp_count = _resampler.filtlen () - 1;
        _resampler.inp_data = 0;
        _resampler.out_count = 0;
        _resampler.out_data = 0;
        _resampler.process ();
    }
    else if (_fsamp < 128000)
    {
        // 88.2 or 96 kHz.
        _upsamp = false;
        _ipsize = 4096;
        _fftlen = 4096;
        _frsize = 256;
    }
    else
    {
        // 192 kHz, double time domain buffers sizes.
        _upsamp = false;
        _ipsize = 8192;
        _fftlen = 8192;
        _frsize = 512;
    }

    // Accepted correlation peak range, corresponding to 60..1200 Hz.
    _ifmin = _fsamp / 1200;
    _ifmax = _fsamp / 60;

    // Various buffers
    _ipbuff = new float[_ipsize + 3];  // Resampled or filtered input
    _xffunc = new float[_frsize];      // Crossfade function
    _fftTwind = (float *) fftwf_malloc (_fftlen * sizeof (float)); // Window function 
    _fftWcorr = (float *) fftwf_malloc (_fftlen * sizeof (float)); // Autocorrelation of window 
    _fftTdata = (float *) fftwf_malloc (_fftlen * sizeof (float)); // Time domain data for FFT
    _fftFdata = (fftwf_complex *) fftwf_malloc ((_fftlen / 2 + 1) * sizeof (fftwf_complex));

    // FFTW3 plans
    _fwdplan = fftwf_plan_dft_r2c_1d (_fftlen, _fftTdata, _fftFdata, FFTW_ESTIMATE);
    _invplan = fftwf_plan_dft_c2r_1d (_fftlen, _fftFdata, _fftTdata, FFTW_ESTIMATE);

    // Clear input buffer.
    memset (_ipbuff, 0, (_ipsize + 1) * sizeof (float));

    // Create crossfade function, half of raised cosine.
    for (i = 0; i < _frsize; i++)
    {
        _xffunc [i] = 0.5 * (1 - cosf (M_PI * i / _frsize));
    }

    // Create window, raised cosine.
    for (i = 0; i < _fftlen; i++)
    {
        _fftTwind [i] = 0.5 * (1 - cosf (2 * M_PI * i / _fftlen));
    }

    // Compute window autocorrelation and normalise it.
    fftwf_execute_dft_r2c (_fwdplan, _fftTwind, _fftFdata);    
    h = _fftlen / 2;
    for (i = 0; i < h; i++)
    {
        x = _fftFdata [i][0];
        y = _fftFdata [i][1];
        _fftFdata [i][0] = x * x + y * y;
        _fftFdata [i][1] = 0;
    }
    _fftFdata [h][0] = 0;
    _fftFdata [h][1] = 0;
    fftwf_execute_dft_c2r (_invplan, _fftFdata, _fftWcorr);    
    t = _fftWcorr [0];
    for (i = 0; i < _fftlen; i++)
    {
        _fftWcorr [i] /= t;
    }

    // Initialise all counters and other state.
    _notebits = 0;
    _lastnote = -1;
    _count = 0;
    _cycle = _frsize;
    _error = 0.0f;
    _ratio = 1.0f;
    _xfade = false;
    _ipindex = 0;
    _frindex = 0;
    _frcount = 0;
    _rindex1 = _ipsize / 2;
    _rindex2 = 0;

    for (int i = 0; i < 12; ++i) {
        _notescale[i] = i;
    }
}


Retuner::~Retuner (void)
{
    delete[] _ipbuff;
    delete[] _xffunc;
    fftwf_free (_fftTwind);
    fftwf_free (_fftWcorr);
    fftwf_free (_fftTdata);
    fftwf_free (_fftFdata);
    fftwf_destroy_plan (_fwdplan);
    fftwf_destroy_plan (_invplan);
}


int Retuner::process (int nfram, float *inp, float *out)
{
    int    i, k, fi;
    float  ph, dp, r1, r2, dr, u1, u2, v;

    // Pitch shifting is done by resampling the input at the
    // required ratio, and eventually jumping forward or back
    // by one or more pitch period(s). Processing is done in
    // fragments of '_frsize' frames, and the decision to jump
    // forward or back is taken at the start of each fragment.
    // If a jump happens we crossfade over one fragment size. 
    // Every 4 fragments a new pitch estimate is made. Since
    // _fftsize = 16 * _frsize, the estimation window moves
    // by 1/4 of the FFT length.

    fi = _frindex;  // Write index in current fragment.
    r1 = _rindex1;  // Read index for current input frame.
    r2 = _rindex2;  // Second read index while crossfading. 

    // No assumptions are made about fragments being aligned
    // with process() calls, so we may be in the middle of
    // a fragment here. 

    while (nfram)
    {
        // Don't go past the end of the current fragment.
        k = _frsize - fi;
        if (nfram < k) k = nfram;
        nfram -= k;

        // At 44.1 and 48 kHz upsample by 2.
        if (_upsamp)
        {
            _resampler.inp_count = k;
            _resampler.inp_data = inp;
            _resampler.out_count = 2 * k;
            _resampler.out_data = _ipbuff + _ipindex;
            _resampler.process ();
            _ipindex += 2 * k;
        }
        // At higher sample rates apply lowpass filter.
        else
        {
            // Not implemented yet, just copy.
            memcpy (_ipbuff + _ipindex, inp, k * sizeof (float));
            _ipindex += k;
        }

        // Extra samples for interpolation.
        _ipbuff [_ipsize + 0] = _ipbuff [0];
        _ipbuff [_ipsize + 1] = _ipbuff [1];
        _ipbuff [_ipsize + 2] = _ipbuff [2];
        inp += k;
        if (_ipindex == _ipsize) _ipindex = 0;

        // Process available samples.
        dr = _ratio;
        if (_upsamp) dr *= 2;
        if (_xfade)
        {
            // Interpolate and crossfade.
            while (k--)
            {
                i = (int) r1;
		u1 = cubic (_ipbuff + i, r1 - i);
                i = (int) r2;
		u2 = cubic (_ipbuff + i, r2 - i);
                v = _xffunc [fi++];
                *out++ = (1 - v) * u1 + v * u2;
                r1 += dr;
                if (r1 >= _ipsize) r1 -= _ipsize;
                r2 += dr;
                if (r2 >= _ipsize) r2 -= _ipsize;
            }
        }
        else
        {
            // Interpolation only.
            fi += k;
            while (k--)
            {
                i = (int) r1;
		*out++ = cubic (_ipbuff + i, r1 - i);
                r1 += dr;
                if (r1 >= _ipsize) r1 -= _ipsize;
            }
        }
 
        // If at end of fragment check for jump.
        if (fi == _frsize) 
        {
            fi = 0;
            // Estimate the pitch every 4th fragment.
            if (++_frcount == 4)
            {
                _frcount = 0;
                findcycle ();
                if (_cycle)
                {
                    // If the pitch estimate succeeds, find the
                    // nearest note and required resampling ratio.
                    _count = 0;
                    finderror ();
                }
                else if (++_count > 5)
                {
                    // If the pitch estimate fails, the current
                    // ratio is kept for 5 fragments. After that
                    // the signal is considered unvoiced and the
                    // pitch error is reset.
                    _count = 5;
                    _cycle = _frsize;
                    _error = 0;
                }
                else if (_count == 2)
                {
                    // Bias is removed after two unvoiced fragments.
                    _lastnote = -1;
                }
                
                _ratio = powf (2.0f, _corroffs / 12.0f - _error * _corrgain);
            }

            // If the previous fragment was crossfading,
            // the end of the new fragment that was faded
            // in becomes the current read position.
            if (_xfade) r1 = r2;

            // A jump must correspond to an integer number
            // of pitch periods, and to avoid reading outside
            // the circular input buffer limits it must be at
            // least one fragment size.
            dr = _cycle * (int)(ceilf (_frsize / _cycle));
            dp = dr / _frsize;
            ph = r1 - _ipindex;
            if (ph < 0) ph += _ipsize;
            if (_upsamp)
            {
                ph /= 2;
                dr *= 2;
            }
            ph = ph / _frsize + 2 * _ratio - 10;
            if (ph > 0.5f)
            {
                // Jump back by 'dr' frames and crossfade.
                _xfade = true;
                r2 = r1 - dr;
                if (r2 < 0) r2 += _ipsize;
            }
            else if (ph + dp < 0.5f)
            {
                // Jump forward by 'dr' frames and crossfade.
                _xfade = true;
                r2 = r1 + dr;
                if (r2 >= _ipsize) r2 -= _ipsize;
            }
            else _xfade = false;
        }
    }

    // Save local state.
    _frindex = fi;
    _rindex1 = r1;
    _rindex2 = r2;

    return 0;
}


void Retuner::findcycle (void)
{
    int    d, h, i, j, k;
    float  f, m, t, x, y, z;
#ifdef MOD
    float rms = 0;
#endif

    d = _upsamp ? 2 : 1;
    h = _fftlen / 2;
    j = _ipindex;
    k = _ipsize - 1;
    for (i = 0; i < _fftlen; i++)
    {
        int jk = j & k;
        _fftTdata [i] = _fftTwind [i] * _ipbuff [jk];
#ifdef MOD
         rms +=  _ipbuff [jk] *  _ipbuff [jk];
#endif
        j += d;
    }
#ifdef MOD // ignore noise-floor
    if (rms < _fftlen * 0.000031623f) { // signal < -45dBFS/RMS
	_cycle = 0;
	return;
    }
#endif
    fftwf_execute_dft_r2c (_fwdplan, _fftTdata, _fftFdata);    
    f = _fsamp / (_fftlen * 3e3f);
    for (i = 0; i < h; i++)
    {
        x = _fftFdata [i][0];
        y = _fftFdata [i][1];
        m = i * f;
        _fftFdata [i][0] = (x * x + y * y) / (1 + m * m);
        _fftFdata [i][1] = 0;
    }
    _fftFdata [h][0] = 0;
    _fftFdata [h][1] = 0;
    fftwf_execute_dft_c2r (_invplan, _fftFdata, _fftTdata);    
    t = _fftTdata [0] + 0.1f;
    for (i = 0; i < h; i++) _fftTdata [i] /= (t * _fftWcorr [i]);
    x = _fftTdata [0];
    for (i = 4; i < _ifmax; i += 4)
    {
        y = _fftTdata [i];
        if (y > x) break;
        x = y;
    }
    i -= 4;
    _cycle = 0;
    if (i >= _ifmax) return;
    if (i <  _ifmin) i = _ifmin;
    x = _fftTdata [--i];
    y = _fftTdata [++i];
    m = 0;
    j = 0;
    while (i <= _ifmax)
    {
        t = y * _fftWcorr [i];
        z = _fftTdata [++i];
        if ((t >  m) && (y >= x) && (y >= z) && (y > 0.8f))
        {
            j = i - 1;
            m = t;
        }
        x = y;
        y = z;
    }
    if (j)
    {
        x = _fftTdata [j - 1];
        y = _fftTdata [j];
        z = _fftTdata [j + 1];
        _cycle = j + 0.5f * (x - z) / (z - 2 * y + x - 1e-9f);
    }
}


void Retuner::finderror (void)
{
    int    i, m, im;
    float  a, am, d, dm, f;

    if (!_notemask)
    {
        _error = 0;
        _lastnote = -1;
        return;
    }

    f = log2f (_fsamp / (_cycle * _refpitch));
    dm = 0;
    am = 1;
    im = -1;
    for (i = 0, m = 1; i < 12; i++, m <<= 1)
    {
        if (_notemask & m)
        {
            d = f - (_notescale[i] - 9) / 12.0f;
            d -= floorf (d + 0.5f);
            a = fabsf (d);
            if (i == _lastnote) a -= _notebias;
            if (a < am)
            {
                am = a;
                dm = d;
                im = i;
            }
        }
    }
    
    if (_lastnote == im)
    {
        _error += _corrfilt * (dm - _error);
    }
    else
    {
        _error = dm;
        _lastnote = im;
    }

    // For display only.
    _notebits |= 1 << im;
}


float Retuner::cubic (float *v, float a)
{
    float b, c;

    b = 1 - a;
    c = a * b;
    return (1.0f + 1.5f * c) * (v[1] * b + v[2] * a)
	    - 0.5f * c * (v[0] * b + v[1] + v[2] + v[3] * a);
}
