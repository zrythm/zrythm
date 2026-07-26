// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <optional>
#include <string_view>

#include "dsp/content_time_warp.h"
#include "dsp/position.h"
#include "dsp/tempo_map_qml_adapter.h"
#include "dsp/timebase.h"
#include "structure/arrangement/arranger_object.h"
#include "utils/icloneable.h"

#include <boost/describe/class.hpp>
#include <nlohmann/json_fwd.hpp>

using std::literals::string_view_literals::operator""sv;

namespace zrythm::structure::arrangement
{

/**
 * @brief Intermediate base class for all clip types.
 *
 * A Clip is a segment of the timeline with its own content coordinate
 * system. It owns the members that only exist on clips: ContentTimeWarp,
 * TimebaseProvider, and loop range positions. These were formerly split
 * across ArrangerObjectBounds and ArrangerObjectLoopRange, which have
 * been deleted.
 *
 * Common members (position, length, name, mute, color) remain on
 * ArrangerObject via feature flags.
 *
 * ### Coordinate spaces
 *
 * Clip content — child objects (notes, chords, automation points) and the
 * clip start/loop positions — lives in "unwound content space": content
 * ticks relative to the clip's data origin, ignoring looping. Clip
 * editors display and edit children in this space. Playback and rendering
 * expand loops in the content domain (see for_each_loop_segment()) and
 * map unwound content positions to the timeline through the
 * ContentTimeWarp; content_position_at_timeline() performs the inverse
 * mapping (timeline position to played content position).
 */
class Clip : public ArrangerObject
{
  Q_OBJECT
  Q_PROPERTY (
    zrythm::dsp::ContentTimeWarp * contentWarp READ contentWarp CONSTANT)
  Q_PROPERTY (
    zrythm::dsp::timebase::TimebaseProvider * timebaseProvider READ
      timebaseProvider CONSTANT)
  Q_PROPERTY (
    double timelineLengthTicks READ timelineLengthTicks NOTIFY
      timelineLengthTicksChanged)

  // Loop range properties (merged from ArrangerObjectLoopRange)
  Q_PROPERTY (
    zrythm::dsp::ContentPosition * clipStartPosition READ clipStartPosition
      CONSTANT)
  Q_PROPERTY (
    zrythm::dsp::ContentPosition * loopStartPosition READ loopStartPosition
      CONSTANT)
  Q_PROPERTY (
    zrythm::dsp::ContentPosition * loopEndPosition READ loopEndPosition CONSTANT)
  Q_PROPERTY (
    bool trackBounds READ trackBounds WRITE setTrackBounds NOTIFY
      trackBoundsChanged)
  Q_PROPERTY (bool looped READ looped NOTIFY loopedChanged)
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  ~Clip () noexcept override;
  Q_DISABLE_COPY_MOVE (Clip)

  dsp::TimelinePosition * position () const override
  {
    return qobject_cast<dsp::TimelinePosition *> (ArrangerObject::position ());
  }

  // ========================================================================
  // QML Interface
  // ========================================================================

  dsp::ContentTimeWarp * contentWarp () const { return content_warp_.get (); }

  dsp::TimebaseProvider * timebaseProvider () const
  {
    return timebase_provider_.get ();
  }

  // Convenience method
  Q_INVOKABLE void setLengthTicks (double ticks)
  {
    length ()->setTicks (ticks);
  }

  /**
   * @brief Timeline delta-tick positions of each loop wrap point.
   *
   * Returns positions relative to the clip start, suitable for placing
   * loop markers in a clip view.
   *
   * @param displayEndTicks Upper bound (exclusive) for returned positions,
   *                        typically max(timelineLengthTicks, display extent).
   */
  Q_INVOKABLE QList<double>
              loopPointTimelineTicks (double displayEndTicks) const;

  /**
   * @brief Warp-aware length of this clip in timeline ticks.
   *
   * Converts the content-space length through ContentTimeWarp, so the
   * result may differ from @c length()->ticks() when there are tempo
   * changes inside the clip.
   */
  double        timelineLengthTicks () const;
  Q_SIGNAL void timelineLengthTicksChanged ();

  /**
   * @brief Returns the content position this clip plays at the given timeline
   * position.
   *
   * Accounts for the clip start and looping in the unwound content domain
   * (the same convention as playback): the unwound content position is
   * offset by the clip start, then wrapped into the loop range.
   *
   * @pre @p timeline_pos is within the clip's timeline span
   * ([position, end)), up to a 1-sample tolerance on either side (positions
   * within the tolerance are clamped; see the implementation).
   */
  dsp::ContentTick
  content_position_at_timeline (dsp::TimelineTick timeline_pos) const;

  /**
   * @brief Returns the end position of this clip in timeline samples.
   *
   * @param end_position_inclusive If true, returns the exact end sample; if
   * false, returns one sample before (for half-open interval queries).
   */
  units::sample_t get_end_position_samples (bool end_position_inclusive) const;

  /**
   * @brief Returns the duration of this clip in timeline samples (end minus
   * start), warp-aware.
   */
  units::sample_t get_sample_duration () const;

  /**
   * @brief Returns whether the clip is hit by the given timeline position.
   */
  [[nodiscard]] bool
  is_hit (const units::sample_t frames, bool object_end_pos_inclusive = false)
    const;

