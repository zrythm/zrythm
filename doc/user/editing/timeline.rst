.. SPDX-FileCopyrightText: © 2020, 2022-2024 Alexandros Theodotou <alex@zrythm.org>
   SPDX-License-Identifier: GFDL-1.3-invariants-or-later
   This is part of the Zrythm Manual.
   See the file index.rst for copying conditions.

Timeline
========

Overview
--------

Editing in the timeline is the most common workflow
inside Zrythm. The timeline section consists of the
:ref:`tracklist <tracks/tracklist:Tracklist>` on the left and the ruler/arranger on the
right, along with a timeline-specific toolbar at the
top.

.. image:: /_static/img/timeline.png
   :align: center

Minimap
-------
The timeline minimap is a little box that represents the
current visible area of the timeline. It can be moved around
and resized to change the visible area.

.. image:: /_static/img/timeline-minimap.png
   :align: center

Ruler
-----

The ruler is mainly used to control looping and
playback.

.. image:: /_static/img/ruler.png
   :align: center

The ruler will display more or less information
depending on the current zoom level. It will also
display the following markers/indicators discussed in
:ref:`loop-points-and-markers`.

Cue point
  Displayed as a blue, right-pointing arrow.
Playhead position
  Shown as a grey, down-facing arrow.
Loop points
  Shown as 2 green arrows, and the area between them is shown in bright green
  if loop is enabled, or grey if disabled. Can be dragged to reposition.

  .. tip:: Looping can be enabled/disabled using the loop button in the :ref:`transport controls <playback-and-recording/transport-controls:Controls>`.

Clicking and dragging on empty space in the
bottom half of the ruler will allow
you to reposition the playhead.

.. tip:: You can :kbd:`Control`-click and drag the loop region to move it.

Setting the Cue Point
~~~~~~~~~~~~~~~~~~~~~
Double click inside the ruler to set the cue point. This
will be used to return to when playback is stopped.

Range Selection
~~~~~~~~~~~~~~~
Clicking and dragging in the top half of the ruler
will create a :term:`range <Range>` selection. If
an existing range exists, you can click an drag it
to move it.

.. figure:: /_static/img/ranges.png
   :align: center

   Range selection

Arranger
--------

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
~~~~~~~~~~~~~~~~
The following arranger objects can exist inside
the timeline.

.. _regions:

Regions
+++++++
Regions (or :term:`clips <Clip>`) are containers
for events or data (such as MIDI notes and audio
clips - see below) that can be edited in an
:ref:`editor <editing/timeline:Editing Region Contents>`. Regions can be repeated,
like below.

.. figure:: /_static/img/region.png
   :align: center

   Region

The following types of regions exist.

Audio Regions
^^^^^^^^^^^^^
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
:ref:`audio editor <editing/audio-editor:Audio Editor>`.

MIDI Regions
^^^^^^^^^^^^
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
:ref:`piano roll <editing/piano-roll:Piano Roll>`.

Automation Regions
^^^^^^^^^^^^^^^^^^
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
:ref:`automation editor <editing/automation-editor:Automation Editor>`.

Chord regions
^^^^^^^^^^^^^

Chord regions contain sequences of chords.

.. figure:: /_static/img/chord-region.png
   :align: center

   Chord region

Chord regions appear inside the chord track.

.. figure:: /_static/img/chord-track-with-region.png
   :align: center

   Chord track with chord region

Double-clicking a chord region will bring up the
:ref:`chord editor <editing/chord-editor:Chord Editor>`.

Markers
^^^^^^^
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
^^^^^^
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
~~~~~~~~~~~~~~~
The following operations apply to regions.

Renaming
++++++++
Regions can be renamed by selecting them and
pressing :kbd:`F2`.

.. figure:: /_static/img/region-rename.png
   :align: center

   Renaming a region

Adjusting Fades
+++++++++++++++
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

Fade Types
^^^^^^^^^^

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

Editing Region Contents
-----------------------

Displaying Region Contents
~~~~~~~~~~~~~~~~~~~~~~~~~~

When regions are selected in the timeline, their contents
will be shown inside the Editor tab in the :ref:`bottom-panel`.
Although there are some common elements shared amongst each
type of editor, the editor section will differ depending on
the type of region selected.

.. image:: /_static/img/editor-tab.png
   :align: center

Editor Ruler
~~~~~~~~~~~~
The editor ruler is similar to the timeline ruler.

.. figure:: /_static/img/editor-ruler.png
   :align: center

   Editor ruler

The ruler will display more or less information
depending on the current zoom level. It will also
display the following indicators.

Editor Ruler Indicators
+++++++++++++++++++++++

Song Markers
  Markers created in the :ref:`Marker track <tracks/track-types:Marker Track>` will be displayed here as read-only.
Clip Start
  Controls the position where the clip will start
  playback from. Displayed as a red, right-pointing arrow.
Playhead
  Current position of the playhead. Displayed as a
  grey, down-facing arrow.
Loop Points
  These control the range where the clip will loop
  after it reaches the loop end point. Displayed
  as 2 green arrows.

You can move these markers by clicking and dragging.

Clicking and dragging on empty space will allow you
to reposition the playhead.

Regions in the Editor Ruler
+++++++++++++++++++++++++++

All regions in the same track as the active region
will be shown inside the ruler. The active region
will be displayed in a more prominent color.

.. figure:: /_static/img/editor-ruler-regions.png
   :align: center

   Regions in the editor ruler
