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

#ifndef __KMETERDSP_H
#define	__KMETERDSP_H

#include "jmeterdsp.h"

namespace LV2M {

class Kmeterdsp : public JmeterDSP
{
public:

    Kmeterdsp (void);
    ~Kmeterdsp (void);

    void process (float *p, int n);
    float read (void);
    void read (float &rms, float &peak);
    void reset (void);

    void init (float fsamp);

private:

		float          _z1;          // filter state
		float          _z2;          // filter state
		float          _rms;         // max rms value since last read()
		float          _peak;        // max peak value since last read()
		int            _cnt;	       // digital peak hold counter
		int            _fpp;	       // frames per period
		float          _fall;        // peak fallback
		bool           _flag;        // flag set by read(), resets _rms

		static float   _omega;       // ballistics filter constant.
		static int     _hold;        // peak hold timeoute
		static float   _fsamp;       // sample-rate

};

};

#endif
