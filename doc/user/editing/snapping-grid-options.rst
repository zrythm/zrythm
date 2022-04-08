.. SPDX-FileCopyrightText: © 2020, 2022 Alexandros Theodotou <alex@zrythm.org>
   SPDX-License-Identifier: GFDL-1.3-invariants-or-later
   This is part of the Zrythm Manual.
   See the file index.rst for copying conditions.

.. _snapping-grid-options:

Snapping and Grid Options
=========================

Snap Toolbar
------------
Zrythm will snap according to the options currently
specified by the snap toolbar buttons in each panel.

.. figure:: /_static/img/snap-toolbar.png
   :align: center

   Snap toolbar

Snap to Grid
  Turn snapping on/off
Keep Offset
  When snapping, keep the offset of objects from
  the nearest snap point on their left

  .. note:: Requires Snap to Grid to be toggled on

Snap to Events
  When snapping, snap to the start/end positions
  other objects

  .. note:: Requires Snap to Grid to be toggled on

Snap/Grid Options
-----------------

The Snap/Grid options control allows fine-tuning
snapping behavior inside arrangers.

.. figure:: /_static/img/snap-grid-options.png
   :align: center

   Snap/Grid options

Snap
~~~~
The Snap section controls the snapping behavior of
the start position of objects.

Note Length
  Note length to snap to
Triplet (t)
  Use triplet note (2/3rds of the selected note
  length)
Dotted (.)
  Use dotted note (3/2nds, or 1.5 times the selected
  note length)
Adaptive
  Adapt snapping to the current zoom level

Length
~~~~~~
The Length section controls the default length of
objects.

Link
  Use the same options as Snap
Last Object
  Use the length of the last created object
