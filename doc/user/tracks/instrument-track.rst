.. This is part of the Zrythm Manual.
   Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

.. _instrument-track:

Instrument Track
================

The instrument track is used for synths and
other instruments. Instrument tracks contain
a special `instrument slot` in the mixer that
will get processed `after` the MIDI FX section
and `before` the Inserts section.

.. image:: /_static/img/instrument-track.png
   :align: center

Instrument tracks are similar to MIDI tracks, except
that they produce audio instead of :term:`MIDI`.

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

If :term:`DSP` usage is high and you have
performance issues, or if you just want to save CPU
power, instrument tracks can be frozen (temporarily
converted to audio), which dramatically lowers
their CPU usage.

Instruments can be frozen by clicking the
Freeze button in the track interface.
