#include "./midi_lane.h"

MidiLane::MidiLane (LanedTrackImpl<MidiLane> * track)
    : TrackLaneImpl<MidiRegion> (track)
{
  RegionOwner::parent_base_qproperties (*this);
}

void
MidiLane::init_after_cloning (const MidiLane &other, ObjectCloneType clone_type)

{
  RegionOwner<MidiRegion>::copy_members_from (other, clone_type);
  TrackLaneImpl::copy_members_from (other, clone_type);
}
