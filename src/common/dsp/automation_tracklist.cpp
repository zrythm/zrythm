// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cstdlib>

#include "gui/cpp/backend/event.h"
#include "gui/cpp/backend/event_manager.h"
#include "gui/cpp/backend/project.h"
#include "gui/cpp/backend/zrythm.h"
#include "gui/cpp/gtk_widgets/zrythm_app.h"

#include "common/dsp/automatable_track.h"
#include "common/dsp/automation_region.h"
#include "common/dsp/automation_track.h"
#include "common/dsp/automation_tracklist.h"
#include "common/dsp/channel.h"
#include "common/dsp/track.h"
#include "common/dsp/tracklist.h"
#include "common/plugins/plugin.h"
#include "common/utils/flags.h"
#include "common/utils/mem.h"
#include "common/utils/objects.h"
#include "common/utils/rt_thread_id.h"
#include "common/utils/string.h"

void
AutomationTracklist::init_loaded (AutomatableTrack * track)
{
  track_ = track;
  for (auto &at : ats_)
    {
      at->init_loaded (this);
      if (at->visible_)
        {
          visible_ats_.push_back (at.get ());
        }
    }
}

AutomatableTrack *
AutomationTracklist::get_track () const
{
  z_return_val_if_fail (track_, nullptr);
  return track_;
}

AutomationTrack *
AutomationTracklist::add_at (std::unique_ptr<AutomationTrack> &&at)
{
  auto &at_ref = ats_.emplace_back (std::move (at));

  at_ref->index_ = ats_.size () - 1;
  at_ref->port_id_.track_name_hash_ = track_->get_name_hash ();

  /* move automation track regions */
  for (const auto region : at_ref->regions_ | type_is<AutomationRegion> ())
    {
      region->set_automation_track (*at_ref);
    }
  return at_ref.get ();
}

AutomationTrack *
AutomationTracklist::get_plugin_at (
  PluginSlotType     slot_type,
  const int          plugin_slot,
  const int          port_index,
  const std::string &symbol)
{
  auto it = std::find_if (ats_.begin (), ats_.end (), [&] (const auto &at) {
    return at->port_id_.owner_type_ == PortIdentifier::OwnerType::Plugin
           && plugin_slot == at->port_id_.plugin_id_.slot_
           && slot_type == at->port_id_.plugin_id_.slot_type_
           && port_index == at->port_id_.port_index_
           && symbol == at->port_id_.sym_;
  });

  return it != ats_.end () ? it->get () : nullptr;
}

void
AutomationTracklist::set_at_index (AutomationTrack &at, int index, bool push_down)
{
  /* special case */
  if (index == static_cast<int> (ats_.size ()) && push_down)
    {
      /* move AT to before last */
      set_at_index (at, index - 1, push_down);
      /* move last AT to before last */
      set_at_index (*ats_.back (), index - 1, push_down);
      return;
    }

  z_return_if_fail (index < static_cast<int> (ats_.size ()) && ats_[index]);

  auto it = std::find_if (ats_.begin (), ats_.end (), [&] (const auto &at_ref) {
    return at_ref.get () == &at;
  });
  z_return_if_fail (
    it != ats_.end () && std::distance (ats_.begin (), it) == at.index_);

  if (at.index_ == index)
    return;

  auto clip_editor_region = CLIP_EDITOR->get_region ();
  int  clip_editor_region_idx = -2;
  if (clip_editor_region)
    {
      auto &clip_editor_region_id = clip_editor_region->id_;

      if (clip_editor_region_id.track_name_hash_ == at.port_id_.track_name_hash_)
        {
          clip_editor_region_idx = clip_editor_region_id.at_idx_;
        }
    }

  if (push_down)
    {
      auto from = at.index_;
      auto to = index;

      auto update_at_index = [&] (int i) {
        const auto idx_before_change = ats_[i]->index_;
        ats_[i]->set_index (i);

        /* update clip editor region if it was affected */
        if (clip_editor_region_idx == idx_before_change)
          {
            CLIP_EDITOR->region_id_.at_idx_ = i;
          }
      };

      auto from_it = ats_.begin () + from;
      auto to_it = ats_.begin () + to;
      if (from < to)
        {
          std::rotate (from_it, from_it + 1, to_it + 1);
        }
      else
        {
          std::rotate (to_it, from_it, from_it + 1);
        }

      for (auto i = std::min (from, to); i <= std::max (from, to); ++i)
        {
          update_at_index (i);
        }
    }
  else
    {
      int prev_index = at.index_;
      std::swap (ats_[index], ats_[prev_index]);
      ats_[index]->set_index (index);
      ats_[prev_index]->set_index (prev_index);

      /* update clip editor region if it was affected */
      if (clip_editor_region_idx == index)
        {
          CLIP_EDITOR->region_id_.at_idx_ = prev_index;
        }
      else if (clip_editor_region_idx == prev_index)
        {
          CLIP_EDITOR->region_id_.at_idx_ = index;
        }

      z_trace (
        "new pos {} ({})", ats_[prev_index]->port_id_.get_label (),
        ats_[prev_index]->index_);
    }
}

