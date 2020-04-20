Installation Instructions
=========================

# Building

The project uses [meson](https://mesonbuild.com), so
the steps are

    meson build
    ninja -C build

To see all available options, type the following
after the build directory is initialized, or look
inside `meson_options.txt`.

    meson configure build

## Dependencies
### Required
- audec (AGPLv3+): <https://git.zrythm.org/cgit/libaudec/>
- GTK+3 (GPLv2+): <https://gitlab.gnome.org/GNOME/gtk>
- guile (GPLv3+): <https://www.gnu.org/software/guile/>
- GtkSourceView (LGPLv2.1+): <https://wiki.gnome.org/Projects/GtkSourceView>
- lilv (ISC): <https://drobilla.net/software/lilv>
- libcyaml (ISC): <https://github.com/tlsa/libcyaml/>
- fftw (GPLv2+): <http://www.fftw.org/>

### Optional
- jack (LGPLv2.1+): <https://jackaudio.org/>
- ffmpeg (LGPL 2.1+, GPLv2+): <https://ffmpeg.org/>
- carla (GPLv2+): <https://kx.studio/Applications:Carla>
- rtaudio (MIT): <http://www.music.mcgill.ca/~gary/rtaudio/>
- rtmidi (MIT): <https://www.music.mcgill.ca/~gary/rtmidi/>
- SDL2 (zlib): <https://www.libsdl.org/>
- libgtop (GPLv2+): <https://developer.gnome.org/libgtop/>

## Installation
Once the program is built, it will need to be
installed the first time before it can run (to
install the [GSettings](https://developer.gnome.org/gio/stable/GSettings.html))

    ninja -C build install

Alternatively if you don't want to install anything
on your system you can run
`glib-compile-schemas data/` and then run zrythm
using
`GSETTINGS_SCHEMA_DIR=data ./build/src/zrythm`.
The built program will be at `build/src/zrythm` by
default

## Running

When running Zrythm from the command line, it is
recommended to use `zrythm_launch` instead of
running the `zrythm` binary directly. This takes
care of using the correct GSettings schemas in the
installed prefix.

----

Copyright (C) 2019-2020 Alexandros Theodotou

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved.  This file is offered as-is,
without any warranty.
