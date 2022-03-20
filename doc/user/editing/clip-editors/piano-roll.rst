.. This is part of the Zrythm Manual.
   Copyright (C) 2020, 2022 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

.. _piano-roll:

Piano Roll
==========

The piano roll is the main editor for editing MIDI regions.

.. image:: /_static/img/piano-roll.png
   :align: center

It consists of the toolbar discussed in :ref:`editor-toolbar`
at the very top, a smaller space containing extra controls, the
ruler discussed in :ref:`editor-ruler`, the piano roll keys
on the left, the piano roll (MIDI) arranger on the right
and the velocity editor at the bottom.

Additional Controls
-------------------
The piano roll contains the following additional controls.

.. image:: /_static/img/piano-roll-controls.png
   :align: center

Drum View
~~~~~~~~~
Enabling this will change the display of the
arranger into a drum editor view, where each
:term:`MIDI note` is drawn as a diamond and the
piano roll keys display the name of each drum.

Listen to notes
~~~~~~~~~~~~~~~
Enabling this will play back each note as you drag
it inside the piano roll. This is useful if you
want to `hear` what you are doing.

.. _piano-roll-keys:

Piano Roll Keys
---------------
These are the keys corresponding to each key found
on a piano.

.. image:: /_static/img/piano-roll-keys.png
   :align: center

Clicking on each key allows you to listen to it, much like
pressing a key on a real piano.

A label for each key is shown on the left side of the
black/white keys, in addition to any highlighting
(if selected).
See :ref:`chord-highlighting` for more information.

.. tip:: The zoom level can be changed by holding
   down :kbd:`Ctrl` and :kbd:`Shift` and scrolling.

MIDI Arranger
-------------
The MIDI arranger refers to the arranger section of the piano
roll.

.. image:: /_static/img/midi-arranger.png
   :align: center

The MIDI arranger contains MIDI notes drawn as rectangles
from their start position to their end position. Zrythm
will send a MIDI note on signal when the start position
of a note is reached, and a note off signal when the end
position of a note is reached.

Editing inside the MIDI arranger is covered in
:ref:`edit-tools` and :ref:`common-operations`.

Drum Mode
---------

.. todo:: Explain/show how the 2 sections above look
   in drum mode.

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

.. image:: /_static/img/ramp-tool.png
   :align: center

Context Menu
------------
.. todo:: Write this section.
