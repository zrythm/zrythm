.. SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
   SPDX-License-Identifier: GFDL-1.3-invariants-or-later
.. This is part of the Zrythm Manual.
   See the file index.rst for copying conditions.

Track Lanes
===========

Some tracks can have multiple lanes for their
regions. By default, all regions are created on the
first lane. Additional lanes will be
auto-created and deleted as you add regions to each
lane.

.. figure:: /_static/img/track-lanes.png
   :align: center

   Instrument track lanes

Lanes can be made visible by clicking the
"hamburger" icon in the track view.

Organization
------------

Track lanes are useful for organizing regions in
a track into layers. For example you can keep high
notes in lane 1 and bass notes in lane 2.

Recording
---------

Lanes are also useful when recording. If you set the
:ref:`recording mode <playback-and-recording/recording:Recording Modes>`
to :guilabel:`Create takes`, each time playback
loops it will start recording in a new lane and
optionally mute the last recorded region in the
previous lane.

MIDI Export
-----------

When exporting MIDI from MIDI and instrument tracks,
MIDI events in each lane can be exported as separate
tracks in the resulting MIDI file.
