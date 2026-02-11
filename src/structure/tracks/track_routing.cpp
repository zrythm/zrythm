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
  z_info ("routing track '{}' to '{}'", source->name (), destination->name ());
  add_or_replace_route (source->get_uuid (), destination->get_uuid ());
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
