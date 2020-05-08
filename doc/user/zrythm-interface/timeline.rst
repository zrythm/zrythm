.. This is part of the Zrythm Manual.
   Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

.. _timeline:

Timeline
========

The timeline consists of the timeline arranger and the
project's tracks on the left side. It has a special
toolbar at the top for timeline-specific actions.

.. image:: /_static/img/timeline.png
   :align: center

Tracklist
---------
The Tracklist contains all of the Tracks in the
project. It is split into the top (pinned)
Tracklist and the bottom (main) Tracklist.

.. image:: /_static/img/tracklist.png
   :align: center

The numbers at the top indicate the number of visible
tracks versus the total number of tracks.

The search box is a TODO feature.

Timeline Arranger
-----------------
The timeline arranger is the main area where the song is
composed. It consists of a collection of events, such as
regions, positioned against time. Some events will open
separate windows for further editing when clicked.

.. image:: /_static/img/timeline-arranger.png
   :align: center

The timeline arranger is split into a top arranger that
remains fixed on top and a bottom arranger below it. This
way you can pin tracks you want to always be visible at
the top.

Ruler
-----
A ruler is used to show the position of events in the
arranger.

.. image:: /_static/img/ruler.png
   :align: center

The ruler will display more or less information depending on
the current zoom level. It will also display the following
markers/indicators.

Cue point
  Displayed as a blue, right-pointing arrow.
Playhead position
  Shown as a grey, down-facing arrow.
Loop points
  Shown as 2 green arrows, and the area between them is
  shown in bright green if loop is enabled, or grey if
  disabled. Can be dragged to reposition.

Clicking and dragging on empty space in the ruler will
allow you to reposition the playhead.

.. tip:: Hold :zbutton:`Shift` to disable snapping
  momentarily while moving things around

Minimap
-------
The timeline minimap is a little box that represents the
current visible area of the timeline. It can be moved around
and resized to change the visible area.

.. image:: /_static/img/timeline-minimap.png
   :align: center
