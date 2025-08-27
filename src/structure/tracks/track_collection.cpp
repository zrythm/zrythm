// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/tracks/track_all.h"
#include "structure/tracks/track_collection.h"

namespace zrythm::structure::tracks
{

TrackCollection::TrackCollection (
  TrackRegistry &track_registry,
  QObject *      parent) noexcept
    : QAbstractListModel (parent), track_registry_ (track_registry)
{
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
    case TrackFoldableRole:
      return track_expanded_.contains (track_ref.id ());
    case TrackExpandedRole:
      {
        auto it = track_expanded_.find (track_ref.id ());
        if (it == track_expanded_.end ())
          return false;

        return it->second;
      }
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
  beginResetModel ();
  tracks_.emplace_back (track_id);

  // Check if this track is foldable and initialize its expanded state
  auto * track = tracks::from_variant (track_id.get_object ());
  if (Track::type_is_foldable (track->type ()))
    {
      track_expanded_[track->get_uuid ()] = true;
    }

  // If inserting at a specific position, move it there
  if (static_cast<size_t> (pos) != tracks_.size () - 1)
    {
      for (int i = static_cast<int> (tracks_.size ()) - 1; i > pos; --i)
        {
          std::swap (tracks_[i], tracks_[i - 1]);
        }
    }
  endResetModel ();
}

void
TrackCollection::remove_track (const Track::Uuid &track_id)
{
  auto track_it = std::ranges::find (tracks_, track_id, &TrackUuidReference::id);
  if (track_it == tracks_.end ())
    return;

  const auto track_index = std::distance (tracks_.begin (), track_it);

  beginRemoveRows (
    {}, static_cast<int> (track_index), static_cast<int> (track_index));

  // Remove from foldable track maps
  track_expanded_.erase (track_id);

  // Remove from folder parent map (both as child and as parent)
  folder_parent_.erase (track_id);
  std::erase_if (folder_parent_, [&] (const auto &pair) {
    return pair.second == track_id;
  });

  tracks_.erase (track_it);
  endRemoveRows ();
}

void
TrackCollection::move_track (const Track::Uuid &track_id, int pos)
{
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
  track_expanded_.clear ();
  folder_parent_.clear ();
  endResetModel ();
}

// ========================================================================
// Serialization
// ========================================================================

void
to_json (nlohmann::json &j, const TrackCollection &collection)
{
  j[TrackCollection::kTracksKey] = collection.tracks_;
  j[TrackCollection::kFolderParentMapKey] = collection.folder_parent_;
  j[TrackCollection::kExpandedMapKey] = collection.track_expanded_;
}

void
from_json (const nlohmann::json &j, TrackCollection &collection)
{
  for (const auto &child_j : j.at (TrackCollection::kTracksKey))
    {
      TrackUuidReference ref{ collection.track_registry_ };
      from_json (child_j, ref);
      collection.tracks_.push_back (ref);
    }
  j.at (TrackCollection::kFolderParentMapKey).get_to (collection.folder_parent_);
  j.at (TrackCollection::kExpandedMapKey).get_to (collection.track_expanded_);
}

// ========================================================================
// Foldable Track Management
// ========================================================================

void
TrackCollection::set_track_expanded (const Track::Uuid &track_id, bool expanded)
{
  if (track_expanded_.contains (track_id))
    {
      beginResetModel ();
      track_expanded_[track_id] = expanded;
      endResetModel ();
    }
}

bool
TrackCollection::get_track_expanded (const Track::Uuid &track_id) const
{
  if (track_expanded_.contains (track_id))
    {
      return track_expanded_.at (track_id);
    }
  return false;
}

void
TrackCollection::set_folder_parent (
  const Track::Uuid &child_id,
  const Track::Uuid &parent_id)
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

  // Remember last child index
  auto last_child_index = get_last_child_index (parent_id);

  // Set the parent-child relationship
  folder_parent_[child_id] = parent_id;

  // Move the child to be the last child of the parent
  auto child_index = get_track_index (child_id);

  if (static_cast<int> (child_index) != static_cast<int> (last_child_index) + 1)
    {
      move_track (child_id, static_cast<int> (last_child_index) + 1);
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
TrackCollection::remove_folder_parent (const Track::Uuid &child_id)
{
  if (!folder_parent_.contains (child_id))
    {
      return;
    }

  // Remember last child index before erasing
  auto parent_id = folder_parent_.at (child_id);
  auto last_child_index = get_last_child_index (parent_id);

  folder_parent_.erase (child_id);

  // Move the child to right after the folder's last child
  auto child_index = get_track_index (child_id);

  if (static_cast<int> (child_index) != static_cast<int> (last_child_index))
    {
      move_track (child_id, static_cast<int> (last_child_index));
    }
}

bool
TrackCollection::is_track_foldable (const Track::Uuid &track_id) const
{
  auto track_var = get_track_at_index (get_track_index (track_id));
  return Track::type_is_foldable (tracks::from_variant (track_var)->type ());
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

  size_t count = 0;
  auto   parent_index = get_track_index (parent_id);

  // Count consecutive children after the parent
  for (size_t i = parent_index + 1; i < tracks_.size (); ++i)
    {
      auto child_id = tracks_[i].id ();
      if (
        folder_parent_.contains (child_id)
        && folder_parent_.at (child_id) == parent_id)
        {
          ++count;
        }
      else
        {
          // Stop when we find a track that's not a child of this parent
          break;
        }
    }

  return count;
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

} // namespace zrythm::structure::tracks
