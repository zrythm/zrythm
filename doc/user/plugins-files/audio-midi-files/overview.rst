.. SPDX-FileCopyrightText: © 2019 Alexandros Theodotou <alex@zrythm.org>
   SPDX-License-Identifier: GFDL-1.3-invariants-or-later
.. This is part of the Zrythm Manual.
   See the file index.rst for copying conditions.

Overview
========

Zrythm can import MIDI and audio files into
the project. The files can be imported by
dragging and dropping from your computer or
from the
:doc:`file browser <file-browser>` into a track.

Supported Formats
-----------------

Audio
~~~~~
The following file formats are supported.

OGG (Vorbis and OPUS)
  An open and patent-free container format mostly
  used for lossy audio files.
:abbr:`FLAC (Free Lossless Audio Codec)`
  An open and royalty-free audio coding format for
  lossless compression of digital audio.
:abbr:`WAV (Waveform Audio File Format)`
  An audio file format for storing audio bistreams
  (uncompressed audio).
MP3
  An (originally patented) coding format for
  compressed audio. Although the patents have
  expired, we recommend using OGG instead.

.. note:: OPUS support requires specific project
   samplerates to work properly.

.. important:: A software patent is a 20-year
   monopoly on the use of a feature in a computer
   program. When a company has a patent on nested
   menus, a video format, or pinch-to-zoom, then no
   one else can implement that feature for 20 years
   unless they get permission from the patent
   holder.

   Computer programs typically contain hundreds or
   thousands of features, and many features that
   users expect, are patented.

   The End Software Patents campaign is against
   software monopolies, and for software development.

   Learn more at `EndSoftPatents`_.

.. _EndSoftPatents: https://endsoftwarepatents.org/

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
