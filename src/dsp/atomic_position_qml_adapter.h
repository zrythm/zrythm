// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

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
  explicit AtomicPositionQmlAdapter (
    AtomicPosition &atomicPos,
    bool            allowNegative,
    QObject *       parent = nullptr);

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
   * @brief Whether to allow negative positions.
   *
   * If this is false, negative values will be clamped to 0.
   */
  bool allow_negative_;
};
} // namespace zrythm::dsp
