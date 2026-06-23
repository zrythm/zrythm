// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cmath>

#include "structure/arrangement/loopable_object.h"

#include <au/math.hh>
#include <nlohmann/json.hpp>

namespace zrythm::structure::arrangement
{
ArrangerObjectLoopRange::ArrangerObjectLoopRange (
  const ArrangerObjectBounds &bounds,
  const dsp::TempoMapWrapper &tempo_map_wrapper,
  QObject *                   parent)
    : QObject (parent), bounds_ (bounds),
      clip_start_pos_ (bounds.length ()->position ().time_conversion_functions ()),
      clip_start_pos_adapter_ (
        utils::make_qobject_unique<dsp::AtomicPositionQmlAdapter> (
          clip_start_pos_,
          tempo_map_wrapper,
          // Clip start must be before loop end and non-negative
          [this] (units::precise_tick_t ticks) {
            ticks = std::min (
              ticks, loop_end_pos_.get_ticks () - units::ticks (-1.0));
            ticks = std::max (ticks, units::ticks (0.0));
            return ticks;
          },
          this)),
      loop_start_pos_ (bounds.length ()->position ().time_conversion_functions ()),
      loop_start_pos_adapter_ (
        utils::make_qobject_unique<dsp::AtomicPositionQmlAdapter> (
          loop_start_pos_,
          tempo_map_wrapper,
          // Loop start must be before loop end and non-negative
          [this] (units::precise_tick_t ticks) {
            ticks = std::max (ticks, units::ticks (0.0));
            ticks =
              std::min (ticks, loop_end_pos_.get_ticks () - units::ticks (1.0));
            return ticks;
          },
          this)),
      loop_end_pos_ (bounds.length ()->position ().time_conversion_functions ()),
      loop_end_pos_adapter_ (
        utils::make_qobject_unique<dsp::AtomicPositionQmlAdapter> (
          loop_end_pos_,
          tempo_map_wrapper,
          // Loop end must be after loop start and clip start and non-negative
          [this] (units::precise_tick_t ticks) {
            ticks = std::max (
              ticks, loop_start_pos_.get_ticks () + units::ticks (1.0));
            ticks = std::max (
              ticks, clip_start_pos_.get_ticks () + units::ticks (1.0));
            ticks = std::max (ticks, units::ticks (0.0));
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
  const auto full_size = length ()->position ().get_ticks ();
  const auto loop_start =
    loopStartPosition ()->position ().get_ticks ()
    - clipStartPosition ()->position ().get_ticks ();
  const auto loop_size = get_loop_length_in_ticks ();
  if (loop_size.in (units::ticks) <= 0.0) [[unlikely]]
    return 0;
  const auto span = (full_size - loop_start).in (units::ticks);
  const auto loop_size_d = loop_size.in (units::ticks);
  const auto full_loops = static_cast<int> (std::floor (span / loop_size_d));
  constexpr double epsilon = 1e-6;
  const bool       add_one =
    count_incomplete && std::fmod (span, loop_size_d) > epsilon;
  return full_loops + (add_one ? 1 : 0);
}

bool
ArrangerObjectLoopRange::looped () const
{
  // Tick-space equality with a tolerance that absorbs floating-point
  // residue from conversions (qFuzzyCompare is unreliable near zero, and
  // length is guaranteed >= 1 tick by the bounds constraint).
  constexpr auto epsilon = units::ticks (1e-6);
  const auto     len = length ()->position ().get_ticks ();
  const auto     loop_end = loopEndPosition ()->position ().get_ticks ();
  const auto     loop_start = loopStartPosition ()->position ().get_ticks ();
  const auto     clip_start = clipStartPosition ()->position ().get_ticks ();
  return loop_start > units::ticks (0.0) || clip_start > units::ticks (0.0)
         || abs (len - loop_end) > epsilon;
}

void
to_json (nlohmann::json &j, const ArrangerObjectLoopRange &object)
{
  using T = ArrangerObjectLoopRange;
  j[T::kClipStartPosKey] = object.clip_start_pos_;
  j[T::kLoopStartPosKey] = object.loop_start_pos_;
  j[T::kLoopEndPosKey] = object.loop_end_pos_;
  j[T::kTrackBoundsKey] = object.track_bounds_;
}

void
from_json (const nlohmann::json &j, ArrangerObjectLoopRange &object)
{
  using T = ArrangerObjectLoopRange;
  j.at (T::kClipStartPosKey).get_to (object.clip_start_pos_);
  j.at (T::kLoopStartPosKey).get_to (object.loop_start_pos_);
  j.at (T::kLoopEndPosKey).get_to (object.loop_end_pos_);
  j.at (T::kTrackBoundsKey).get_to (object.track_bounds_);
  Q_EMIT object.trackBoundsChanged (object.track_bounds_);
}
}
