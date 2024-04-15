.. SPDX-FileCopyrightText: © 2019, 2022, 2024 Alexandros Theodotou <alex@zrythm.org>
   SPDX-License-Identifier: GFDL-1.3-invariants-or-later
.. This is part of the Zrythm Manual.
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
   * - Folder
     - None
     - None
     - No
     - None
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

All tracks types except Chord and Folder tracks
have automation lanes available.

Audio Track
-----------

An Audio Track contains audio regions and can be
used for recording and playing back audio clips.

.. figure:: /_static/img/audio-track.png
   :align: center

   Audio track with an audio region

Audio FX Track
--------------
Audio FX tracks can be used for effects by
sending audio signals to them with
:ref:`track-sends`.

.. figure:: /_static/img/audio-fx-track.png
   :align: center

   Audio FX track

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

.. figure:: /_static/img/audio-group-track.png
   :align: center

   Audio Group track

.. hint:: Group tracks are foldable.

Chord Track
-----------

The Chord Track contains chord and scale
objects that are used to specify when the song
is using a particular chord or scale. Its main
purpose is to assist with chord progressions.

.. figure:: /_static/img/chord-track.png
   :align: center

   Chord track with a chord region and a scale

For more information about chords, see the section
:ref:`chords-and-scales/intro:Chords and Scales`.

For editing chords inside chord regions, see
:ref:`editing/clip-editors/chord-editor:Chord Editor`.

Live Audition
~~~~~~~~~~~~~

You can route the output of the chord track to
a MIDI or Instrument track for live auditioning of
the chord track's events. You can also play the
chords live with a MIDI keyboard.

Folder Track
------------

Folder tracks are used to organize tracks.

.. figure:: /_static/img/folder-track.png
   :align: center

   Folder track with a MIDI FX track as a child

Moving/Copying Tracks Into a Foldable Track
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
To move (or copy by holding :kbd:`Control`) tracks inside folder (or foldable) tracks, simply select the tracks you want and drag-and-drop them inside the folder track.
While dragging one or more tracks into a folder track, a line is shown
to indicate where the tracks will be dropped.
Dropping the tracks in the middle of a Folder track (orange line shown in the middle) will insert the tracks into the folder track as children.

.. figure:: /_static/img/moving-tracks-inside-folder.png
   :align: center

   Moving tracks inside a folder track

.. note:: Dropping the tracks above (or below) the folder track will cause the tracks to simply be moved above (or below) the folder track and *NOT* inside it.

Folding/Unfolding
~~~~~~~~~~~~~~~~~
Clicking the folder button will hide and expand the
folder track's children.

.. tip:: Audio/MIDI group tracks are also foldable.

Instrument Track
----------------

The instrument track is used for synths and
other instruments. Instrument tracks contain
a special `instrument slot` in the mixer that
will get processed `after` the MIDI FX section
and `before` the Inserts section.

.. figure:: /_static/img/instrument-track.png
   :align: center

   Instrument track with a MIDI region

Instrument tracks are similar to MIDI tracks, except
that they produce audio instead of :term:`MIDI`.

Show/Hide Plugin UI
~~~~~~~~~~~~~~~~~~~

Instrument tracks contain a button (computer screen icon) that allows
showing/hiding the instrument plugin's UI.

Marker Track
------------

The marker track holds :ref:`song markers <playback-and-recording/loop-points-and-markers:Custom Markers>` - either custom
or pre-defined ones - that
make it easier to jump to or to export specific
sections of the song. Each project can only
have one marker track and it cannot be deleted
(but can be hidden).

.. figure:: /_static/img/marker-track.png
   :align: center

   Marker track with 2 markers

Master Track
------------

The master track is a special type of Audio Group Track that Zrythm uses
to route the resulting audio signal after all the processing is done to the
audio backend.

.. figure:: /_static/img/master-track.png
   :align: center

   Master track

Modulator Track
---------------

The modulator track is a special track that is
used for global modulators like LFOs and macro
knobs that can be assigned to any automatable
control inside Zrythm.

.. figure:: /_static/img/modulator-track.png
   :align: center

   Modulator track with automation for macro knob 1

For more details, see
:ref:`modulators/intro:Modulators`.

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

.. figure:: /_static/img/midi-track.png
   :align: center

   MIDI track with a MIDI region

MIDI FX Track
-------------

MIDI FX tracks can be used for MIDI effects by
sending MIDI signals to them using
:ref:`track-sends`.

.. figure:: /_static/img/midi-fx-track.png
   :align: center

   MIDI FX track

MIDI Group Track
----------------

These are similar to Audio Group tracks,
except that they act on MIDI signals instead
of audio signals.

.. figure:: /_static/img/midi-group-track.png
   :align: center

   MIDI Group track

.. hint:: Group tracks are foldable.

Tempo Track
-----------

The tempo track is a special track that allows
automating the BPM and time signature.

.. figure:: /_static/img/tempo-track.png
   :align: center

   Tempo track with BPM automation

.. warning:: BPM and time signature automation is currently experimental. **Projects using this functionality may break.** Only use it at your own risk.
