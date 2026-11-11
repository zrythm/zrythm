// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <optional>
#include <unordered_set>
#include <utility>
#include <vector>

#include "commands/command.h"
#include "dsp/port.h"
#include "dsp/port_fwd.h"
#include "structure/tracks/channel_send.h"
#include "structure/tracks/track_collection.h"
#include "structure/tracks/track_routing.h"

namespace zrythm::commands
{

/**
 * @brief Removes tracks from the collection.
 *
 * Routes and channel sends that target the removed tracks are severed
 * with them and restored on undo, so the routing graph never references
 * ports of absent tracks.
 */
class DeleteTracksCommand : public Command
{
public:
  DeleteTracksCommand (
    structure::tracks::TrackCollection                &collection,
    structure::tracks::TrackRouting                   &router,
    std::vector<structure::tracks::TrackUuidReference> track_refs);

  void undo () override;
  void redo () override;

private:
  struct TrackInfo
  {
    structure::tracks::TrackUuidReference         ref;
    int                                           original_position;
    std::optional<structure::tracks::Track::Uuid> original_folder_parent;
    bool                                          original_expanded_state;
  };

  /** A send destination severed because its target track was deleted. */
  struct ClearedSend
  {
    structure::tracks::ChannelSendUuidReference send;
    dsp::PortUuid                               destination;
  };

  /** Collects the ports of the tracks being deleted. */
  [[nodiscard]] std::unordered_set<dsp::PortUuid>
  deleted_tracks_port_ids () const;

  structure::tracks::TrackCollection &collection_;
  structure::tracks::TrackRouting    &router_;

  std::vector<TrackInfo>                             tracks_;
  std::unordered_set<structure::tracks::Track::Uuid> deleted_uuids_;

  /** Routes severed by the deletion, as (source, destination) pairs;
   * covers routes into the deleted tracks and the deleted tracks' own
   * outgoing routes. */
  std::vector<
    std::pair<structure::tracks::Track::Uuid, structure::tracks::Track::Uuid>>
    removed_routes_;

  std::vector<ClearedSend> cleared_sends_;
};

} // namespace zrythm::commands
