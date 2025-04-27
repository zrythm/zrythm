// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <cstdlib>

#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/backend/channel.h"
#include "gui/dsp/automatable_track.h"
#include "gui/dsp/automation_region.h"
#include "gui/dsp/automation_track.h"
#include "gui/dsp/automation_tracklist.h"
#include "gui/dsp/plugin.h"
#include "gui/dsp/track.h"
#include "gui/dsp/tracklist.h"
#include "utils/rt_thread_id.h"
#include "utils/views.h"

AutomationTracklist::AutomationTracklist (
  const DeserializationDependencyHolder &dh)
    : AutomationTracklist (
        dh.get<std::reference_wrapper<PortRegistry>> ().get (),
        dh.get<std::reference_wrapper<ArrangerObjectRegistry>> ().get (),
        dh.get<std::reference_wrapper<AutomatableTrack>> ().get ())
{
}

AutomationTracklist::AutomationTracklist (
  PortRegistry           &port_registry,
  ArrangerObjectRegistry &obj_registry,
  AutomatableTrack       &track,
  QObject *               parent)
    : QAbstractListModel (parent), object_registry_ (obj_registry),
      port_registry_ (port_registry), track_ (track)
{
}

void
AutomationTracklist::init_loaded ()
{
  for (auto &at : ats_)
    {
      at->init_loaded ();
      if (at->visible_)
        {
          visible_ats_.push_back (at);
        }
    }
}

void
AutomationTracklist::init_after_cloning (
  const AutomationTracklist &other,
  ObjectCloneType            clone_type)
{
  ats_.clear ();
  ats_.reserve (other.ats_.size ());
// TODO
#if 0
  for (const auto &at : other.ats_)
    {
      auto * cloned_at = at->clone_raw_ptr ();
      cloned_at->setParent (this);
      ats_.push_back (cloned_at);
    }
#endif
}

// ========================================================================
// QML Interface
// ========================================================================
QHash<int, QByteArray>
AutomationTracklist::roleNames () const
{
  static QHash<int, QByteArray> roles = {
    { AutomationTrackPtrRole, "automationTrack" },
  };
  return roles;
}

int
AutomationTracklist::rowCount (const QModelIndex &parent) const
{
  if (parent.isValid ())
    return 0;
  return static_cast<int> (ats_.size ());
}

QVariant
AutomationTracklist::data (const QModelIndex &index, int role) const
{
  if (!index.isValid () || index.row () >= static_cast<int> (ats_.size ()))
    return {};

  switch (role)
    {
    case AutomationTrackPtrRole:
      return QVariant::fromValue (ats_.at (index.row ()));
    default:
      return {};
    }
}

void
AutomationTracklist::showNextAvailableAutomationTrack (
  AutomationTrack * current_automation_track)
{
  auto * new_at = get_first_invisible_at ();

  /* if any invisible at exists, show it */
  if (new_at != nullptr)
    {
      if (!new_at->created_)
        new_at->created_ = true;
      set_at_visible (*new_at, true);

      /* move it after the clicked automation track */
      set_at_index (*new_at, current_automation_track->index_ + 1, true);
      Q_EMIT dataChanged (
        index (new_at->index_), index ((int) ats_.size () - 1));
    }
}

void
AutomationTracklist::hideAutomationTrack (
  AutomationTrack * current_automation_track)
{
  /* don't allow deleting if no other visible automation tracks */
  if (visible_ats_.size () > 1)
    {
      set_at_visible (*current_automation_track, false);
      Q_EMIT dataChanged (
        index (current_automation_track->index_),
        index ((int) ats_.size () - 1));
    }
}

// ========================================================================

TrackPtrVariant
AutomationTracklist::get_track () const
{
  return convert_to_variant<TrackPtrVariant> (&track_);
}

ControlPort &
AutomationTracklist::get_port (dsp::PortIdentifier::PortUuid id) const
{
  return *std::get<ControlPort *> (port_registry_.find_by_id_or_throw (id));
}

AutomationTrack *
AutomationTracklist::add_automation_track (AutomationTrack &at)
{
  ats_.push_back (&at);
  auto * at_ref = ats_.back ();
  at_ref->setParent (this);

  at_ref->index_ = static_cast<int> (ats_.size ()) - 1;
  const auto &port = get_port (at_ref->port_id_);
  port.id_->set_track_id (track_.get_uuid ());

  /* move automation track regions */
  for (auto * region : at_ref->get_children_view ())
    {
      region->set_automation_track (*at_ref);
    }

  return at_ref;
}

