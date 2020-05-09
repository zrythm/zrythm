.. This is part of the Zrythm Manual.
   Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

.. _bottom-panel:

Bottom Panel
============

The bottom panel contains various editors (such as the piano
roll and automation editor) as well as the mixer and chord
pads.

.. image:: /_static/img/bot-panel.png
   :align: center

Editor
------
The Editor tab contains an arranger for editing regions, as
well as various controls that vary depending on the type of
region selected.

.. image:: /_static/img/editor-tab.png
   :align: center

Editing inside the Editor tab is covered in :ref:`editors`.

Piano Roll
~~~~~~~~~~

The Piano Roll, or MIDI Arranger, is the most commonly used
editor.
It can be used to edit MIDI regions, which contain MIDI notes.
When a MIDI Region is selected, the Editor tab will display the
Piano Roll, allowing you to edit that region.

.. image:: /_static/img/piano_roll.png
   :align: center

The piano roll is covered in detail in :ref:`piano-roll`.

Automation Editor
~~~~~~~~~~~~~~~~~

In Zrythm, automation is also enclosed in regions (called Automation
Regions). This allows automation to be repeated, much like MIDI Regions.
The Automation Editor will appear in the Editor tab when an Automation
Region is selected.

.. image:: /_static/img/automation-editor.png
   :align: center

The automation editor is covered in detail in
:ref:`automation-editor`.

Audio Editor
~~~~~~~~~~~~
The audio editor is used to view the contents of
audio regions.
The audio editor will appear in the Editor tab when an
audio region is selected.

.. image:: /_static/img/audio-editor.png
   :align: center

The audio editor is covered in detail in
:ref:`audio-editor`.

Chord Editor
~~~~~~~~~~~~
The chord editor is used for editing chord regions. It
will appear in the Editor tab when a chord region is selected
in the chord track.

.. image:: /_static/img/chord-editor.png
   :align: center

The chord editor is covered in detail in
:ref:`chord-editor`.

Mixer
-----
The Mixer tab contains each channel and various controls for
changing the signal as it passes through the channel.

.. image:: /_static/img/mixer-tab.png
   :align: center

Mixing is covered in :ref:`mixing`.
