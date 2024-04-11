.. SPDX-FileCopyrightText: © 2020, 2022 Alexandros Theodotou <alex@zrythm.org>
   SPDX-License-Identifier: GFDL-1.3-invariants-or-later
.. This is part of the Zrythm Manual.
   See the file index.rst for copying conditions.

Plugin Overview
===============

There are various :term:`plugin <Plugin>`
formats supported by Zrythm, covered in
:ref:`scanning-plugins`.

.. hint:: In Zrythm, :term:`SFZ`/:term:`SF2` are also used as plugins.

.. _plugin-types:

Plugin Types
------------

Plugins are classified into the following categories.

|icon_instrument| Instrument
  These plugins will create an
  :ref:`Instrument Track <tracks/track-types:Instrument Track>`
  when added to the project.

|icon_audio-insert| Effect
  These plugins can be dragged into the insert slots
  of existing channels in the
  :ref:`mixer <mixing/overview:Mixer>` or can be
  instantiated to create new FX tracks.

|icon_modulator| Modulator
  These plugins output :term:`CV` signals and can be
  used to modulate other plugin or track parameters.

|icon_signal-midi| MIDI Effect
  These plugins modify incoming :term:`MIDI` signals
  and can be used in the MIDI signal chain of an
  :ref:`Instrument <tracks/track-types:Instrument Track>` or
  :ref:`MIDI Track <tracks/track-types:MIDI Track>`.

.. note::
  Zrythm looks inside the plugin's metadata to
  determine what type the plugin is. If that is not
  enough, Zrythm makes a decision based on the
  number of audio, MIDI, control and CV input and
  output ports a plugin has.

Plugin Ports
------------
Plugins consist of controls (parameters) and
a variety of audio, :term:`MIDI` and :term:`CV`
:ref:`ports <ports>`.

Zrythm first creates all plugins with the following
special control ports.
After these ports are created, Zrythm appends
the rest of the plugin's ports.

Enabled
  Controls whether the plugin is active or bypassed.

Gain
  Increases the output volume of the plugin (if
  applicable).

.. seealso::
   All of the plugin's ports will be shown in the
   plugin's
   :ref:`inspector page <plugins-files/plugins/inspector-page:Inspector Page>`
