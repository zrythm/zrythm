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
  - GTK+3 (library GPLv2+): <https://gitlab.gnome.org/GNOME/gtk>
  - jack (LGPLv2.1+): <https://jackaudio.org/>
  - lv2 (ISC): <https://lv2plug.in/>
  - lilv (ISC): <https://drobilla.net/software/lilv>
  - libsndfile (LGPLv3): <https://www.mega-nerd.com/libsndfile>
  - libyaml (MIT): <https://pyyaml.org/wiki/LibYAML>
  - libsamplerate (2-clause BSD): <http://www.mega-nerd.com/libsamplerate>
  - fftw (GPLv2): <http://www.fftw.org/>
  - suil (ISC): <http://drobilla.net/software/suil/>

### Optional
  - portaudio (MIT): <http://www.portaudio.com/>
  - ffmpeg (LGPL 2.1+, GPLv2+): <https://ffmpeg.org/>

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

Copyright (C) 2019 Alexandros Theodotou

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved.  This file is offered as-is,
without any warranty.
