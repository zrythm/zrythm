.. This is part of the Zrythm Manual.
   Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

Overview
========

Chords and scales can be used to easily try out various
chords and to make it easier to follow chord progressions.

Chord Pad
---------

The chord pad contains 12 pads (1 for each key on a
piano keyboard) that can be used to trigger chords.
The chord pad can be found in the bottom panel.

.. image:: /_static/img/chord-pad.png
   :align: center

Chord Track
-----------

Chord regions and scale objects can be created inside
the chord track, and these can then be used to
dictate what chord and scale is active when.
For example, you can specify a chord progression as
follows.

.. figure:: /_static/img/chord-region-and-scale.png
   :figwidth: image
   :align: center

   Chord region and scale inside the chord track in
   the timeline.

.. figure:: /_static/img/chords-in-chord-region.png
   :figwidth: image
   :align: center

   Chord objects inside the chord region above.

.. _chord-highlighting:

Highlighting
~~~~~~~~~~~~

You can then use highlighting inside MIDI regions
to highlight the notes corresponding to the scale,
chord, or both at the position of the playhead.

.. figure:: /_static/img/chord-highlighting.png
   :figwidth: image
   :align: center

   Chord highlighting.

.. figure:: /_static/img/scale-highlighting.png
   :figwidth: image
   :align: center

   Scale highlighting.

.. figure:: /_static/img/scale-and-chord-highlighting.png
   :figwidth: image
   :align: center

   Scale and chord highlighting.

Live Audition
~~~~~~~~~~~~~

You can route the output of the chord track to
a MIDI or Instrument track for live auditioning of
the chord track's events.

.. note:: This is a TODO feature.
