/* -*- c-basic-offset: 4 -*-  vi:set ts=8 sts=4 sw=4: */

/* trivial_sampler.c

   DSSI Soft Synth Interface
   Constructed by Chris Cannam, Steve Harris and Sean Bolton

   A straightforward DSSI plugin sampler.

   This example file is in the public domain.
*/

#ifndef _TRIVIAL_SAMPLER_H_
#define _TRIVIAL_SAMPLER_H_

#define Sampler_OUTPUT_LEFT 0
#define Sampler_RETUNE 1
#define Sampler_BASE_PITCH 2
#define Sampler_SUSTAIN 3
#define Sampler_RELEASE 4

#define Sampler_OUTPUT_RIGHT 5
#define Sampler_BALANCE 6

#define Sampler_Mono_COUNT 5
#define Sampler_Stereo_COUNT 7

#define Sampler_BASE_PITCH_MIN 0
// not 127, as we want 120/2 = 60 as the default:
#define Sampler_BASE_PITCH_MAX 120

#define Sampler_RELEASE_MIN 0.001f
#define Sampler_RELEASE_MAX 2.0f

#define Sampler_NOTES 128
#define Sampler_FRAMES_MAX 1048576

#define Sampler_Mono_LABEL "mono_sampler"
#define Sampler_Stereo_LABEL "stereo_sampler"

#endif
