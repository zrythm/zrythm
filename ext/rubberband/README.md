
# Rubber Band Library

An audio time-stretching and pitch-shifting library and utility program.

Written by Chris Cannam, chris.cannam@breakfastquay.com.
Published by Particular Programs Ltd t/a Breakfast Quay.
Copyright 2007-2024 Particular Programs Ltd.

Rubber Band is a library and utility program that permits changing the
tempo and pitch of an audio recording independently of one another.

* About Rubber Band: https://breakfastquay.com/rubberband/
* Code repository: https://hg.sr.ht/~breakfastquay/rubberband
* Github mirror: https://github.com/breakfastquay/rubberband

CI builds:

* [![Build status](https://builds.sr.ht/~breakfastquay/rubberband.svg)](https://builds.sr.ht/~breakfastquay/rubberband?) (Linux)
* [![Build Status](https://github.com/breakfastquay/rubberband/workflows/macOS%20and%20iOS%20CI/badge.svg)](https://github.com/breakfastquay/rubberband/actions?query=workflow%3A%22macOS+and+iOS+CI%22) (macOS, iOS)
* [![Build Status](https://github.com/breakfastquay/rubberband/workflows/Windows%20CI/badge.svg)](https://github.com/breakfastquay/rubberband/actions?query=workflow%3A%22Windows+CI%22) (Windows)


## Licence

Rubber Band Library is distributed under the GNU General Public
License (GPL). You can redistribute it and/or modify it under the
terms of the GPL; either version 2 of the License, or (at your option)
any later version. See the file COPYING for more information.

If you wish to distribute code using Rubber Band Library under terms
other than those of the GNU General Public License, you must obtain a
commercial licence from us before doing so. In particular, you may not
legally distribute through any Apple App Store unless you have a
commercial licence.  See https://breakfastquay.com/rubberband/ for
licence terms.

If you have obtained a valid commercial licence, your licence
supersedes this README and the enclosed COPYING file and you may
redistribute and/or modify Rubber Band under the terms described in
that licence. Please refer to your licence agreement for more details.

Rubber Band includes a .NET interface generously contributed by
Jonathan Gilbert under a BSD-like licence. The files in the
`dotnet/rubberband-dll` and `dotnet/rubberband-sharp` directories fall
under this licence. If you make use of this interface, please ensure
you comply with the terms of its licence.

Rubber Band may link with other libraries or be compiled with other
source code, depending on its build configuration. It is your
responsibility to ensure that you redistribute these only in
accordance with their own licence terms, regardless of the conditions
under which you are redistributing the Rubber Band code itself. The
licences for some relevant library code are as follows, to the best of
our knowledge. See also the file [COMPILING.md](COMPILING.md) for more
details.

 * FFTW3 - GPL with commercial proprietary option
 * Intel IPP - Proprietary of some nature
 * SLEEF - BSD-like
 * KissFFT - BSD-like
 * libsamplerate - BSD-like from version 0.1.9 onwards
 * Speex - BSD-like
 * Pommier math functions - BSD-like
 

## Compiling Rubber Band Library

Please refer to the file [COMPILING.md](COMPILING.md) for details of
how to configure and build the library.


## Contents of this README

1. Code components
2. Using the Rubber Band command-line tool
3. Using Rubber Band Library


## 1. Code components

Rubber Band consists of:

 * The Rubber Band Library code.  This is the code that will normally
   be used by your applications.  The headers for this are in the
   `rubberband/` directory, and the source code is in `src/`.
   The Rubber Band Library may also depend upon external resampler
   and FFT code, if so configured; see section 7 of COMPILING.md for
   details.

 * The Rubber Band command-line tool.  This is in `main/main.cpp`.
   This program uses Rubber Band Library and also requires libsndfile
   (http://www.mega-nerd.com/libsndfile/, licensed under the GNU Lesser
   General Public License) for audio file loading.

 * A pitch-shifter audio effects plugin in LADSPA and LV2 formats.
   These are in `ladspa-lv2/`. They require the LADSPA SDK header
   `ladspa.h` and LV2 header `lv2.h` respectively (not included).

 * A Vamp audio analysis plugin which may be used to inspect the
   dynamic stretch ratios and other decisions taken by the Rubber Band
   Library when in use.  This is in `vamp/`.  It requires the Vamp
   plugin SDK (https://www.vamp-plugins.org/develop.html) (not included).


## 2. Using the Rubber Band command-line tool

The Rubber Band command-line tool builds as `build/rubberband`.  The
basic incantation is

```
  $ rubberband -t <timeratio> -p <semitones> <infile.wav> <outfile.wav>
```

For example,

```
  $ rubberband -t 1.5 -p 2.0 test.wav output.wav
```

stretches the file `test.wav` to 50% longer than its original
duration, shifts it up in pitch by a whole tone, and writes the output
to `output.wav`.

Several further options are available: run `rubberband -h` for help.

The most important are the options `-2` and `-3`. These select between
two different processing engines, known as the R2 (Faster) engine and
the R3 (Finer) engine. The R3 engine produces higher-quality results
than R2 for most material, especially complex mixes, vocals and other
sounds that have soft onsets and smooth pitch changes, and music with
substantial bass content. However, it uses much more CPU than the R2
engine.

The R2 engine was the only method available in Rubber Band Library up
to versions 2.x, and for compatibility it remains the default (in the
case that neither `-2` nor `-3` is requested explicitly) whenever the
command-line tool is invoked as `rubberband`. The R3 engine is the
default if the tool is invoked as `rubberband-r3`.

Many further options are available, most of which only have an effect
when using the R2 engine.  In particular, different types of music may
benefit from different "crispness" options (`-c` flag, with a
numerical argument from 0 to 6).


## 3. Using Rubber Band Library

Rubber Band has a public API that consists of two C++ classes living
in the `RubberBand` namespace, called `RubberBandStretcher` and
`RubberBandLiveShifter`. The former is the main Rubber Band class for
general time and pitch manipulation, the latter a simpler API for
block-by-block pitch-shifting.  You should `#include
<rubberband/RubberBandStretcher.h>` or
`<rubberband/RubberBandLiveShifter.h>` to use these classes.  There is
extensive documentation in the headers.

A header with C language bindings is also provided in
`<rubberband/rubberband-c.h>`.  This is a wrapper around the C++
implementation, and as the implementation is the same, it also
requires linkage against the C++ standard libraries.  It is not yet
documented separately from the C++ headers.  You should include either
C++ or C headers, not both.

A .NET interface is also included, contributed by Jonathan Gilbert;
see the files in the `dotnet` directory for details.

The source code for the command-line utility (`main/main.cpp`)
provides an example of how to use Rubber Band in offline mode; the
pitch shifter plugins (in `ladspa-lv2`) may be used examples of Rubber
Band in real-time mode.

**IMPORTANT:** Please ensure you have read and understood the
licensing terms for Rubber Band before using it in your application.
This library is provided under the GNU General Public License, which
means that any application that uses it must also be published under
the GPL or a compatible licence (i.e. with its full source code also
available for modification and redistribution) unless you have
separately acquired a commercial licence from the author.


## 4. Further documentation

 * The [API documentation](https://breakfastquay.com/rubberband/code-doc/index.html) is thorough and we encourage you to read it
 * [Conceptual notes and examples](https://breakfastquay.com/rubberband/integration.html) for integration into an application
 * [Help text](https://breakfastquay.com/rubberband/usage.txt) of the command-line application
 * [Rubber Band Library home page](https://breakfastquay.com/rubberband/)
 
