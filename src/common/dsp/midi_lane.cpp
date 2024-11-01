#include "./midi_lane.h"

MidiLane::MidiLane () = default;

MidiLane::MidiLane (LanedTrackImpl<MidiLane> * track, int pos)
    : TrackLaneImpl<MidiRegion> (track, pos)
{
}

void
MidiLane::init_after_cloning (const MidiLane &other)
{
  TrackLaneImpl::copy_members_from (other);
}