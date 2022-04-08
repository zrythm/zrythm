.. SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
   SPDX-License-Identifier: GFDL-1.3-invariants-or-later
   This is part of the Zrythm Manual.
   See the file index.rst for copying conditions.

.. _timeline-ruler:

Timeline Ruler
==============

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
  Shown as 2 green arrows, and the area between them is
  shown in bright green if loop is enabled, or grey if
  disabled. Can be dragged to reposition.

Clicking and dragging on empty space in the
bottom half of the ruler will allow
you to reposition the playhead.

Setting the Cue Point
---------------------
Double click inside the ruler to set the cue point. This
will be used to return to when playback is stopped.

Range Selection
---------------
Clicking and dragging in the top half of the ruler
will create a :term:`range <Range>` selection. If
an existing range exists, you can click an drag it
to move it.

.. figure:: /_static/img/ranges.png
   :align: center

   Range selection
