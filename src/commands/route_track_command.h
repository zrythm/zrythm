// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <utility>

#include "structure/tracks/track_all.h"
#include "structure/tracks/track_routing.h"

#include <QUndoCommand>

namespace zrythm::commands
{
/**
 * @brief Command that routes (or re-routes) a track to an optional target.
 */
class RouteTrackCommand : public QUndoCommand
{
public:
  static constexpr auto CommandId = 6451638;

  RouteTrackCommand (
    structure::tracks::TrackRouting              &router,
    structure::tracks::Track::Uuid                source_id,
    std::optional<structure::tracks::Track::Uuid> target_id)
      : QUndoCommand (QObject::tr ("Route Track")), router_ (router),
        source_id_ (source_id), target_id_ (target_id)
  {
  }

  int id () const override { return CommandId; }

  void undo () override
  {
    // restore previous routing

    const auto prev_target = target_id_;

    // re-store current target
    const auto cur_target = router_.get_output_track (source_id_);
    if (cur_target.has_value ())
      {
        target_id_ = cur_target->id ();
      }
    else
      {
        target_id_ = std::nullopt;
      }

    // replace
    if (prev_target.has_value ())
      {
        router_.add_or_replace_route (source_id_, prev_target.value ());
      }
    else
      {
        router_.remove_route_for_source (source_id_);
      }
  }
  void redo () override
  {
    // get new target
    const auto new_target = target_id_;

    // store previous target
    const auto prev_target = router_.get_output_track (source_id_);
    if (prev_target.has_value ())
      {
        target_id_ = prev_target->id ();
      }
    else
      {
        target_id_ = std::nullopt;
      }

    // replace
    if (new_target.has_value ())
      {
        router_.add_or_replace_route (source_id_, new_target.value ());
      }
    else
      {
        router_.remove_route_for_source (source_id_);
      }
  }

private:
  structure::tracks::TrackRouting              &router_;
  structure::tracks::Track::Uuid                source_id_;
  std::optional<structure::tracks::Track::Uuid> target_id_;
};

} // namespace zrythm::commands
