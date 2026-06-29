// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <functional>

#include "dsp/tick_types.h"
#include "utils/units.h"

#include <QObject>
#include <QtQmlIntegration/qqmlintegration.h>

#include <nlohmann/json_fwd.hpp>

namespace zrythm::dsp
{

/**
 * @brief An observable, serializable musical position stored as ticks.
 *
 * The stored value's meaning (timeline ticks vs. content ticks) is determined
 * by the owner context — Position does not encode coordinate space. Callers
 * know from the property/variable name whether they are working with a timeline
 * position (e.g. `position_`) or a content-space value (e.g. `length_`,
 * `loop_start_pos_`).
 *
 * Non-atomic: arranger-object positions are never read from the audio thread.
 * For Transport marker positions, see Transport's farbot::RealtimeObject
 * snapshot which pre-computes samples for RT access.
 */
class Position : public QObject
{
  Q_OBJECT
  Q_PROPERTY (double ticks READ ticks WRITE setTicks NOTIFY positionChanged)
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  /// Function that clamps/adjusts tick values on setTicks (e.g. min-length
  /// enforcement).
  using ConstraintFunction =
    std::function<units::precise_tick_t (units::precise_tick_t)>;

  /**
   * @brief Construct a Position without a value constraint.
   * @param parent QObject parent.
   */
  explicit Position (QObject * parent = nullptr);

  /**
   * @brief Construct a Position with a value constraint.
   * @param constraint Function that clamps/adjusts tick values on setTicks
   * (e.g. min-length enforcement).
   * @param parent QObject parent.
   */
  explicit Position (ConstraintFunction constraint, QObject * parent = nullptr);

  /// The stored tick value (meaning depends on owner context).
  double ticks () const { return ticks_.in (units::ticks); }

  void setTicks (double ticks);

  Q_INVOKABLE void addTicks (double ticks)
  {
    setTicks (this->ticks () + ticks);
  }

Q_SIGNALS:
  void positionChanged ();

private:
  static constexpr auto kValue = "value";

  units::precise_tick_t ticks_{};

  ConstraintFunction constraint_;

  friend void to_json (nlohmann::json &j, const Position &pos);
  friend void from_json (const nlohmann::json &j, Position &pos);
};

/**
 * @brief A Position whose ticks are absolute timeline ticks.
 *
 * Inherits all Position behavior (Q_PROPERTY, serialization, signals).
 * The @ref asTick method provides a typed @ref TimelineTick for compile-time
 * coordinate-space safety.
 */
class TimelinePosition : public Position
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  using Position::Position;

  TimelineTick asTick () const
  {
    return TimelineTick{ units::ticks (ticks ()) };
  }
};

/**
 * @brief A Position whose ticks are content-space (clip-local) ticks.
 *
 * All durations, loop bounds, and fade offsets are ContentPosition.
 * Convert to timeline coordinates via ContentTimeWarp::contentToTimeline().
 */
class ContentPosition : public Position
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  using Position::Position;

  ContentTick asTick () const { return ContentTick{ units::ticks (ticks ()) }; }
};

} // namespace zrythm::dsp
