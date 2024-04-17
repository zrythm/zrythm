.. SPDX-FileCopyrightText: © 2020, 2024 Alexandros Theodotou <alex@zrythm.org>
   SPDX-License-Identifier: GFDL-1.3-invariants-or-later
   This is part of the Zrythm Manual.
   See the file index.rst for copying conditions.

.. _piano-roll:

Piano Roll
==========

The piano roll is the main editor for editing MIDI
regions, and is displayed when a MIDI region is
selected.

.. figure:: /_static/img/piano-roll.png
   :align: center

   Piano roll

Additional Controls
-------------------
The piano roll contains the following additional
controls.

.. figure:: /_static/img/piano-roll-controls.png
   :align: center

   Piano roll controls

Drum View
  Switch to drum editor view (or `Drum mode`),
  where each
  :term:`MIDI note` is drawn as a diamond and the
  piano roll keys display the name of each drum.

  .. figure:: /_static/img/drum-mode.png
     :align: center

     Drum mode

Listen to notes
  Play back each note as you drag it inside the
  piano roll.

.. _piano-roll-keys:

Piano Roll Keys
---------------
These are the keys corresponding to each key found
on a piano.

.. figure:: /_static/img/piano-roll-keys.png
   :align: center

   Piano roll keys

Clicking on each key allows you to listen to it,
much like pressing a key on a real piano.

A label for each key is shown on the left side of the
black/white keys, in addition to any highlighting
(if selected).
See :ref:`editing/editor-toolbars:Chord/Scale Highlighting` for more information.

.. tip:: The vertical zoom level can be changed by
   holding down :kbd:`Ctrl` and :kbd:`Shift` and
   scrolling.

MIDI Arranger
-------------
The MIDI arranger refers to the arranger section of the piano
roll.

.. figure:: /_static/img/midi-arranger.png
   :align: center

   MIDI arranger with notes

The MIDI arranger contains MIDI notes drawn as rectangles
from their start position to their end position. Zrythm
will send a MIDI note on signal when the start position
of a note is reached, and a note off signal when the end
position of a note is reached.

Velocity Editor
---------------
There is another arranger at the bottom of the MIDI arranger
for editing the velocity of each note.

.. image:: /_static/img/velocity-editor.png
   :align: center

Velocity refers to the force with which a note is played.
Each instrument handles various velocities for the same note
differently, but generally a higher velocity will result in
a louder or `stronger` note. Velocities are useful for adding
expression to melodies.

The velocity of each note is drawn as a bar, where higher
means a larger value. Clicking and dragging the tip of each
velocity bar allows you to change it.

.. tip:: You can also edit multiple velocities at once by
  selecting multiple MIDI notes before editing a velocity
  bar.

The :ref:`ramp-mode` can also be used in the velocity editor to
`ramp` velocities.

.. figure:: /_static/img/ramp-tool.png
   :align: center

   Using the ramp tool on MIDI note velocities

