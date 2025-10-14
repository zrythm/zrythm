// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/atomic_position_qml_adapter.h"

namespace zrythm::dsp
{
AtomicPositionQmlAdapter::AtomicPositionQmlAdapter (
  AtomicPosition                   &atomicPos,
  std::optional<ConstraintFunction> constraints,
  QObject *                         parent)
    : QObject (parent), atomic_pos_ (atomicPos),
      constraints_ (std::move (constraints))
{
}

double
AtomicPositionQmlAdapter::ticks () const
{
  return atomic_pos_.get_ticks ().in (units::ticks);
}

void
AtomicPositionQmlAdapter::setTicks (double ticks)
{
  auto tick_value = units::ticks (ticks);
  if (constraints_)
    {
      tick_value = (*constraints_) (tick_value);
    }

  atomic_pos_.set_ticks (tick_value);
  Q_EMIT positionChanged ();
}

double
AtomicPositionQmlAdapter::seconds () const
{
  return atomic_pos_.get_seconds ().in (units::seconds);
}

void
AtomicPositionQmlAdapter::setSeconds (double seconds)
{
  auto second_value = units::seconds (seconds);
  if (constraints_)
    {
      // Convert to ticks, apply constraint, convert back to seconds
      auto tick_value =
        atomic_pos_.time_conversion_functions ().seconds_to_tick (second_value);
      tick_value = (*constraints_) (tick_value);
      second_value =
        atomic_pos_.time_conversion_functions ().tick_to_seconds (tick_value);
    }

  atomic_pos_.set_seconds (second_value);
  Q_EMIT positionChanged ();
}

qint64
AtomicPositionQmlAdapter::samples () const
{
  return atomic_pos_.get_samples ().in (units::samples);
}

void
AtomicPositionQmlAdapter::setSamples (double samples)
{
  auto sample_value = units::samples (samples);
  if (constraints_)
    {
      // Convert to ticks, apply constraint, convert back to samples
      auto tick_value =
        atomic_pos_.time_conversion_functions ().samples_to_tick (sample_value);
      tick_value = (*constraints_) (tick_value);
      sample_value =
        atomic_pos_.time_conversion_functions ().tick_to_samples (tick_value);
    }

  atomic_pos_.set_samples (sample_value);
  Q_EMIT positionChanged ();
}

TimeFormat
AtomicPositionQmlAdapter::mode () const
{
  return atomic_pos_.get_current_mode ();
}

void
AtomicPositionQmlAdapter::setMode (TimeFormat format)
{
  atomic_pos_.set_mode (format);
  Q_EMIT positionChanged ();
}

} // namespace zrythm::dsp
