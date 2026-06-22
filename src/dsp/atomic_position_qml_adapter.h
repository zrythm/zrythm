// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <functional>
#include <optional>

#include "dsp/atomic_position.h"

#include <QObject>
#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::dsp
{
class TempoMapWrapper;

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
  using TimeFormat = AtomicPosition::TimeFormat;

  Q_ENUM (TimeFormat)

  explicit AtomicPositionQmlAdapter (
    AtomicPosition                   &atomicPos,
    const TempoMapWrapper            &tempo_map_wrapper,
    std::optional<ConstraintFunction> constraints = std::nullopt,
    QObject *                         parent = nullptr);

  /**
   * @brief Constructor with anchor for durations/offsets.
   *
   * When an anchor is provided, the @ref ticks property in Absolute mode is
   * computed as a delta relative to the anchor's timeline position, rather
   * than by integrating tempo from timeline zero. This gives correct tick
   * values for positions like region length, loop points, and fade offsets
   * when tempo changes exist before the region on the timeline.
   *
   * In Musical mode, the anchor is ignored.
   *
   * @par Lifetime contract
   * @p anchor must outlive this adapter. In practice both are QObjects sharing
   * a common ancestor (e.g. an ArrangerObject owns both the position adapter
   * used as anchor and the length/loop/fade adapters that anchor to it), so
   * their lifetimes are tied and Qt's auto-disconnect tears down the signal
   * link on either side's destruction.
   */
  explicit AtomicPositionQmlAdapter (
    AtomicPosition                   &atomicPos,
    const TempoMapWrapper            &tempo_map_wrapper,
    const AtomicPositionQmlAdapter   &anchor,
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

  /// Anchor adapter for delta-relative tick computation (Absolute mode only).
  /// Raw non-owning pointer; see the anchor-constructor's lifetime contract.
  const AtomicPositionQmlAdapter * anchor_ = nullptr;
};
} // namespace zrythm::dsp
