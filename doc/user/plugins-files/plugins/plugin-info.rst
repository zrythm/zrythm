.. This is part of the Zrythm Manual.
   Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

Plugin Overview
===============

A plugin is an external module that provides audio processing
capabilities to Zrythm, such as an SFZ instrument or an LV2
reverb plugin. There are various types of plugins
supported by Zrythm mentioned in :ref:`scanning-plugins`.

.. tip:: In Zrythm, SFZ/SF2 are also used as plugins.

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
  These plugins output CV signals and can be used to modulate
  other plugin or track parameters.
MIDI Effects
  These plugins modify incoming MIDI signals and
  can be used
  in the MIDI signal chain of an Instrument or MIDI
  track (coming soon).

.. note::
  Zrythm looks inside the Plugin's metadata to
  determine what type the Plugin is. If that is not
  enough, Zrythm makes a decision based on the
  number of audio, MIDI, control and CV input and
  output ports a Plugin has.

Plugin Ports
------------
Plugins consist of controls (parameters) and
a variety of audio, MIDI and CV ports. Ports
are explained in :ref:`ports`.

Zrythm first creates all plugins with a special `Enabled`
control port that controls bypassing, and the rest
of the plugin's ports are appended.

All of the plugin's ports can be found in the plugin
inspector page described in :ref:`plugin-inspector-page`
