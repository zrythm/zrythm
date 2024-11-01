#include "./audio_lane.h"

AudioLane::AudioLane () = default;

AudioLane::AudioLane (LanedTrackImpl<AudioLane> * track, int pos)
    : TrackLaneImpl<AudioRegion> (track, pos)
{
}

void
AudioLane::init_after_cloning (const AudioLane &other)
{
  TrackLaneImpl::copy_members_from (other);
}
