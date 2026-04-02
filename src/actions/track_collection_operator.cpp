// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <unordered_set>

#include "actions/track_collection_operator.h"
#include "commands/move_tracks_command.h"

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
  std::vector<structure::tracks::TrackUuidReference> track_refs;
  track_refs.reserve (tracks.size ());

  auto &registry = collection_->get_track_registry ();
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
  // When a folder is moved, its children must move with it.
  auto seen =
    track_refs
    | std::views::transform ([&] (const auto &ref) { return ref.id (); })
    | std::ranges::to<std::unordered_set> ();

  // Collect descendants for each foldable track, preserving list order
  auto expanded_refs = track_refs;
  for (const auto &ref : track_refs)
    {
      if (collection_->is_track_foldable (ref.id ()))
        {
          for (
            const auto &desc_id : collection_->get_all_descendants (ref.id ()))
            {
              if (!seen.contains (desc_id))
                {
                  expanded_refs.emplace_back (desc_id, registry);
                  seen.insert (desc_id);
                }
            }
        }
    }
  track_refs = std::move (expanded_refs);

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

} // namespace zrythm::actions
