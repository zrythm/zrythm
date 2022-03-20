.. This is part of the Zrythm Manual.
   Copyright (C) 2020, 2022 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

.. _chord-editor:

Chord Editor
============
The chord editor is displayed when a chord region is
selected.

.. figure:: /_static/img/chord-editor.png
   :align: center

   Chord editor

The chord editor has a list of chords on the left
side that can be changed, and an arranger on the
right side for creating chord progressions.

Chords
------
A chord in the context of the chord editor refers
to a chord on the left side of the editor.

.. figure:: /_static/img/chord-editor-chords.png
   :align: center

   Chords inside the chord editor

These chords correspond to each chord in the
:ref:`Chord pad <chords-and-scales/overview:Chord Pad>`.


Each chord row contains the following controls.

Chord Selector
  Change the selected chord.

  .. figure:: /_static/img/chord-selector.png
     :align: center

     Chord selector

  At the moment, only the chord creator is
  functional. A chord can be created by selecting
  its root note, its type,
  its accent and its bass note.

  Selecting :guilabel:`In scale` under
  :guilabel:`Visibility`
  will only show options that correspond to chords
  inside the scale active at the start of the
  region. This makes it easy to create chords that
  stay in the current scale.

Inversion
  Invert the chord.

.. todo:: Move this part to the chords and scales
   chapter.

Chord Arranger
--------------
The chord arranger refers to the arranger section
of the chord editor.

.. figure:: /_static/img/chord-arranger.png
   :align: center

   Chord arranger

The chord editor contains `chord objects` that
signify the start of a chord.

.. note:: Chord objects do not have a length.

Editing in the chord arranger follows
:ref:`edit-tools` and
:ref:`common-operations`.
