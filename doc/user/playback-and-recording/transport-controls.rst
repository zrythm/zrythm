.. SPDX-FileCopyrightText: © 2019-2021 Alexandros Theodotou <alex@zrythm.org>
   SPDX-License-Identifier: GFDL-1.3-invariants-or-later
.. This is part of the Zrythm Manual.
   See the file index.rst for copying conditions.

.. _transport-controls:

Transport Controls
==================

Playback is controlled by the following
transport controls.

.. image:: /_static/img/transport-controls.png
   :align: center

Controls
--------

Metronome toggle
  Toggles the metronome on/off.

  When the metronome is enabled, you will hear
  metronome
  ticks during playback at each bar and each beat.
  The tick at each bar will be more emphasized.
  This feature is useful for making sure the song
  stays on beat.

  .. tip:: The metronome samples can be overridden by
    placing your own samples in your Zrythm directory.
    This is a TODO feature.

Metronome options
  Sets the volume of the metronome
Return to cue point on stop
  Toggles whether to return to the
  :ref:`playback-and-recording/loop-points-and-markers:Cue Point`
  when playback stops.
Record
  Arms the project for recording.
Record options
  Sets the settings to use when recording.
Play
  If stopped, the song will start playing. If
  already playing, the Playhead will move to
  the Cue point.
Stop
  Pauses playback. If clicked twice, goes
  back to the Cue point.
Backward
  Moves the Playhead backward by the size of
  1 snap point.
Forward
  Moves the Playhead forward by the size of
  1 snap point.
Loop
  If enabled, the Playhead will move back to
  the Loop Start point when it reaches the
  Loop End point.

Playhead Display
----------------
The playhead position can be displayed in either
BBT notation (`bars.beats.sixteenths.ticks`)
or actual time (`mm:ss.ms`). This can be toggled by
right-clicking on the playhead position display.

.. image:: /_static/img/playhead-display-settings.png
   :align: center

JACK Transport
--------------
Zrythm can sync to
`JACK transport <https://jackaudio.org/api/transport-design.html>`_
or become the JACK transport master.
These options are available by right-clicking the
playhead position display when using the JACK
audio backend and will be remembered across
projects.

.. image:: /_static/img/jack-transport-settings.png
   :align: center