AutomationTrack *
AutomationTracklist::get_plugin_at (
  dsp::PluginSlot    slot,
  const int          port_index,
  const std::string &symbol)
{
  auto it = std::find_if (ats_.begin (), ats_.end (), [&] (const auto &at) {
    const auto &port = get_port (at->port_id_);
    const auto &port_id = *port.id_;
    if (
      port_id.owner_type_ != dsp::PortIdentifier::OwnerType::Plugin
      || port_index != static_cast<int> (port_id.port_index_)
      || symbol != port_id.get_symbol ())
      return false;

    auto plugin_uuid = port_id.get_plugin_id ();
    z_return_val_if_fail (plugin_uuid.has_value (), false);
    auto plugin_var = PROJECT->find_plugin_by_id (plugin_uuid.value ());
    z_return_val_if_fail (plugin_var.has_value (), false);

    return slot == PluginSpan::slot_projection (*plugin_var);
  });

  return it != ats_.end () ? *it : nullptr;
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

  auto it = std::ranges::find_if (ats_, [&] (const auto &at_ref) {
    return at_ref == &at;
  });
  z_return_if_fail (
    it != ats_.end () && std::distance (ats_.begin (), it) == at.index_);

  if (at.index_ == index)
    return;

  auto clip_editor_region_opt = CLIP_EDITOR->get_region ();
  // int  clip_editor_region_idx = -2;
  if (clip_editor_region_opt)
    {
#if 0
      std::visit (
        [&] (auto &&clip_editor_region) {
          if (clip_editor_region->get_track_id () == track_.get_uuid ())
            {
              clip_editor_region_idx = clip_editor_region_id.at_idx_;
            }
        },
        clip_editor_region_opt.value ());
#endif
    }

  if (push_down)
    {
      auto from = at.index_;
      auto to = index;

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
          ats_[i]->set_index (i);
        }
    }
  else
    {
      int prev_index = at.index_;
      std::swap (ats_[index], ats_[prev_index]);
      ats_[index]->set_index (index);
      ats_[prev_index]->set_index (prev_index);

      z_trace (
        "new pos {} ({})", ats_[prev_index]->getLabel (),
        ats_[prev_index]->index_);
    }
}

void
AutomationTracklist::unselect_all ()
{
  for (auto &at : ats_)
    {
      for (auto * child : at->get_children_view ())
        {
          auto sel_mgr =
            ArrangerObjectFactory::get_instance ()
              ->get_selection_manager_for_object (*child);
          sel_mgr.remove_from_selection (child->get_uuid ());
        }
    }
}

void
AutomationTracklist::clear_objects ()
{
  for (auto &at : ats_)
    {
      at->clear_objects ();
    }
}

AutomationTrack *
AutomationTracklist::get_prev_visible_at (const AutomationTrack &at) const
{
  auto it = std::find_if (
    ats_.rbegin () + (ats_.size () - at.index_ + 1), ats_.rend (),
    [] (const auto &at_ref) { return at_ref->created_ && at_ref->visible_; });
  return it == ats_.rend () ? nullptr : (*std::prev (it.base ()));
}

