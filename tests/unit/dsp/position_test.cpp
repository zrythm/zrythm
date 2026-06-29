// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/position.h"
#include "utils/units.h"

#include <QSignalSpy>

#include "helpers/scoped_qcoreapplication.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

namespace zrythm::dsp::tests
{

class PositionTest
    : public ::testing::Test,
      private test_helpers::ScopedQCoreApplication
{
protected:
  void SetUp () override { pos = std::make_unique<Position> (); }

  std::unique_ptr<Position> pos;
};

TEST_F (PositionTest, DefaultConstruction)
{
  EXPECT_DOUBLE_EQ (pos->ticks (), 0.0);
}

TEST_F (PositionTest, SetTicks)
{
  pos->setTicks (1920.0);
  EXPECT_DOUBLE_EQ (pos->ticks (), 1920.0);
}

TEST_F (PositionTest, AddTicks)
{
  pos->setTicks (100.0);
  pos->addTicks (50.0);
  EXPECT_DOUBLE_EQ (pos->ticks (), 150.0);
}

TEST_F (PositionTest, ConstraintFunction)
{
  auto constrained = std::make_unique<Position> (
    [] (units::precise_tick_t ticks) {
      return std::max (ticks, units::ticks (1.0));
    },
    nullptr);
  constrained->setTicks (-5.0);
  EXPECT_DOUBLE_EQ (constrained->ticks (), 1.0);
}

TEST_F (PositionTest, Serialization)
{
  pos->setTicks (3840.0);
  nlohmann::json j;
  to_json (j, *pos);

  auto new_pos = std::make_unique<Position> ();
  from_json (j, *new_pos);
  EXPECT_DOUBLE_EQ (new_pos->ticks (), 3840.0);
}

TEST_F (PositionTest, SetTicksSameValueDoesNotEmit)
{
  pos->setTicks (1920.0);
  QSignalSpy spy (pos.get (), &Position::positionChanged);
  pos->setTicks (1920.0);
  EXPECT_EQ (spy.count (), 0);
}

TEST_F (PositionTest, SetTicksDifferentValueEmits)
{
  QSignalSpy spy (pos.get (), &Position::positionChanged);
  pos->setTicks (1920.0);
  EXPECT_EQ (spy.count (), 1);
}

// --- Typed Position subclass tests ---

TEST (TypedPositionTest, TimelinePositionAsTick)
{
  TimelinePosition tp;
  tp.setTicks (1920.0);
  auto tick = tp.asTick ();
  static_assert (std::is_same_v<decltype (tick), TimelineTick>);
  EXPECT_DOUBLE_EQ (tick.asDouble (), 1920.0);
}

TEST (TypedPositionTest, ContentPositionAsTick)
{
  ContentPosition cp;
  cp.setTicks (960.0);
  auto tick = cp.asTick ();
  static_assert (std::is_same_v<decltype (tick), ContentTick>);
  EXPECT_DOUBLE_EQ (tick.asDouble (), 960.0);
}

TEST (TypedPositionTest, InheritedBehavior)
{
  TimelinePosition tp;
  tp.addTicks (100.0);
  EXPECT_DOUBLE_EQ (tp.ticks (), 100.0);
}

} // namespace zrythm::dsp::tests
