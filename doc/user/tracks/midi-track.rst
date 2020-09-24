.. This is part of the Zrythm Manual.
   Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

MIDI Track
==========

A MIDI track contains MIDI regions and its purpose
is playing back MIDI events and routing
those MIDI signals to other instruments
(including external instruments and hardware).

MIDI tracks, unlike instrument tracks, produce MIDI
output and so their channel strips will only have
MIDI effects (plugins that change MIDI signals).

MIDI tracks also have automation lanes for automating
various parameters.

.. image:: /_static/img/midi-track.png
   :align: center

Top Buttons
-----------

Record
  Arms the track for recording.
Solo
  Soloes the track.
Mute
  Mutes the track.

Bottom Buttons
--------------

Lock
  Not used at the moment.
Show Lanes
  Toggles the visibility of lanes.
Show Automation
  Toggles the visibility of automation tracks.

Inspector
---------

.. image:: /_static/img/midi-track-inspector.png
   :align: center

Properties
~~~~~~~~~~

Track Name
  The name of the track. You can double click to
  change it.
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

.. _midi-track-sends:

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
created in lane 1. Additional lanes will be auto-created
and deleted as you add regions to each lane.

.. image:: /_static/img/track_lanes.png
   :align: center

Lanes are useful when recording,
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

.. image:: /_static/img/automation_tracks.png
   :align: center

More information about automation tracks can be found
in the :ref:`automation` chapter.

Context Menu
------------

TODO

MIDI Transformations
--------------------

Transpose
~~~~~~~~~

TODO
