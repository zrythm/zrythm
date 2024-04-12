.. SPDX-FileCopyrightText: © 2019, 2024 Alexandros Theodotou <alex@zrythm.org>
   SPDX-License-Identifier: GFDL-1.3-invariants-or-later
.. This is part of the Zrythm Manual.
   See the file index.rst for copying conditions.

Overview
========

Zrythm allows importing the following MIDI and audio file formats.

Supported Formats
-----------------

Audio
~~~~~
The following file formats are supported:

* OGG (Vorbis and OPUS)
* :abbr:`FLAC (Free Lossless Audio Codec)`
* :abbr:`WAV (Waveform Audio File Format)`
* MP3

MIDI
~~~~
Zrythm supports :abbr:`SMF (Standard MIDI File)`
files of type 0 and type 1. Each type is explained
below for reference.

Type 0
  An entire performance merged into a single track.
Type 1
  An entire performance in one or more tracks that
  are performed simultaneously
Type 2
  Multiple arrangements, each having its own track
  and intended to be played in sequence

.. note:: Type 2 SMF files are rarely used and are
   not supported.
