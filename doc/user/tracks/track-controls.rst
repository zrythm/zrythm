.. This is part of the Zrythm Manual.
   Copyright (C) 2022 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

Track Controls
==============

Tracks have various controls available in the track
views inside the tracklist and in their corresponding
channels inside the mixer. Each available control
is explained below.

Signal Flow
-----------

Record
  Arm the track for recording.
Mute
  Mute the track, meaning no signal will be sent to
  its direct out.

  .. note:: The track will still be processed, so if
     you are looking to decrease :term:`DSP` usage,
     try disabling plugins instead.

Solo
  Solo the track. If any track is soloed, only the
  soloed tracks will produce sounds.
Listen
  Similar to solo, except it dims the volume of other
  tracks instead of muting them. The dim amount can
  be controlled in the
  :ref:`Monitor Section <routing/monitor-section:Monitor Section>`.
Monitor
  Listen to incoming signal when recording.

  .. note:: This is only used on audio tracks.

Visibility
----------

Show Lanes
  Toggle the visibility of
  :ref:`track lanes <tracks/track-lanes:Track Lanes>`.
Show Automation
  Toggle the visibility of
  :ref:`automation tracks <tracks/automation:Automation Tracks>`.
Show UI
  Show or hide the UI of the instrument plugin.

  .. note:: Only available on instrument tracks.

Other
-----

Lock
  Prevent any edits on the track while locked.

  .. note:: Not implemented yet.

Freeze
  Bounce the track to an audio file internally and
  prevent edits while frozen. This is intended to
  reduce :term:`DSP` load on DSP-hungry tracks.

  .. note:: Not implemented yet.
