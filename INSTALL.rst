.. SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex at zrythm dot org>
.. SPDX-License-Identifier: FSFAP

Installation Instructions
=========================

Prerequisites
-------------

Build Tools
~~~~~~~~~~~

You will need the GNU toolchain, a C/C++ compiler
and the following

`meson (Apache-2.0) <https://mesonbuild.com/>`_
  Build system

`guile (GPLv3+) <https://www.gnu.org/software/guile/>`_
  Used in various build scripts

`sassc (Expat) <https://github.com/sass/sassc>`_
  CSS compilation

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
`breeze-icons (LGPLv3+) <https://github.com/KDE/breeze-icons>`_
  Various icons

`carla (GPLv2+) <https://kx.studio/Applications:Carla>`_
  Support for various plugin formats

`fftw (GPLv2+) <http://www.fftw.org/>`_
  Threaded fftw for plugins

`gtk4 (LGPLv2.1+) <https://gtk.org/>`_
  GUI toolkit

`gtksourceview (LGPLv2.1+) <https://wiki.gnome.org/Projects/GtkSourceView>`_
  Source code editor widget

`json-glib (LGPLv2.1+) <https://wiki.gnome.org/Projects/JsonGlib>`_
  JSON manipulation

`libadwaita (LGPLv2.1+) <https://gitlab.gnome.org/GNOME/libadwaita>`_
  Additional widgets

`libaudec (AGPLv3+) <https://git.zrythm.org/zrythm/libaudec/>`_
  Audio file loading

`libbacktrace (3-Clause BSD) <https://github.com/ianlancetaylor/libbacktrace>`_
  Backtraces

`libcurl (X11) <https://curl.se/libcurl/>`_
  Network connections

`libcyaml (ISC) <https://github.com/tlsa/libcyaml/>`_
  Serialization into YAML

`lilv (ISC) <https://drobilla.net/software/lilv>`_
  LV2 host support

`pcre2 (3-Clause BSD) <https://www.pcre.org/>`_
  Regex

`reproc (Expat) <https://github.com/DaanDeMeyer/reproc>`_
  Process launching

`rubberband (GPLv2+) <https://breakfastquay.com/rubberband/>`_
  Time-stretching and pitch-shifting

`vamp-plugin-sdk (X11) <https://vamp-plugins.org/>`_
  Audio analysis

`xxhash (2-Clause BSD) <https://cyan4973.github.io/xxHash/>`_
  Hashing

`zstd (3-Clause BSD) <https://github.com/facebook/zstd>`_
  Compression

Recommended
+++++++++++

`boost (Boost) <https://www.boost.org/>`_
  C++ utilities required for bundled plugins

`jack (LGPLv2.1+) <https://jackaudio.org/>`_
  Low latency audio/MIDI backend

`lsp-dsp-lib (LGPLv3+) <https://github.com/sadko4u/lsp-dsp-lib>`_
  SIMD-optimized signal processing

`libguile (GPLv3+) <https://www.gnu.org/software/guile/>`_
  Scripting support

Optional
++++++++

`graphviz (EPLv1.0) <http://graphviz.org/>`_
  Process graph export

`rtaudio (Expat) <http://www.music.mcgill.ca/~gary/rtaudio/>`_
  Various audio backends

`rtmidi (Expat) <https://www.music.mcgill.ca/~gary/rtmidi/>`_
  Various MIDI backends

`SDL2 (zlib) <https://www.libsdl.org/>`_
  Audio backend

Dependency package names for various distros
can be found `here <https://git.sr.ht/~alextee/zrythm-builds/tree/master/item/.builds>`_
and `here <https://git.sr.ht/~alextee/zrythm-builds2/tree/master/item/.builds>`_

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
