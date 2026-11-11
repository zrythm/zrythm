// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <ranges>

#include "commands/delete_tracks_command.h"
#include "plugins/plugin.h"
#include "structure/tracks/track_all.h"

namespace zrythm::commands
{

DeleteTracksCommand::DeleteTracksCommand (
  structure::tracks::TrackCollection                &collection,
  structure::tracks::TrackRouting                   &router,
  std::vector<structure::tracks::TrackUuidReference> track_refs)
    : Command (
        track_refs.size () == 1
          ? QObject::tr ("Delete Track")
          : QObject::tr ("Delete Tracks"),
        EngineRequirement::RecalculateGraph),
      collection_ (collection), router_ (router)
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

std::unordered_set<dsp::PortUuid>
DeleteTracksCommand::deleted_tracks_port_ids () const
{
  std::unordered_set<dsp::PortUuid> port_ids;

  for (const auto &info : tracks_)
    {
      const auto * track = info.ref.get ();
      if (track == nullptr)
        continue;

      if (const auto * processor = track->get_track_processor ())
        {
          port_ids.merge (processor->get_all_port_ids ());
        }
      if (auto * channel = track->channel ())
        {
          for (const auto &plugin_ref : channel->get_plugins ())
            {
              if (const auto * plugin = plugin_ref.get ())
                {
                  port_ids.merge (plugin->get_all_port_ids ());
                }
            }
        }
    }

  return port_ids;
}

void
DeleteTracksCommand::redo ()
{
  // Capture and sever references to the deleted tracks before removing
  // them: the routing graph must never point at ports of absent tracks
  removed_routes_.clear ();
  cleared_sends_.clear ();

  const auto deleted_port_ids = deleted_tracks_port_ids ();

  for (const auto &deleted_uuid : deleted_uuids_)
    {
      // Routes of other tracks into the deleted track
      for (
        const auto &source :
        router_.remove_routes_for_destination (deleted_uuid))
        {
          removed_routes_.emplace_back (source, deleted_uuid);
        }

      // The deleted track's own outgoing route
      if (
        const auto destination = router_.get_output_track (deleted_uuid);
        destination.has_value ())
        {
          removed_routes_.emplace_back (deleted_uuid, destination->id ());
          router_.remove_route_for_source (deleted_uuid);
        }
    }

  for (const auto &track_ref : collection_.tracks ())
    {
      const auto * track = track_ref.get ();
      if (
        track == nullptr || track->channel () == nullptr
        || deleted_uuids_.contains (track->get_uuid ()))
        continue;

      for (const auto &send : track->channel ()->sends ()->get_sends ())
        {
          auto *     send_ptr = send.get ();
          const auto destination = send_ptr->destination_port ();
          if (
            !destination.has_value ()
            || !deleted_port_ids.contains (destination->id ()))
            continue;

          cleared_sends_.push_back (ClearedSend{ send, destination->id () });
          send_ptr->clear_destination_port ();
        }
    }

  // Remove tracks in reverse position order to avoid index shifting
  auto sorted = tracks_ | std::ranges::to<std::vector> ();
  std::ranges::sort (
    sorted, std::ranges::greater{}, &TrackInfo::original_position);
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
      if (structure::tracks::Track::type_is_foldable (info.ref.get ()->type ()))
        {
          collection_.set_track_expanded (
            info.ref.id (), info.original_expanded_state);
        }
    }

  // Restore the severed routes and send destinations
  for (const auto &[source, destination] : removed_routes_)
    {
      router_.add_or_replace_route (source, destination);
    }
  for (const auto &cleared : cleared_sends_)
    {
      if (auto * send = cleared.send.get ())
        {
          send->set_destination_port (
            dsp::PortUuidReference{ cleared.destination, send->registry () });
        }
    }

  collection_.notify_tracks_moved (deleted_uuids_);
}

} // namespace zrythm::commands
