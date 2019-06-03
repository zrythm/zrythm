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


#ifndef __RETUNER_H
#define __RETUNER_H


#include <fftw3.h>
#include "resampler.h"

namespace LV2AT {

class Retuner
{
public:

    Retuner (int fsamp);
    ~Retuner (void);

    int process (int nfram, float *inp, float *out);

    void set_refpitch (float v)
    {
        _refpitch = v;
    }

    void set_notebias (float v)
    {
        _notebias = v / 13.0f;
    }

    void set_corrfilt (float v)
    {
        _corrfilt = (4 * _frsize) / (v * _fsamp);
    }

    void set_corrgain (float v)
    {
        _corrgain = v;
    }

    void set_corroffs (float v)
    {
        _corroffs = v;
    }

    void set_notemask (int k)
    {
        _notemask = k;
    }

    void set_notescale (int i, float v)
    {
        _notescale[i] = i + v;
    }

    int get_noteset (void)
    {
        int k;

        k = _notebits;
        _notebits = 0;
        return k;
    }

    float get_error (void)
    {
        return 12.0f * _error;
    }


private:

    void  findcycle (void);
    void  finderror (void);
    float cubic (float *v, float a);

    int              _fsamp;
    int              _ifmin;
    int              _ifmax;
    bool             _upsamp;
    int              _fftlen;
    int              _ipsize;
    int              _frsize;
    int              _ipindex;
    int              _frindex;
    int              _frcount;
    float            _refpitch;
    float            _notebias;
    float            _corrfilt; 
    float            _corrgain;
    float            _corroffs;
    int              _notemask;
    int              _notebits;
    int              _lastnote;
    int              _count;
    float            _cycle;
    float            _error;
    float            _ratio;
    float            _phase;
    bool             _xfade;
    float            _rindex1;
    float            _rindex2;
    float           *_ipbuff;
    float           *_xffunc;
    float           *_fftTwind;
    float           *_fftWcorr;
    float           *_fftTdata;
    fftwf_complex   *_fftFdata;
    fftwf_plan       _fwdplan;
    fftwf_plan       _invplan;
		LV2AT::Resampler _resampler;

    float            _notescale[12];

};

};

#endif
