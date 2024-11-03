// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/dsp/laned_track.h"
#include "common/utils/rt_thread_id.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"

template <typename TrackLaneT> LanedTrackImpl<TrackLaneT>::LanedTrackImpl ()
{
  // lanes_.set_track (this);
  add_lane (false);
}

template <typename TrackLaneT>
void
LanedTrackImpl<TrackLaneT>::remove_empty_last_lanes ()
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
      auto lane = std::get<TrackLaneT *> (lanes_.at (i));
      auto prev_lane = std::get<TrackLaneT *> (lanes_.at (i - 1));
      z_return_if_fail (lane && prev_lane);

      if (!lane->regions_.empty ())
        break;

      if (lane->regions_.empty () && prev_lane->regions_.empty ())
        {
          z_info ("removing lane {}", i);
          lanes_.erase (i);
          removed = true;
        }
    }

  if (removed)
    {
      // EVENTS_PUSH (EventType::ET_TRACK_LANE_REMOVED, nullptr);
    }
}

template <typename TrackLaneT>
void
LanedTrackImpl<TrackLaneT>::clear_objects ()
{
  while (!lanes_.empty ())
    {
      std::get<TrackLaneT *> (lanes_.back ())->clear_regions ();
      // lane might have been deleted above
      if (
        !lanes_.empty ()
        && std::get<TrackLaneT *> (lanes_.back ())->regions_.empty ())
        {
          lanes_.pop_back ();
        }
    }
}

void
LanedTrack::set_lanes_visible (bool visible)
{
  lanes_visible_ = visible;
  // EVENTS_PUSH (EventType::ET_TRACK_LANES_VISIBILITY_CHANGED, this);
}

template <typename TrackLaneT>
double
LanedTrackImpl<TrackLaneT>::get_visible_lane_heights () const
{
  double height = 0;
  if (lanes_visible_)
    {
      for (const auto &lane_var : lanes_)
        {
          const auto * lane = std::get<TrackLaneT *> (lane_var);
          z_warn_if_fail (lane->height_ > 0);
          height += lane->height_;
        }
    }
  return height;
}

template <typename TrackLaneT>
void
LanedTrackImpl<TrackLaneT>::add_lane (bool fire_events)
{
  auto new_lane = new TrackLaneT (this, lanes_.size ());
  lanes_.push_back (new_lane);

  if (fire_events)
    {
      // EVENTS_PUSH (EventType::ET_TRACK_LANE_ADDED, lanes_.back ().get ());
    }
}

template <typename TrackLaneT>
bool
LanedTrackImpl<TrackLaneT>::validate_base () const
{
  /* verify regions */
  for (const auto &lane_var : lanes_)
    {
      auto lane = std::get<TrackLaneT *> (lane_var);
      for (const auto &region : lane->regions_)
        {
          region->validate (is_in_active_project (), 0);
        }
    }

  return true;
}

template <typename TrackLaneT>
void
LanedTrackImpl<TrackLaneT>::set_playback_caches ()
{
  lane_snapshots_.clear ();
  lane_snapshots_.reserve (lanes_.size ());
  for (const auto &lane_var : lanes_)
    {
      auto lane = std::get<TrackLaneT *> (lane_var);

      lane_snapshots_.push_back (lane->gen_snapshot ());
    }
}

template <typename TrackLaneT>
void
LanedTrackImpl<TrackLaneT>::update_name_hash (unsigned int new_name_hash)
{
  for (auto &lane_var : lanes_)
    {
      auto lane = std::get<TrackLaneT *> (lane_var);

      lane->update_track_name_hash ();
    }
}

template <typename TrackLaneT>
void
LanedTrackImpl<TrackLaneT>::init_loaded ()
{
  for (auto &lane_var : lanes_)
    {
      auto lane = std::get<TrackLaneT *> (lane_var);

      lane->init_loaded (this);
    }
}

template <typename TrackLaneT>
void
LanedTrackImpl<TrackLaneT>::copy_members_from (const LanedTrackImpl &other)
{
  lanes_.copy_members_from (other.lanes_);
  for (auto &lane_var : lanes_)
    {
      auto lane = std::get<TrackLaneT *> (lane_var);
      lane->track_ = this;
    }
  lanes_visible_ = other.lanes_visible_;
}

template <typename TrackLaneT>
bool
LanedTrackImpl<TrackLaneT>::create_missing_lanes (const int pos)
{
  if (block_auto_creation_and_deletion_)
    return false;

  bool created_new_lane = false;
  while (pos + 2 > static_cast<int> (lanes_.size ()))
    {
      add_lane (false);
      created_new_lane = true;
    }
  return created_new_lane;
}

template <typename TrackLaneT>
void
LanedTrackImpl<TrackLaneT>::get_regions_in_range (
  std::vector<Region *> &regions,
  const Position *       p1,
  const Position *       p2)
{
  for (auto &lane : lanes_)
    {
      std::visit (
        [&] (auto &&l) {
          for (auto &region : l->regions_)
            {
              add_region_if_in_range (p1, p2, regions, region.get ());
            }
        },
        lane);
    }
}

template class LanedTrackImpl<MidiLane>;
template class LanedTrackImpl<AudioLane>;