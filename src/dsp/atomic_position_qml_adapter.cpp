// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/atomic_position_qml_adapter.h"

namespace zrythm::dsp
{
AtomicPositionQmlAdapter::AtomicPositionQmlAdapter (
  AtomicPosition &atomicPos,
  QObject *       parent)
    : QObject (parent), atomic_pos_ (atomicPos)
{
}

double
AtomicPositionQmlAdapter::ticks () const
{
  return atomic_pos_.get_ticks ();
}

void
AtomicPositionQmlAdapter::setTicks (double ticks)
{
  atomic_pos_.set_ticks (ticks);
  Q_EMIT positionChanged ();
}

double
AtomicPositionQmlAdapter::seconds () const
{
  return atomic_pos_.get_seconds ();
}

void
AtomicPositionQmlAdapter::setSeconds (double seconds)
{
  atomic_pos_.set_seconds (seconds);
  Q_EMIT positionChanged ();
}

qint64
AtomicPositionQmlAdapter::samples () const
{
  return atomic_pos_.get_samples ();
}

void
AtomicPositionQmlAdapter::setSamples (double samples)
{
  atomic_pos_.set_samples (samples);
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

QString
AtomicPositionQmlAdapter::getStringDisplay () const
{
  const auto musical_pos = atomic_pos_.get_tempo_map ().tick_to_musical_position (
    static_cast<int64_t> (atomic_pos_.get_ticks ()));
  return QString::fromStdString (
    fmt::format (
      "{}.{}.{}.{:03}", musical_pos.bar, musical_pos.beat,
      musical_pos.sixteenth, musical_pos.tick));
}

} // namespace zrythm::dsp