AutomationTrack *
AutomationTracklist::get_next_visible_at (const AutomationTrack &at) const
{
  auto it = std::find_if (
    ats_.begin () + at.index_ + 1, ats_.end (),
    [] (const auto &at_ref) { return at_ref->created_ && at_ref->visible_; });
  return it == ats_.end () ? nullptr : (*it);
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
AutomationTracklist::get_at_from_port (const ControlPort &port) const
{
  auto it = std::find_if (ats_.begin (), ats_.end (), [&] (const auto &at) {
    const auto &at_port = get_port (at->port_id_);
    return std::addressof (at_port) == std::addressof (port);
  });

  return it != ats_.end () ? *it : nullptr;
}

AutomationTrack *
AutomationTracklist::get_automation_track_by_port_id (
  dsp::PortIdentifier::PortUuid id) const
{
  auto it = std::find_if (ats_.begin (), ats_.end (), [&id] (const auto &at) {
    return at->port_id_ == id;
  });
  return it != ats_.end () ? *it : nullptr;
}

AutomationTrack *
AutomationTracklist::get_first_invisible_at () const
{
  /* prioritize automation tracks with existing lanes */
  auto it = std::find_if (ats_.begin (), ats_.end (), [] (const auto &at) {
    return at->created_ && !at->visible_;
  });

  if (it != ats_.end ())
    return *it;

  it = std::find_if (ats_.begin (), ats_.end (), [] (const auto &at) {
    return !at->created_;
  });

  return it != ats_.end () ? *it : nullptr;
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

AutomationTrack *
AutomationTracklist::
  remove_at (AutomationTrack &at, bool free_at, bool fire_events)
{
  beginRemoveRows ({}, at.index_, at.index_);

  int deleted_idx = 0;

  {
    auto it = std::find (visible_ats_.begin (), visible_ats_.end (), &at);
    if (it != visible_ats_.end ())
      {
        visible_ats_.erase (it);
      }
  }

  z_trace (
    "[track {} atl] removing automation track at: {} '{}'", track_.get_index (),
    deleted_idx, at.getLabel ());

  if (free_at)
    {
      /* this needs to be called before removing the automation track in case
       * the region is referenced elsewhere (e.g., clip editor) */
      at.clear_objects ();
    }

  auto it = std::ranges::find_if (ats_, [&at] (const auto &ptr) {
    return ptr == &at;
  });
  if (it == ats_.end ())
    {
      z_warning (
        "[track {} atl] automation track not found", track_.get_index ());
      endRemoveRows ();
      return nullptr;
    }
  auto * deleted_at = *it;
  it = ats_.erase (it);

  /* move automation track regions for automation tracks after the deleted one*/
  for (auto cur_it = it; cur_it != ats_.end (); ++cur_it)
    {
      auto &cur_at = *cur_it;
      cur_at->index_ = std::distance (ats_.begin (), cur_it);
      for (auto * region : cur_at->get_children_view ())
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
              /* EVENTS_PUSH (
                EventType::ET_AUTOMATION_TRACK_ADDED, first_invisible_at); */
            }
        }
    }

  endRemoveRows ();

  if (fire_events)
    {
      // EVENTS_PUSH (EventType::ET_AUTOMATION_TRACKLIST_AT_REMOVED, this);
    }

  if (free_at)
    {
      deleted_at->setParent (nullptr);
      deleted_at->deleteLater ();
      return nullptr;
    }

  return deleted_at;
}

void
AutomationTracklist::append_objects (std::vector<ArrangerObject *> objects) const
{
  for (auto &at : ats_)
    {
      std::ranges::copy (at->get_children_view (), std::back_inserter (objects));
    }
}

void
AutomationTracklist::print_ats () const
{
  std::string str =
    "Automation tracklist (track '" + track_.get_name () + "')\n";

  for (size_t i = 0; i < ats_.size (); i++)
    {
      const auto &at = ats_[i];
      const auto &port = get_port (at->port_id_);
      str += fmt::format (
        "[{}] '{}' (sym '{}')\n", i, at->getLabel (), port.id_->get_symbol ());
    }

  z_info (str);
}

int
AutomationTracklist::get_num_visible () const
{
  return std::ranges::count_if (ats_, [] (const auto &at) {
    return at->created_ && at->visible_;
  });
}

bool
AutomationTracklist::validate () const
{
  auto track_uuid = track_.get_uuid ();
  for (int i = 0; i < static_cast<int> (ats_.size ()); i++)
    {
      const auto &at = ats_[i];
      const auto &port = get_port (at->port_id_);
      z_return_val_if_fail (port.id_->get_track_id () == track_uuid, false);
      z_return_val_if_fail (at->index_ == i, false);
      if (!at->validate ())
        return false;
    }

  return true;
}

int
AutomationTracklist::get_num_regions () const
{
  return std::ranges::fold_left (
    ats_ | std::views::transform ([] (const auto &at) {
      return at->get_children_vector ().size ();
    }),
    0, std::plus{});
}

void
AutomationTracklist::set_caches (CacheType types)
{
  auto * track = &track_;
  z_return_if_fail (track);

  if (track->is_auditioner ())
    return;

  if (ENUM_BITSET_TEST (types, CacheType::AutomationLaneRecordModes))
    {
      ats_in_record_mode_.clear ();
    }

  for (auto &at : ats_)
    {
      at->set_caches (types);

      if (
        ENUM_BITSET_TEST (types, CacheType::AutomationLaneRecordModes)
        && at->automation_mode_ == AutomationMode::Record)
        {
          ats_in_record_mode_.push_back (at);
        }
    }
}

void
AutomationTracklist::print_regions () const
{
  auto * track = &track_;
  z_return_if_fail (track);

  std::string str = fmt::format (
    "Automation regions for track {} (total automation tracks {}):",
    track->get_name (), ats_.size ());

  for (const auto &[index, at] : utils::views::enumerate (ats_))
    {
      if (at->get_children_vector ().empty ())
        continue;

      str += fmt::format (
        "\n  [{}] port '{}': {} regions", index, at->getLabel (),
        at->get_children_vector ().size ());
    }

  z_info (str);
}
