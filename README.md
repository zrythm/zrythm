Zrythm
======

[![translated](https://hosted.weblate.org/widgets/zrythm/-/svg-badge.svg "Translation Status")](https://hosted.weblate.org/engage/zrythm/?utm_source=widget)
[![builds.sr.ht status](https://builds.sr.ht/~alextee/zrythm.svg)](https://builds.sr.ht/~alextee/zrythm?)
[![code grade](https://img.shields.io/codacy/grade/c16bdc22f6ae4e539aa6417274e71d17)](https://www.codacy.com/manual/alex-tee/zrythm)
[![code coverage](https://img.shields.io/coveralls/github/zrythm/zrythm)](https://coveralls.io/github/zrythm/zrythm)

Zrythm is a digital audio workstation designed to be
featureful and easy to use. It allows limitless
automation through curves, LFOs and envelopes,
supports multiple plugin formats including LV2, VST2
and VST3, works with multiple backends including
JACK, RtAudio/RtMidi and SDL2, assists with chord
progressions via a special Chord Track and chord
pads, and can be used in multiple languages
including English, French, Portuguese, Japanese and
German.

Zrythm is [free software](https://www.gnu.org/philosophy/free-sw.html)
written in C using the GTK+3 toolkit and can be
extended using [GNU Guile](https://www.gnu.org/software/guile/).

[Home page](https://www.zrythm.org)

# Current state

Zrythm is currently in alpha. Most essential
DAW features are implemented and we are working
towards a stable release.

![screenshot](https://www.zrythm.org/static/images/feb-20-2021.png)

### Supported plugins/instruments
- LV2, VST2, VST3, AU, SFZ, SF2, DSSI, LADSPA

Support for all formats besides LV2 is
possible thanks to
[Carla](https://github.com/falkTX/Carla/).

### Supported backends
- Audio: JACK (PipeWire), PulseAudio, SDL2, RtAudio (ALSA/WASAPI/CoreAudio)
- MIDI: JACK (PipeWire), WindowsMME, RtMidi (ALSA sequencer/Windows MME/CoreMIDI)

### Supported platforms
- GNU/Linux, FreeBSD, Windows, MacOS

## Building and Installation
See [INSTALL.md](INSTALL.md) for build instructions.
Prebuilt packages/installers available at
<https://www.zrythm.org/en/download.html>.

## Using
See the [user manual](http://manual.zrythm.org/).

## Contributing
See [CONTRIBUTING.md](CONTRIBUTING.md).

## Hacking
See [HACKING.md](HACKING.md) and the
[developer docs](https://docs.zrythm.org/).

## Packaging
See [PACKAGING.md](PACKAGING.md).

## Mailing lists
See [mailing lists on sr.ht](https://sr.ht/~alextee/zrythm/lists).

## Issue trackers
See the [Feature tracker](https://todo.sr.ht/~alextee/zrythm-feature) and [Bug tracker](https://todo.sr.ht/~alextee/zrythm-bug).

## Releases
<https://www.zrythm.org/releases>

## Copying Zrythm
[![agpl-3.0](https://www.gnu.org/graphics/agplv3-with-text-162x68.png)](https://www.gnu.org/licenses/agpl-3.0)

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

### Additional terms under AGPLv3 Section 7
Zrythm and the Zrythm logo are trademarks of
Alexandros Theodotou and are governed by the Zrythm
[Trademark Policy](TRADEMARKS.md).
You may distribute unaltered copies of Zrythm that
include the Zrythm trademarks without express
permission from Alexandros Theodotou. However,
if you make any changes to Zrythm, you may not
redistribute that product using any Zrythm trademark
without Alexandros Theodotou’s prior written consent.
For example, you may not distribute a modified form
of Zrythm and continue to call it Zrythm, or
include the Zrythm logo, unless explicitly allowed
by the [Trademark Policy](TRADEMARKS.md).

## Support
If you would like to support this project please
donate below or purchase a binary installer from
<https://www.zrythm.org/en/download.html> - creating a DAW
takes thousands of hours of work and contributions
enable us to spend more time working on the project.

- [liberapay.com/Zrythm](https://liberapay.com/Zrythm/donate)
- [paypal.me/zrythm](https://paypal.me/zrythm)
- [opencollective.com/zrythm](https://opencollective.com/zrythm/donate)

----

Copyright (C) 2018-2021 Alexandros Theodotou

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved.  This file is offered as-is,
without any warranty.
