.. This is part of the Zrythm Manual.
   Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

Basic Concepts and Terminology
==============================

Here are a few terms you should be aware of when using Zrythm. They are explained further in their corresponding chapters.

Region
------
A Region (Clip) is a container for MIDI Notes,
audio or other events. Regions can be repeated,
like below.

.. image:: /_static/img/region.png
   :align: center

The content of the Regions can be edited in an
Editor, such as the Piano Roll, found in the Editor
tab in the bottom panel.

.. image:: /_static/img/piano_roll.png
   :align: center

Track
-----
A Track is a single slot in the Timeline containing
various Regions and Automation. It may contain
various lanes, such as Automation Lanes.
There are some special tracks like the Chord Track
and the Marker Track that contain chords and
markers respectively.

.. image:: /_static/img/track.png
   :align: center

Channel
-------
A Channel is a single slot in the Mixer. Most types
of Tracks have a corresponding Channel, which is
used for adjusting the Track's volume, pan and
other settings.

.. image:: /_static/img/channel.png
   :align: center

Range
------
A Range is a selection of time between two positions.

.. image:: /_static/img/ranges.png
   :align: center

MIDI Note
---------
MIDI Notes are used to trigger virtual (or
hardware) instruments.

.. image:: /_static/img/midi_note.png
   :align: center
