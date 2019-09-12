.. This is part of the Zrythm Manual.
   Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

Track Types
===========

Zrythm has the following types of Tracks, and
they are explained further in their own sections.

MIDI Track
  A Track that contains regions holding MIDI
  notes. It also has automation lanes for
  automating its components.

  *Input:* **MIDI**, *Output:* **MIDI**

  *Can record:* **Yes**
Instrument Track
  Similar to a MIDI track, except that an Instrument
  Track is bound to an instrument plugin.

  *Input:* **MIDI**, *Output:* **Audio**

  *Can record:* **Yes**
Audio Track
  A Track containing audio regions, cross-fades, fades and automation.

  *Input:* **Audio**, *Output:* **Audio**

  *Can record:* **Yes**
Bus Track (Audio)
  A Track corresponding to a mixer bus. Bus tracks
  only contain automation

  *Input:* **Audio**, *Output:* **Audio**

  *Can record:* **No**
Bus Track (MIDI)
  Similar to an audio Bus Track, except that it
  handles MIDI instead of audio.

  *Input:* **MIDI**, *Output:* **MIDI**

  *Can record:* **No**
Group Track (Audio)
  A Group Track is used to route signals from
  multiple Tracks into one Track (or "group" them).
  It behaves like
  a Bus Track with the exception that other Tracks
  can
  route their output signal directly into Group
  Track.
  *Input:* **Audio**, *Output:* **Audio**

  *Can record:* **No**
Group Track (MIDI)
  Similar to an audio Group Track, except that it
  handles MIDI instead of audio.

  *Input:* **MIDI**, *Output:* **MIDI**

  *Can record:* **No**
Master Track
  The Master track is a special type of Bus Track
  that controls the master fader and contains
  additional automation options.

  *Input:* **Audio**, *Output:* **Audio**

  *Can record:* **No**
Chord Track
  A Chord Track is a special kind of Track that
  contains chords and scales and is a great tool
  for assisting with chord progressions.

  *Input:* **MIDI**, *Output:* **MIDI**

  *Can record:* **No**
Marker Track
  A Marker Track is a special kind of Track that
  contains song markers - either custom or
  pre-defined ones like the song start and
  end markers.

  *Input:* **None**, *Output:* **None**

  *Can record:* **No**
