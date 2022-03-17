.. This is part of the Zrythm Manual.
   Copyright (C) 2019, 2022 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

Track Types
===========

Zrythm has the following types of Tracks, and
they are explained further in their own sections.

.. list-table:: Track types and characteristics
   :width: 100%
   :widths: auto
   :header-rows: 1

   * - Track type
     - Input
     - Output
     - Can record
     - Objects
   * - Audio
     - Audio
     - Audio
     - Yes
     - Audio regions
   * - Audio Group
     - Audio
     - Audio
     - No
     - None
   * - Audio FX
     - Audio
     - Audio
     - No
     - None
   * - Chord
     - MIDI
     - MIDI
     - Yes
     - Chord regions, scales
   * - Instrument
     - MIDI
     - Audio
     - Yes
     - MIDI regions
   * - Marker
     - None
     - None
     - No
     - Markers
   * - Master
     - Audio
     - Audio
     - No
     - None
   * - Modulators
     - None
     - None
     - No
     - None
   * - MIDI
     - MIDI
     - MIDI
     - Yes
     - MIDI regions
   * - MIDI Group
     - MIDI
     - MIDI
     - No
     - None
   * - MIDI FX
     - MIDI
     - MIDI
     - No
     - None
   * - Tempo
     - None
     - None
     - No
     - None

All tracks except the :doc:`chord-track` have
automation lanes available.
