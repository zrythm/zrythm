// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <ranges>

#include "commands/delete_tracks_command.h"
#include "plugins/plugin_all.h"
#include "structure/tracks/track_all.h"

namespace zrythm::commands
{

DeleteTracksCommand::DeleteTracksCommand (
  structure::tracks::TrackCollection                &collection,
  std::vector<structure::tracks::TrackUuidReference> track_refs)
    : QUndoCommand (
        track_refs.size () == 1
          ? QObject::tr ("Delete Track")
          : QObject::tr ("Delete Tracks")),
      collection_ (collection)
{
  tracks_ =
    track_refs | std::views::transform ([&] (const auto &ref) {
      return TrackInfo{
        .ref = ref,
        .original_position =
          static_cast<int> (collection.get_track_index (ref.id ())),
        .original_folder_parent = collection.get_folder_parent (ref.id ()),
        .original_expanded_state = collection.get_track_expanded (ref.id ()),
      };
    })
    | std::ranges::to<std::vector> ();

  deleted_uuids_ =
    tracks_
    | std::views::transform ([] (const auto &info) { return info.ref.id (); })
    | std::ranges::to<std::unordered_set> ();
}

void
DeleteTracksCommand::redo ()
{
  // Close plugin windows for all tracks being deleted
  for (const auto &info : tracks_)
    {
      auto * track = structure::tracks::from_variant (info.ref.get_object ());
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
    }

  // Remove tracks in reverse position order to avoid index shifting
  auto sorted = tracks_ | std::ranges::to<std::vector> ();
  std::ranges::sort (sorted, std::greater{}, &TrackInfo::original_position);
  for (const auto &info : sorted)
    {
      collection_.remove_track (info.ref.id ());
    }
}

void
DeleteTracksCommand::undo ()
{
  // Re-insert tracks at their original positions in forward order
  auto sorted = tracks_ | std::ranges::to<std::vector> ();
  std::ranges::sort (sorted, {}, &TrackInfo::original_position);
  for (const auto &info : sorted)
    {
      collection_.insert_track (info.ref, info.original_position);
    }

  // Restore original folder parents
  for (const auto &info : tracks_)
    {
      if (info.original_folder_parent.has_value ())
        {
          collection_.set_folder_parent (
            info.ref.id (), info.original_folder_parent.value ());
        }
    }

  // Restore original expanded states for foldable tracks
  for (const auto &info : tracks_)
    {
      if (
        structure::tracks::Track::type_is_foldable (
          structure::tracks::from_variant (info.ref.get_object ())->type ()))
        {
          collection_.set_track_expanded (
            info.ref.id (), info.original_expanded_state);
        }
    }

  collection_.notify_tracks_moved (deleted_uuids_);
}

} // namespace zrythm::commands