  /**
   * @brief Whether the clip is hit by the given range.
   */
  bool is_hit_by_range (
    std::pair<units::sample_t, units::sample_t> global_frames,
    bool                                        range_start_inclusive = true,
    bool                                        range_end_inclusive = true,
    bool object_end_pos_inclusive = false) const;

  // ========================================================================
  // Loop Range (merged from ArrangerObjectLoopRange)
  // ========================================================================

  dsp::ContentPosition * clipStartPosition () const
  {
    return clip_start_pos_.get ();
  }
  dsp::ContentPosition * loopStartPosition () const
  {
    return loop_start_pos_.get ();
  }
  dsp::ContentPosition * loopEndPosition () const
  {
    return loop_end_pos_.get ();
  }

  bool          trackBounds () const { return track_bounds_; }
  void          setTrackBounds (bool track);
  Q_SIGNAL void trackBoundsChanged (bool track);

  /**
   * @brief Set the three loop positions atomically, bypassing the per-position
   * clamping constraints.
   *
   * Each position's @ref dsp::Position::setTicks constraint normally clamps
   * against the *current* sibling values, which corrupts values when setting
   * an interdependent triplet sequentially (e.g. expanding @p loop_start beyond
   * the current @p loop_end). This method writes all three without per-set
   * clamping, so an interdependent target is applied as-is.
   *
   * Inputs are clamped holistically to the loop invariants
   * (@p loop_end >= max(@p clip_start, @p loop_start) + 1, all >= 0);
   * already-valid triplets are applied unchanged.
   *
   * If length-tracking (@ref trackBounds) is on, it is disabled for the
   * duration of the set so the explicit values are not overwritten by the
   * length-tracking connection. Tracking is left disabled; callers that want
   * it re-engaged (e.g. when the result is the default range) must call
   * @ref setTrackBounds themselves.
   */
  void set_loop_range (
    dsp::ContentTick clip_start,
    dsp::ContentTick loop_start,
    dsp::ContentTick loop_end);

  bool          looped () const;
  Q_SIGNAL void loopedChanged ();

  /**
   * @brief Emitted when the clip's content changes.
   *
   * Each concrete Clip subclass forwards its ArrangerObjectListModel's
   * contentChanged signal to this signal in its constructor, so consumers
   * (e.g. canvas items) can connect uniformly without knowing the specific
   * clip type or its child models.
   */
  Q_SIGNAL void contentChanged ();

  /**
   * @brief Emitted when any loop-related property changes.
   *
   * Fired in addition to the more specific signals (@ref loopedChanged,
   * @ref trackBoundsChanged) so consumers can trigger a single recalculation.
   */
  Q_SIGNAL void loopablePropertiesChanged ();

  /**
   * Returns the number of loops, optionally including incomplete ones.
   */
  int get_num_loops (bool count_incomplete) const;

  /**
   * Returns the length of the loop in ticks.
   */
  dsp::ContentTick get_loop_length_in_ticks () const
  {
    return max (
      dsp::ContentTick (units::ticks (0.0)),
      loop_end_pos_->asTick () - loop_start_pos_->asTick ());
  }

  /**
   * @brief Shifts all child objects by @p delta content ticks.
   */
  virtual void shift_all_children (dsp::ContentTick delta) = 0;

  /**
   * @brief Returns the position of the first child, if any.
   *
   * Used by undo commands to capture/restore child positions.
   */
  virtual std::optional<dsp::ContentTick> first_child_position () const = 0;

  // ========================================================================

protected:
  Clip (
    Type                        type,
    const dsp::TempoMapWrapper &tempo_map_wrapper,
    QObject *                   parent = nullptr) noexcept;

  friend void
  init_from (Clip &obj, const Clip &other, utils::ObjectCloneType clone_type);
  friend void to_json (nlohmann::json &j, const Clip &clip);
  friend void from_json (const nlohmann::json &j, Clip &clip);

private:
  static constexpr auto kTimebaseKey = "timebase"sv;
  static constexpr auto kClipStartPosKey = "clipStartPosition"sv;
  static constexpr auto kLoopStartPosKey = "loopStartPosition"sv;
  static constexpr auto kLoopEndPosKey = "loopEndPosition"sv;
  static constexpr auto kTrackBoundsKey = "trackBounds"sv;

private:
  utils::QObjectUniquePtr<dsp::ContentTimeWarp>  content_warp_;
  utils::QObjectUniquePtr<dsp::TimebaseProvider> timebase_provider_;

  // Loop range members (merged from ArrangerObjectLoopRange)

  /// Clip start position relative to the clip's start.
  utils::QObjectUniquePtr<dsp::ContentPosition> clip_start_pos_;

  /// Loop start position relative to the clip's start.
  utils::QObjectUniquePtr<dsp::ContentPosition> loop_start_pos_;

  /// Loop end position relative to the clip's start.
  utils::QObjectUniquePtr<dsp::ContentPosition> loop_end_pos_;

  /**
   * @brief Whether loop positions track the clip length.
   *
   * If true:
   * - Clip start position will be set to 0
   * - Loop start position will be set to 0
   * - Loop end position will be set to the clip's length
   */
  bool                                   track_bounds_{ true };
  std::optional<QMetaObject::Connection> track_bounds_connection_;

  BOOST_DESCRIBE_CLASS (Clip, (ArrangerObject), (), (), ())
};

} // namespace zrythm::structure::arrangement
