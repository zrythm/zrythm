// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/tracks/track_all.h"
#include "structure/tracks/track_routing.h"

namespace zrythm::structure::tracks
{
QVariant
TrackRouting::getOutputTrack (const TrackUuid &source) const
{
  auto output = get_output_track (source);
  return output ? QVariant::fromStdVariant (output.value ().get_object ())
                : QVariant{};
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

} // namespace zrythm::structure::tracks
