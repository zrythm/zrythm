.. This is part of the Zrythm Manual.
   Copyright (C) 2019, 2022 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

Track Types
===========

Zrythm has the following types of Tracks.

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
   * - Audio FX
     - Audio
     - Audio
     - No
     - None
   * - Audio Group
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
   * - MIDI FX
     - MIDI
     - MIDI
     - No
     - None
   * - MIDI Group
     - MIDI
     - MIDI
     - No
     - None
   * - Tempo
     - None
     - None
     - No
     - None

All tracks except the Chord Track have
automation lanes available.

Audio Track
-----------

An Audio Track contains audio regions and can be
used for recording and playing back audio clips.

.. image:: /_static/img/audio-track.png
   :align: center

Audio FX Track
--------------
Audio FX tracks can be used for effects by
sending audio signals to them with
:ref:`track-sends`.

.. image:: /_static/img/audio-bus-track.png
   :align: center

A common use case is to create a separate Audio
FX track with a reverb plugin and to send audio
from another track to it. This way, the reverb
signal can be managed separately from its source,
or even be shared among several tracks.

Audio Group Track
-----------------

Audio group tracks can be used for grouping
audio signals together via direct routing.
For example, kicks, snares and hihats can be
routed to a "Drums" audio group track so you
can add effects and control their volume
collectively instead of separately.

.. image:: /_static/img/audio-group-track.png
   :align: center

Chord Track
-----------

The Chord Track contains chord and scale
objects that are used to specify when the song
is using a particular chord or scale. Its main
purpose is to assist with chord progressions.

.. image:: /_static/img/chord-track.png
   :align: center

For more information, see the section
:doc:`../chords-and-scales/intro`.

Instrument Track
----------------

The instrument track is used for synths and
other instruments. Instrument tracks contain
a special `instrument slot` in the mixer that
will get processed `after` the MIDI FX section
and `before` the Inserts section.

.. image:: /_static/img/instrument-track.png
   :align: center

Instrument tracks are similar to MIDI tracks, except
that they produce audio instead of :term:`MIDI`.

Marker Track
------------

The marker track holds song markers - either custom
or pre-defined ones - that
make it easier to jump to or to export specific
sections of the song. Each project can only
have one marker track and it cannot be deleted
(but can be hidden).

.. image:: /_static/img/marker-track.png
   :align: center

Master Track
------------

The master track is a special type of
Audio Group Track that Zrythm uses
to route the resulting audio signal after
all the processing is done to the
audio backend.

.. image:: /_static/img/master-track.png
   :align: center

Modulator Track
---------------

.. todo:: Write description.

MIDI Track
----------

A MIDI track contains MIDI regions and its purpose
is playing back MIDI events and routing
those MIDI signals to other instruments
(including external instruments and hardware).

MIDI tracks, unlike instrument tracks, produce MIDI
output and so their channel strips will only have
MIDI effects (plugins that change MIDI signals).

MIDI tracks also have automation lanes for automating
various parameters.

.. image:: /_static/img/midi-track.png
   :align: center

MIDI FX Track
-------------

MIDI FX tracks can be used for MIDI effects by
sending MIDI signals to them using
:ref:`track-sends`.

.. image:: /_static/img/midi-bus-track.png
   :align: center

MIDI Group Track
----------------

These are similar to Audio Group tracks,
except that they act on MIDI signals instead
of audio signals.

.. image:: /_static/img/midi-group-track.png
   :align: center

Tempo Track
-----------

.. todo:: Write description.
