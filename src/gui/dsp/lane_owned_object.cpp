// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/dsp/lane_owned_object.h"
#include "gui/dsp/laned_track.h"

template <typename RegionT>
LaneOwnedObjectImpl<RegionT>::TrackLaneT *
LaneOwnedObjectImpl<RegionT>::get_lane () const
{
  if (owner_lane_ != nullptr)
    {
      return owner_lane_;
    }

  auto * region = dynamic_cast<const RegionT *> (this);
  auto   track_var = get_track ();
  return std::visit (
    [&] (auto &&track) -> TrackLaneT * {
      using TrackT = base_type<decltype (track)>;
      if constexpr (std::derived_from<TrackT, LanedTrack>)
        {
          z_return_val_if_fail (
            region->id_.lane_pos_ < (int) track->lanes_.size (), nullptr);

          return std::get<TrackLaneT *> (
            track->lanes_.at (region->id_.lane_pos_));
        }
      else
        {
          z_return_val_if_reached (nullptr);
        }
    },
    track_var);
}

template <typename RegionT>
void
LaneOwnedObjectImpl<RegionT>::set_lane (TrackLaneT &lane)
{
  z_return_if_fail (lane.track_ != nullptr);

  is_auditioner_ = lane.is_auditioner ();

  owner_lane_ = &lane;

  if (auto * region = dynamic_cast<Region *> (this))
    {
      region->id_.lane_pos_ = lane.pos_;
      region->id_.track_uuid_ = lane.track_->get_uuid ();
    }
  track_id_ = lane.track_->get_uuid ();
}

template class LaneOwnedObjectImpl<MidiRegion>;
template class LaneOwnedObjectImpl<AudioRegion>;
