.. SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
   SPDX-License-Identifier: GFDL-1.3-invariants-or-later
.. This is part of the Zrythm Manual.
   See the file index.rst for copying conditions.

Recording
=========

Arming Tracks
-------------
Tracks can be armed for recording by clicking the
record button, either inside the track or inside its
fader section in the channel or
:ref:`track inspector <track-inspector>`.

.. image:: /_static/img/armed-track.png
   :align: center

When tracks are armed for recording, they start
listening for signals in their :ref:`track-inputs`
and these signals will be passed along in the
processing chain.

For example, if an instrument track is armed for
recording and it has a MIDI keyboard connected in
its inputs, you will hear sound when you press a
key on your MIDI keyboard.

Recording Audio & MIDI
----------------------
After arming the tracks you want to record events
into, enable recording in :ref:`transport-controls`
and press play. Any signals received will be
recorded inside new regions in the timeline.

.. figure:: /_static/img/audio-track-recording.png
   :align: center

   Recording audio

Inputs
~~~~~~
Audio and MIDI/Instrument tracks have an
:ref:`tracks/inspector-page:Inputs`
section in their
:ref:`Inspector Page <tracks/inspector-page:Track Inspector>`
to adjust inputs for recording.

.. figure:: /_static/img/audio-track-inputs.png
   :align: center

   Track inputs

Recording Settings
~~~~~~~~~~~~~~~~~~
Clicking the arrow next to the record button will
bring up the recording settings where various options
and recording modes can be selected.

.. figure:: /_static/img/recording-modes.png
   :align: center

   Recording settings

Options
+++++++
Punch in/out
  Whether to use punch in/out markers. When this
  is enabled, recording will start when the playhead
  reaches the punch in marker and stop when the
  playhead reaches the punch out marker.

  .. figure:: /_static/img/punch-in-out-markers.png
     :align: center

     Punch in/out range, indicated by red markers

Start on MIDI input
  Automatically start recording when an event is
  received from a MIDI device

Recording Modes
+++++++++++++++
Overwrite events
  Overwrite any existing events in the track during
  recording
Merge events
  Merge the newly recorded events with the existing
  events in the track
Create takes
  Create a new lane (`take`) for each new recording
Create takes (mute previous)
  Similar to `Create takes` but mutes the last
  recorded take before creating a new one

  .. figure:: /_static/img/audio-recording-takes-mute-previous.png
     :align: center

     Create takes (mute previous)

Recording Automation
--------------------
Automation can be recorded in `latch` mode or
`touch` mode. The mode can be selected and toggled
by clicking on it inside automation tracks.

Latch
~~~~~

.. image:: /_static/img/automation-latch.png
   :align: center

In latch mode, automation will be written constantly
and overwrite previous automation.

Touch
~~~~~

.. image:: /_static/img/automation-touch.png
   :align: center

In touch mode, automation will be written only when
there are changes. When changes stop being received
(after a short amount of time) automation will stop
being written until a change is received again.
This is useful for making minor changes to existing
automation.

.. note:: Unlike MIDI/audio recording, automation recording
  does not require `Record` to be enabled in the
  :ref:`transport-controls`.
