<!---
SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
SPDX-License-Identifier: FSFAP
-->

Zrythm
======

[![translated](https://hosted.weblate.org/widgets/zrythm/-/svg-badge.svg "Translation Status")](https://hosted.weblate.org/engage/zrythm/?utm_source=widget)

*a highly automated and intuitive digital audio
workstation*

![screenshot](https://www.zrythm.org/static/images/screenshots/screenshot-20221015.png)

Zrythm is a digital audio workstation designed to be
featureful and easy to use.
It offers streamlined editing workflows with flexible
tools, limitless automation capabilities, powerful
mixing features, chord assistance and support for
various plugin and file formats.

Zrythm is
[free software](https://www.gnu.org/philosophy/free-sw.html)
written in C using the
[GTK4](https://docs.gtk.org/gtk4/overview.html)
toolkit and can be extended with user scripts
written in Scheme or ECMAScript.

## Features

- Object looping, cloning, linking and stretching
- Adaptive snapping
- Multiple lanes per track
- Bounce anything to audio or MIDI
- Piano roll (MIDI editor) with chord integration, drum mode and a lollipop velocity editor
- Audio editor with part editing (including in external app) and adjustable gain/fades
- Event viewers (list editors) with editable object parameters
- Built-in and scriptable per-context object functions
- Audio/MIDI/automation recording with options to punch in/out, record on MIDI input and create takes
- Device-bindable parameters for external control
- Wide variety of track types for every purpose
- Signal manipulation with signal groups, aux sends and direct anywhere-to-anywhere connections
- In-context listening by dimming other tracks
- Automate anything using automation events or CV signal from modulator plugins and macro knobs
- Detachable views for multi-monitor setups
- Searchable preferences
- Support for LV2/CLAP/VST2/VST3/AU/LADSPA/DSSI plugins, SFZ/SF2 SoundFonts, Type 0 and 1 MIDI files, and almost every audio file format
- Flexible built-in file and plugin browsers
- Optional plugin sandboxing (bridging)
- Stem export
- Chord pad with built-in and user presets, including the ability to generate chords from scales
- Automatic project backups
- Undoable user actions with serializable undo history
- User scripting capabilities via Guile API
- Hardware-accelerated UI
- SIMD-optimized DSP
- Cross-platform, cross-audio/MIDI backend and cross-architecture
- Available in multiple languages including Chinese, Japanese, Russian, Portuguese, French and German

For a full list of features, see the
[Features page](https://www.zrythm.org/en/features.html)
on our website.

## Current state

Zrythm is currently in beta. The project format is
stable and we are working towards a v1 release.

### Audio backends

|  Backend  |  Status  |
| ---- | ---- |
|  JACK  |  Supported and recommended |
|  JACK (via PipeWire)  |  Supported  |
|  PulseAudio (RtAudio)  |  Supported  |
|  PulseAudio |  Has known issues |
|  SDL2 |  Has known issues |
|  ALSA (RtAudio) |  Supported |
|  ALSA |  Broken |
|  WASAPI (RtAudio) |  Supported |
|  CoreAudio (RtAudio) |  Supported |

### MIDI backends

|  Backend  |  Status  |
| ---- | ---- |
|  JACK MIDI |  Supported and recommended |
|  JACK MIDI (via PipeWire) |  Supported |
|  WindowsMME |  Has known issues  |
|  ALSA Sequencer (RtMidi) |  Supported |
|  WindowsMME (RtMidi) |  Supported |
|  CoreMIDI (RtMidi) |  Supported |

### Platforms

|  Platform  |  x86_64/AMD64/x64  | AArch64/ARM64/ARMv8 | ARMv7 | PowerPC64 | i386 | i686 |
| ---- | ---- | ---- | ---- | ---- | ---- | ---- |
|  GNU/Linux  |  ○ | ○ | △| △ | △ | △ |
|  FreeBSD  |  ○ | △ | △ | △ | △ | △ |
|  Windows  |  ○ | × | × | × | × | × |
|  MacOS  |  ○ | ○ | × | × | × | × |

○: Supported
△: Untested
×: Not supported

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

## Forum
See [our forum](https://forum.zrythm.org).

## Chat
* [#zrythmdaw:matrix.org on Matrix](https://matrix.to/#/#zrythmdaw:matrix.org).
* [#zrythm on Libera.Chat IRC](https://web.libera.chat/#zrythm).

## Issue tracker
See [Issues on GitLab](https://gitlab.zrythm.org/zrythm/zrythm/issues).

## Releases
<https://www.zrythm.org/releases>

## Copying Zrythm
[![agpl-3.0](https://www.gnu.org/graphics/agplv3-with-text-162x68.png)](https://www.gnu.org/licenses/agpl-3.0)

See [COPYING](COPYING) for general copying
conditions and [TRADEMARKS.md](TRADEMARKS.md) for
our trademark policy.

## Support
If you would like to support this project please
donate below or purchase a binary installer from
<https://www.zrythm.org/en/download.html> - creating
a DAW takes years of work and contributions enable
us to spend more time working on the project.

- [liberapay.com/Zrythm](https://liberapay.com/Zrythm/donate)
- [paypal.me/zrythmdaw](https://paypal.me/zrythmdaw)
- [ko-fi.com/zrythm](https://ko-fi.com/zrythm)
- [opencollective.com/zrythm](https://opencollective.com/zrythm/donate)
- Bitcoin (BTC): `bc1qjfyu2ruyfwv3r6u4hf2nvdh900djep2dlk746j`
- Litecoin (LTC): `ltc1qpva5up8vu8k03r8vncrfhu5apkd7p4cgy4355a`
- Monero (XMR): `87YVi6nqwDhAQwfAuB8a7UeD6wrr81PJG4yBxkkGT3Ri5ng9D1E91hdbCCQsi3ZzRuXiX3aRWesS95S8ij49YMBKG3oEfnr`
