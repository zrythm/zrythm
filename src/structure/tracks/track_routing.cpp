// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/format_qt.h"

#include "structure/tracks/track_all.h"
#include "structure/tracks/track_routing.h"
#include "utils/utf8_string.h"

#include <magic_enum/magic_enum.hpp>

namespace zrythm::structure::tracks
{
TrackRouting::TrackRouting (utils::IObjectRegistry &registry, QObject * parent)
    : QObject (parent), registry_ (registry)
{
}

QVariant
TrackRouting::getOutputTrack (const Track * source) const
{
  auto output = get_output_track (source->get_uuid ());
  return output ? QVariant::fromValue (output->get ()) : QVariant{};
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
  return !route_refusal_reason (source, destination).has_value ();
}

std::optional<utils::Utf8String>
TrackRouting::route_refusal_reason (
  const Track * source,
  const Track * destination) const
{
  // Null source cannot be routed
  if (source == nullptr)
    return utils::Utf8String{ u8"no source track" };

  // Null destination means "unroute" - always valid if source is valid
  if (destination == nullptr)
    return std::nullopt;

  // Reasons are formatted on the refusal branches only, so allowed
  // pairings stay free of string conversions
  const auto track_name = [] (const Track * track) {
    return utils::Utf8String::from_qstring (track->name ());
  };

  // Cannot route a track to itself
  if (source->get_uuid () == destination->get_uuid ())
    return utils::Utf8String::from_utf8_encoded_string (
      fmt::format ("'{}' cannot be routed to itself", track_name (source)));

  // Master track cannot be routed to anything (it routes internally to
  // device output)
  if (source->is_master ())
    return utils::Utf8String::from_utf8_encoded_string (
      fmt::format (
        "'{}' is the master track and routes to the audio device",
        track_name (source)));

  // Destination must be able to be a group target
  if (!destination->can_be_group_target ())
    return utils::Utf8String::from_utf8_encoded_string (
      fmt::format ("'{}' cannot be a routing target", track_name (destination)));

  // Check for circular routes - follow destination's output chain
  // to see if it eventually leads back to source
  auto current_uuid = destination->get_uuid ();
  while (true)
    {
      auto output_it = track_routes_.find (current_uuid);
      if (output_it == track_routes_.end ())
        break; // No more routes in chain

      if (output_it->second == source->get_uuid ())
        return utils::Utf8String::from_utf8_encoded_string (
          fmt::format (
            "routing '{}' to '{}' would create a cycle", track_name (source),
            track_name (destination)));

      current_uuid = output_it->second;
    }

  // Signal types must be compatible
  const auto source_out_type = source->output_signal_type ();
  const auto dest_in_type = destination->input_signal_type ();

  if (
    !source_out_type.has_value () || !dest_in_type.has_value ()
    || source_out_type.value () != dest_in_type.value ())
    {
      const auto type_name = [] (const auto &type) {
        return type
          .transform ([] (auto t) {
            return std::string (magic_enum::enum_name (t));
          })
          .value_or ("no signal");
      };
      return utils::Utf8String::from_utf8_encoded_string (
        fmt::format (
          "'{}' outputs {} but '{}' takes {}", track_name (source),
          type_name (source_out_type), track_name (destination),
          type_name (dest_in_type)));
    }

  return std::nullopt;
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
std::vector<TrackUuid>
TrackRouting::remove_routes_for_destination (const TrackUuid &destination)
{
  std::vector<TrackUuid> removed_sources;
  std::erase_if (track_routes_, [&destination, &removed_sources] (const auto &kv) {
    if (kv.second == destination)
      {
        removed_sources.push_back (kv.first);
        return true;
      }
    return false;
  });
  if (!removed_sources.empty ())
    {
      Q_EMIT routingChanged ();
    }
  return removed_sources;
}
std::optional<TrackUuidReference>
TrackRouting::get_output_track (const TrackUuid &source) const
{
  auto it = track_routes_.find (source);
  if (it != track_routes_.end ())
    {
      return TrackUuidReference{ it->second, registry_ };
    }

  return std::nullopt;
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
