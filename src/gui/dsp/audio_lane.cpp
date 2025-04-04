#include "./audio_lane.h"

AudioLane::AudioLane (LanedTrackImpl<AudioLane> * track)
    : TrackLaneImpl<AudioRegion> (track)
{
  RegionOwner::parent_base_qproperties (*this);
}

void
AudioLane::init_after_cloning (const AudioLane &other, ObjectCloneType clone_type)
{
  RegionOwner<AudioRegion>::copy_members_from (other, clone_type);
  TrackLaneImpl::copy_members_from (other, clone_type);
}
