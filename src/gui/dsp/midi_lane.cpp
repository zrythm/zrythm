#include "./midi_lane.h"

MidiLane::MidiLane ()
{
  RegionOwnerImpl::parent_base_qproperties (*this);
}

MidiLane::MidiLane (LanedTrackImpl<MidiLane> * track, int pos)
    : TrackLaneImpl<MidiRegion> (track, pos)
{
  RegionOwnerImpl::parent_base_qproperties (*this);
}

void
MidiLane::init_after_cloning (const MidiLane &other)
{
  RegionOwnerImpl<MidiRegion>::copy_members_from (other);
  TrackLaneImpl::copy_members_from (other);
}