Zrythm
======

[![translated](https://hosted.weblate.org/widgets/zrythm/-/svg-badge.svg "Translation Status")](https://hosted.weblate.org/engage/zrythm/?utm_source=widget)

Zrythm is a highly automated Digital Audio Workstation (DAW) designed to be featureful and intuitive to use. Zrythm sets itself apart from other DAWs by allowing extensive automation via built-in LFOs and envelopes and intuitive MIDI or audio editing and arranging via clips.

In the usual Composing -> Mixing -> Mastering workflow, Zrythm puts the most focus on the Composing part. It allows musicians to quickly lay down and process their musical ideas without taking too much time for unnecessary work.

It is written in C and uses the GTK+3 toolkit, with bits and pieces taken from other programs like Ardour and Jalv.

More info at https://www.zrythm.org

# Current state

Zrythm is currently in alpha.
![screenshot 1](https://www.zrythm.org/static/images/may_28_2019.png)
![screenshot 2](https://www.zrythm.org/static/images/may_28_2019_2.png)

## Currently supported plugin protocols:
- LV2

# Building

The project uses meson, so the steps are

    meson _build
    ninja -C _build

## Dependencies
### Required
TODO make this a table (name|arch pkg name|license|upstream URL|use)
- GTK+3 (library GPLv2+): https://gitlab.gnome.org/GNOME/gtk
- jack (LGPLv2.1+): http://jackaudio.org/
- lv2 (ISC): http://lv2plug.in/
- lilv (ISC): https://drobilla.net/software/lilv
- libsndfile (LGPLv3): http://www.mega-nerd.com/libsndfile
- libyaml
- libsamplerate (2-clause BSD): http://www.mega-nerd.com/libsamplerate

### Optional
- portaudio (MIT): www.portaudio.com/
- ffmpeg (LGPL 2.1+, GPL 2+): https://ffmpeg.org/
- Qt5

## Installation
Once the program is built, it will need to be installed the first time before it can run (to install the GSettings)

    ninja -C _build install

Alternatively if you don't want to install anything on your system you can run `glib-compile-schemas data/` and then run zrythm using `GSETTINGS_SCHEMA_DIR=data ./_build/src/zrythm`. The built program will be at `_build/src/zrythm` by default

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

## Packages
For easy package installation
see
[Installation](https://manual.zrythm.org/en/configuration/installation/intro.html) in the manual.

## Using
At the moment, Zrythm works with Jack (recommended) and ALSA. For Jack setup instructions see https://libremusicproduction.com/articles/demystifying-jack-%E2%80%93-beginners-guide-getting-started-jack

For more information see the [manual](https://manual.zrythm.org).

## Contributing
For contributing guidelines see [CONTRIBUTING.md](CONTRIBUTING.md). Be sure to take a look at the
[Developer Docs](https://docs.zrythm.org)

For any bugs please raise an issue or join a chatroom below

## Chatrooms
### Matrix/IRC (Freenode)
`#zrythm` channel (for Matrix users `#zrythmdaw:matrix.org`)

## Mailing lists
zrythm-dev@nongnu.org for developers, zrythm-user@nongnu.org for users

## License
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as
published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

The full text of the license can be found in the
[COPYING](COPYING) file.

For the copyright years, Zrythm uses a range (“2008-2010”) instead of
listing individual years (“2008, 2009, 2010”) if and only if every year
in the range, inclusive, is a “copyrightable” year that would be listed
individually.

## Support
We appreciate contributions of any size -- donations enable us to spend more time working on the project, and help cover our infrastructure expenses.

### Paypal
https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=LZWVK6228PQGE&source=url
### LiberaPay
https://liberapay.com/Zrythm/
### Bitcoin
bc1qjfyu2ruyfwv3r6u4hf2nvdh900djep2dlk746j

----

Copyright (C) 2018-2019 Alexandros Theodotou

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved.  This file is offered as-is,
without any warranty.
