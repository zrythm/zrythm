// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <span>
#include <vector>

#include "dsp/position.h"
#include "dsp/tempo_map_qml_adapter.h"
#include "dsp/tick_types.h"
#include "utils/units.h"

#include <QObject>
#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::dsp
{

/**
 * @brief Maintains the mapping between clip-native content ticks and project
 * timeline ticks.
 *
 * Warp points store timeline DELTA ticks (offset from clip start), not
 * absolute positions. contentToTimeline() adds clip_position.ticks()
 * to produce absolute timeline positions.
 *
 * Empty warp point list means identity delta (content ticks map 1:1 to
 * timeline delta ticks). This is the default for MIDI clips and audio
 * clips in musical mode ON.
 *
 * contentToTimeline() and contentToTimelineSamples() return ABSOLUTE timeline
 * positions (measured from the timeline origin at 0, not relative to the
 * clip start). contentToTimelineTicksRelative() returns the warp delta only
 * (timeline ticks relative to the clip start).
 *
 * Main-thread only — not real-time safe.
 *
 * @see Clip for an overview of the content/timeline coordinate spaces and
 * the unwound-content-space convention used by clip editors and playback.
 */
class ContentTimeWarp : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  struct WarpPoint
  {
    ContentTick  content_ticks;
    TimelineTick timeline_delta_ticks;
  };

  ContentTimeWarp (
    const TempoMapWrapper   &tempo_map_wrapper,
    const TimelinePosition * clip_position,
    const ContentPosition *  clip_length,
    QObject *                parent = nullptr);
  ~ContentTimeWarp () override = default;

  std::span<const WarpPoint> warpPoints () const { return warp_points_; }

  /**
   * @brief Convert content ticks to ABSOLUTE timeline ticks.
   *
   * Returns the position measured from the timeline origin (tick 0), NOT
   * relative to the clip's start. Computed as:
   *   clip_position.ticks() + warp_lookup(content_ticks)
   *
   * Null-safe in Project/Warped modes: if clip_position is null, treats the
   * clip start as tick 0. Source mode requires clip_position to be non-null
   * (see @ref source_content_to_timeline).
   *
   * @note Main-thread only. Traverses @ref warp_points_, which is mutated by
   * @c rebuild() on the main thread — not real-time safe, no synchronization.
   */
  TimelineTick contentToTimeline (ContentTick content) const;

  /**
   * @brief Convert content ticks to timeline ticks RELATIVE to the clip's
   * start position.
   *
   * Returns only the warp delta (the stretched/compressed offset from the
   * clip start), without adding the clip's absolute position.
   * Used for positioning items inside a clip view.
   */
  Q_INVOKABLE double contentToTimelineTicksRelative (double contentTicks) const;

  /// Strong-typed overload for C++ callers (see the double version above).
  TimelineTick contentToTimelineTicksRelative (ContentTick content) const;

  /**
   * @brief Convert timeline ticks RELATIVE to the clip's start position back to
   * content ticks.
   *
   * The inverse of @ref contentToTimelineTicksRelative. Given a position
   * expressed as a timeline offset from the clip start (the warp delta),
   * returns the corresponding position within the clip's content. Used to map
   * mouse positions in a clip-editor ruler back onto the clip's content
   * coordinate space.
   *
   * @note Main-thread only (delegates to @ref timelineToContent).
   */
  Q_INVOKABLE double
  timelineTicksRelativeToContent (double timelineTicksRelative) const;

  /// Strong-typed overload for C++ callers (see the double version above).
  ContentTick
  timelineTicksRelativeToContent (TimelineTick timeline_delta) const;

  /**
   * @brief Convert content ticks to ABSOLUTE timeline samples.
   *
   * Returns the sample position measured from the timeline origin (sample 0),
   * NOT relative to the clip's start.
   *
   * @note Main-thread only (see @ref contentToTimeline).
   */
  units::sample_t contentToTimelineSamples (ContentTick content) const;

  /**
   * @brief Convert ABSOLUTE timeline ticks to content ticks.
   *
   * The inverse of @ref contentToTimeline. Given a position on the timeline,
   * returns the corresponding position within the clip's content.
   *
   * @note Main-thread only (see @ref contentToTimeline).
   */
  ContentTick timelineToContent (TimelineTick timeline) const;

  /// Configures Project mode (identity warp — clip follows project tempo).
  /// When @p source_bpm > 0, identity warp points are generated so that
  /// @ref to_time_warp_map can derive the sample-space stretch mapping.
  /// When @p source_bpm == 0 (e.g. unknown BPM or MIDI clips), the warp
  /// point list stays empty — positioning works but no rendering is possible.
  void configure_as_project (units::bpm_t source_bpm = units::bpm (0.0));
  void configure_as_source (units::bpm_t source_bpm);
  void configure_as_warped (
    units::bpm_t               source_bpm,
    std::span<const WarpPoint> user_markers);

  /**
   * @brief Copies the configuration (mode, source BPM and user markers) from
   * @p other.
   *
   * The warp points are rebuilt from the copied configuration against this
   * warp's own clip position/length and tempo map.
   */
  void copy_configuration_from (const ContentTimeWarp &other);

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

  // Source-mode exact conversions (bypass warp points for exact results
  // even beyond the current clip length). Both are pure functions of the
  // tempo map, clip_position_, and source BPM.
  //
  // @pre clip_position_ != nullptr — Source mode requires a known clip
  //      start position (asserted).
  TimelineTick source_content_to_timeline (ContentTick content) const;
  ContentTick  source_timeline_to_content (TimelineTick timeline) const;

  enum class Mode
  {
    Project,
    Source,
    Warped
  };

  const TempoMapWrapper   &tempo_map_wrapper_;
  const TimelinePosition * clip_position_;
  const ContentPosition *  clip_length_;
  units::bpm_t             source_bpm_ = units::bpm (0.0);
  Mode                     mode_ = Mode::Project;
  std::vector<WarpPoint>   user_markers_;
  std::vector<WarpPoint>   warp_points_;

  QMetaObject::Connection tempo_conn_;
  QMetaObject::Connection pos_conn_;
  QMetaObject::Connection len_conn_;
};

/**
 * @brief Piecewise-linear lookup on a span of warp points.
 *
 * Empty span returns content_ticks unchanged (identity).
 * Extrapolation before the first / beyond the last warp point uses the
 * nearest segment's slope.
 */
TimelineTick
warp_lookup (
  std::span<const ContentTimeWarp::WarpPoint> warp_points,
  ContentTick                                 content_ticks);

/**
 * @brief Reverse piecewise-linear lookup: timeline delta → content ticks.
 *
 * The inverse of @ref warp_lookup. Same extrapolation rules.
 */
ContentTick
reverse_warp_lookup (
  std::span<const ContentTimeWarp::WarpPoint> warp_points,
  TimelineTick                                timeline_delta_ticks);

} // namespace zrythm::dsp
