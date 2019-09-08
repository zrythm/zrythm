.. This is part of the Zrythm Manual.
   Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

Basic Concepts and Terminology
==============================

Here are a few terms you should be aware of when using Zrythm. They are explained further in their corresponding chapters.

Regions
-------
A Region (Clip) is a container for MIDI Notes or audio. This is what a Region looks like in the arranger.

.. image:: /_static/img/clip_arranger.png

Regions are edited in the Editor Panel. In this case, the clip is looped.

.. image:: /_static/img/midi_clip_editor.png

Timeline
--------
The Timeline is where the song is arranged, also known as Arranger.

.. image:: /_static/img/timeline.png

Track
-----
A Track is a single slot in the Timeline containing various Regions and Automation. It may contain various lanes, such as Automation Lanes.
There are some special tracks like the Chord Track and the Marker Track that contain chords and markers respectively.

.. image:: /_static/img/track.png

Tracklist
---------
The Tracklist contains all of the Tracks in the project. It is split into the top (pinned) Tracklist and the bottom (main) Tracklist.

.. image:: /_static/img/tracklist.png

Channel
-------
A Channel is a single slot in the Mixer. Most types of Tracks have a corresponding Channel.

.. image:: /_static/img/channel.png

Mixer
-----
The Mixer contains all of the Channels in the Project and is used to mix the audio signals from each Channel.

.. image:: /_static/img/mixer.png

Range
------
A Range is a selection of time between two positions.

.. image:: /_static/img/ranges.png

MIDI Note
---------
MIDI Notes are used to trigger virtual (or hardware) instruments.
