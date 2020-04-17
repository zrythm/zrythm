Zrythm
======

[![translated](https://hosted.weblate.org/widgets/zrythm/-/svg-badge.svg "Translation Status")](https://hosted.weblate.org/engage/zrythm/?utm_source=widget)
[![builds.sr.ht status](https://builds.sr.ht/~alextee/zrythm.svg)](https://builds.sr.ht/~alextee/zrythm?)
[![travis build status](https://img.shields.io/travis/zrythm/zrythm?label=travis%20build)](https://travis-ci.org/zrythm/zrythm)
[![code grade](https://img.shields.io/codacy/grade/c16bdc22f6ae4e539aa6417274e71d17)](https://www.codacy.com/manual/alex-tee/zrythm)
[![code coverage](https://img.shields.io/coveralls/github/zrythm/zrythm)](https://coveralls.io/github/zrythm/zrythm)

Zrythm is a digital audio workstation designed to be
featureful and easy to use. It allows limitless
automation, supports LV2 plugins, works with the JACK
audio backend, provides chord assistance, is free
software and can be used in English, French,
Portuguese, Japanese and German.

It is written in C using the GTK+3 toolkit and uses
the meson build system.

More info can be found at https://www.zrythm.org

# Current state

Zrythm is currently in alpha. Most essential
DAW features are implemented and we are working
towards a stable release.

![screenshot](https://www.zrythm.org/static/images/apr-10-2020.png)

### Supported plugin protocols
- Full support: LV2, VST2
- Experimental: VST3

Support for all plugin protocols besides LV2 is
possible thanks to
[Carla](https://github.com/falkTX/Carla/).

### Supported backends
- Audio: JACK, SDL2, RtAudio (ALSA/WASAPI/CoreAudio)
- MIDI: JACK, WindowsMME, RtMidi (ALSA sequencer/Windows MME/CoreMIDI)

### Supported platforms
- GNU/Linux, FreeBSD, Windows, MacOS

## Building and Installation
See [INSTALL.md](INSTALL.md).

## Using
See the
[user manual](http://manual.zrythm.org/), which is
also available in PDF, epub and HTML-zip.

## Contributing
See [CONTRIBUTING.md](CONTRIBUTING.md).

## Hacking
See [HACKING.md](HACKING.md).

## Packaging
See [PACKAGING.md](PACKAGING.md).

## Mailing lists
[user@zrythm.org](https://lists.zrythm.org/lists/listinfo/user), [dev@zrythm.org](https://lists.zrythm.org/lists/listinfo/dev)

## Bug Tracker
<https://redmine.zrythm.org/projects/zrythm/issues>

## Releases
<https://www.zrythm.org/releases>

## Copying Zrythm
Zrythm is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as
published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.

The full text of the license can be found in the
[COPYING](COPYING) file.

For the copyright years, Zrythm uses a range (“2008-2010”) instead of
listing individual years (“2008, 2009, 2010”) if and only if every year
in the range, inclusive, is a “copyrightable” year that would be listed
individually.

Some files, where specified, are licensed under
different licenses.

## Trademarks

Zrythm and the Zrythm logo are trademarks of Alexandros Theodotou.

## Support
If you would like to support this project please
donate below or purchase a binary installer from
<https://www.zrythm.org/en/download.html> - creating a DAW
takes thousands of hours of work and contributions
enable us to spend more time working on the project.

- [liberapay.com/Zrythm](https://liberapay.com/Zrythm)
- [paypal.me/zrythm](https://paypal.me/zrythm)
- [opencollective.com/zrythm](https://opencollective.com/zrythm)

----

Copyright (C) 2018-2020 Alexandros Theodotou

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved.  This file is offered as-is,
without any warranty.
