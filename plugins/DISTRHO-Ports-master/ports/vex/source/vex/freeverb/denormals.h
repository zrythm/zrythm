// Macro for killing denormalled numbers
//
// Written by Jezar at Dreampoint, June 2000
// http://www.dreampoint.co.uk
// Based on IS_DENORMAL macro by Jon Watte
// This code is public domain

#ifndef _denormals_
#define _denormals_

#define undenormalise(sample) { float* const _tmp_s(&sample); if (((*(unsigned int*)_tmp_s)&0x7f800000)==0) sample=0.0f; }

#endif//_denormals_

//ends
