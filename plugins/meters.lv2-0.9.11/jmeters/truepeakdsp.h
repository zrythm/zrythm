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



#ifndef __TRUEPEAKDSP_H
#define	__TRUEPEAKDSP_H

#include "jmeterdsp.h"
#include "../zita-resampler/resampler.h"

namespace LV2M {

class TruePeakdsp : public JmeterDSP
{
public:

    TruePeakdsp (void);
    ~TruePeakdsp (void);

    void process (float *p, int n);
    void process_max (float *p, int n);
    float read (void);
    void  read (float &m, float &p);
    void  reset (void);

    void init (float fsamp);

private:

    float      _m;
    float      _p;
    float      _z1;
    float      _z2;
    bool       _res;
		float     *_buf;
		Resampler  _src;

    float   _w1;  // attack filter coefficient
    float   _w2;  // attack filter coefficient
    float   _w3;  // release filter coefficient
    float   _g;   // gain factor
};

};


#endif
