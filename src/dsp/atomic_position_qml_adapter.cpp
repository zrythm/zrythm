// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/atomic_position_qml_adapter.h"

namespace zrythm::dsp
{
AtomicPositionQmlAdapter::AtomicPositionQmlAdapter (
  AtomicPosition &atomicPos,
  bool            allowNegative,
  QObject *       parent)
    : QObject (parent), atomic_pos_ (atomicPos), allow_negative_ (allowNegative)
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
  if (!allow_negative_ && ticks < 0.0)
    {
      ticks = 0.0;
    }

  atomic_pos_.set_ticks (units::ticks (ticks));
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
  if (!allow_negative_ && seconds < 0.0)
    {
      seconds = 0.0;
    }

  atomic_pos_.set_seconds (units::seconds (seconds));
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
  if (!allow_negative_ && samples < 0.0)
    {
      samples = 0.0;
    }

  atomic_pos_.set_samples (units::samples (samples));
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
