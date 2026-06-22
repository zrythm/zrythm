// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/atomic_position_qml_adapter.h"
#include "dsp/tempo_map_qml_adapter.h"

namespace zrythm::dsp
{
AtomicPositionQmlAdapter::AtomicPositionQmlAdapter (
  AtomicPosition                   &atomicPos,
  const TempoMapWrapper            &tempo_map_wrapper,
  std::optional<ConstraintFunction> constraints,
  QObject *                         parent)
    : QObject (parent), atomic_pos_ (atomicPos),
      constraints_ (std::move (constraints))
{
  QObject::connect (
    &tempo_map_wrapper, &TempoMapWrapper::tempoEventsChanged, this, [this] () {
      if (
        atomic_pos_.get_current_mode () == AtomicPosition::TimeFormat::Absolute)
        Q_EMIT positionChanged ();
    });
}

AtomicPositionQmlAdapter::AtomicPositionQmlAdapter (
  AtomicPosition                   &atomicPos,
  const TempoMapWrapper            &tempo_map_wrapper,
  const AtomicPositionQmlAdapter   &anchor,
  std::optional<ConstraintFunction> constraints,
  QObject *                         parent)
    : AtomicPositionQmlAdapter (
        atomicPos,
        tempo_map_wrapper,
        std::move (constraints),
        parent)
{
  anchor_ = &anchor;
  QObject::connect (
    &anchor, &AtomicPositionQmlAdapter::positionChanged, this, [this] () {
      if (
        atomic_pos_.get_current_mode () == AtomicPosition::TimeFormat::Absolute)
        Q_EMIT positionChanged ();
    });
}

double
AtomicPositionQmlAdapter::ticks () const
{
  if (atomic_pos_.get_current_mode () == AtomicPosition::TimeFormat::Musical)
    return atomic_pos_.get_ticks ().in (units::ticks);

  if (anchor_ != nullptr)
    {
      const auto &tcf = atomic_pos_.time_conversion_functions ();
      const auto  anchor_seconds = units::seconds (anchor_->seconds ());
      const auto  duration_seconds = atomic_pos_.get_seconds ();
      const auto  end_seconds = anchor_seconds + duration_seconds;
      const auto  end_ticks = tcf.seconds_to_tick (end_seconds);
      const auto  anchor_ticks = tcf.seconds_to_tick (anchor_seconds);
      return (end_ticks - anchor_ticks).in (units::ticks);
    }

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

  if (
    atomic_pos_.get_current_mode () == AtomicPosition::TimeFormat::Absolute
    && anchor_ != nullptr)
    {
      const auto &tcf = atomic_pos_.time_conversion_functions ();
      const auto  anchor_ticks =
        tcf.seconds_to_tick (units::seconds (anchor_->seconds ()));
      const auto end_ticks = anchor_ticks + tick_value;
      const auto end_seconds = tcf.tick_to_seconds (end_ticks);
      const auto anchor_seconds = units::seconds (anchor_->seconds ());
      atomic_pos_.set_seconds (end_seconds - anchor_seconds);
    }
  else
    {
      atomic_pos_.set_ticks (tick_value);
    }
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

AtomicPosition::TimeFormat
AtomicPositionQmlAdapter::mode () const
{
  return atomic_pos_.get_current_mode ();
}

void
AtomicPositionQmlAdapter::setMode (AtomicPosition::TimeFormat format)
{
  atomic_pos_.set_mode (format);
  Q_EMIT positionChanged ();
}

} // namespace zrythm::dsp
