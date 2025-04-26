/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Rubber Band Library
    An audio time-stretching and pitch-shifting library.
    Copyright 2007-2024 Particular Programs Ltd.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.

    Alternatively, if you have a valid commercial licence for the
    Rubber Band Library obtained by agreement with the copyright
    holders, you may redistribute and/or modify it under the terms
    described in that licence.

    If you wish to distribute code using the Rubber Band Library
    under terms other than those of the GNU General Public License,
    you must obtain a valid commercial licence before doing so.
*/

/*
    RubberBandSingle.cpp
 
    This is a single-file compilation unit for Rubber Band Library.
  
    To use the library in your project without building it separately,
    include in your code either rubberband/RubberBandStretcher.h
    and/or rubberband/RubberBandLiveShifter.h for use in C++, or
    rubberband/rubberband-c.h if you need the C interface, then add
    this single C++ source file to your build.
 
    Don't move this file into your source tree - keep it in the same
    place relative to the other Rubber Band code, so that the relative
    include paths work, and just add it to your list of build files.
  
    This produces a build using the built-in FFT and resampler
    implementations, except on Apple platforms, where the vDSP FFT is
    used (and where you will need the Accelerate framework when
    linking). It should work correctly on any supported platform, but
    may not be the fastest configuration available. For further
    customisation, consider using the full build system to make a
    standalone library.
*/

#ifndef ALREADY_CONFIGURED

#define USE_BQRESAMPLER 1

#define NO_TIMING 1
#define NO_THREADING 1
#define NO_THREAD_CHECKS 1

#if defined(__APPLE__)
#define HAVE_VDSP 1
#else
#define USE_BUILTIN_FFT 1
#endif

#endif

#include "../src/faster/AudioCurveCalculator.cpp"
#include "../src/faster/CompoundAudioCurve.cpp"
#include "../src/faster/HighFrequencyAudioCurve.cpp"
#include "../src/faster/SilentAudioCurve.cpp"
#include "../src/faster/PercussiveAudioCurve.cpp"
#include "../src/common/Log.cpp"
#include "../src/common/Profiler.cpp"
#include "../src/common/FFT.cpp"
#include "../src/common/Resampler.cpp"
#include "../src/common/BQResampler.cpp"
#include "../src/common/Allocators.cpp"
#include "../src/common/StretchCalculator.cpp"
#include "../src/common/sysutils.cpp"
#include "../src/common/mathmisc.cpp"
#include "../src/common/Thread.cpp"
#include "../src/faster/StretcherChannelData.cpp"
#include "../src/faster/R2Stretcher.cpp"
#include "../src/faster/StretcherProcess.cpp"
#include "../src/finer/R3Stretcher.cpp"
#include "../src/finer/R3LiveShifter.cpp"

#include "../src/RubberBandStretcher.cpp"
#include "../src/RubberBandLiveShifter.cpp"
#include "../src/rubberband-c.cpp"

