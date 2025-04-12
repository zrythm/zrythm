// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/dsp/laned_track.h"

#include "utils/rt_thread_id.h"

template <typename TrackLaneT> LanedTrackImpl<TrackLaneT>::LanedTrackImpl ()
{
  // lanes_.set_track (this);
  add_lane ();
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

  lanes_.beginResetModel ();

  z_info ("removing empty last lanes from {}", name_);
  bool removed = false;
  for (int i = lanes_.size () - 1; i >= 1; i--)
    {
      auto lane = std::get<TrackLaneT *> (lanes_.at (i));
      auto prev_lane = std::get<TrackLaneT *> (lanes_.at (i - 1));
      z_return_if_fail (lane && prev_lane);

      if (!lane->region_list_->regions_.empty ())
        break;

      if (
        lane->region_list_->regions_.empty ()
        && prev_lane->region_list_->regions_.empty ())
        {
          z_info ("removing lane {}", i);
          lanes_.erase (i);
          removed = true;
        }
    }

  lanes_.endResetModel ();

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
        && std::get<TrackLaneT *> (lanes_.back ())
             ->region_list_->regions_.empty ())
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
LanedTrackImpl<TrackLaneT>::add_lane ()
{
  auto new_lane = new TrackLaneT (this);
  lanes_.push_back (new_lane);
}

template <typename TrackLaneT>
bool
LanedTrackImpl<TrackLaneT>::validate_base () const
{
  /* verify regions */
  for (const auto &lane_var : lanes_)
    {
      auto lane = std::get<TrackLaneT *> (lane_var);
      for (const auto &region_var : lane->region_list_->get_region_vars())
        {
          std::get<typename TrackLaneT::RegionT *> (region_var)
            ->validate (is_in_active_project (), 0);
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
LanedTrackImpl<TrackLaneT>::init_loaded (
  PluginRegistry &plugin_registry,
  PortRegistry   &port_registry)
{
  for (auto &lane_var : lanes_)
    {
      auto lane = std::get<TrackLaneT *> (lane_var);

      lane->init_loaded (this);
    }
}

template <typename TrackLaneT>
void
LanedTrackImpl<TrackLaneT>::copy_members_from (
  const LanedTrackImpl &other,
  ObjectCloneType       clone_type)
{
  lanes_.copy_members_from (other.lanes_, clone_type);
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
      add_lane ();
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
          for (const auto &region_var : l->region_list_->get_region_vars ())
            {
              add_region_if_in_range (
                p1, p2, regions, std::get<RegionT *> (region_var));
            }
        },
        lane);
    }
}

template class LanedTrackImpl<MidiLane>;
template class LanedTrackImpl<AudioLane>;
