// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/position.h"

#include <nlohmann/json.hpp>

namespace zrythm::dsp
{

Position::Position (QObject * parent) : QObject (parent) { }

Position::Position (ConstraintFunction constraint, QObject * parent)
    : QObject (parent), constraint_ (std::move (constraint))
{
}

void
Position::setTicks (double ticks)
{
  auto tick_value = units::ticks (ticks);
  if (constraint_)
    tick_value = constraint_ (tick_value);
  if (!qFuzzyCompare (ticks_.in (units::ticks), tick_value.in (units::ticks)))
    {
      ticks_ = tick_value;
      Q_EMIT positionChanged ();
    }
}

void
to_json (nlohmann::json &j, const Position &pos)
{
  j[Position::kValue] = pos.ticks_.in (units::ticks);
}

void
from_json (const nlohmann::json &j, Position &pos)
{
  double value{};
  j.at (Position::kValue).get_to (value);
  pos.ticks_ = units::ticks (value);
}

} // namespace zrythm::dsp
