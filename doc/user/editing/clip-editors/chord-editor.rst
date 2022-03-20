.. This is part of the Zrythm Manual.
   Copyright (C) 2020, 2022 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

.. _chord-editor:

Chord Editor
============
The chord editor is displayed when a chord region is
selected.

.. image:: /_static/img/chord-editor.png
   :align: center

Its ruler behaves similarly to the editor ruler explained
in :ref:`editor-ruler`.

The chord editor has a list of chords on the left side
that can be changed, and an arranger on the right side
for creating chord progressions.

Chords
------
A chord in the context of the chord editor refers to a
chord on the left side of the editor.

.. image:: /_static/img/chord-editor-chords.png
   :align: center

These chords act similarly to the piano roll keys explained
in :ref:`piano-roll-keys`.

Chord Selector
--------------
Double-clicking on a chord will bring up the chord selector,
which allows you to change that chord.

.. image:: /_static/img/chord-selector.png
   :align: center

At the moment, only the chord creator is functional. A
chord can be created by selecting its root note, its type,
its accent and its bass note.

Selecting :zbutton:`In scale` under `Visibility` will
only show options that correspond to chords inside the
scale active at the start of the region. This makes it
easy to create chords that stay in the current scale.

Chord Arranger
--------------
The chord arranger refers to the arranger section of the
chord editor.

.. image:: /_static/img/chord-arranger.png
   :align: center

The chord editor contains `chord objects` that signify the
start of a chord. Chord objects do not have a length.

Editing in the chord arranger follows :ref:`edit-tools` and
:ref:`common-operations`.

Context Menu
------------
.. todo:: Write this section.
