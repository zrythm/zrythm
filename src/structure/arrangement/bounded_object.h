// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/atomic_position_qml_adapter.h"
#include "utils/icloneable.h"

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

  // Convenience method
  Q_INVOKABLE void setLengthTicks (double ticks)
  {
    length ()->setTicks (ticks);
  }

  // ========================================================================

  units::sample_t get_end_position_samples (bool end_position_inclusive) const;

  /**
   * @brief Returns whether the object is hit by the given position (local
   * position if non-timeline object).
   *
   * @param frames Local position if non-timeline object.
   * @param object_end_pos_inclusive Whether @ref end_pos_ is considered as part
   * of the object. This is probably always false.
   */
  [[nodicard]] bool
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
