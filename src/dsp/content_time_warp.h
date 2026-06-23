// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <span>
#include <vector>

#include "dsp/atomic_position_qml_adapter.h"
#include "dsp/tempo_map_qml_adapter.h"
#include "utils/units.h"

#include <QObject>
#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::dsp
{

/**
 * @brief Maintains the mapping between clip-native content ticks and project
 * timeline ticks.
 *
 * Warp points store timeline DELTA ticks (offset from region start), not
 * absolute positions. content_to_timeline_ticks() adds region_position.ticks()
 * to produce absolute positions.
 *
 * Empty warp point list means identity delta (content ticks map 1:1 to
 * timeline delta ticks). This is the default for MIDI regions and audio
 * regions in musical mode ON.
 *
 * content_to_timeline_ticks() and content_to_timeline_samples() return ABSOLUTE
 * timeline positions.
 *
 * Main-thread only — not real-time safe.
 */
class ContentTimeWarp : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  struct WarpPoint
  {
    units::precise_tick_t content_ticks;
    units::precise_tick_t timeline_delta_ticks;
  };

  ContentTimeWarp (
    const TempoMapWrapper           &tempo_map_wrapper,
    const AtomicPositionQmlAdapter * region_position,
    const AtomicPositionQmlAdapter * region_length,
    QObject *                        parent = nullptr);
  ~ContentTimeWarp () override = default;

  std::span<const WarpPoint> warpPoints () const { return warp_points_; }

  /**
   * @brief Convert content ticks to absolute timeline ticks.
   *
   * Uniformly computes region_position.ticks() + warp_lookup(warp_points_,
   * content_ticks). No branching: empty warp points -> identity delta.
   * Null-safe: returns content_ticks if region_position is null.
   *
   * @note Main-thread only. Traverses @ref warp_points_, which is mutated by
   * @c rebuild() on the main thread — not real-time safe, no synchronization.
   */
  units::precise_tick_t
  content_to_timeline_ticks (units::precise_tick_t content_ticks) const;

  /**
   * @brief Convert content ticks to absolute timeline samples.
   *
   * @note Main-thread only (see @ref content_to_timeline_ticks).
   */
  units::sample_t
  content_to_timeline_samples (units::precise_tick_t content_ticks) const;

  /// Configures Project mode (identity warp — clip follows project tempo).
  /// When @p source_bpm > 0, identity warp points are generated so that
  /// @ref to_time_warp_map can derive the sample-space stretch mapping.
  /// When @p source_bpm == 0 (e.g. unknown BPM or MIDI regions), the warp
  /// point list stays empty — positioning works but no rendering is possible.
  void configure_as_project (double source_bpm = 0.0);
  void configure_as_source (double source_bpm);
  void configure_as_warped (
    double                     source_bpm,
    std::span<const WarpPoint> user_markers);

  /**
   * @brief Returns true if the warp mapping is 1:1 (no timestretch needed).
   *
   * Empty warp points (Project mode) or all-delta-equals-content points
   * mean the clip plays at native speed.
   */
  bool is_identity () const;

Q_SIGNALS:
  void mapChanged ();

private:
  void rebuild ();
  void connect_for_mode ();
  void disconnect_all ();

  enum class Mode
  {
    Project,
    Source,
    Warped
  };

  const TempoMapWrapper           &tempo_map_wrapper_;
  const AtomicPositionQmlAdapter * region_position_;
  const AtomicPositionQmlAdapter * region_length_;
  double                           source_bpm_ = 0.0;
  Mode                             mode_ = Mode::Project;
  std::vector<WarpPoint>           user_markers_;
  std::vector<WarpPoint>           warp_points_;

  QMetaObject::Connection tempo_conn_;
  QMetaObject::Connection pos_conn_;
  QMetaObject::Connection len_conn_;
};

/**
 * @brief Piecewise-linear lookup on a span of warp points.
 *
 * Empty span returns content_ticks unchanged (identity).
 * Values before the first warp point clamp to the first point's delta.
 * Extrapolation beyond the last warp point uses the last segment's slope.
 */
units::precise_tick_t
warp_lookup (
  std::span<const ContentTimeWarp::WarpPoint> warp_points,
  units::precise_tick_t                       content_ticks);

} // namespace zrythm::dsp
