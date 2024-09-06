.. SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex at zrythm dot org>
.. SPDX-License-Identifier: FSFAP

Installation Instructions
=========================

Prerequisites
-------------

Build Tools
~~~~~~~~~~~

You will need the GNU toolchain, a C++17-compatible compiler
and the following

`meson (Apache-2.0) <https://mesonbuild.com/>`_
  Build system

`sassc (Expat) <https://github.com/sass/sassc>`_
  CSS compilation

`lilv (ISC) <https://drobilla.net/software/lilv>`_
  LV2 plugin hosting library used in tests only

.. `blueprint-compiler (LGPLv3+) <https://gitlab.gnome.org/jwestman/blueprint-compiler>`_
..   UI file compilation

If your meson version is too old, you can either
install meson from
`pip <https://pypi.org/project/pip/>`_
or run ``meson.py`` directly from
`meson's source code <https://github.com/mesonbuild/meson>`_.

Dependencies
~~~~~~~~~~~~

The following dependencies must be installed before
attempting to build or run Zrythm.

Required
++++++++

`carla (GPLv2+) <https://kx.studio/Applications:Carla>`_
  Support for various plugin formats

`fftw (GPLv2+) <http://www.fftw.org/>`_
  Threaded fftw for plugins

`gtk4 (LGPLv2.1+) <https://gtk.org/>`_
  GUI toolkit

`libadwaita (LGPLv2.1+) <https://gitlab.gnome.org/GNOME/libadwaita>`_
  Additional widgets

Note: if you want to avoid the ``appstream`` dependency use `our fork <https://gitlab.zrythm.org/zrythm/libadwaita>`_ (``1.5.3-without-appstream`` branch).

`libcurl (X11) <https://curl.se/libcurl/>`_
  Network connections (GNU/Linux only)

`libdw from elfutils (GPLv2+) <https://sourceware.org/elfutils/>`_
  Backtraces (GNU/Linux only)

`libpanel (LGPLv3+) <https://gitlab.gnome.org/chergert/libpanel/>`_
  Dock and panel widgets

Note: if you want detach support use `our fork <https://gitlab.zrythm.org/zrythm/libpanel-detach>`_ (``zrythm_detach`` branch).

`libsndfile (LGPLv2.1+) <http://libsndfile.github.io/libsndfile/>`_
  Audio file IO

`magic_enum (MIT) <https://github.com/Neargye/magic_enum>`_
  Reflection for C++ enums

`rubberband (GPLv2+) <https://breakfastquay.com/rubberband/>`_
  Time-stretching and pitch-shifting

`soxr (LGPLv2.1+) <https://sourceforge.net/projects/soxr/>`_
  Resampling

`vamp-plugin-sdk (X11) <https://vamp-plugins.org/>`_
  Audio analysis

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

`lsp-dsp-lib (LGPLv3+) <https://github.com/sadko4u/lsp-dsp-lib>`_
  SIMD-optimized signal processing

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

    meson setup build -Doption_name=value

To see all available options, type the following
after the build directory is initialized, or look
inside `meson_options.txt <meson_options.txt>`_.
Built-in meson options can be found
`here <https://mesonbuild.com/Builtin-options.html>`_::

    meson configure build

To change an option after configuration, use::

    meson configure build -Doption_name=value

To change environment variables (such as ``CC`` and
``CXX``) while keeping the current configuration, use::

    MY_ENV_VARIABLE=myvalue meson build --wipe

To start from scratch, remove the ``build`` directory::

    rm -rf build

Optimization
~~~~~~~~~~~~

The default build type is ``debugoptmized``, which
is equivalent to ``-Ddebug=true -Doptimization=2``
(``-O2 -g``). This works well in most cases. For
extremely optimized builds, we suggest building with
the following options::

    -Ddebug=true -Doptimization=3 -Dextra_optimizations=true -Dnative_build=true

We suggest always keeping ``-Ddebug=true`` to assist
with meaningful stack traces and bug reports.

Compilation
-----------

Compile after configuring the build directory::

    meson compile -C build

To clean the build directory while keeping the
current configuration, use::

    meson compile --clean -C build

Installation
------------

Once the program is built, it will need to be
installed the first time before it can run (to
install the `GSettings <https://developer.gnome.org/gio/stable/GSettings.html>`_ among other things)::

    meson install -C build

If you don't want to install anything permanent on
your system, you can install it somewhere
temporary by configuring with
``--prefix=/tmp/zrythm`` for example, and
then you can run it with
``/tmp/zrythm/bin/zrythm_launch``.

Running
-------

When running Zrythm from the command line, it is
recommended to use ``zrythm_launch`` instead of
running the ``zrythm`` binary directly. This takes
care of using the correct GSettings schemas and
other resources in the installed prefix.

For debugging and other developer tools, see
`HACKING.md <HACKING.md>`_.
