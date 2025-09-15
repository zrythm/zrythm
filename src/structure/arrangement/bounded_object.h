// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/atomic_position_qml_adapter.h"

namespace zrythm::structure::arrangement
{

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
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  ArrangerObjectBounds (
    const dsp::AtomicPositionQmlAdapter &start_position,
    QObject *                            parent = nullptr);
  ~ArrangerObjectBounds () override = default;
  Z_DISABLE_COPY_MOVE (ArrangerObjectBounds)

  // ========================================================================
  // QML Interface
  // ========================================================================

  dsp::AtomicPositionQmlAdapter * length () const
  {
    return length_adapter_.get ();
  }

  /**
   * @brief Helper to set the length based on an end position
   */
  Q_INVOKABLE void setEndPositionTicks (double ticks);

  // Convenience method
  Q_INVOKABLE void setLengthTicks (double ticks)
  {
    length ()->setTicks (ticks);
  }

  // ========================================================================

  auto get_end_position_samples (bool end_position_inclusive) const
  {
    return static_cast<signed_frame_t> (
             std::round (length_.get_tempo_map ().tick_to_samples (
               position ()->ticks () + length_.get_ticks ())))
           + (end_position_inclusive ? 0 : -1);
  }

  /**
   * @brief Returns whether the object is hit by the given position (local
   * position if non-timeline object).
   *
   * @param frames Local position if non-timeline object.
   * @param object_end_pos_inclusive Whether @ref end_pos_ is considered as part
   * of the object. This is probably always false.
   */
  [[nodicard]] bool
  is_hit (const signed_frame_t frames, bool object_end_pos_inclusive = false) const
  {
    const signed_frame_t obj_start = position ()->samples ();
    const signed_frame_t obj_end =
      get_end_position_samples (object_end_pos_inclusive);

    return obj_start <= frames && obj_end >= frames;
  }

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
    std::pair<signed_frame_t, signed_frame_t> global_frames,
    bool                                      range_start_inclusive = true,
    bool                                      range_end_inclusive = true,
    bool object_end_pos_inclusive = false) const
  {
    /*
     * Case 1: Object start is inside range
     *        |----- Range -----|
     *                |-- Object --|
     *          |-- Object --|           // also covers whole object inside
     * range
     *
     * Case 2: Object end is inside range
     *        |----- Range -----|
     *     |-- Object --|
     *
     * Case 3: Range start is inside object
     *       |-- Object --|
     *          |---- Range ----|
     *        |- Range -|              // also covers whole range inside object
     *
     * Case 4: Range end is inside object
     *            |-- Object --|
     *       |---- Range ----|
     */

    /* get adjusted values */
    const signed_frame_t range_start =
      range_start_inclusive ? global_frames.first : global_frames.first + 1;
    const signed_frame_t range_end =
      range_end_inclusive ? global_frames.second : global_frames.second - 1;
    const signed_frame_t obj_start = position ()->samples ();
    const signed_frame_t obj_end =
      get_end_position_samples (object_end_pos_inclusive);

    // invalid range
    if (range_start > range_end)
      return false;

    /* 1. object start is inside range */
    if (obj_start >= range_start && obj_start <= range_end)
      return true;

    /* 2. object end is inside range */
    if (obj_end >= range_start && obj_end <= range_end)
      return true;

    /* 3. range start is inside object */
    if (range_start >= obj_start && range_start <= obj_end)
      return true;

    /* 4. range end is inside object */
    return (range_end >= obj_start && range_end <= obj_end);
  }

private:
  friend void init_from (
    ArrangerObjectBounds       &obj,
    const ArrangerObjectBounds &other,
    utils::ObjectCloneType      clone_type);

  static constexpr auto kLengthKey = "length"sv;
  friend auto to_json (nlohmann::json &j, const ArrangerObjectBounds &object)
  {
    j[kLengthKey] = object.length_;
  }
  friend auto from_json (const nlohmann::json &j, ArrangerObjectBounds &object)
  {
    j.at (kLengthKey).get_to (object.length_);
  }

  auto position () const -> const dsp::AtomicPositionQmlAdapter *
  {
    return std::addressof (position_);
  }

private:
  const dsp::AtomicPositionQmlAdapter &position_;

  /// Length of the object.
  dsp::AtomicPosition                                    length_;
  utils::QObjectUniquePtr<dsp::AtomicPositionQmlAdapter> length_adapter_;

  BOOST_DESCRIBE_CLASS (ArrangerObjectBounds, (), (), (), (length_))
};

} // namespace zrythm::structure::arrangement
