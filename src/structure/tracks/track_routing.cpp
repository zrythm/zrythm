// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/tracks/track_all.h"
#include "structure/tracks/track_routing.h"

namespace zrythm::structure::tracks
{
TrackRouting::TrackRouting (TrackRegistry &track_registry, QObject * parent)
    : QObject (parent), track_registry_ (track_registry)
{
}

QVariant
TrackRouting::getOutputTrack (const Track * source) const
{
  auto output = get_output_track (source->get_uuid ());
  return output ? QVariant::fromStdVariant (output.value ().get_object ())
                : QVariant{};
}

void
TrackRouting::setOutputTrack (const Track * source, const Track * destination)
{
  if (!canRouteTo (source, destination))
    {
      z_warning (
        "cannot route track '{}' to '{}' - incompatible signal types or invalid source",
        source ? source->name () : "(null)",
        destination ? destination->name () : "(null)");
      return;
    }

  if (destination == nullptr)
    {
      z_info ("unrouting track '{}'", source->name ());
      remove_route_for_source (source->get_uuid ());
    }
  else
    {
      z_info (
        "routing track '{}' to '{}'", source->name (), destination->name ());
      add_or_replace_route (source->get_uuid (), destination->get_uuid ());
    }
}

bool
TrackRouting::canRouteTo (const Track * source, const Track * destination) const
{
  // Null source cannot be routed
  if (source == nullptr)
    return false;

  // Null destination means "unroute" - always valid if source is valid
  if (destination == nullptr)
    return true;

  // Cannot route a track to itself
  if (source->get_uuid () == destination->get_uuid ())
    return false;

  // Master track cannot be routed to anything (it routes internally to device
  // output)
  if (source->is_master ())
    return false;

  // Destination must be able to be a group target
  if (!destination->can_be_group_target ())
    return false;

  // Check for circular routes - follow destination's output chain
  // to see if it eventually leads back to source
  auto current_uuid = destination->get_uuid ();
  while (true)
    {
      auto output_it = track_routes_.find (current_uuid);
      if (output_it == track_routes_.end ())
        break; // No more routes in chain

      if (output_it->second == source->get_uuid ())
        return false; // Found cycle: destination -> ... -> source

      current_uuid = output_it->second;
    }

  // Signal types must be compatible
  const auto source_out_type = source->output_signal_type ();
  const auto dest_in_type = destination->input_signal_type ();

  // Both must have signal types defined and they must match
  return source_out_type.has_value () && dest_in_type.has_value ()
         && source_out_type.value () == dest_in_type.value ();
}

void
TrackRouting::add_or_replace_route (
  const TrackUuid &source,
  const TrackUuid &destination)
{
  track_routes_.insert_or_assign (source, destination);
  Q_EMIT routingChanged ();
}
void
TrackRouting::remove_route_for_source (const TrackUuid &source)
{
  track_routes_.erase (source);
  Q_EMIT routingChanged ();
}
void
TrackRouting::remove_routes_for_destination (const TrackUuid &destination)
{
  std::erase_if (track_routes_, [&destination] (const auto &kv) {
    return kv.second == destination;
  });
  Q_EMIT routingChanged ();
}

std::optional<TrackUuidReference>
TrackRouting::get_output_track (const TrackUuid &source) const
{
  auto it = track_routes_.find (source);
  if (it == track_routes_.end ())
    {
      return std::nullopt;
    }

  return TrackUuidReference{ it->second, track_registry_ };
}

void
to_json (nlohmann::json &j, const TrackRouting &t)
{
  j[TrackRouting::kTrackRoutesKey] = t.track_routes_;
}
void
from_json (const nlohmann::json &j, TrackRouting &t)
{
  j.at (TrackRouting::kTrackRoutesKey).get_to (t.track_routes_);
}

} // namespace zrythm::structure::tracks
