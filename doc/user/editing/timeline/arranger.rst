.. This is part of the Zrythm Manual.
   Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
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

.. image:: /_static/img/region.png
   :align: center

The following types of regions exist.

Audio Regions
+++++++++++++
Audio regions contain audio clips from audio files.

.. image:: /_static/img/audio-region.png
   :align: center

Audio regions belong to track lanes and appear inside
Audio tracks.

.. image:: /_static/img/audio-track-with-region.png
   :align: center

Double-clicking an audio region will bring up the
:doc:`Audio Editor <../../editing/clip-editors/audio-editor>`.

MIDI Regions
++++++++++++
MIDI regions contain :term:`MIDI` notes.

.. image:: /_static/img/midi-region.png
   :align: center

MIDI regions belong to track lanes and appear inside
MIDI or Instrument tracks.

.. image:: /_static/img/midi-track-with-region.png
   :align: center

Double-clicking a MIDI region will bring up the
:doc:`Piano Roll <../../editing/clip-editors/piano-roll>`.

Automation Regions
++++++++++++++++++
Automation regions contain automation events.

.. image:: /_static/img/automation-region.png
   :align: center

Automation regions appear inside automation lanes.

.. image:: /_static/img/automation-lane-with-region.png
   :align: center

Double-clicking an automation region will bring up
the
:doc:`Automation Editor <../../editing/clip-editors/automation-editor>`.

Chord regions
+++++++++++++
Chord regions contain sequences of chords.

.. todo:: Add pic.

Chord regions appear inside the chord track.

.. todo:: Add pic.

Double-clicking a chord region will bring up the
:doc:`Chord Editor <../../editing/clip-editors/chord-editor>`.

Markers
~~~~~~~
Markers are used to mark the start of a logical
section inside the song, such as `Chorus` or
`Intro`.

.. image:: /_static/img/marker.png
   :align: center

Markers appear inside the marker track.

.. todo:: Add pic.

There are two special markers that signify the
start and end of the song that are used for
exporting the song and cannot be deleted.

Scales
~~~~~~
Scales are used to indicate the
start of a section using a specific musical scale.

.. image:: /_static/img/scale-object.png
   :align: center

Scales appear inside the Chord track.

.. todo:: Add pic.

Editing Regions
---------------
The following operations apply to regions.

Looping
~~~~~~~
Regions can be repeated, and hence they have
:ref:`editable loop points and a clip start position <editing/clip-editors/ruler:Loop Points>`
in the
:doc:`Editor Ruler <../../editing/clip-editors/ruler>`
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

.. todo:: Add pic to show this.

You can verify that a link exists on a region by
the link icon that shows in the top right.

.. todo:: Add pic to show this.

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
by right-clicking on the fade.

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

.. todo:: Decide what to do about cross-fades
   (just use fades?).
