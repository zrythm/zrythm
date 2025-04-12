// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/dsp/audio_region.h"
#include "gui/dsp/region_owner.h"

#include "utils/rt_thread_id.h"

template <typename RegionT>
RegionOwner<RegionT>::RegionOwner () : region_list_ (new RegionList ())
{
}

template <typename RegionT>
void
RegionOwner<RegionT>::parent_base_qproperties (QObject &derived)
{
  region_list_->setParent (&derived);
}

template <typename RegionT>
void
RegionOwner<RegionT>::foreach_region (std::function<void (RegionT &)> func)
{
  for (const auto &region_var : region_list_->get_region_vars ())
    {
      func (*std::get<RegionT *> (region_var));
    }
}

template <typename RegionT>
void
RegionOwner<RegionT>::foreach_region (std::function<void (RegionT &)> func) const
{
  for (const auto &region_var : region_list_->get_region_vars ())
    {
      func (*std::get<RegionT *> (region_var));
    }
}

template <typename RegionT>
void
RegionOwner<RegionT>::copy_members_from (
  const RegionOwner &other,
  ObjectCloneType    clone_type)
{
  auto * derived = dynamic_cast<QObject *> (this);
  region_list_ = other.region_list_->clone_raw_ptr ();
  region_list_->setParent (derived);
}

template <typename RegionT>
void
RegionOwner<RegionT>::unselect_all ()
{
  foreach_region ([&] (auto &region) { region.setSelected (false); });
}

template <typename RegionT>
bool
RegionOwner<RegionT>::remove_region (const Region::Uuid &region_id)
{
  auto it_to_remove = std::ranges::find (
    region_list_->regions_, region_id, &ArrangerObjectUuidReference::id);
  if (it_to_remove == region_list_->regions_.end ())
    {
      z_warning ("region to remove not found: {}", region_id);
      return false;
    }
  z_trace ("removing region: {}", region_id);

  const auto remove_idx =
    std::distance (region_list_->regions_.begin (), it_to_remove);
  region_list_->beginRemoveRows ({}, remove_idx, remove_idx);

  auto region_ref = *it_to_remove;
  std::visit (
    [&] (auto &&region) {
      if constexpr (std::derived_from<base_type<decltype (region)>, Region>)
        {
          if (is_in_active_project () && !is_auditioner ())
            {
              // caution: this can remove lanes
              region->disconnect_region ();

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
        }
    },
    region_ref.get_object ());

  auto next_it = region_list_->regions_.erase (it_to_remove);
  std::for_each (next_it, region_list_->regions_.end (), [&] (auto &r_var) {
    auto * r = std::get<RegionT *> (r_var.get_object ());
    // r->id_.idx_ = std::distance (region_list_->regions_.begin (), next_it);
    r->update_identifier ();
    ++next_it;
  });

  after_remove_region ();

  region_list_->endRemoveRows ();

  return true;
}

template <typename RegionT>
void
RegionOwner<RegionT>::clear_regions ()
{
  region_list_->beginResetModel ();
  clearing_ = true;
  for (const auto &region_ref : std::views::reverse (region_list_->regions_))
    {
      remove_region (region_ref.id ());
    }
  region_snapshots_.clear ();
  clearing_ = false;
  region_list_->endResetModel ();
}

template <typename RegionT>
void
RegionOwner<RegionT>::insert_region (
  const ArrangerObjectUuidReference &region_ref,
  int                                idx)
{
  static_assert (FinalRegionSubclass<RegionT>);
  z_return_if_fail (idx >= 0);

  auto * region = std::get<RegionT *> (region_ref.get_object ());
  z_return_if_fail (!region->get_name ().empty ());

  region_list_->beginInsertRows ({}, idx, idx);
  region_list_->regions_.insert (
    region_list_->regions_.begin () + idx, region_ref);
  region->setParent (region_list_);

  if constexpr (std::is_same_v<RegionT, AutomationRegion>)
    {
      region->set_automation_track (static_cast<AutomationTrack &> (*this));
    }
  else if constexpr (std::derived_from<RegionT, LaneOwnedObject>)
    {
      using TrackLaneT = TrackLaneImpl<RegionT>::TrackLaneT;
      region->set_lane (dynamic_cast<TrackLaneT *> (this));
    }
  else if constexpr (std::is_same_v<RegionT, ChordRegion>)
    {
      auto * chord_track = dynamic_cast<ChordTrack *> (this);
      region->track_id_ = chord_track->get_uuid ();
    }

  region_list_->endInsertRows ();

  /* verify we have a clip */
  if constexpr (std::is_same_v<RegionT, AudioRegion>)
    {
      auto clip = region->get_clip ();
      z_return_if_fail (clip);
    }
}

template class RegionOwner<AudioRegion>;
template class RegionOwner<AutomationRegion>;
template class RegionOwner<ChordRegion>;
template class RegionOwner<MidiRegion>;
