.. This is part of the Zrythm Manual.
   Copyright (C) 2022 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

Track Lanes
===========

Some tracks can have multiple lanes for their
regions. By default, all regions you create will be
created in lane 1. Additional lanes will be
auto-created and deleted as you add regions to each
lane.

.. image:: /_static/img/track_lanes.png
   :align: center

Recording
---------

Lanes are useful when recording,
as each time the playback loops it will start
recording in a new lane and mute the region in the
previous lane.

Lanes can be made visible by clicking
the "hamburger" icon.

Organization
------------

Lanes are also useful for generally organizing
regions. For example you can keep high notes in
lane 1 and bass notes in lane 2.

MIDI Export
-----------

For MIDI and instrument tracks, MIDI events in each
lane can be exported as separate tracks in the
resulting MIDI file.
