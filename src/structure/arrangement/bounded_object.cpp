// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/bounded_object.h"

namespace zrythm::structure::arrangement
{
ArrangerObjectBounds::ArrangerObjectBounds (
  const dsp::AtomicPositionQmlAdapter &start_position,
  QObject *                            parent)
    : QObject (parent), position_ (start_position),
      length_ (start_position.position ().time_conversion_functions ()),
      length_adapter_ (
        utils::make_qobject_unique<dsp::AtomicPositionQmlAdapter> (
          length_,
          // Length must be at least 1 tick
          [] (units::precise_tick_t ticks) {
            return std::max (ticks, units::ticks (1.0));
          },
          this))
{
}

units::sample_t
ArrangerObjectBounds::get_end_position_samples (bool end_position_inclusive) const
{
  return au::round_as<int64_t> (
           units::samples,
           length_.time_conversion_functions ().tick_to_samples (
             units::ticks (position ()->ticks ()) + length_.get_ticks ()))
         + units::samples ((end_position_inclusive ? 0 : -1));
}

bool
ArrangerObjectBounds::is_hit (
  const units::sample_t frames,
  bool                  object_end_pos_inclusive) const
{
  const auto obj_start = units::samples (position ()->samples ());
  const auto obj_end = get_end_position_samples (object_end_pos_inclusive);

  return obj_start <= frames && obj_end >= frames;
}

bool
ArrangerObjectBounds::is_hit_by_range (
  std::pair<units::sample_t, units::sample_t> global_frames,
  bool                                        range_start_inclusive,
  bool                                        range_end_inclusive,
  bool                                        object_end_pos_inclusive) const
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
  const auto range_start =
    range_start_inclusive
      ? global_frames.first
      : global_frames.first + units::samples (1);
  const auto range_end =
    range_end_inclusive
      ? global_frames.second
      : global_frames.second - units::samples (1);
  const auto obj_start = units::samples (position ()->samples ());
  const auto obj_end = get_end_position_samples (object_end_pos_inclusive);

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

void
init_from (
  ArrangerObjectBounds       &obj,
  const ArrangerObjectBounds &other,
  utils::ObjectCloneType      clone_type)
{
  obj.length_.set_ticks (other.length_.get_ticks ());
}
}
