// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <string_view>

#include "dsp/atomic_position_qml_adapter.h"
#include "dsp/content_time_warp.h"
#include "utils/icloneable.h"

#include <QPointer>

#include <boost/describe/class.hpp>
#include <nlohmann/json_fwd.hpp>

using std::literals::string_view_literals::operator""sv;

namespace zrythm::structure::arrangement
{

using namespace std::string_view_literals;

/**
 * @brief Adds length functionality to arranger objects.
 *
 * It provides common functionality and properties shared by objects with length,
 * and methods to resize and check if the object is hit by a position or range.
 *
 * ArrangerObjectBounds depends on the start position of an ArrangerObject.
 */
class ArrangerObjectBounds : public QObject
{
  Q_OBJECT
  Q_PROPERTY (
    zrythm::dsp::AtomicPositionQmlAdapter * length READ length CONSTANT)
  Q_PROPERTY (
    double timelineLengthTicks READ timelineLengthTicks NOTIFY
      timelineLengthTicksChanged)
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  ArrangerObjectBounds (
    const dsp::AtomicPositionQmlAdapter &start_position,
    const dsp::TempoMapWrapper          &tempo_map_wrapper,
    QObject *                            parent = nullptr);
  ~ArrangerObjectBounds () override = default;
  Q_DISABLE_COPY_MOVE (ArrangerObjectBounds)

  // ========================================================================
  // QML Interface
  // ========================================================================

  dsp::AtomicPositionQmlAdapter * length () const
  {
    return length_adapter_.get ();
  }

  const dsp::AtomicPositionQmlAdapter * position () const
  {
    return std::addressof (position_);
  }

  // Convenience method
  Q_INVOKABLE void setLengthTicks (double ticks)
  {
    length ()->setTicks (ticks);
  }

  double        timelineLengthTicks () const;
  Q_SIGNAL void timelineLengthTicksChanged ();

  // ========================================================================

  /**
   * @brief Sets the warp used for timeline position queries.
   *
   * ArrangerObjectBounds and ContentTimeWarp have a genuine circular
   * dependency: ContentTimeWarp needs this object's length adapter (to listen
   * for length changes and drive @c rebuild()), while this object needs
   * ContentTimeWarp (to convert content ticks to timeline samples/ticks in
   * @c get_end_position_samples() and @c timelineLengthTicks()).
   *
   * Neither can be fully constructed before the other, so the composition root
   * (ArrangerObject) creates both, then calls this method to inject the warp.
   * @c nullptr is valid — non-region objects (e.g. MidiNote) have no warp and
   * fall back to direct @c tick_to_samples conversion.
   */
  void set_content_warp (dsp::ContentTimeWarp * warp) { content_warp_ = warp; }

  /**
   * @brief Returns the end position of this object in timeline samples.
   *
   * Uses ContentTimeWarp when available (regions), otherwise falls back to
   * direct tick-to-samples conversion (MidiNote, etc.).
   *
   * @note Main-thread only. Reads ContentTimeWarp's warp points which are
   * mutated by @c rebuild() on the main thread. All current callers are
   * offline/cache-generation paths (e.g. @c serialize_to_buffer,
   * waveform canvas, timeline data provider).
   *
   * @param end_position_inclusive If true, returns the exact end sample; if
   * false, returns one sample before (for half-open interval queries).
   */
  units::sample_t get_end_position_samples (bool end_position_inclusive) const;

  /**
   * @brief Returns whether the object is hit by the given position (local
   * position if non-timeline object).
   *
   * @param frames Local position if non-timeline object.
   * @param object_end_pos_inclusive Whether @ref end_pos_ is considered as part
   * of the object. This is probably always false.
   */
  [[nodiscard]] bool
  is_hit (const units::sample_t frames, bool object_end_pos_inclusive = false)
    const;

  /**
   * @brief Whether the object is hit by the given range.
   *
   * @param global_frames Start and end positions of the range, in samples.
   * @param range_start_inclusive Whether the start of the range is considered
   * as part of the range.
   * @param range_end_inclusive  Whether the end of the range is considered as
   * part of the range.
   * @param object_end_pos_inclusive  Whether the end position of the object is
   * considered as part of the object (this is probably always false).
   */
  bool is_hit_by_range (
    std::pair<units::sample_t, units::sample_t> global_frames,
    bool                                        range_start_inclusive = true,
    bool                                        range_end_inclusive = true,
    bool object_end_pos_inclusive = false) const;

private:
  friend void init_from (
    ArrangerObjectBounds       &obj,
    const ArrangerObjectBounds &other,
    utils::ObjectCloneType      clone_type);

  static constexpr auto kLengthKey = "length"sv;
  friend void to_json (nlohmann::json &j, const ArrangerObjectBounds &object);
  friend void from_json (const nlohmann::json &j, ArrangerObjectBounds &object);

private:
  const dsp::AtomicPositionQmlAdapter &position_;

  /// Length of the object.
  dsp::AtomicPosition                                    length_;
  utils::QObjectUniquePtr<dsp::AtomicPositionQmlAdapter> length_adapter_;

  /// Warp for content→timeline conversion. Set by ArrangerObject after both
  /// are constructed (circular dependency — see @ref set_content_warp).
  /// Nullptr for non-region objects (MidiNote etc.).
  QPointer<dsp::ContentTimeWarp> content_warp_;

  BOOST_DESCRIBE_CLASS (ArrangerObjectBounds, (), (), (), (length_))
};

} // namespace zrythm::structure::arrangement
