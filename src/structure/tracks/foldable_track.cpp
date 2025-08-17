// SPDX-FileCopyrightText: © 2021, 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/tracks/foldable_track.h"
#include "structure/tracks/track_all.h"

namespace zrythm::structure::tracks
{

FoldableTrackMixin::FoldableTrackMixin (
  TrackRegistry &track_registry,
  QObject *      parent) noexcept
    : QAbstractListModel (parent), track_registry_ (track_registry)
{
}

// ========================================================================
// QML Interface
// ========================================================================

QHash<int, QByteArray>
FoldableTrackMixin::roleNames () const
{
  QHash<int, QByteArray> roles;
  roles[TrackPtrRole] = "track";
  return roles;
}

int
FoldableTrackMixin::rowCount (const QModelIndex &parent) const
{
  return static_cast<int> (children_.size ());
}

QVariant
FoldableTrackMixin::data (const QModelIndex &index, int role) const
{
  if (!index.isValid ())
    return {};

  auto track_id = children_.at (index.row ());
  auto track = track_id.get_object ();

  switch (role)
    {
    case TrackPtrRole:
      return QVariant::fromStdVariant (track);
    default:
      return {};
    }
}

// ========================================================================

void
to_json (nlohmann::json &j, const FoldableTrackMixin &track)
{
  j[FoldableTrackMixin::kChildrenKey] = track.children_;
  j[FoldableTrackMixin::kFoldedKey] = track.folded_;
}
void
from_json (const nlohmann::json &j, FoldableTrackMixin &track)
{
  for (const auto &child_j : j.at (FoldableTrackMixin::kChildrenKey))
    {
      TrackUuidReference ref{ track.track_registry_ };
      from_json (child_j, ref);
      track.children_.push_back (ref);
    }
  j.at (FoldableTrackMixin::kFoldedKey).get_to (track.folded_);
}
}
