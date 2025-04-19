// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "./audio_lane.h"

AudioLane::AudioLane (LanedTrackImpl<AudioLane> * track)
    : TrackLaneImpl<AudioRegion> (track)
{
}

void
AudioLane::init_after_cloning (const AudioLane &other, ObjectCloneType clone_type)
{
  ArrangerObjectOwner::copy_members_from (other, clone_type);
  TrackLaneImpl::copy_members_from (other, clone_type);
}
