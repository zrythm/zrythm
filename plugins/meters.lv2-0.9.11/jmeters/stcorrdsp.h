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


#ifndef __STCORRDSP_H
#define	__STCORRDSP_H

namespace LV2M {

class Stcorrdsp
{
public:

    Stcorrdsp (void);
    ~Stcorrdsp (void);

    void process (float *pl, float *pr, int n);  
    float read (void);

    static void init (int fsamp, float flp, float tcf); 

private:

    float          _zl;          // filter states
    float          _zr;
    float          _zlr;
    float          _zll;
    float          _zrr;

    static float   _w1;          // lowpass filter coefficient
    static float   _w2;          // correlation filter coeffient
};

};

#endif
