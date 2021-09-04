Installation Instructions
=========================

# Building

The project uses [meson](https://mesonbuild.com), so
the steps to configure and build are

    meson build
    meson compile -C build

To pass options, use the following syntax

    meson build -Doption_name=value

To see all available options, type the following
after the build directory is initialized, or look
inside [meson_options.txt](meson_options.txt).
Built-in meson options can be found
[here](https://mesonbuild.com/Builtin-options.html).

    meson configure build

To change an option after configuration, use

    meson configure build -Doption_name=value

To clean the build directory while keeping the
current configuration, use

    meson compile --clean -C build

To change environment variables (such as `CC` and
`CXX`) while keeping the current configuration, use

    MY_ENV_VARIABLE=myvalue meson build --wipe

To start from scratch, remove the `build` directory

    rm -rf build

*Hint: If your distro's meson package is too old,
you can either install meson from
[pip](https://pypi.org/project/pip/) or run
`meson.py` directly from
[meson's source code](https://github.com/mesonbuild/meson).*

## Optimization

The default build type is `debugoptmized`, which
is equivalent to `-Ddebug=true -Doptimization=2`
(`-O2 -g`). This works well in most cases. For
extremely optimized builds, we suggest building with

    meson build -Ddebug=true -Doptimization=3 -Dextra_optimizations=true -Dnative_build=true

We suggest always keeping `-Ddebug=true` to assist
with meaningful stack traces and bug reports.

## Dependencies
### Required
- breeze-icons (LGPLv3+): <https://github.com/KDE/breeze-icons>
- fftw (GPLv2+): <http://www.fftw.org/>
- gtk+3 (LGPLv2.1+): <https://gitlab.gnome.org/GNOME/gtk>
- gtksourceview (LGPLv2.1+): <https://wiki.gnome.org/Projects/GtkSourceView>
- guile (GPLv3+): <https://www.gnu.org/software/guile/>
- json-glib (LGPLv2.1+): <https://wiki.gnome.org/Projects/JsonGlib>
- libaudec (AGPLv3+): <https://git.zrythm.org/zrythm/libaudec/>
- libbacktrace (3-Clause BSD): <https://github.com/ianlancetaylor/libbacktrace>
- libcyaml (ISC): <https://github.com/tlsa/libcyaml/>
- lilv (ISC): <https://drobilla.net/software/lilv>
- pcre2 (3-Clause BSD): <https://www.pcre.org/>
- reproc (Expat): <https://github.com/DaanDeMeyer/reproc>
- vamp-plugin-sdk (X11): <https://vamp-plugins.org/>
- xxhash (2-Clause BSD): <https://cyan4973.github.io/xxHash/>
- zstd (3-Clause BSD): <https://github.com/facebook/zstd>

### Recommended
- carla (GPLv2+): <https://kx.studio/Applications:Carla>
- jack (LGPLv2.1+): <https://jackaudio.org/>
- lsp-dsp-lib (LGPLv3+): <https://github.com/sadko4u/lsp-dsp-lib>

### Optional
- graphviz (EPLv1.0): <http://graphviz.org/>
- rtaudio (Expat): <http://www.music.mcgill.ca/~gary/rtaudio/>
- rtmidi (Expat): <https://www.music.mcgill.ca/~gary/rtmidi/>
- SDL2 (zlib): <https://www.libsdl.org/>

*Tip: dependency package names for various distros
can be found [here](https://git.sr.ht/~alextee/zrythm-builds/tree/master/item/.builds)
and [here](https://git.sr.ht/~alextee/zrythm-builds2/tree/master/item/.builds)*

## Installation
Once the program is built, it will need to be
installed the first time before it can run (to
install the [GSettings](https://developer.gnome.org/gio/stable/GSettings.html) among other things).

    meson install -C build

If you don't want to install anything permanent on
your system, you can install it somewhere
temporary by configuring with
`meson build --prefix=/tmp/zrythm` for example, and
then you can run it with
`/tmp/zrythm/bin/zrythm_launch`.

## Running

When running Zrythm from the command line, it is
recommended to use `zrythm_launch` instead of
running the `zrythm` binary directly. This takes
care of using the correct GSettings schemas and
other resources in the installed prefix.

----

Copyright (C) 2019-2021 Alexandros Theodotou

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved.  This file is offered as-is,
without any warranty.
