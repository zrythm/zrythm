.. SPDX-FileCopyrightText: © 2020, 2022 Alexandros Theodotou <alex@zrythm.org>
   SPDX-License-Identifier: GFDL-1.3-invariants-or-later
   This is part of the Zrythm Manual.
   See the file index.rst for copying conditions.

.. _timeline-arranger:

Timeline Arranger
=================

The timeline arranger is the main area where the
song is composed. It consists of a collection of
events, such as regions, positioned against time.
Some events (such as regions) will open separate
windows for further editing when clicked.

.. image:: /_static/img/timeline-arranger.png
   :align: center

The timeline arranger is split into a top arranger
that remains fixed on top and a bottom arranger
below it. This way you can pin tracks you want to
always be visible at the top.

Arranger Objects
----------------
The following arranger objects can exist inside
the timeline.

.. _regions:

Regions
~~~~~~~
Regions (or :term:`clips <Clip>`) are containers
for events or data (such as MIDI notes and audio
clips - see below) that can be edited in an
:ref:`editor <editors>`. Regions can be repeated,
like below.

.. figure:: /_static/img/region.png
   :align: center

   Region

The following types of regions exist.

Audio Regions
+++++++++++++
Audio regions contain audio clips from audio files.

.. figure:: /_static/img/audio-region.png
   :align: center

   Audio region

Audio regions belong to track lanes and appear inside
Audio tracks.

.. figure:: /_static/img/audio-track-with-region.png
   :align: center

   Audio track with audio region

Double-clicking an audio region will bring up the
:doc:`Audio Editor <../../editing/clip-editors/audio-editor>`.

MIDI Regions
++++++++++++
MIDI regions contain :term:`MIDI` notes.

.. figure:: /_static/img/midi-region.png
   :align: center

   MIDI region

MIDI regions belong to track lanes and appear inside
MIDI or Instrument tracks.

.. figure:: /_static/img/midi-track-with-region.png
   :align: center

   MIDI track with MIDI region

Double-clicking a MIDI region will bring up the
:doc:`Piano Roll <../../editing/clip-editors/piano-roll>`.

Automation Regions
++++++++++++++++++
Automation regions contain automation events.

.. figure:: /_static/img/automation-region.png
   :align: center

   Automation region

Automation regions appear inside automation lanes.

.. figure:: /_static/img/automation-lane-with-region.png
   :align: center

   Automation lane with automation region

Double-clicking an automation region will bring up
the
:doc:`Automation Editor <../../editing/clip-editors/automation-editor>`.

Chord regions
+++++++++++++
Chord regions contain sequences of chords.

.. figure:: /_static/img/chord-region.png
   :align: center

   Chord region

Chord regions appear inside the chord track.

.. figure:: /_static/img/chord-track-with-region.png
   :align: center

   Chord track with chord region

Double-clicking a chord region will bring up the
:doc:`Chord Editor <../../editing/clip-editors/chord-editor>`.

Markers
~~~~~~~
Markers are used to mark the start of a logical
section inside the song, such as `Chorus` or
`Intro`.

.. figure:: /_static/img/marker.png
   :align: center

   Marker

Markers appear inside the marker track.

.. figure:: /_static/img/marker-track-with-marker.png
   :align: center

   Marker track with marker

There are two special markers that signify the
start and end of the song that are used for
exporting the song and cannot be deleted.

Scales
~~~~~~
Scales are used to indicate the
start of a section using a specific musical scale.

.. figure:: /_static/img/scale-object.png
   :align: center

   Scale

Scales appear inside the Chord track.

.. figure:: /_static/img/chord-track-with-scale.png
   :align: center

   Chord track with scale

Editing Regions
---------------
The following operations apply to regions.

Looping
~~~~~~~
Regions can be repeated, and hence they have
:ref:`editable loop points and a clip start position <editing/clip-editors/ruler:Markers>`
in the
:doc:`Editor ruler <../../editing/clip-editors/ruler>`
to modify the looping (repeating) behavior.

Regions can also be looped inside the timeline,
by moving the cursor to the bottom-left or
bottom-right edge of the region, then clicking
and dragging.

.. figure:: /_static/img/looping-regions.png
   :align: center

   Looping (loop-resizing) a region

.. note:: If the region is already repeated, it
   cannot be resized anymore until its loop points
   match exactly the region's start and end points.

Link-Moving
~~~~~~~~~~~
Linked regions can be created by holding down
:kbd:`Alt` while moving.

.. figure:: /_static/img/link-moving-regions.png
   :align: center

   Link-moving a MIDI region

You can verify that a link exists on a region by
the link icon that shows in the top right.

.. figure:: /_static/img/linked-regions.png
   :align: center

   Linked MIDI regions

Renaming
~~~~~~~~
Regions can be renamed by selecting them and
pressing :kbd:`F2`.

.. figure:: /_static/img/region-rename.png
   :align: center

   Renaming a region

Adjusting Fades
~~~~~~~~~~~~~~~
Audio regions can have fades.
Fades are gradual increases or
decreases in the level of the audio signal, and
their positions can be adjusted by clicking and
dragging the top left/right corners of the region.

.. figure:: /_static/img/audio-region-fade-out1.png
   :align: center

   Adjusting fade out point (click & drag)

.. figure:: /_static/img/audio-region-fade-out2.png
   :align: center

   Adjusting fade out point (drop)

Clicking and dragging the grey part up or down
will adjust the curviness of the fade.

.. figure:: /_static/img/audio-region-fade-out-curviness.png
   :align: center

   Adjusting curviness

The type of fade algorithm used can also be changed
by right-clicking on the fade and selecting
:menuselection:`Fade preset`.

.. figure:: /_static/img/audio-region-fade-context-menu.png
   :align: center

   Fade context menu

The various types of fade algorithms available are
illustrated below.

.. list-table:: Fade algorithms

   * - .. figure:: /_static/img/fade-linear.png
          :align: center

          Linear

     - .. figure:: /_static/img/fade-exponential.png
          :align: center

          Exponential

     - .. figure:: /_static/img/fade-superellipse.png
          :align: center

          Elliptic (Superellipse)

     - .. figure:: /_static/img/fade-vital.png
          :align: center

          Vital

.. note:: All audio regions have some additional,
   built-in fade in and fade out that cannot be
   disabled. This is used to avoid clipping and
   should be unnoticable.

