.. SPDX-FileCopyrightText: © 2020-2022 Alexandros Theodotou <alex@zrythm.org>
   SPDX-License-Identifier: GFDL-1.3-invariants-or-later
   This is part of the Zrythm Manual.
   See the file index.rst for copying conditions.

.. _editor-ruler:

Editor Ruler
============
The editor ruler is similar to the timeline ruler
explained in :ref:`timeline-ruler`, with some
differences.

.. figure:: /_static/img/editor-ruler.png
   :align: center

   Editor ruler

The ruler will display more or less information
depending on the current zoom level. It will also
display the following markers and indicators.

Markers
-------

Clip Start
  Controls the position where the clip will start
  playback from. Displayed as a blue, right-pointing
  arrow.
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

Regions
-------

All regions in the same track as the active region
will be shown inside the ruler. The active region
will be displayed in a more prominent color.

.. figure:: /_static/img/editor-ruler-regions.png
   :align: center

   Regions in the editor ruler
