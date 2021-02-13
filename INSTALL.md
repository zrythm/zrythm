Installation Instructions
=========================

# Building

The project uses [meson](https://mesonbuild.com), so
the steps to configure and build are

    meson build
    ninja -C build

To pass options, use the following syntax

    meson build -Doption_name=value

To see all available options, type the following
after the build directory is initialized, or look
inside [meson_options.txt](meson_options.txt).

    meson configure build

To change an option after configuration, use

    meson build --reconfigure -Doption_name=value

To clean the build directory while keeping the
current configuration, use

    meson compile --clean -C build

## Optimization

The default build type is `debugoptmized`, which
is equivalent to `-Ddebug=true -Doptimization=2`
(`-O2 -g`). This works well in most cases. For
extremely optimized builds, we suggest building with

    meson build -Ddebug=true -Doptimization=3 -Dextra_optimizations=true

We suggest always keeping `-Ddebug=true` to assist
with meaningful stack traces and bug reports.

## Dependencies
### Required
- breeze-icons (LGPLv3+): <https://github.com/KDE/breeze-icons>
- fftw (GPLv2+): <http://www.fftw.org/>
- gtk+3 (LGPLv2.1+): <https://gitlab.gnome.org/GNOME/gtk>
- gtksourceview (LGPLv2.1+): <https://wiki.gnome.org/Projects/GtkSourceView>
- guile (GPLv3+): <https://www.gnu.org/software/guile/>
- libaudec (AGPLv3+): <https://git.zrythm.org/cgit/libaudec/>
- libcyaml (ISC): <https://github.com/tlsa/libcyaml/>
- lilv (ISC): <https://drobilla.net/software/lilv>
- reproc (Expat): <https://github.com/DaanDeMeyer/reproc>
- zstd (3-Clause BSD): <https://github.com/facebook/zstd>

### Optional
- carla (GPLv2+): <https://kx.studio/Applications:Carla>
- graphviz (EPLv1.0): <http://graphviz.org/>
- jack (LGPLv2.1+): <https://jackaudio.org/>
- lsp-dsp-lib (LGPLv3+): <https://github.com/sadko4u/lsp-dsp-lib>
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

    ninja -C build install

If you don't want to install anything permanent on
your system, you can install it somewhere
temporary by configuring with
`meson build --prefix=/tmp/zrythm` for example, and
then you can run it with
`GSETTINGS_SCHEMA_DIR=/tmp/zrythm/share/glib-2.0/schemas ./build/src/zrythm`.
The built program will be at `build/src/zrythm` by
default.

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