void
AutomationTracklist::update_track_name_hash (AutomatableTrack &track)
{
  track_ = &track;
  auto track_name_hash = track.get_name_hash ();
  for (auto &at : ats_)
    {
      at->port_id_.track_name_hash_ = track_name_hash;
      if (at->port_id_.owner_type_ == PortIdentifier::OwnerType::Plugin)
        {
          at->port_id_.plugin_id_.track_name_hash_ = track_name_hash;
        }
      for (auto &region : at->regions_)
        {
          region->id_.track_name_hash_ = track_name_hash;
          region->update_identifier ();
        }
    }
}

void
AutomationTracklist::unselect_all ()
{
  for (auto &at : ats_)
    {
      at->unselect_all ();
    }
}

void
AutomationTracklist::clear_objects ()
{
  for (auto &at : ats_)
    {
      at->clear_regions ();
    }
}

AutomationTrack *
AutomationTracklist::get_prev_visible_at (const AutomationTrack &at) const
{
  auto it = std::find_if (
    ats_.rbegin () + (ats_.size () - at.index_ + 1), ats_.rend (),
    [] (const auto &at_ref) { return at_ref->created_ && at_ref->visible_; });
  return it == ats_.rend () ? nullptr : (*std::prev (it.base ())).get ();
}

AutomationTrack *
AutomationTracklist::get_next_visible_at (const AutomationTrack &at) const
{
  auto it = std::find_if (
    ats_.begin () + at.index_ + 1, ats_.end (),
    [] (const auto &at_ref) { return at_ref->created_ && at_ref->visible_; });
  return it == ats_.end () ? nullptr : (*it).get ();
}

AutomationTrack *
AutomationTracklist::get_visible_at_after_delta (
  const AutomationTrack &at,
  int                    delta) const
{
  if (delta > 0)
    {
      auto vis_at = &at;
      while (delta > 0)
        {
          vis_at = get_next_visible_at (*vis_at);

          if (!vis_at)
            return nullptr;

          delta--;
        }
      return const_cast<AutomationTrack *> (vis_at);
    }
  else if (delta < 0)
    {
      auto vis_at = &at;
      while (delta < 0)
        {
          vis_at = get_prev_visible_at (*vis_at);

          if (!vis_at)
            return nullptr;

          delta++;
        }
      return const_cast<AutomationTrack *> (vis_at);
    }
  else
    return const_cast<AutomationTrack *> (&at);
}

int
AutomationTracklist::get_visible_at_diff (
  const AutomationTrack &src,
  const AutomationTrack &dest) const
{
  int count = 0;
  if (src.index_ < dest.index_)
    {
      for (int i = src.index_; i < dest.index_; i++)
        {
          const auto &at = ats_[i];
          if (at->created_ && at->visible_)
            {
              count++;
            }
        }
    }
  else if (src.index_ > dest.index_)
    {
      for (int i = dest.index_; i < src.index_; i++)
        {
          const auto &at = ats_[i];
          if (at->created_ && at->visible_)
            {
              count--;
            }
        }
    }

  return count;
}

AutomationTrack *
AutomationTracklist::get_at_from_port (const Port &port) const
{
  auto it = std::find_if (ats_.begin (), ats_.end (), [&port] (const auto &at) {
    auto at_port = Port::find_from_identifier (at->port_id_);
    return at_port == &port;
  });

  return it != ats_.end () ? it->get () : nullptr;
}

AutomationTrack *
AutomationTracklist::get_first_invisible_at () const
{
  /* prioritize automation tracks with existing lanes */
  auto it = std::find_if (ats_.begin (), ats_.end (), [] (const auto &at) {
    return at->created_ && !at->visible_;
  });

  if (it != ats_.end ())
    return it->get ();

  it = std::find_if (ats_.begin (), ats_.end (), [] (const auto &at) {
    return !at->created_;
  });

  return it != ats_.end () ? it->get () : nullptr;
}

void
AutomationTracklist::set_at_visible (AutomationTrack &at, bool visible)
{
  z_return_if_fail (at.created_);
  at.visible_ = visible;
  auto it = std::find (visible_ats_.begin (), visible_ats_.end (), &at);
  if (visible)
    {
      if (it == visible_ats_.end ())
        {
          visible_ats_.push_back (&at);
        }
    }
  else if (it != visible_ats_.end ())
    {
      visible_ats_.erase (it);
    }
}

