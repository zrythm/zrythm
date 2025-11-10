// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/loopable_object.h"

namespace zrythm::structure::arrangement
{
ArrangerObjectLoopRange::ArrangerObjectLoopRange (
  const ArrangerObjectBounds &bounds,
  QObject *                   parent)
    : QObject (parent), bounds_ (bounds),
      clip_start_pos_ (bounds.length ()->position ().time_conversion_functions ()),
      clip_start_pos_adapter_ (
        utils::make_qobject_unique<dsp::AtomicPositionQmlAdapter> (
          clip_start_pos_,
          // Clip start must be before loop end and non-negative
          [this] (units::precise_tick_t ticks) {
            ticks = std::max (ticks, units::ticks (0.0));
            ticks = std::min (ticks, loop_end_pos_.get_ticks ());
            return ticks;
          },
          this)),
      loop_start_pos_ (bounds.length ()->position ().time_conversion_functions ()),
      loop_start_pos_adapter_ (
        utils::make_qobject_unique<dsp::AtomicPositionQmlAdapter> (
          loop_start_pos_,
          // Loop start must be before loop end and non-negative
          [this] (units::precise_tick_t ticks) {
            ticks = std::max (ticks, units::ticks (0.0));
            ticks = std::min (ticks, loop_end_pos_.get_ticks ());
            return ticks;
          },
          this)),
      loop_end_pos_ (bounds.length ()->position ().time_conversion_functions ()),
      loop_end_pos_adapter_ (
        utils::make_qobject_unique<dsp::AtomicPositionQmlAdapter> (
          loop_end_pos_,
          // Loop end must be after loop start and clip start and non-negative
          [this] (units::precise_tick_t ticks) {
            ticks = std::max (ticks, units::ticks (0.0));
            ticks = std::max (ticks, loop_start_pos_.get_ticks ());
            ticks = std::max (ticks, clip_start_pos_.get_ticks ());
            return ticks;
          },
          this))
{
  QObject::connect (
    this, &ArrangerObjectLoopRange::trackBoundsChanged, this,
    [this] (bool track) {
      if (track && !track_bounds_connection_.has_value ())
        {
          track_bounds_connection_ = QObject::connect (
            bounds_.length (), &dsp::AtomicPositionQmlAdapter::positionChanged,
            this, [this] () {
              // Set all positions to defaults when tracking bounds
              clipStartPosition ()->setTicks (0.0);
              loopStartPosition ()->setTicks (0.0);
              loopEndPosition ()->setTicks (length ()->ticks ());
            });

          // also emit the signal to update all positions since we are
          // now tracking the bounds
          Q_EMIT bounds_.length ()->positionChanged ();
        }
      else if (!track && track_bounds_connection_.has_value ())
        {
          QObject::disconnect (track_bounds_connection_.value ());
          track_bounds_connection_.reset ();
        }
    });

  QObject::connect (
    loopStartPosition (), &dsp::AtomicPositionQmlAdapter::positionChanged, this,
    &ArrangerObjectLoopRange::loopedChanged);
  QObject::connect (
    loopEndPosition (), &dsp::AtomicPositionQmlAdapter::positionChanged, this,
    &ArrangerObjectLoopRange::loopedChanged);
  QObject::connect (
    clipStartPosition (), &dsp::AtomicPositionQmlAdapter::positionChanged, this,
    &ArrangerObjectLoopRange::loopedChanged);
  QObject::connect (
    length (), &dsp::AtomicPositionQmlAdapter::positionChanged, this,
    &ArrangerObjectLoopRange::loopedChanged);

  Q_EMIT trackBoundsChanged (track_bounds_);

  QObject::connect (
    this, &ArrangerObjectLoopRange::trackBoundsChanged, this,
    &ArrangerObjectLoopRange::loopableObjectPropertiesChanged);
  QObject::connect (
    loopStartPosition (), &dsp::AtomicPositionQmlAdapter::positionChanged, this,
    &ArrangerObjectLoopRange::loopableObjectPropertiesChanged);
  QObject::connect (
    loopEndPosition (), &dsp::AtomicPositionQmlAdapter::positionChanged, this,
    &ArrangerObjectLoopRange::loopableObjectPropertiesChanged);
  QObject::connect (
    clipStartPosition (), &dsp::AtomicPositionQmlAdapter::positionChanged, this,
    &ArrangerObjectLoopRange::loopableObjectPropertiesChanged);
  QObject::connect (
    this, &ArrangerObjectLoopRange::loopedChanged, this,
    &ArrangerObjectLoopRange::loopableObjectPropertiesChanged);
}

int
ArrangerObjectLoopRange::get_num_loops (bool count_incomplete) const
{
  const auto full_size = units::samples (length ()->samples ());
  const auto loop_start = units::samples (
    loopStartPosition ()->samples () - clipStartPosition ()->samples ());
  const auto loop_size = get_loop_length_in_frames ();

  // Special case
  if (loop_size == units::samples (0)) [[unlikely]]
    {
      return 0;
    }

  // Calculate the number of full loops using integer division
  const auto full_loops = (full_size - loop_start) / loop_size;

  // Add 1 if we want to count incomplete loops
  const auto add_one =
    (count_incomplete
     && (full_size - loop_start) % loop_size != units::samples (0));
  return static_cast<int> (full_loops) + (add_one ? 1 : 0);
}
}
