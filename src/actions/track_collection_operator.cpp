// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <stdexcept>

#include "actions/track_collection_operator.h"
#include "commands/delete_tracks_command.h"
#include "commands/move_tracks_command.h"

#include <fmt/format.h>

namespace zrythm::actions
{

void
TrackCollectionOperator::moveTracks (
  const QList<structure::tracks::Track *> &tracks,
  int                                      targetPosition,
  structure::tracks::Track *               targetFolder)
{
  if ((collection_ == nullptr) || (undo_stack_ == nullptr) || tracks.isEmpty ())
    return;

  // Convert Track* list to TrackUuidReference vector
  auto &registry = collection_->get_track_registry ();
  std::vector<structure::tracks::TrackUuidReference> track_refs;
  track_refs.reserve (tracks.size ());

  for (auto * track : tracks)
    {
      if (track != nullptr)
        {
          track_refs.emplace_back (track->get_uuid (), registry);
        }
    }

  if (track_refs.empty ())
    return;

  // Expand the move list to include all descendants of any foldable tracks.
  track_refs = expand_with_descendants (track_refs);

  // Determine folder target.
  // targetPosition is a pre-removal index (the raw drop target from QML),
  // so get_enclosing_folder can be called directly on the current collection.
  std::optional<structure::tracks::Track::Uuid> target_folder_uuid;

  if (
    targetFolder != nullptr
    && structure::tracks::Track::type_is_foldable (targetFolder->type ()))
    {
      target_folder_uuid = targetFolder->get_uuid ();
    }
  else if (
    auto enclosing =
      collection_->get_enclosing_folder (static_cast<size_t> (targetPosition)))
    {
      target_folder_uuid = enclosing;
    }

  auto cmd = std::make_unique<commands::MoveTracksCommand> (
    *collection_, std::move (track_refs), targetPosition, target_folder_uuid);
  undo_stack_->push (cmd.release ());
}

void
TrackCollectionOperator::deleteTracks (
  const QList<structure::tracks::Track *> &tracks)
{
  if ((collection_ == nullptr) || (undo_stack_ == nullptr) || tracks.isEmpty ())
    return;

  auto &registry = collection_->get_track_registry ();
  std::vector<structure::tracks::TrackUuidReference> track_refs;
  track_refs.reserve (tracks.size ());

  for (const auto * track : tracks)
    {
      if (track == nullptr)
        continue;
      if (!structure::tracks::Track::type_is_deletable (track->type ()))
        {
          throw std::invalid_argument (
            fmt::format (
              "Cannot delete non-deletable track: {}",
              track->name ().toStdString ()));
        }
      track_refs.emplace_back (track->get_uuid (), registry);
    }

  if (track_refs.empty ())
    return;

  auto expanded_refs = expand_with_descendants (track_refs);

  auto cmd = std::make_unique<commands::DeleteTracksCommand> (
    *collection_, std::move (expanded_refs));
  undo_stack_->push (cmd.release ());
}

} // namespace zrythm::actions
