
# Compiling Rubber Band Library

## Contents of this file

1. General instructions
2. Building on Linux
3. Building on macOS
4. Building for iOS
5. Building on Windows
6. Building for Android and Java integration
7. FFT and resampler selection
8. Other supported #defines
9. Copyright notes for bundled libraries


## 1. General instructions

**Full configurable build.** The primary supported build system for
Rubber Band on all platforms is Meson (https://mesonbuild.com). The
Meson build system can be used to build all targets (static and
dynamic library, command-line utility, and plugins) and to
cross-compile. See below for details.

**Single-file build.** If you want to include Rubber Band in a C++
project and would prefer not to build it as a separate library, there
is a single `.cpp` file at `single/RubberBandSingle.cpp` which can be
added to your project as-is.  It produces a single compilation-unit
build using the built-in FFT and resampler implementations with no
further library dependencies. See the comments at the top of that file
for more information.

**Other build options.** If you only need a static library and don't
wish to use Meson, some alternative build files (Makefiles and Visual
C++ projects) are included in the `otherbuilds` directory. See the
platform-specific build sections below for more details.

To build with Meson, ensure Meson and Ninja are installed and run:

```
$ meson setup build && ninja -C build
```

This checks for necessary dependencies, reports what it finds, and if
all is well, builds the code into a subdirectory called `build`. It
will build everything it can find the requisite dependencies for:
static and dynamic libraries, LADSPA, LV2, and Vamp plugins, and
command-line utility.

Some configuration options are provided, described in the
`meson_options.txt` file. To set one of these, add a `-D` option to
Meson:

```
$ meson setup build -Dipp_path=/opt/intel/ipp
```

The options are documented in the library- and platform-specific
sections below.

You can also enable or disable these optional build components:

 * `jni` - Java/JVM (JNI) interface to Rubber Band Library
 * `ladspa` - LADSPA version of example pitch-shifter plugin
 * `lv2` - LV2 version of example pitch-shifter plugin
 * `vamp` - Vamp analysis plugin, mostly used for development debugging
 * `cmdline` - The `rubberband` and `rubberband-r3` command-line utilities
 * `tests` - Unit tests

The default behaviour is to check whether the requirements for these
are found on the system and build them only if they are.

To force a component *not* to be built even when its requirements are
available, set the corresponding flag to `disabled`, e.g.

```
$ meson setup build -Djni=disabled
```

To require a component to be built (and therefore fail if its
requirements are not met), set it to `enabled`. You can also use the
special Meson `auto_features` flag to set all features to disabled
unless explicitly enabled, e.g.

```
$ meson setup build -Dauto_features=disabled -Dcmdline=enabled
```

By default the build produces both static and dynamic library
targets. To produce only one of these, use either

```
$ meson setup build -Ddefault_library=static
$ meson setup build -Ddefault_library=shared
```

Rubber Band Library is written entirely in C++ and requires a C++11
compiler. It is unlikely to make any difference (performance or
otherwise) which C++ standard you compile with, as long as it's no
older than C++11.

If you are building this software using either of the Speex or KissFFT
library options, please be sure to review the terms for those
libraries in `src/ext/speex/COPYING` and `src/ext/kissfft/COPYING` as
applicable.


## 2. Building on Linux

Optionally, if you want the command-line tool and plugins to be built,
first install libsndfile and the LADSPA, LV2, and Vamp plugin headers
so they can be found using `pkg-config`. Then

```
$ meson setup build && ninja -C build
```

See "FFT and resampler selection" below for further build options.

Alternatively, if you only need the static library and prefer a
Makefile, try

```
$ make -f otherbuilds/Makefile.linux
```


## 3. Building on macOS

Ensure the Xcode command-line tools are installed, and if you want the
command-line tool to be built, also install libsndfile.

To build for the default architecture:

```
$ meson setup build && ninja -C build
```

Which architecture is the default may depend on the version of Meson
and/or the current shell. To force a particular architecture you can
use a Meson cross-file, as follows.

To build for Apple Silicon (arm64):

```
$ meson setup build --cross-file cross/macos-arm64.txt && ninja -C build
```

To build for Intel (x86_64):

```
$ meson setup build --cross-file cross/macos-x86_64.txt && ninja -C build
```

You can build a universal binary library for both architectures like
this:

```
$ meson setup build --cross-file cross/macos-universal.txt && ninja -C build
```

Note that the universal cross file also sets the minimum OS version to
the earliest supported macOS versions for both architectures. (In
practice, compatibility will also depend on how the dependent
libraries have been compiled.)  You can edit this in the
`cross/macos-universal.txt` file if you want a specific target.

See "FFT and resampler selection" below for further build options.

Note that you cannot legally distribute applications using Rubber Band
in the Mac App Store, unless you have first obtained a commercial
licence for Rubber Band Library.  GPL code is not permitted in the app
store.  See https://breakfastquay.com/technology/license.html for
commercial terms.


## 4. Building for iOS

Ensure the Xcode command-line tools are installed, and

```
$ meson setup build_ios --cross-file cross/ios.txt && ninja -C build_ios
```

The output files will be found in the `build_ios` directory.

To build for the simulator,

```
$ meson setup build_sim --cross-file cross/ios-simulator.txt && ninja -C build_sim
```

The output files will be found in the `build_sim` directory.

See "FFT and resampler selection" below for further build options.

Note that you cannot legally distribute applications using Rubber Band
in the iOS App Store, unless you have a first obtained a commercial
licence for Rubber Band Library. GPL code is not permitted in the app
store.  See https://breakfastquay.com/technology/license.html for
commercial terms.


## 5. Building on Windows

If you only need to build the static library for integration into your
project, and you prefer a Visual Studio project file, you can find a
simple one in `otherbuilds\rubberband-library.vcxproj`.

The rest of this section describes the "full" build system, which uses
Meson, just as on the other platforms. To build this way, first ensure
Meson and Ninja are installed and available. Then, in a terminal
window with the compiler tools available in the path (e.g. a Visual
Studio command-line prompt for the relevant build architecture) run

```
> meson setup build
> ninja -C build
```

The output files will be found in the `build` directory.

The Rubber Band code is compatible with both the traditional Visual
C++ compiler (`cl`) and the Clang front-end (`clang`), and the build
system will use whichever appears (first) in your path.

To build against a specific Visual C++ runtime, use the built-in Meson
option `b_vscrt`:

```
> meson setup build -Db_vscrt=mt
```

Accepted arguments include `mt` for the static runtime (`libcmt`),
`md` for the dynamic one (`msvcrt`), `mtd` for the static debug
runtime, and `mdd` for the dynamic debug one.

See "FFT and resampler selection" below for further build options.


## 6. Building for Android and Java integration

Currently only a very old Android NDK build file is provided, as
`otherbuilds/Android.mk`. This includes compile definitions for a
shared library built for ARM architectures which can be loaded from a
Java application using the Java native interface (i.e. the Android
NDK).

The Java side of the interface can be found in
`com/breakfastquay/rubberband/RubberBandStretcher.java`.

See
https://hg.sr.ht/~breakfastquay/rubberband-android-simple-sample
for a very trivial example of integration with Android Java code.

The supplied `.mk` file uses KissFFT and the Speex resampler.


## 7. FFT and resampler selection

Rubber Band requires the selection of library code for FFT calculation
and resampling.  Several libraries are supported.  The selection is
controlled (in Meson) using `-D` options and (in the code itself)
using preprocessor flags set by the build system. These options and
flags are detailed in the tables below.

At least one resampler implementation and one FFT implementation must
be enabled. It is technically possible to enable more than one, but
it's confusing and not often useful.

If you are building this software using the bundled Speex or KissFFT
library code, please be sure to review the terms for those libraries
in `src/ext/speex/COPYING` and `src/ext/kissfft/COPYING` as applicable.

If you are proposing to package Rubber Band for a Linux distribution,
please select either the built-in FFT (the default) or FFTW
(`-Dfft=fftw`), and either the built-in resampler (the default) or
libsamplerate (`-Dresampler=libsamplerate`).

On some configurations (e.g. 32-bit platforms) libsamplerate is much
faster than the built-in resampler, and it makes a reasonable choice
for platform builds for which you don't know the requirements of any
specific application.

### FFT libraries supported

The choice of FFT library makes no difference to output quality, only
to processing speed.

```
Library     Build option    CPP define         Notes
----        ------------    ----------         -----

Built-in    -Dfft=builtin   -DUSE_BUILTIN_FFT  Default except on macOS/iOS.

vDSP        -Dfft=vdsp      -DHAVE_VDSP        Default on macOS/iOS (in
                                               the Accelerate framework).
                                               Best option on these platforms.

FFTW3       -Dfft=fftw      -DHAVE_FFTW3       A bit faster than built-in,
                                               a bit slower than vDSP.
                                               GPL with commercial option.

SLEEF       -Dfft=sleef     -DHAVE_SLEEF       Usually very fast. Not as widely
                                               distributed as FFTW3. Requires
                                               both libsleef and libsleefdft.
                                               BSD-ish licence.

KissFFT     -Dfft=kissfft   -DHAVE_KISSFFT     Single precision.
                                               Only advisable when using
                                               single-precision sample type
                                               (see below).
                                               BSD-ish licence.

Intel IPP   -Dfft=ipp       -DHAVE_IPP         Very fast on Intel hardware.
                                               Proprietary, can only be used with
                                               Rubber Band commercial licence.
```

### Resampler libraries supported

The choice of resampler affects both output quality, when
pitch-shifting, and CPU usage.

```
Library        Build option               CPP define            Notes
-------        ------------               ----------            -----

Built-in       -Dresampler=builtin        -DUSE_BQRESAMPLER     Default.
                                                                Intended to give high quality
                                                                for time-varying pitch shifts
                                                                in real-time mode.
                                                                Not the fastest option;
                                                                especially slow in 32-bit
                                                                builds.

libsamplerate  -Dresampler=libsamplerate  -DHAVE_LIBSAMPLERATE  Good choice in most cases.
                                                                High quality; usually a bit
                                                                faster than the built-in option;
                                                                much faster in 32-bit builds.
                                                                BSD-ish licence.

libspeexdsp    -Dresampler=libspeexdsp    -DHAVE_LIBSPEEXDSP    Very fast.
                                                                May not be artifact-free for
                                                                time-varying pitch shifts.
                                                                BSD-ish licence.

Bundled Speex  -Dresampler=speex          -DUSE_SPEEX           Older Speex code, bundled for
                                                                compatibility with some existing
                                                                projects.
                                                                Avoid for new projects.
```

## 8. Other supported #defines

Other known preprocessor symbols are as follows. (Usually the supplied
build files will handle these for you.)

    -DLACK_BAD_ALLOC
    Define on systems lacking std::bad_alloc in the C++ library.

    -DLACK_POSIX_MEMALIGN
    Define on systems lacking posix_memalign.

    -DUSE_OWN_ALIGNED_MALLOC
    Define on systems lacking any aligned malloc implementation.

    -DLACK_SINCOS
    Define on systems lacking sincos().
    
    -DNO_EXCEPTIONS
    Build without use of C++ exceptions.

    -DNO_THREADING
    Build without any multithread support.

    -DUSE_PTHREADS
    Use the pthreads library (required unless NO_THREADING or on Windows)

    -DPROCESS_SAMPLE_TYPE=float
    Select single precision for internal calculations. The default is
    double precision. Consider in conjunction with single-precision
    KissFFT for mobile architectures with slower double-precision
    support.

    -DUSE_POMMIER_MATHFUN
    Select the Julien Pommier implementations of trig functions for ARM
    NEON or x86 SSE architectures. These are usually faster but may be
    of lower precision than system implementations. Consider using this
    for 32-bit mobile architectures.


## 9. Copyright notes for bundled libraries

### 5a. Speex

```
[files in src/ext/speex]

Copyright 2002-2007     Xiph.org Foundation
Copyright 2002-2007     Jean-Marc Valin
Copyright 2005-2007     Analog Devices Inc.
Copyright 2005-2007     Commonwealth Scientific and Industrial Research 
                        Organisation (CSIRO)
Copyright 1993, 2002, 2006 David Rowe
Copyright 2003          EpicGames
Copyright 1992-1994     Jutta Degener, Carsten Bormann

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

- Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.

- Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.

- Neither the name of the Xiph.org Foundation nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
```

### 5b. KissFFT

```
[files in src/ext/kissfft]

Copyright (c) 2003-2004 Mark Borgerding

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.
    * Neither the author nor the names of any contributors may be used
      to endorse or promote products derived from this software
      without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
```

### 5c. Pommier math functions

```
[files in src/ext/pommier]

Copyright (C) 2011  Julien Pommier

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
```

### 5d. float_cast

```
[files in src/ext/float_cast]

Copyright (C) 2001 Erik de Castro Lopo <erikd AT mega-nerd DOT com>

Permission to use, copy, modify, distribute, and sell this file for any 
purpose is hereby granted without fee, provided that the above copyright 
and this permission notice appear in all copies.  No representations are
made about the suitability of this software for any purpose.  It is 
provided "as is" without express or implied warranty.
```

### 5e. getopt

```
[files in src/ext/getopt, used by command-line tool on some platforms]

Copyright (c) 2000 The NetBSD Foundation, Inc.
All rights reserved.

This code is derived from software contributed to The NetBSD Foundation
by Dieter Baron and Thomas Klausner.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. All advertising materials mentioning features or use of this software
   must display the following acknowledgement:
       This product includes software developed by the NetBSD
       Foundation, Inc. and its contributors.
4. Neither the name of The NetBSD Foundation nor the names of its
   contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
```

### 5f. rubberband-sharp

```
[files in rubberband-dll and rubberband-sharp]

Copyright 2018-2019 Jonathan Gilbert

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use, copy,
modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR
ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Jonathan Gilbert
shall not be used in advertising or otherwise to promote the sale,
use or other dealings in this Software without prior written
authorization.
```