std::unique_ptr<AutomationTrack>
AutomationTracklist::
  remove_at (AutomationTrack &at, bool free_at, bool fire_events)
{
  int deleted_idx = 0;

  {
    auto it = std::find (visible_ats_.begin (), visible_ats_.end (), &at);
    if (it != visible_ats_.end ())
      {
        visible_ats_.erase (it);
      }
  }

  z_trace (
    "[track {} atl] removing automation track at: {} '{}'",
    track_ ? track_->pos_ : -1, deleted_idx, at.port_id_.label_);

  if (free_at)
    {
      /* this needs to be called before removing the automation track in case
       * the region is referenced elsewhere (e.g., clip editor) */
      at.clear_regions ();
    }

  auto it = std::find_if (ats_.begin (), ats_.end (), [&at] (const auto &ptr) {
    return ptr.get () == &at;
  });
  if (it == ats_.end ())
    {
      z_warning (
        "[track {} atl] automation track not found", track_ ? track_->pos_ : -1);
      return nullptr;
    }
  auto deleted_at = std::move (*it);
  it = ats_.erase (it);

  /* move automation track regions for automation tracks after the deleted one*/
  for (auto cur_it = it; cur_it != ats_.end (); ++cur_it)
    {
      auto &cur_at = *cur_it;
      cur_at->index_ = std::distance (ats_.begin (), cur_it);
      for (auto region : cur_at->regions_ | type_is<AutomationRegion> ())
        {
          region->set_automation_track (*cur_at);
        }
    }

  /* if the deleted at was the last visible/created at, make the next one
   * visible */
  if (visible_ats_.empty ())
    {
      auto first_invisible_at = get_first_invisible_at ();
      if (first_invisible_at)
        {
          if (!first_invisible_at->created_)
            first_invisible_at->created_ = true;

          set_at_visible (*first_invisible_at, true);

          if (fire_events)
            {
              EVENTS_PUSH (
                EventType::ET_AUTOMATION_TRACK_ADDED, first_invisible_at);
            }
        }
    }

  if (fire_events)
    {
      EVENTS_PUSH (EventType::ET_AUTOMATION_TRACKLIST_AT_REMOVED, this);
    }

  return deleted_at;
}

void
AutomationTracklist::append_objects (std::vector<ArrangerObject *> objects) const
{
  for (auto &at : ats_)
    {
      for (auto &r : at->regions_)
        {
          objects.push_back (r.get ());
        }
    }
}

void
AutomationTracklist::print_ats () const
{
  std::string str =
    "Automation tracklist (track '" + track_->get_name () + "')\n";

  for (size_t i = 0; i < ats_.size (); i++)
    {
      const auto &at = ats_[i];
      str += fmt::format (
        "[{}] '{}' (sym '{}')\n", i, at->port_id_.get_label (),
        at->port_id_.sym_);
    }

  z_info (str);
}

int
AutomationTracklist::get_num_visible () const
{
  return std::count_if (ats_.begin (), ats_.end (), [] (const auto &at) {
    return at->created_ && at->visible_;
  });
}

bool
AutomationTracklist::validate () const
{
  z_return_val_if_fail (track_, false);

  auto track_name_hash = track_->get_name_hash ();
  for (int i = 0; i < static_cast<int> (ats_.size ()); i++)
    {
      const auto &at = ats_[i];
      z_return_val_if_fail (
        at->port_id_.track_name_hash_ == track_name_hash && at->index_ == i,
        false);
      if (!at->validate ())
        return false;
    }

  return true;
}

int
AutomationTracklist::get_num_regions () const
{
  return std::accumulate (
    ats_.begin (), ats_.end (), 0,
    [] (int sum, const auto &at) { return sum + at->regions_.size (); });
}

void
AutomationTracklist::set_caches (CacheType types)
{
  auto track = get_track ();
  z_return_if_fail (track);

  if (track->is_auditioner ())
    return;

  if (ENUM_BITSET_TEST (CacheType, types, CacheType::AutomationLaneRecordModes))
    {
      ats_in_record_mode_.clear ();
    }

  for (auto &at : ats_)
    {
      at->set_caches (types);

      if (
        ENUM_BITSET_TEST (CacheType, types, CacheType::AutomationLaneRecordModes)
        && at->automation_mode_ == AutomationMode::Record)
        {
          ats_in_record_mode_.push_back (at.get ());
        }
    }
}

void
AutomationTracklist::print_regions () const
{
  Track * track = get_track ();
  z_return_if_fail (track);

  std::string str = fmt::format (
    "Automation regions for track {} "
    "(total automation tracks {}):",
    track->get_name (), ats_.size ());

  for (size_t i = 0; i < ats_.size (); i++)
    {
      const auto &at = ats_[i];
      if (at->regions_.empty ())
        continue;

      str += fmt::format (
        "\n  [{}] port '{}': {} regions", i, at->port_id_.get_label (),
        at->regions_.size ());
    }

  z_info (str);
}