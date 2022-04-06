.. SPDX-FileCopyrightText: © 2019, 2022 Alexandros Theodotou <alex@zrythm.org>
   SPDX-License-Identifier: GFDL-1.3-invariants-or-later
.. This is part of the Zrythm Manual.
   See the file index.rst for copying conditions.

Chord Pad
=========

Overview
--------

The chord pad contains 12 pads (1 for each key on a
piano keyboard) that can be used to trigger chords.
The chord pad can be found in the bottom panel.

.. image:: /_static/img/chord-pad.png
   :align: center

These 12 chords are linked to the chords in the
:ref:`Chord Editor <editing/clip-editors/chord-editor:Chord Editor>`.

Presets
-------

Zrythm comes with pre-installed chord presets, and
it also allows you to generate chord presets from
scales, or even save your own.

Preset Packs
~~~~~~~~~~~~

A list of preset packs and presets is found in the
:ref:`Right Panel <zrythm-interface/right-panel:Right Panel>`
under :guilabel:`Chord Preset Browser`.

.. figure:: /_static/img/chord-preset-browser.png
   :align: center

   Chord preset browser

Filtering
~~~~~~~~~

Clicking a preset pack in the top will filter the
list of presets to presets in the selected pack.

Auditioning
~~~~~~~~~~~

You can audition chord presets by selecting an
instrument plugin from the list and using the available
playback controls. This works similarly to
:ref:`auditioning in the File Browser <plugins-files/audio-midi-files/file-browser:Auditioning>`.

Standard Packs
~~~~~~~~~~~~~~

Zrythm comes with some standard packs that cannot be
renamed or deleted.

Custom Packs
~~~~~~~~~~~~

Creating Custom Packs
+++++++++++++++++++++

You can create your own pack by clicking the
:guilabel:`Add` button below the list of chord
preset packs.

.. figure:: /_static/img/chord-preset-pack-creation.png
   :align: center

   Creating a chord preset pack

Editing Custom Packs
++++++++++++++++++++

Custom packs can be renamed or deleted by
right-clicking.

.. figure:: /_static/img/chord-preset-pack-right-click.png
   :align: center

   Renaming or deleting a custom chord preset pack

Chord presets under the selected pack can also be
renamed and deleted.

Saving Presets
++++++++++++++

Clicking on the :guilabel:`Save Preset` button in the
top part of the chord pad will allow you to save the
current chord configuration in a preset.

.. figure:: /_static/img/chord-preset-save.png
   :align: center

   Saving a chord preset

Loading Presets
~~~~~~~~~~~~~~~

To load a preset, click the :guilabel:`Load Preset`
button in the top part of the chord pad, then select
the preset you want to use.

.. figure:: /_static/img/chord-preset-selection.png
   :align: center

   Selecting a chord preset

.. tip:: You can also load a preset by double clicking
   on it in the preset browser.

Generating Chords from Scale
----------------------------

Chords can be generated from a scale and a root note
by clicking on the :guilabel:`Load Preset` button,
then selecting a scale and a root note.

.. figure:: /_static/img/chord-preset-selection-from-scale.png
   :align: center

   Generating chords from a scale

Transpose
---------

Clicking on the up/down transpose buttons will
transpose all chords by 1 semitone up or down.

Editing Chords
--------------

Chord Selection
~~~~~~~~~~~~~~~

Clicking on the chord button will bring up the
chord selector window that allows you to change
the current chord.

.. figure:: /_static/img/chord-selector.png
   :align: center

   Chord selector

At the moment, only the :guilabel:`Chord Creator` tab
is functional. A chord can be created by selecting
its root note, its type, its accent and its bass note.

Selecting :guilabel:`In scale` under
:guilabel:`Visibility`
will only show options that correspond to chords
inside the currently active scale.
This makes it easy to create chords that
stay in the current scale.

Inversions
~~~~~~~~~~

Clicking on the left or right arrows will allow you
to invert the chord. Each inversion towards the right
moves the lowest note in the chord one octave higher,
and each inversion towards the left moves the highest
note in the chord one octave below.
