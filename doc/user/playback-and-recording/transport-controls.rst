.. This is part of the Zrythm Manual.
   Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

Record
  Arms the project for recording.
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

Metronome
---------
Clicking the metronome button will enable/disable
the metronome.

.. image:: /_static/img/metronome-button.png
   :align: center

When the metronome is enabled, you will hear
metronome
ticks during playback at each bar and each beat.
The tick at each bar will be more emphasized.
This feature is useful for making sure the song
stays on beat.

.. tip:: The metronome samples can be overridden by
  placing your own samples in your Zrythm directory.
  This is a TODO feature.
