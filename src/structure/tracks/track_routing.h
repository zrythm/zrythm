// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/tracks/track.h"

namespace zrythm::structure::tracks
{
/**
 * @brief Management of track-to-track connections.
 */
class TrackRouting : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")
public:
  TrackRouting (TrackRegistry &track_registry, QObject * parent = nullptr);

  // ========================================================================
  // QML Interface
  // ========================================================================

  Q_INVOKABLE QVariant getOutputTrack (const Track * source) const;

  Q_INVOKABLE void
  setOutputTrack (const Track * source, const Track * destination);

  /**
   * @brief Emitted when a change was made in the routing.
   */
  Q_SIGNAL void routingChanged ();

  // ========================================================================

  void
  add_or_replace_route (const TrackUuid &source, const TrackUuid &destination);
  void remove_route_for_source (const TrackUuid &source);
  void remove_routes_for_destination (const TrackUuid &destination);

  std::optional<TrackUuidReference>
  get_output_track (const TrackUuid &source) const;

private:
  static constexpr auto kTrackRoutesKey = "trackRoutes"sv;
  friend void           to_json (nlohmann::json &j, const TrackRouting &t);
  friend void           from_json (const nlohmann::json &j, TrackRouting &t);

private:
  TrackRegistry &track_registry_;

  /**
   * @brief Track routes (source-destination mappings).
   */
  std::unordered_map<TrackUuid, TrackUuid> track_routes_;
};
} // namespace zrythm::structure::tracks
