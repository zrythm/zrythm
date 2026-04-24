// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "plugins/plugin_all.h"
#include "structure/tracks/track_all.h"
#include "structure/tracks/track_collection.h"

namespace zrythm::structure::tracks
{

TrackCollection::TrackCollection (
  TrackRegistry &track_registry,
  QObject *      parent) noexcept
    : QAbstractListModel (parent), track_registry_ (track_registry)
{
  // connect signals to track muted/soloed/listened counts
  QObject::connect (
    this, &TrackCollection::rowsInserted, this,
    [this] (const QModelIndex &, int first, int last) {
      for (int i = first; i <= last; ++i)
        {
          const auto  &track_ref = tracks ().at (i);
          const auto * track = track_ref.get_object_base ();
          if (auto * channel = track->channel ())
            {
              QObject::connect (
                &channel->fader ()->get_solo_param (),
                &dsp::ProcessorParameter::baseValueChanged, this,
                &TrackCollection::numSoloedTracksChanged);
              QObject::connect (
                &channel->fader ()->get_mute_param (),
                &dsp::ProcessorParameter::baseValueChanged, this,
                &TrackCollection::numMutedTracksChanged);
              QObject::connect (
                &channel->fader ()->get_listen_param (),
                &dsp::ProcessorParameter::baseValueChanged, this,
                &TrackCollection::numListenedTracksChanged);
            }
        }
      Q_EMIT numSoloedTracksChanged ();
      Q_EMIT numMutedTracksChanged ();
      Q_EMIT numListenedTracksChanged ();
    });
  QObject::connect (
    this, &TrackCollection::rowsAboutToBeRemoved, this,
    [this] (const QModelIndex &, int first, int last) {
      for (int i = first; i <= last; ++i)
        {
          const auto  &track_ref = tracks ().at (i);
          const auto * track = track_ref.get_object_base ();
          if (auto * channel = track->channel ())
            {
              QObject::disconnect (
                &channel->fader ()->get_solo_param (),
                &dsp::ProcessorParameter::baseValueChanged, this,
                &TrackCollection::numSoloedTracksChanged);
              QObject::disconnect (
                &channel->fader ()->get_mute_param (),
                &dsp::ProcessorParameter::baseValueChanged, this,
                &TrackCollection::numMutedTracksChanged);
              QObject::disconnect (
                &channel->fader ()->get_listen_param (),
                &dsp::ProcessorParameter::baseValueChanged, this,
                &TrackCollection::numListenedTracksChanged);
            }
        }
    });
  QObject::connect (this, &TrackCollection::rowsRemoved, this, [this] () {
    Q_EMIT numSoloedTracksChanged ();
    Q_EMIT numMutedTracksChanged ();
    Q_EMIT numListenedTracksChanged ();
  });
}

// ========================================================================
// QML Interface
// ========================================================================

QHash<int, QByteArray>
TrackCollection::roleNames () const
{
  QHash<int, QByteArray> roles;
  roles[TrackPtrRole] = "track";
  roles[TrackFoldableRole] = "foldable";
  roles[TrackExpandedRole] = "expanded";
  roles[TrackDepthRole] = "depth";
  roles[TrackNameRole] = "trackName";
  return roles;
}

int
TrackCollection::rowCount (const QModelIndex &parent) const
{
  return static_cast<int> (tracks_.size ());
}

QVariant
TrackCollection::data (const QModelIndex &index, int role) const
{
  if (!index.isValid ())
    return {};

  auto track_ref = tracks_.at (index.row ());
  auto track_var = track_ref.get_object ();

  switch (role)
    {
    case TrackPtrRole:
      return QVariant::fromStdVariant (track_var);
    case TrackNameRole:
      return track_ref.get_object_base ()->name ();
    case TrackFoldableRole:
      return Track::type_is_foldable (track_ref.get_object_base ()->type ());
    case TrackExpandedRole:
      return expanded_tracks_.contains (track_ref.id ());
    case TrackDepthRole:
      {
        int         depth = 0;
        Track::Uuid cur_track = track_ref.id ();
        while (folder_parent_.contains (cur_track))
          {
            ++depth;
            cur_track = folder_parent_.at (cur_track);
          }
        return depth;
      }
    default:
      return {};
    }
}

void
TrackCollection::setTrackExpanded (const Track * track, bool expanded)
{
  set_track_expanded (track->get_uuid (), expanded);
}

int
TrackCollection::numSoloedTracks () const
{
  return static_cast<int> (get_track_span ().get_num_soloed_tracks ());
}
int
TrackCollection::numMutedTracks () const
{
  return static_cast<int> (get_track_span ().get_num_muted_tracks ());
}
int
TrackCollection::numListenedTracks () const
{
  return static_cast<int> (get_track_span ().get_num_listened_tracks ());
}

// ========================================================================
// Track Management
// ========================================================================

TrackPtrVariant
TrackCollection::get_track_at_index (size_t index) const
{
  if (index >= tracks_.size ())
    throw std::out_of_range ("Track index out of range");
  return get_track_span ().at (index);
}

TrackUuidReference
TrackCollection::get_track_ref_at_index (size_t index) const
{
  if (index >= tracks_.size ())
    throw std::out_of_range ("Track index out of range");
  return tracks_.at (index);
}

bool
TrackCollection::contains (const Track::Uuid &track_id) const
{
  return std::ranges::find (tracks_, track_id, &TrackUuidReference::id)
         != tracks_.end ();
}

void
TrackCollection::add_track (const TrackUuidReference &track_id)
{
  insert_track (track_id, static_cast<int> (tracks_.size ()));
}

void
TrackCollection::insert_track (const TrackUuidReference &track_id, int pos)
{
  beginInsertRows ({}, pos, pos);
  tracks_.emplace_back (track_id);

  // Check if this track is foldable and initialize its expanded state
  auto * track = tracks::from_variant (track_id.get_object ());
  if (Track::type_is_foldable (track->type ()))
    {
      expanded_tracks_.insert (track->get_uuid ());
    }

  // If inserting at a specific position, move it there
  if (static_cast<size_t> (pos) != tracks_.size () - 1)
    {
      for (int i = static_cast<int> (tracks_.size ()) - 1; i > pos; --i)
        {
          std::swap (tracks_[i], tracks_[i - 1]);
        }
    }
  endInsertRows ();
}

void
TrackCollection::remove_track (const Track::Uuid &track_id)
{
  auto track_it = std::ranges::find (tracks_, track_id, &TrackUuidReference::id);
  if (track_it == tracks_.end ())
    return;

  const auto track_index = std::distance (tracks_.begin (), track_it);

  auto *                                 track = track_it->get_object_base ();
  std::vector<plugins::PluginPtrVariant> plugins;
  track->collect_plugins (plugins);
  for (const auto &plugin_var : plugins)
    {
      auto * plugin = plugins::plugin_ptr_variant_to_base (plugin_var);
      if (plugin->uiVisible ())
        {
          plugin->setUiVisible (false);
        }
    }

  beginRemoveRows (
    {}, static_cast<int> (track_index), static_cast<int> (track_index));

  // Remove from expanded tracks set
  expanded_tracks_.erase (track_id);

  // Remove from folder parent map (both as child and as parent)
  folder_parent_.erase (track_id);
  std::erase_if (folder_parent_, [&] (const auto &pair) {
    return pair.second == track_id;
  });

  tracks_.erase (track_it);
  endRemoveRows ();
}

void
TrackCollection::detach_track (const Track::Uuid &track_id)
{
  auto track_it = std::ranges::find (tracks_, track_id, &TrackUuidReference::id);
  if (track_it == tracks_.end ())
    return;

  const auto track_index = std::distance (tracks_.begin (), track_it);

  beginRemoveRows (
    {}, static_cast<int> (track_index), static_cast<int> (track_index));

  // Intentionally do NOT clear expanded_tracks_ or folder_parent_ -
  // the caller will reattach the track shortly.
  // Note: rowsAboutToBeRemoved will disconnect solo/mute/listen signals.
  // A matching reattach_track() call will reconnect them via rowsInserted.

  tracks_.erase (track_it);
  endRemoveRows ();
}

void
TrackCollection::reattach_track (const TrackUuidReference &track_id, int pos)
{
  pos = std::clamp (pos, 0, static_cast<int> (tracks_.size ()));
  beginInsertRows ({}, pos, pos);
  tracks_.insert (std::next (tracks_.begin (), pos), track_id);
  endInsertRows ();
}

void
TrackCollection::move_track (const Track::Uuid &track_id, int pos)
{
  if (pos < 0 || pos >= static_cast<int> (tracks_.size ()))
    throw std::out_of_range ("move_track position out of range");

  auto track_it = std::ranges::find (tracks_, track_id, &TrackUuidReference::id);
  if (track_it == tracks_.end ())
    return;

  const auto track_index = std::distance (tracks_.begin (), track_it);
  if (pos == track_index)
    return;

  bool move_higher = pos < track_index;

  beginMoveRows (
    {}, static_cast<int> (track_index), static_cast<int> (track_index), {},
    move_higher ? pos : (pos + 1));

  if (move_higher)
    {
      // move all other tracks 1 track further
      for (
        auto i = track_index; i > static_cast<decltype (track_index)> (pos); i--)
        {
          std::swap (tracks_[i], tracks_[i - 1]);
        }
    }
  else
    {
      // move all other tracks 1 track earlier
      for (
        auto i = track_index; i < static_cast<decltype (track_index)> (pos); i++)
        {
          std::swap (tracks_[i], tracks_[i + 1]);
        }
    }

  endMoveRows ();
}

void
TrackCollection::clear ()
{
  beginResetModel ();
  tracks_.clear ();
  expanded_tracks_.clear ();
  folder_parent_.clear ();
  endResetModel ();
}

// ========================================================================
// Foldable Track Management
// ========================================================================

void
TrackCollection::set_track_expanded (const Track::Uuid &track_id, bool expanded)
{
  if (expanded)
    {
      expanded_tracks_.insert (track_id);
    }
  else
    {
      expanded_tracks_.erase (track_id);
    }

  const auto   track_index = static_cast<int> (get_track_index (track_id));
  QModelIndex  model_index = createIndex (track_index, 0);
  QVector<int> roles;
  roles << TrackExpandedRole;
  Q_EMIT dataChanged (model_index, model_index, roles);

  // Also emit dataChanged for all descendants so proxy filters re-evaluate.
  // This ensures child tracks appear/disappear when a folder is
  // expanded/collapsed.
  const auto descendants = get_all_descendants (track_id);
  for (const auto &desc_id : descendants)
    {
      const auto  desc_idx = static_cast<int> (get_track_index (desc_id));
      QModelIndex desc_mi = createIndex (desc_idx, 0);
      Q_EMIT dataChanged (desc_mi, desc_mi, roles);
    }
}

bool
TrackCollection::get_track_expanded (const Track::Uuid &track_id) const
{
  return expanded_tracks_.contains (track_id);
}

void
TrackCollection::set_folder_parent (
  const Track::Uuid &child_id,
  const Track::Uuid &parent_id,
  bool               auto_reposition)
{
  // Ensure the parent track exists and is foldable
  if (!contains (parent_id) || !is_track_foldable (parent_id))
    {
      throw std::invalid_argument ("Parent track must exist and be foldable");
    }

  // Ensure the child track exists
  if (!contains (child_id))
    {
      throw std::invalid_argument ("Child track must exist");
    }

  // Remember last child index before setting the relationship, so the new
  // child isn't counted yet.
  size_t last_child_index = 0;
  if (auto_reposition)
    {
      last_child_index = get_last_child_index (parent_id);
    }

  // Set the parent-child relationship
  folder_parent_[child_id] = parent_id;

  if (auto_reposition)
    {
      auto child_index = get_track_index (child_id);

      if (
        static_cast<int> (child_index)
        != static_cast<int> (last_child_index) + 1)
        {
          move_track (child_id, static_cast<int> (last_child_index) + 1);
        }
    }
}

std::optional<Track::Uuid>
TrackCollection::get_folder_parent (const Track::Uuid &child_id) const
{
  if (folder_parent_.contains (child_id))
    {
      return folder_parent_.at (child_id);
    }
  return std::nullopt;
}

void
TrackCollection::remove_folder_parent (
  const Track::Uuid &child_id,
  bool               auto_reposition)
{
  if (!folder_parent_.contains (child_id))
    {
      return;
    }

  // Remember last child index before erasing
  auto parent_id = folder_parent_.at (child_id);
  auto last_child_index = get_last_child_index (parent_id);

  folder_parent_.erase (child_id);

  if (auto_reposition)
    {
      // Move the child to right after the folder's last child
      auto child_index = get_track_index (child_id);

      if (static_cast<int> (child_index) != static_cast<int> (last_child_index))
        {
          move_track (child_id, static_cast<int> (last_child_index));
        }
    }
}

bool
TrackCollection::is_track_foldable (const Track::Uuid &track_id) const
{
  auto track_var = get_track_at_index (get_track_index (track_id));
  return Track::type_is_foldable (tracks::from_variant (track_var)->type ());
}

bool
TrackCollection::is_ancestor_of (
  const Track::Uuid &possible_ancestor,
  const Track::Uuid &track_id) const
{
  auto cur = track_id;
  while (folder_parent_.contains (cur))
    {
      cur = folder_parent_.at (cur);
      if (cur == possible_ancestor)
        return true;
    }
  return false;
}

std::optional<Track::Uuid>
TrackCollection::get_enclosing_folder (size_t index) const
{
  if (tracks_.empty () || index == 0)
    return std::nullopt;

  // Walk backward from index to find the nearest expanded foldable track
  // whose child range covers `index`.
  auto candidates =
    tracks_ | std::views::take (index) | std::views::reverse
    | std::views::filter ([&] (const auto &ref) {
        return is_track_foldable (ref.id ()) && get_track_expanded (ref.id ())
               && get_last_child_index (ref.id ()) >= index;
      });

  auto it = candidates.begin ();
  if (it != candidates.end ())
    return it->id ();

  return std::nullopt;
}

// ========================================================================
// Foldable Track Child Management
// ========================================================================

size_t
TrackCollection::get_child_count (const Track::Uuid &parent_id) const
{
  if (!is_track_foldable (parent_id))
    {
      return 0;
    }

  auto parent_index = get_track_index (parent_id);

  // Count all descendants (direct children and nested) after the parent.
  // Descendants are contiguous in the list, so take_while stops at the
  // first non-descendant.
  return std::ranges::distance (
    tracks_ | std::views::drop (parent_index + 1)
    | std::views::take_while ([&] (const auto &ref) {
        return is_ancestor_of (parent_id, ref.id ());
      }));
}

size_t
TrackCollection::get_last_child_index (const Track::Uuid &parent_id) const
{
  if (!is_track_foldable (parent_id))
    {
      return get_track_index (parent_id);
    }

  auto parent_index = get_track_index (parent_id);
  auto child_count = get_child_count (parent_id);

  return parent_index + child_count;
}

std::vector<Track::Uuid>
TrackCollection::get_all_descendants (const Track::Uuid &parent_id) const
{
  auto parent_index = get_track_index (parent_id);

  return tracks_ | std::views::drop (parent_index + 1)
         | std::views::take_while ([&] (const auto &ref) {
             return is_ancestor_of (parent_id, ref.id ());
           })
         | std::views::transform ([] (const auto &ref) { return ref.id (); })
         | std::ranges::to<std::vector> ();
}

// ========================================================================
// Serialization
// ========================================================================

void
to_json (nlohmann::json &j, const TrackCollection &collection)
{
  j[TrackCollection::kTracksKey] = collection.tracks_;
  j[TrackCollection::kFolderParentsKey] = collection.folder_parent_;
  j[TrackCollection::kExpandedTracksKey] = collection.expanded_tracks_;
}

void
from_json (const nlohmann::json &j, TrackCollection &collection)
{
  for (const auto &child_j : j.at (TrackCollection::kTracksKey))
    {
      TrackUuidReference ref{ collection.track_registry_ };
      from_json (child_j, ref);
      collection.add_track (ref);
    }

  if (j.contains (TrackCollection::kFolderParentsKey))
    {
      j.at (TrackCollection::kFolderParentsKey)
        .get_to (collection.folder_parent_);
    }

  if (j.contains (TrackCollection::kExpandedTracksKey))
    {
      j.at (TrackCollection::kExpandedTracksKey)
        .get_to (collection.expanded_tracks_);
    }
}

TrackCollection::~TrackCollection () noexcept = default;

} // namespace zrythm::structure::tracks
