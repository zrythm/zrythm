Zrythm
======

[![translated](https://hosted.weblate.org/widgets/zrythm/-/svg-badge.svg "Translation Status")](https://hosted.weblate.org/engage/zrythm/?utm_source=widget)
[![builds.sr.ht status](https://builds.sr.ht/~alextee/zrythm.svg)](https://builds.sr.ht/~alextee/zrythm?)

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

Zrythm is
[free software](https://www.gnu.org/philosophy/free-sw.html)
written in C using the
[GTK4](https://docs.gtk.org/gtk4/overview.html)
toolkit and can be
extended using
[GNU Guile](https://www.gnu.org/software/guile/).

[Home page](https://www.zrythm.org)

# Current state

Zrythm is currently in beta. The project format is
stable and we are working towards a v1 release.

![screenshot](https://www.zrythm.org/static/images/screenshots/mar-16-2022.png)

### Supported plugins/instruments
- Native support (currently disabled): [LV2](https://lv2plug.in/)
- Support via [Carla](https://github.com/falkTX/Carla/): LV2, VST2, VST3, AU, SFZ, SF2, DSSI, LADSPA

### Supported file formats
- Audio: OGG (Vorbis), FLAC, WAV, MP3
- MIDI: SMF Type 0, SMF Type 1

### Supported backends
- Audio: JACK (PipeWire), PulseAudio, SDL2, RtAudio (ALSA/WASAPI/CoreAudio)
- MIDI: JACK (PipeWire), WindowsMME, RtMidi (ALSA sequencer/Windows MME/CoreMIDI)

### Supported platforms
- GNU/Linux, FreeBSD, Windows, MacOS

## Building and Installation
See [INSTALL.rst](INSTALL.rst) for build
instructions. Prebuilt packages/installers
available at
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

## Chat
* [#zrythmdaw:matrix.org on Matrix](https://matrix.to/#/#zrythmdaw:matrix.org).
* [#zrythm on Libera.Chat IRC](https://web.libera.chat/#zrythm).

## Issue trackers
See the [Feature tracker](https://todo.sr.ht/~alextee/zrythm-feature) and [Bug tracker](https://todo.sr.ht/~alextee/zrythm-bug).

## Releases
<https://www.zrythm.org/releases>

## Copying Zrythm
[![agpl-3.0](https://www.gnu.org/graphics/agplv3-with-text-162x68.png)](https://www.gnu.org/licenses/agpl-3.0)

See [COPYING](COPYING) for copying conditions and
[TRADEMARKS.md](TRADEMARKS.md) for our trademark
policy.

## Support
If you would like to support this project please
donate below or purchase a binary installer from
<https://www.zrythm.org/en/download.html> - creating
a DAW takes thousands of hours of work and
contributions enable us to spend more time working
on the project.

- [liberapay.com/Zrythm](https://liberapay.com/Zrythm/donate)
- [paypal.me/zrythm](https://paypal.me/zrythm)
- [opencollective.com/zrythm](https://opencollective.com/zrythm/donate)
- Bitcoin (BTC): `bc1qjfyu2ruyfwv3r6u4hf2nvdh900djep2dlk746j`
- Litecoin (LTC): `ltc1qpva5up8vu8k03r8vncrfhu5apkd7p4cgy4355a`
- Monero (XMR): `87YVi6nqwDhAQwfAuB8a7UeD6wrr81PJG4yBxkkGT3Ri5ng9D1E91hdbCCQsi3ZzRuXiX3aRWesS95S8ij49YMBKG3oEfnr`

----

Copyright (C) 2018-2022 Alexandros Theodotou

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved.  This file is offered as-is,
without any warranty.
