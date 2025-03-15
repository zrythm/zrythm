// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/dsp/audio_region.h"
#include "gui/dsp/region_owner.h"

#include "utils/rt_thread_id.h"

template <typename RegionT>
RegionOwnerImpl<RegionT>::RegionOwnerImpl () : region_list_ (new RegionList ())
{
}

template <typename RegionT>
void
RegionOwnerImpl<RegionT>::parent_base_qproperties (QObject &derived)
{
  region_list_->setParent (&derived);
}

template <typename RegionT>
void
RegionOwnerImpl<RegionT>::foreach_region (std::function<void (RegionT &)> func)
{
  for (auto &region_var : region_list_->regions_)
    {
      func (*std::get<RegionT *> (region_var));
    }
}

template <typename RegionT>
void
RegionOwnerImpl<RegionT>::foreach_region (
  std::function<void (RegionT &)> func) const
{
  for (auto &region_var : region_list_->regions_)
    {
      func (*std::get<RegionT *> (region_var));
    }
}

template <typename RegionT>
void
RegionOwnerImpl<RegionT>::copy_members_from (
  const RegionOwnerImpl &other,
  ObjectCloneType        clone_type)
{
  auto * derived = dynamic_cast<QObject *> (this);
  region_list_ = other.region_list_->clone_raw_ptr ();
  region_list_->setParent (derived);
}

template <typename RegionT>
void
RegionOwnerImpl<RegionT>::unselect_all ()
{
  foreach_region ([&] (auto &region) { region.setSelected (false); });
}

template <typename RegionT>
bool
RegionOwnerImpl<
  RegionT>::remove_region (RegionT &region, bool free_region, bool fire_events)
{
  auto it_to_remove = std::find_if (
    region_list_->regions_.begin (), region_list_->regions_.end (),
    [&region] (const auto &r) { return std::get<RegionT *> (r) == &region; });
  if (it_to_remove == region_list_->regions_.end ())
    {
      z_warning ("region to remove not found: {}", region);
      return false;
    }
  z_trace ("removing region: {}", region);

  const auto remove_idx =
    std::distance (region_list_->regions_.begin (), it_to_remove);
  region_list_->beginRemoveRows ({}, remove_idx, remove_idx);

  if (is_in_active_project () && !is_auditioner ())
    {
      // caution: this can remove lanes
      region.disconnect_region ();

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
  for (auto it = it_to_remove + 1; it != region_list_->regions_.end (); ++it)
    {
      auto &r = std::get<RegionT *> (*it);
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

  auto next_it = region_list_->regions_.erase (it_to_remove);
  region.setParent (nullptr);
  if (free_region)
    {
      region.deleteLater ();
    }
  std::for_each (next_it, region_list_->regions_.end (), [&] (auto &r_var) {
    auto * r = std::get<RegionT *> (r_var);
    r->id_.idx_ = std::distance (region_list_->regions_.begin (), next_it);
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

  region_list_->endRemoveRows ();

  return true;
}

template <typename RegionT>
void
RegionOwnerImpl<RegionT>::clear_regions ()
{
  region_list_->beginResetModel ();
  clearing_ = true;
  for (int i = region_list_->regions_.size (); i > 0; --i)
    {
      remove_region (
        *std::get<RegionT *> (region_list_->regions_.at (i - 1)), true, false);
    }
  region_snapshots_.clear ();
  clearing_ = false;
  region_list_->endResetModel ();
}

template <typename RegionT>
void
RegionOwnerImpl<RegionT>::insert_region (RegionTPtr region, int idx)
{
  static_assert (FinalRegionSubclass<RegionT>);
  z_return_if_fail (idx >= 0);
  z_return_if_fail (!region->name_.empty ());

  region_list_->beginInsertRows ({}, idx, idx);
  region_list_->regions_.insert (region_list_->regions_.begin () + idx, region);
  region->setParent (region_list_);

  if constexpr (std::is_same_v<RegionT, AutomationRegion>)
    {
      region->set_automation_track (static_cast<AutomationTrack &> (*this));
    }
  else if constexpr (std::derived_from<RegionT, LaneOwnedObjectImpl<RegionT>>)
    {
      using TrackLaneT = TrackLaneImpl<RegionT>::TrackLaneT;
      region->set_lane (*dynamic_cast<TrackLaneT *> (this));
    }
  else if constexpr (std::is_same_v<RegionT, ChordRegion>)
    {
      auto * chord_track = dynamic_cast<ChordTrack *> (this);
      region->id_.track_uuid_ = chord_track->get_uuid ();
    }

  for (int i = region_list_->regions_.size () - 1; i >= idx; --i)
    {
      auto &r_var = region_list_->regions_[i];
      auto &r = std::get<RegionT *> (r_var);

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

  region_list_->endInsertRows ();

  /* verify we have a clip */
  if constexpr (std::is_same_v<RegionT, AudioRegion>)
    {
      auto clip = region->get_clip ();
      z_return_if_fail (clip);
    }
}

template class RegionOwnerImpl<AudioRegion>;
template class RegionOwnerImpl<AutomationRegion>;
template class RegionOwnerImpl<ChordRegion>;
template class RegionOwnerImpl<MidiRegion>;
