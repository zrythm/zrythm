// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <functional>
#include <optional>

#include "dsp/atomic_position.h"

#include <QtQmlIntegration>

namespace zrythm::dsp
{
class AtomicPositionQmlAdapter : public QObject
{
  Q_OBJECT
  Q_PROPERTY (double ticks READ ticks WRITE setTicks NOTIFY positionChanged)
  Q_PROPERTY (
    double seconds READ seconds WRITE setSeconds NOTIFY positionChanged)
  Q_PROPERTY (
    qint64 samples READ samples WRITE setSamples NOTIFY positionChanged)
  Q_PROPERTY (TimeFormat mode READ mode WRITE setMode NOTIFY positionChanged)
  QML_NAMED_ELEMENT (AtomicPosition)
  QML_UNCREATABLE ("")

public:
  using ConstraintFunction =
    std::function<units::precise_tick_t (units::precise_tick_t)>;

  explicit AtomicPositionQmlAdapter (
    AtomicPosition                   &atomicPos,
    std::optional<ConstraintFunction> constraints = std::nullopt,
    QObject *                         parent = nullptr);

  double           ticks () const;
  void             setTicks (double ticks);
  Q_INVOKABLE void addTicks (double ticks_to_add)
  {
    setTicks (ticks () + ticks_to_add);
  }

  double           seconds () const;
  void             setSeconds (double seconds);
  Q_INVOKABLE void addSeconds (double seconds_to_add)
  {
    setSeconds (seconds () + seconds_to_add);
  }

  qint64 samples () const;
  void   setSamples (double samples);

  TimeFormat mode () const;
  void       setMode (TimeFormat format);

  /// Only cost access allowed to the non-QML position to avoid updates without
  /// signals.
  const AtomicPosition &position () const { return atomic_pos_; }

Q_SIGNALS:
  void positionChanged ();

private:
  AtomicPosition &atomic_pos_; // Reference to existing DSP object

  /**
   * @brief Optional constraint function for position values.
   *
   * If provided, this function takes a tick value and returns the constrained
   * tick value. This allows implementing complex constraints like:
   * - Minimum/maximum values
   * - Loop point constraints (loop end must be after loop start)
   * - Minimum object lengths
   * - Runtime-changing constraints
   *
   * If std::nullopt, no constraints are applied.
   */
  std::optional<ConstraintFunction> constraints_;
};
} // namespace zrythm::dsp
