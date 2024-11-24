.. SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex at zrythm dot org>
.. SPDX-License-Identifier: FSFAP

Installation Instructions
=========================

Prerequisites
-------------

Build Tools
~~~~~~~~~~~

You will need a C++20-compatible compiler and the following

`CMake (BSD-3-Clause) <https://cmake.org/>`_
  Build system

`lilv (ISC) <https://drobilla.net/software/lilv>`_
  LV2 plugin hosting library used in tests only

Dependencies
~~~~~~~~~~~~

The following dependencies must be installed before
attempting to build or run Zrythm.

Required
++++++++

.. `carla (GPLv2+) <https://kx.studio/Applications:Carla>`_
..   Support for various plugin formats

`fmt (MIT) <https://github.com/fmtlib/fmt>`_
  Formatting library

`fftw (GPLv2+) <http://www.fftw.org/>`_
  Threaded fftw for plugins

`libcurl (X11) <https://curl.se/libcurl/>`_
  Network connections (GNU/Linux only)

`libdw from elfutils (GPLv2+) <https://sourceware.org/elfutils/>`_
  Backtraces (GNU/Linux only)

`libsndfile (LGPLv2.1+) <http://libsndfile.github.io/libsndfile/>`_
  Audio file IO

`magic_enum (MIT) <https://github.com/Neargye/magic_enum>`_
  Reflection for C++ enums

`rubberband (GPLv2+) <https://breakfastquay.com/rubberband/>`_
  Time-stretching and pitch-shifting

`soxr (LGPLv2.1+) <https://sourceforge.net/projects/soxr/>`_
  Resampling

`spdlog (MIT) <https://github.com/gabime/spdlog>`_
  Logging

.. `vamp-plugin-sdk (X11) <https://vamp-plugins.org/>`_
..   Audio analysis

`xxhash (2-Clause BSD) <https://cyan4973.github.io/xxHash/>`_
  Hashing

`yyjson (MIT) <https://github.com/ibireme/yyjson>`_
  JSON manipulation

`zstd (3-Clause BSD) <https://github.com/facebook/zstd>`_
  Compression

Recommended
+++++++++++

`boost (Boost) <https://www.boost.org/>`_
  C++ utilities required for building the bundled plugins

`jack (LGPLv2.1+) <https://jackaudio.org/>`_
  Low latency audio/MIDI backend

`rtaudio (Expat) <http://www.music.mcgill.ca/~gary/rtaudio/>`_
  Various audio backends

`rtmidi (Expat) <https://www.music.mcgill.ca/~gary/rtmidi/>`_
  Various MIDI backends

Optional
++++++++

`graphviz (EPLv1.0) <http://graphviz.org/>`_
  Process graph export (only used for debugging)

`libcyaml (ISC) <https://github.com/tlsa/libcyaml/>`_
  Serialization into YAML (only needed to upgrade old projects created before `v1.0.0-beta.5.0`)

Configuration
-------------

Configure the build directory, optionally passing options::

    cmake -B builddir -DOPTION_NAME=value

To see all available options, look inside `CMakeLists.txt <CMakeLists.txt>`_.
Only options that begin with ``ZRYTHM_`` are meant to configurable by users.
For built-in CMake options see the official CMake documentation.

Recommended Options
~~~~~~~~~~~~~~~~~~~

For optimized builds, use ``-DCMAKE_BUILD_TYPE=RelWithDebInfo`` (release with debug info).

We suggest always keeping debug info enabled so that you can obtain meaningful
stack traces for bug reports.

Compilation
-----------

Compile after configuring the build directory::

    cmake --build builddir

Installation
------------

Install after compiling::

    cmake --install builddir

Running
-------

When running Zrythm from the command line, it is recommended to use ``zrythm_launch`` instead of
running the ``zrythm`` binary directly. This takescare of using the correct GSettings schemas and
other resources in the installed prefix.

For debugging and other developer tools, see `HACKING.md <HACKING.md>`_.
