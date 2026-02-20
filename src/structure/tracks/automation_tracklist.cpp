// SPDX-FileCopyrightText: © 2018-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>

#include "structure/tracks/automation_tracklist.h"

namespace zrythm::structure::tracks
{
AutomationTracklist::AutomationTracklist (
  AutomationTrackHolder::Dependencies registries,
  QObject *                           parent)
    : QAbstractListModel (parent), dependencies_ (registries)
{
}

void
init_from (
  AutomationTracklist       &obj,
  const AutomationTracklist &other,
  utils::ObjectCloneType     clone_type)
{
  // TODO
  obj.automation_visible_ = other.automation_visible_;
}

// ========================================================================
// QML Interface
// ========================================================================
QHash<int, QByteArray>
AutomationTracklist::roleNames () const
{
  QHash<int, QByteArray> roles = {
    { AutomationTrackHolderRole, "automationTrackHolder" },
    { AutomationTrackRole,       "automationTrack"       },
  };
  return roles;
}

int
AutomationTracklist::rowCount (const QModelIndex &parent) const
{
  if (parent.isValid ())
    return 0;
  return static_cast<int> (automation_track_holders ().size ());
}

QVariant
AutomationTracklist::data (const QModelIndex &index, int role) const
{
  if (
    !index.isValid ()
    || index.row () >= static_cast<int> (automation_track_holders ().size ()))
    return {};

  switch (role)
    {
    case AutomationTrackHolderRole:
      return QVariant::fromValue (
        automation_track_holders ().at (index.row ()).get ());
    case AutomationTrackRole:
      return QVariant::fromValue (automation_track_at (index.row ()));
    default:
      return {};
    }
}

void
AutomationTracklist::showNextAvailableAutomationTrack (
  AutomationTrack * current_automation_track)
{
  auto * new_at = get_first_invisible_automation_track_holder ();

  /* if any invisible at exists, show it */
  if (new_at != nullptr)
    {
      new_at->created_by_user_ = true;
      new_at->setVisible (true);

      /* move it after the clicked automation track */
      set_automation_track_index (
        *new_at->automationTrack (),
        get_automation_track_index (*current_automation_track) + 1, true);
    }
}

void
AutomationTracklist::hideAutomationTrack (
  AutomationTrack * current_automation_track)
{
  /* don't allow deleting if no other visible automation tracks */
  if (
    std::ranges::count (
      automation_track_holders (), true,
      [] (const auto &ath) { return ath->visible (); })
    == 0)
    {
      z_warning (
        "cannot hide automation track: no other visible automation tracks found");
      return;
    }

  (*get_iterator_for_automation_track (*current_automation_track))
    ->setVisible (false);
}

// ========================================================================

AutomationTrack *
AutomationTracklist::add_automation_track (
  utils::QObjectUniquePtr<AutomationTrack> &&at)
{
  beginInsertRows (
    {}, static_cast<int> (automation_track_holders ().size ()),
    static_cast<int> (automation_track_holders ().size ()));
  automation_track_holders ().emplace_back (
    utils::make_qobject_unique<AutomationTrackHolder> (
      dependencies_, std::move (at), this));
  auto &ath_ref = automation_track_holders ().back ();
  endInsertRows ();

  return ath_ref->automationTrack ();
}

AutomationTrack *
AutomationTracklist::add_automation_track (
  utils::QObjectUniquePtr<AutomationTrackHolder> &&ath)
{
  beginInsertRows (
    {}, static_cast<int> (automation_track_holders ().size ()),
    static_cast<int> (automation_track_holders ().size ()));
  automation_track_holders ().emplace_back (std::move (ath));
  auto &ath_ref = automation_track_holders ().back ();
  ath_ref->setParent (this);
  endInsertRows ();

  return ath_ref->automationTrack ();
}

void
AutomationTracklist::set_automation_track_index (
  AutomationTrack &at,
  const int        index,
  bool             push_down)
{
  auto      &at_holders = automation_track_holders ();
  const auto ats_size = at_holders.size ();

  /* special case */
  if (index == static_cast<int> (ats_size) && push_down)
    {
      /* move AT to before last */
      set_automation_track_index (at, index - 1, push_down);
      /* move last AT to before last */
      set_automation_track_index (
        *at_holders.back ()->automationTrack (), index - 1, push_down);
      return;
    }

  assert (index < static_cast<int> (ats_size));

  const auto at_index_before = get_automation_track_index (at);

  if (at_index_before == index)
    return;

  beginResetModel ();

  if (push_down)
    {
      const auto from = at_index_before;
      const auto to = static_cast<decltype (from)> (index);

      auto from_it = at_holders.begin () + from;
      auto to_it = at_holders.begin () + to;
      if (from < to)
        {
          std::rotate (from_it, from_it + 1, to_it + 1);
        }
      else
        {
          std::rotate (to_it, from_it, from_it + 1);
        }
    }
  else
    {
      std::swap (at_holders[index], at_holders[at_index_before]);
    }

  endResetModel ();
}

void
AutomationTracklist::clear_arranger_objects ()
{
  for (auto &ath : automation_track_holders ())
    {
      ath->automationTrack ()->clear_objects ();
    }
}

AutomationTrack *
AutomationTracklist::get_previous_visible_automation_track (
  const AutomationTrack &at) const
{
  return get_visible_automation_track_after_delta (at, -1);
}

AutomationTrack *
AutomationTracklist::get_next_visible_automation_track (
  const AutomationTrack &at) const
{
  return get_visible_automation_track_after_delta (at, 1);
}

AutomationTrack *
AutomationTracklist::get_visible_automation_track_after_delta (
  const AutomationTrack &at,
  int                    delta) const
{
  if (delta == 0)
    {
      return const_cast<AutomationTrack *> (&at);
    }

  const auto visible_projection = [] (const auto &ath) {
    return ath->visible ();
  };

  const auto get_result = [&] (auto &&base_range) -> AutomationTrack * {
    // Filter visible tracks and take required count
    auto result =
      base_range | std::views::filter (visible_projection)
      | std::views::take (std::abs (delta));

    // Check if we have enough tracks
    if (std::ranges::distance (result) < std::abs (delta))
      return nullptr;

    // Get the last element in the result range
    return (*std::ranges::begin (result))->automationTrack ();
  };

  // Get result depending on direction
  if (delta > 0)
    {
      return get_result (
        std::ranges::subrange (
          get_iterator_for_automation_track (at) + 1, // start after current
          automation_track_holders ().end ()));
    }

  return get_result (
    std::ranges::subrange (
      automation_track_holders ().begin (),
      get_iterator_for_automation_track (at)) // end before current
    | std::views::reverse);
}

int
AutomationTracklist::get_visible_automation_track_count_between (
  const AutomationTrack &src,
  const AutomationTrack &dest) const
{
  const auto visible_projection = [] (const auto &ath) {
    return ath->visible ();
  };
  const auto &ath = automation_track_holders ();
  const auto  src_it = get_iterator_for_automation_track (src);
  const auto  dest_it = get_iterator_for_automation_track (dest);
  if (
    std::ranges::distance (ath.begin (), src_it)
    <= std::ranges::distance (ath.begin (), dest_it))
    {
      return static_cast<int> (std::ranges::count (
        std::ranges::subrange (src_it, dest_it), true, visible_projection));
    }
  return -static_cast<int> (std::ranges::count (
    std::ranges::subrange (dest_it, src_it), true, visible_projection));
}

AutomationTrackHolder *
AutomationTracklist::get_first_invisible_automation_track_holder () const
{
  const auto &ath = automation_track_holders ();

  /* prioritize automation tracks with existing lanes */
  auto it = std::ranges::find_if (ath, [] (const auto &at) {
    return at->created_by_user_ && !at->visible ();
  });

  if (it != ath.end ())
    return (*it).get ();

  it = std::ranges::find_if (ath, [] (const auto &at) {
    return !at->created_by_user_;
  });

  return it != ath.end () ? (*it).get () : nullptr;
}

utils::QObjectUniquePtr<AutomationTrackHolder>
AutomationTracklist::remove_automation_track (AutomationTrack &at)
{
  const auto at_index = get_automation_track_index (at);
  beginRemoveRows ({}, at_index, at_index);

  auto it_to_remove = get_iterator_for_automation_track (at);
  auto ath_to_return = std::move (*it_to_remove);
  automation_tracks_.erase (it_to_remove);

  endRemoveRows ();

  // if the deleted track was the last visible/created track, make the next one
  // visible
  if (std::ranges::none_of (automation_track_holders (), [] (const auto &ath) {
        return ath->visible ();
      }))
    {
      auto * first_invisible_at = get_first_invisible_automation_track_holder ();
      if (first_invisible_at != nullptr)
        {
          if (!first_invisible_at->created_by_user_)
            first_invisible_at->created_by_user_ = true;

          first_invisible_at->setVisible (true);
        }
    }

  return ath_to_return;
}

void
AutomationTracklist::setAutomationVisible (const bool visible)
{
  if (automation_visible_ == visible)
    return;

  automation_visible_ = visible;

  if (visible)
    {
      /* if no visible automation tracks, set the first one visible */
      const auto &ath = automation_track_holders ();

      if (
        std::ranges::none_of (ath, [] (const auto &h) { return h->visible (); }))
        {
          auto * at = get_first_invisible_automation_track_holder ();
          if (at != nullptr)
            {
              at->created_by_user_ = true;
              at->setVisible (true);
            }
        }
    }

  Q_EMIT automationVisibleChanged (visible);
}

void
to_json (nlohmann::json &j, const AutomationTrackHolder &nfo)
{
  to_json (j, *nfo.automation_track_);
  j[AutomationTrackHolder::kCreatedByUserKey] = nfo.created_by_user_;
  j[AutomationTrackHolder::kVisible] = nfo.visible_;
  j[AutomationTrackHolder::kHeightKey] = nfo.height_;
}

void
from_json (const nlohmann::json &j, AutomationTrackHolder &nfo)
{
  dsp::ProcessorParameterUuidReference param_ref{
    nfo.dependencies_.param_registry_
  };
  j.at (AutomationTrack::kParameterKey).get_to (param_ref);
  nfo.automation_track_ = utils::make_qobject_unique<AutomationTrack> (
    nfo.dependencies_.tempo_map_, nfo.dependencies_.file_audio_source_registry_,
    nfo.dependencies_.object_registry_, param_ref);

  j.at (AutomationTrackHolder::kCreatedByUserKey).get_to (nfo.created_by_user_);
  j.at (AutomationTrackHolder::kVisible).get_to (nfo.visible_);
  j.at (AutomationTrackHolder::kHeightKey).get_to (nfo.height_);
}

void
to_json (nlohmann::json &j, const AutomationTracklist &ats)
{
  j[AutomationTracklist::kAutomationTracksKey] = ats.automation_tracks_;
  j[AutomationTracklist::kAutomationVisibleKey] = ats.automation_visible_;
}

void
from_json (const nlohmann::json &j, AutomationTracklist &ats)
{
  for (const auto &ath_json : j.at (AutomationTracklist::kAutomationTracksKey))
    {
      auto automation_track_holder =
        utils::make_qobject_unique<AutomationTrackHolder> (ats.dependencies_);
      from_json (ath_json, *automation_track_holder);
      ats.automation_tracks_.emplace_back (std::move (automation_track_holder));
    }
  j.at (AutomationTracklist::kAutomationVisibleKey)
    .get_to (ats.automation_visible_);
}
}
