Installation Instructions
=========================

# Building

The project uses meson, so the steps are

    meson build
    ninja -C build

To see all available options, type the following
after the build directory is initialized, or look
inside `meson_options.txt`.

    meson configure build

## Dependencies
### Required
- GTK+3 (GPLv2+): <https://gitlab.gnome.org/GNOME/gtk>
- audec (AGPLv3+): <https://git.zrythm.org/cgit/libaudec/>
- lilv (ISC): <https://drobilla.net/software/lilv>
- libcyaml (ISC): <https://github.com/tlsa/libcyaml/>
- fftw (GPLv2+): <http://www.fftw.org/>

### Optional
- jack (LGPLv2.1+): <https://jackaudio.org/>
- ffmpeg (LGPL 2.1+, GPLv2+): <https://ffmpeg.org/>
- carla (GPLv2+): <https://kx.studio/Applications:Carla>
- guile (GPLv3+): <https://www.gnu.org/software/guile/>
- rtaudio (MIT): <http://www.music.mcgill.ca/~gary/rtaudio/>
- rtmidi (MIT): <https://www.music.mcgill.ca/~gary/rtmidi/>
- SDL2 (zlib): <https://www.libsdl.org/>
- libgtop (GPLv2+): <https://developer.gnome.org/libgtop/>

## Installation
Once the program is built, it will need to be
installed the first time before it can run (to
install the GSettings)

    ninja -C build install

Alternatively if you don't want to install anything
on your system you can run
`glib-compile-schemas data/` and then run zrythm
using
`GSETTINGS_SCHEMA_DIR=data ./build/src/zrythm`.
The built program will be at `build/src/zrythm` by
default

## Non-standard locations

When installing in non-standard locations, glib
needs to find the corresponding gsettings schema.
At runtime, GSettings looks for schemas in the
`glib-2.0/schemas` subdirectories of all directories
specified in the `XDG_DATA_DIRS`.
It is possible to set
the `GSETTINGS_SCHEMA_DIR` environment variable to
`<your prefix>/share/glib-2.0/schemas` or prepend
`XDG_DATA_DIRS` with `<your prefix>/share` before
running `<your prefix>/bin/zrythm` to make glib
use the schema installed in the custom location.

There are also translations installed in the custom
location so `XDG_DATA_DIRS` might be a better idea.

Generally, we recommend installing under `/usr/local`
(default) or `/usr` to avoid these problems.

----

Copyright (C) 2019-2020 Alexandros Theodotou

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved.  This file is offered as-is,
without any warranty.
