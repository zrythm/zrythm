=================
audio track
=================

Scheme Procedure: **midi-track-new** *idx name*
   Returns a new track with name name to be placed at position idx in
   the tracklist.


Scheme Procedure: **track-get-name** *track*
   Returns the name of track.


Scheme Procedure: **track-get-processor** *track*
   Returns the processor of track.


Scheme Procedure: **track-get-channel** *track*
   Returns the channel of track.


Scheme Procedure: **track-set-muted** *track muted*
   Sets whether track is muted or not. This creates an undoable action
   and performs it.


Scheme Procedure: **track-add-lane-region** *track region lane_pos*
   Adds region to track track. To be used for regions with lanes
   (midi/audio)


