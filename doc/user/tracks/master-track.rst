.. This is part of the Zrythm Manual.
   Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

Master Track
============

The master track is a special type of
:ref:`audio-group-track` that Zrythm uses
to route the resulting audio signal after
all the processing is done to the
audio backend.

.. image:: /_static/img/master-track.png
   :align: center

Top Buttons
-----------

Solo
  Soloes the track.
Mute
  Mutes the track.

Bottom Buttons
--------------

Show Automation
  Toggles the visibility of automation tracks.

Inspector
---------

.. image:: /_static/img/master-track-inspector.png
   :align: center

Properties
~~~~~~~~~~

Track Name
  The name of the track. This cannot be changed.
Direct Out
  This is automatically routed to the audio engine
  and cannot be changed.

Sends
~~~~~

These are similar to :ref:`instrument-track-sends`.

Automation Tracks
-----------------

These are similar to :ref:`midi-track-automation-tracks`.
