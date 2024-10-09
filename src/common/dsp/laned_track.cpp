// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/dsp/laned_track.h"
#include "common/utils/rt_thread_id.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"

template <typename RegionT>
void
LanedTrackImpl<RegionT>::remove_empty_last_lanes ()
{
  if (block_auto_creation_and_deletion_)
    return;

  /* if currently have a temporary last lane created skip removing */
  if (last_lane_created_ > 0)
    return;

  z_info ("removing empty last lanes from {}", name_);
  bool removed = false;
  for (int i = lanes_.size () - 1; i >= 1; i--)
    {
      auto lane = lanes_[i].get ();
      auto prev_lane = lanes_[i - 1].get ();
      z_return_if_fail (lane && prev_lane);

      if (!lane->regions_.empty ())
        break;

      if (lane->regions_.empty () && prev_lane->regions_.empty ())
        {
          z_info ("removing lane {}", i);
          lanes_.erase (lanes_.begin () + i);
          removed = true;
        }
    }

  if (removed)
    {
      // EVENTS_PUSH (EventType::ET_TRACK_LANE_REMOVED, nullptr);
    }
}

template <typename RegionT>
void
LanedTrackImpl<RegionT>::clear_objects ()
{
  while (!lanes_.empty ())
    {
      lanes_.back ()->clear_regions ();
      // lane might have been deleted above
      if (!lanes_.empty () && lanes_.back ()->regions_.empty ())
        {
          lanes_.erase (lanes_.end () - 1);
        }
    }
}

void
LanedTrack::set_lanes_visible (bool visible)
{
  lanes_visible_ = visible;
  // EVENTS_PUSH (EventType::ET_TRACK_LANES_VISIBILITY_CHANGED, this);
}

template <typename RegionT>
double
LanedTrackImpl<RegionT>::get_visible_lane_heights () const
{
  double height = 0;
  if (lanes_visible_)
    {
      for (const auto &lane : lanes_)
        {
          z_warn_if_fail (lane->height_ > 0);
          height += lane->height_;
        }
    }
  return height;
}

template <typename RegionT>
void
LanedTrackImpl<RegionT>::add_lane (bool fire_events)
{
  lanes_.emplace_back (
    std::make_unique<TrackLaneImpl<RegionT>> (this, lanes_.size ()));

  if (fire_events)
    {
      // EVENTS_PUSH (EventType::ET_TRACK_LANE_ADDED, lanes_.back ().get ());
    }
}

template <typename RegionT>
bool
LanedTrackImpl<RegionT>::validate_base () const
{
  /* verify regions */
  for (const auto &lane : lanes_)
    {
      for (const auto &region : lane->regions_)
        {
          region->validate (is_in_active_project (), 0);
        }
    }

  return true;
}

template <typename RegionT>
void
LanedTrackImpl<RegionT>::set_playback_caches ()
{
  lane_snapshots_.clear ();
  lane_snapshots_.reserve (lanes_.size ());
  for (const auto &lane : lanes_)
    {
      lane_snapshots_.push_back (lane->gen_snapshot ());
    }
}

template <typename RegionT>
void
LanedTrackImpl<RegionT>::update_name_hash (unsigned int new_name_hash)
{
  for (auto &lane : lanes_)
    {

      lane->update_track_name_hash ();
    }
}

template class LanedTrackImpl<MidiRegion>;
template class LanedTrackImpl<AudioRegion>;