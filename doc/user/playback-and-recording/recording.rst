.. This is part of the Zrythm Manual.
   Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

Recording
=========

Arming Tracks
-------------
Tracks can be armed for recording by clicking the
record button, either inside the track or inside its
fader section in the channel or
:ref:`track inspector <track-inspector>`.
When tracks are armed for recording, they start
listening for signals in their :ref:`track-inputs`
and these signals will be passed along in the
processing chain.

.. todo:: Add illustration.

For example, if an instrument track is armed for
recording and it has a MIDI keyboard connected in
its inputs, you will hear sound when you press a
key on your MIDI keyboard.

Recording Audio & MIDI
----------------------
After arming the tracks you want to record events
into, enable recording in :ref:`transport-controls`
and press play.

.. todo:: Expand, add illustrations, etc.

Any signals received will be recorded inside new
regions in the timeline.

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
