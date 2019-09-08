.. This is part of the Zrythm Manual.
   Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

Ruler
=====

A ruler is used to show the position of events in a given arranger,
whether it is the timeline arranger or the piano roll or the sample
editor.

.. image:: /_static/img/ruler.png
   :align: center

The ruler will display more or less information depending on the
current zoom level. It will also display the following markers/
indicators.

Cue point
  Displayed as a blue, right-pointing arrow.
Playhead position
  Shown as a grey, down-facing arrow.
Loop points
  Shown as 2 green arrows, and the area between them is
  shown in bright green if loop is enabled, or grey if
  disabled. Can be dragged to reposition.

  In the timeline arranger, these are the global loop points.
  In arrangers found in the editor, these are the region loop
  points.

Clicking and dragging on empty space in the ruler will allow you
to reposition the playhead.

.. tip:: Hold :zbutton:`Shift` to disable snapping momentarily while
  moving things around
