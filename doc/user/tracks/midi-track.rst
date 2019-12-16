.. This is part of the Zrythm Manual.
   Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

MIDI Track
==========

A MIDI track contains MIDI regions and its purpose
is for routing MIDI signals to other instruments
(including external instruments and hardware).

MIDI tracks, unlike instrument tracks, produce MIDI
output and so their channel strips will only have
MIDI effects (plugins that change MIDI signals).

Inspector
---------

.. image:: /_static/img/midi-track-inspector.png
   :align: center

Properties
~~~~~~~~~~

Direct Out
  The track to route the output of the MIDI track to.

.. _midi-track-inputs:

Inputs
~~~~~~

Input Device
  The input device(s) to receive MIDI signals from
  during recording.

Input Channel
  The MIDI channel(s) to listen to for recording
  MIDI events. This acts as a filter so MIDI events
  on channels not selected will be ignored.

Sends
~~~~~

Pre-Fader Out
  The MIDI signal before the MIDI fader is applied.

Fader Out
  The MIDI signal after the MIDI fader is applied.

.. tip:: The MIDI fader uses MIDI volume CC.

.. note:: The MIDI fader is a TODO feature and currently
   does nothing (it lets the signal pass through).

.. _midi-track-lanes:

Lanes
-----

The MIDI track can have multiple lanes for saving
regions. By default, all regions you create will be
created in lane 1. Lanes are useful when recording,
as each time the playback loops it will start
recording in a new lane and mute the region in the
previous lane.

Lanes are also useful for generally organizing your
regions. For example you can keep high notes in
lane 1 and bass notes in lane 2.

Lanes can be made visible by clicking
the "hamburger" icon.

.. _midi-track-automation-tracks:

Automation Tracks
-----------------

Automation tracks are used to create automation events
for various parameters of the MIDI track, and any
plugins in the MIDI track's channel. To show the
automation tracks, click on the cusp icon in the MIDI
track.

More information about automation tracks can be found
in the :ref:`automation` chapter.

MIDI Transformations
--------------------

Transpose
~~~~~~~~~

TODO
