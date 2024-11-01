// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/dsp/audio_region.h"
#include "common/dsp/region_owner.h"
#include "common/utils/rt_thread_id.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"

template <typename RegionT>
bool
RegionOwnerImpl<RegionT>::remove_region (RegionT &region, bool fire_events)
{
  auto it_to_remove = std::find_if (
    regions_.begin (), regions_.end (),
    [&region] (const auto &r) { return r.get () == &region; });
  if (it_to_remove == regions_.end ())
    {
      z_warning ("region to remove not found: {}", region);
      return false;
    }
  z_trace ("removing region: {}", region);

  if (is_in_active_project () && !is_auditioner ())
    {
      // caution: this can remove lanes
      region.disconnect ();

#if 0
      // disconnect() handles the clip editor - this should not be needed

      /* if clip editor region index is greater than this index, decrement it */
      auto clip_editor_r = CLIP_EDITOR->get_region ();
      if (
        clip_editor_r
        && clip_editor_r->id_.track_name_hash_ == region.id_.track_name_hash_
        && clip_editor_r->id_.lane_pos_ == region.id_.lane_pos_
        && clip_editor_r->id_.idx_ > region.id_.idx_)
        {
          CLIP_EDITOR->region_id_.idx_--;
        }
#endif
    }

  /* prepare for link group adjustment by faking the indeces in the link group
   * to the expected new ones */
  for (auto it = it_to_remove + 1; it != regions_.end (); ++it)
    {
      auto &r = *it;
      if (r->has_link_group ())
        {
          auto group = r->get_link_group ();
          for (auto &id : group->ids_)
            {
              if (r->id_ == id)
                {
                  --id.idx_;
                }
            }
        }
    }

  auto next_it = regions_.erase (it_to_remove);
  std::for_each (next_it, regions_.end (), [&] (auto &r) {
    r->id_.idx_ = std::distance (regions_.begin (), next_it);
    r->update_identifier ();
    ++next_it;
  });

  if (fire_events)
    {
      // EVENTS_PUSH (
      //   EventType::ET_ARRANGER_OBJECT_REMOVED,
      //   ENUM_VALUE_TO_INT (ArrangerObject::Type::Region));
    }

  after_remove_region ();
  return true;
}

template <typename RegionT>
void
RegionOwnerImpl<RegionT>::clear_regions ()
{
  clearing_ = true;
  for (size_t i = regions_.size (); i > 0; --i)
    {
      remove_region (*regions_[i - 1], false);
    }
  region_snapshots_.clear ();
  clearing_ = false;
}

template <typename RegionT>
std::shared_ptr<RegionT>
RegionOwnerImpl<RegionT>::insert_region (std::shared_ptr<RegionT> region, int idx)
{
  static_assert (FinalRegionSubclass<RegionT>);
  z_return_val_if_fail (idx >= 0, nullptr);
  z_return_val_if_fail (!region->name_.empty (), nullptr);

  regions_.insert (regions_.begin () + idx, region);

  if constexpr (std::is_same_v<RegionT, AutomationRegion>)
    {
      region->set_automation_track (static_cast<AutomationTrack &> (*this));
    }
  else if constexpr (std::derived_from<RegionT, LaneOwnedObjectImpl<RegionT>>)
    {
      using TrackLaneT = TrackLaneImpl<RegionT>::TrackLaneT;
      region->set_lane (*dynamic_cast<TrackLaneT *> (this));
    }

  for (int i = regions_.size () - 1; i >= idx; --i)
    {
      auto &r = regions_[i];

      if (r->has_link_group ())
        {
          auto group = r->get_link_group ();
          for (auto &id_in_group : group->ids_)
            {
              if (r->id_ == id_in_group)
                {
                  id_in_group.idx_++;
                }
            }
        }

      r->id_.idx_ = i;
      r->update_identifier ();
    }

  /* verify we have a clip */
  if constexpr (std::is_same_v<RegionT, AudioRegion>)
    {
      auto clip = region->get_clip ();
      z_return_val_if_fail (clip, nullptr);
    }

  return regions_[idx];
}

template class RegionOwnerImpl<AudioRegion>;
template class RegionOwnerImpl<AutomationRegion>;
template class RegionOwnerImpl<ChordRegion>;
template class RegionOwnerImpl<MidiRegion>;