.. This is part of the Zrythm Manual.
   Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

Instrument Track
================

The instrument track is used for synths and
other instruments. The first plugin in the
strip of the instrument track's channel must
be an instrument plugin. This is done
automatically when instrument tracks are
created from instrument plugins.

.. image:: /_static/img/instrument-track.png
   :align: center

Top Buttons
-----------

Record
  Arms the track for recording.
Solo
  Soloes the track.
Mute
  Mutes the track.
Show UI
  Displays the UI of the main plugin.

Bottom Buttons
--------------

Lock
  Not used at the moment.
Freeze
  Freezes the track (not operational at the moment).
Show Lanes
  Toggles the visibility of lanes.
Show Automation
  Toggles the visibility of automation tracks.

Inspector
---------

.. image:: /_static/img/instrument-track-inspector.png
   :align: center

Properties
~~~~~~~~~~

Direct Out
  The track to route the audio output of the instrument track to.

Inputs
~~~~~~

These are similar to :ref:`midi-track-inputs`.

.. _instrument-track-sends:

Sends
~~~~~

Pre-Fader Out L/R
  The stereo audio signal before the fader is applied.

Fader Out L/R
  The stereo audio signal after the fader is applied.

.. tip:: You can use these for side-chaining.

Lanes
-----

These are similar to :ref:`midi-track-lanes`.

Automation Tracks
-----------------

These are similar to :ref:`midi-track-automation-tracks`.

Freezing
--------

If DSP usage is high and you have performance issues, or
if you just want to save CPU power, instrument tracks can
be frozen (temporarily converted to audio), which
dramatically lowers their CPU usage.

Instruments can be frozen by clicking the
Freeze button in the track interface.
