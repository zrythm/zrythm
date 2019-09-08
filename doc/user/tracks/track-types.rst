.. This is part of the Zrythm Manual.
   Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

Track Types
===========

Zrythm has the following types of Tracks, and
they are explained further in their own sections.

Instrument Track
  A Track that contains regions holding MIDI
  notes. It also has automation lanes for
  automating its components.
Audio Track
  A Track containing audio regions, cross-fades, fades and automation.
Bus Track
  A Track corresponding to a mixer bus. Bus tracks only contain automation
Master Track
  The Master track is a special type of Bus Track that controls the master fader and contains additional automation options.
Group Track
  A Group Track is used to route signals from
  multiple Tracks into one Track. It behaves like
  a Bus Track with the exception that other Tracks can
  route their output signal directly into Group
  Track.
Chord Track
  A Chord Track is a special kind of Track that
  contains chords and scales and is a great tool
  for assisting with chord progressions.
Marker Track
  A Marker Track is a special kind of Track that
  contains song markers - either custom or
  pre-defined ones like the song start and
  end markers.
