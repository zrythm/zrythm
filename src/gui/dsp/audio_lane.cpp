#include "./audio_lane.h"

AudioLane::AudioLane ()
{
  RegionOwnerImpl::parent_base_qproperties (*this);
}

AudioLane::AudioLane (LanedTrackImpl<AudioLane> * track, int pos)
    : TrackLaneImpl<AudioRegion> (track, pos)
{
  RegionOwnerImpl::parent_base_qproperties (*this);
}

void
AudioLane::init_after_cloning (const AudioLane &other, ObjectCloneType clone_type)
{
  RegionOwnerImpl<AudioRegion>::copy_members_from (other, clone_type);
  TrackLaneImpl::copy_members_from (other, clone_type);
}
