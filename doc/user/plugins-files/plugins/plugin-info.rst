.. This is part of the Zrythm Manual.
   Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

Plugin Overview
===============

There are various types of plugins (see
:term:`plugin`) supported by
Zrythm, mentioned in :ref:`scanning-plugins`.

.. tip:: In Zrythm, :term:`SFZ`/:term:`SF2` are also
   used as plugins.

.. _plugin-types:

Plugin Types
------------

Plugins are classified into the following categories.

Instrument
  These plugins will create an instrument track when added
  to the project.
Effects
  These plugins can be dragged into the insert slots of
  existing channels in the mixer or can be instantiated to
  create new FX tracks.
Modulators
  These plugins output :term:`CV` signals and can be used to modulate
  other plugin or track parameters.
MIDI Effects
  These plugins modify incoming :term:`MIDI` signals and
  can be used
  in the MIDI signal chain of an Instrument or MIDI
  track.

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
ports. Ports are explained in :ref:`ports`.

Zrythm first creates all plugins with the following
special control ports:

Enabled
  Controls whether the plugin is active or bypassed.
Gain
  Increases the output volume of the plugin (if
  applicable).

After the above ports are created, Zrythm appends
the rest of the plugin's ports.

All of the plugin's ports can be found in the plugin
inspector page explained in
:ref:`plugin-inspector-page`
