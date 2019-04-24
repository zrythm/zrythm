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


#ifndef __MSPPMDSP_H
#define	__MSPPMDSP_H

namespace LV2M {

class Msppmdsp
{
public:

    Msppmdsp (float mdb);
    ~Msppmdsp (void);

    void processM (float *pl, float *pr, int n);  
    void processS (float *pl, float *pr, int n);  
    float read (void);

    static void init (float fsamp); 

private:

    float          _z1;          // filter state
    float          _z2;          // filter state
    float          _m;           // max value since last read()
    bool           _res;         // flag to reset m
    float          _mv;          // dB offset m3, m6

    static float   _w1;          // attack filter coefficient
    static float   _w2;          // attack filter coefficient
    static float   _w3;          // release filter coefficient
    static float   _g;           // gain factor
};

};

#endif
