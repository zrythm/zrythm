// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <map>
#include <optional>
#include <string_view>

#include "structure/tracks/track_fwd.h"
#include "utils/iobject_registry.h"

#include <QObject>
#include <QPointF>
#include <QtQmlIntegration/qqmlintegration.h>

#include <nlohmann/json_fwd.hpp>

using namespace std::string_view_literals;

namespace zrythm::structure::project
{

/**
 * @brief Persisted layout state of the Modular view.
 *
 * Owns the state saved with a project so the Modular view resumes where it
 * was left:
 * - Node positions, keyed by projection node ID.
 * - Canvas viewports (pan/zoom), one per scope.
 * - The active scope and the selected (Track scope) or linked (Global scope)
 *   track.
 *
 * Entries whose referenced track no longer exists are skipped when the
 * project is saved. Processor entries are always kept.
 */
class ModularViewState : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  /** Pan/zoom state of the Modular view canvas for one scope. */
  struct Viewport
  {
    double pan_x = 0.0;
    double pan_y = 0.0;
    double zoom = 1.0;
  };

  /** Projection scope of the Modular view. */
  enum class Scope
  {
    Track,
    Global,
  };

  using TrackUuid = structure::tracks::TrackUuid;

  ModularViewState (utils::IObjectRegistry &registry, QObject * parent = nullptr);

  /**
   * @brief Stores the canvas position of a projection node.
   *
   * @param node_id Projection node ID: "track:<track uuid>" or
   * "processor:<processor uuid>". Keys with other prefixes are stored as-is;
   * "track:" keys are pruned on save when their track no longer exists.
   * @param position Node position in canvas coordinates.
   */
  void set_node_position (const QString &node_id, const QPointF &position);
  /**
   * @brief Finds the stored canvas position of a projection node.
   *
   * @return The stored position, or nullopt if the node has none.
   */
  std::optional<QPointF> find_node_position (const QString &node_id) const;
  /**
   * @brief Discards the stored position of a projection node.
   */
  void clear_node_position (const QString &node_id);

  /**
   * @brief Sets the canvas viewport stored under the given key.
   *
   * @param key Viewport storage key: "global" or "track:<track uuid>".
   */
  void set_viewport (const QString &key, const Viewport &viewport);
  /**
   * @brief Finds the canvas viewport stored under the given key.
   *
   * @return The stored viewport, or nullopt if none exists for the key.
   */
  std::optional<Viewport> find_viewport (const QString &key) const;

  Scope scope () const;
  void  set_scope (Scope scope);

  std::optional<TrackUuid> selected_track () const;
  void                     set_selected_track (std::optional<TrackUuid> track);

  static constexpr auto kNodePositionsKey = "nodePositions"sv;
  static constexpr auto kViewportsKey = "viewports"sv;
  static constexpr auto kScopeKey = "scope"sv;
  static constexpr auto kSelectedTrackKey = "selectedTrack"sv;

  /** Serialized scope identifiers. The global identifier doubles as the
   * global viewport key; track viewports are keyed "track:<track uuid>". */
  static constexpr auto kScopeTrackString = "track"sv;
  static constexpr auto kScopeGlobalString = "global"sv;

private:
  friend void to_json (nlohmann::json &j, const ModularViewState &state);
  friend void from_json (const nlohmann::json &j, ModularViewState &state);

  /**
   * @brief Whether a "track:"-prefixed key points at a track that no longer
   * exists in the registry.
   *
   * Keys with other prefixes never reference registry tracks and are never
   * stale.
   */
  bool key_references_removed_track (const QString &key) const;

  utils::IObjectRegistry &registry_;

  /** Node positions, keyed by projection node ID. */
  std::map<QString, QPointF> node_positions_;

  /** Canvas viewports, keyed by "global" or "track:<track uuid>". */
  std::map<QString, Viewport> viewports_;

  Scope scope_ = Scope::Track;

  /** Track selected (Track scope) or linked (Global scope) when saved. */
  std::optional<TrackUuid> selected_track_;
};

} // namespace zrythm::structure::project
