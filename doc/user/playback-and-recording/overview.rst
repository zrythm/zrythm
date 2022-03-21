.. This is part of the Zrythm Manual.
   Copyright (C) 2019, 2022 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

Overview
========

Here is a list of terms and definitions that
will be useful in the following sections.

Playhead
--------
The :dfn:`playhead` is the current position in the
song. It is displayed as a red line inside
arrangers.

.. figure:: /_static/img/playhead.png
   :align: center

   The playhead

Position
--------

Zrythm uses the following format for positions:

::

  bar.beat.sixteenth-note.tick

Each sixteenth note is fixed to have 240
ticks.

.. hint:: Ticks are expressed in decimals.
