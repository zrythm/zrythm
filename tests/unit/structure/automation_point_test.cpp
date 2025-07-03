// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <memory>

#include "dsp/atomic_position.h"
#include "dsp/atomic_position_qml_adapter.h"
#include "dsp/tempo_map.h"
#include "structure/arrangement/automation_point.h"

#include "helpers/mock_qobject.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace zrythm::structure::arrangement
{
class AutomationPointTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    tempo_map = std::make_unique<dsp::TempoMap> (44100.0);
    parent = std::make_unique<MockQObject> ();

    // Create automation point
    point = std::make_unique<AutomationPoint> (*tempo_map, parent.get ());
  }

  std::unique_ptr<dsp::TempoMap>   tempo_map;
  std::unique_ptr<MockQObject>     parent;
  std::unique_ptr<AutomationPoint> point;
};

// Test initial state
TEST_F (AutomationPointTest, InitialState)
{
  EXPECT_EQ (point->type (), ArrangerObject::Type::AutomationPoint);
  EXPECT_EQ (point->position ()->samples (), 0);
  EXPECT_EQ (point->value (), 0.0f);
  EXPECT_FALSE (point->getSelected ());
}

// Test value property
TEST_F (AutomationPointTest, ValueProperty)
{
  // Set value
  point->setValue (0.5f);
  EXPECT_FLOAT_EQ (point->value (), 0.5f);

  // Set same value (should be no-op)
  testing::MockFunction<void (float)> mockValueChanged;
  QObject::connect (
    point.get (), &AutomationPoint::valueChanged, parent.get (),
    mockValueChanged.AsStdFunction ());

  EXPECT_CALL (mockValueChanged, Call (0.5f)).Times (0);
  point->setValue (0.5f);
}

// Test valueChanged signal
TEST_F (AutomationPointTest, ValueChangedSignal)
{
  // Setup signal watcher
  testing::MockFunction<void (float)> mockValueChanged;
  QObject::connect (
    point.get (), &AutomationPoint::valueChanged, parent.get (),
    mockValueChanged.AsStdFunction ());

  // First set
  EXPECT_CALL (mockValueChanged, Call (0.3f)).Times (1);
  point->setValue (0.3f);

  // Change value
  EXPECT_CALL (mockValueChanged, Call (0.7f)).Times (1);
  point->setValue (0.7f);
}

// Test position operations
TEST_F (AutomationPointTest, PositionOperations)
{
  point->position ()->setSamples (1000);
  EXPECT_EQ (point->position ()->samples (), 1000);

  point->position ()->setTicks (1920.0);
  EXPECT_DOUBLE_EQ (point->position ()->ticks (), 1920.0);
}

// Test selection status
TEST_F (AutomationPointTest, SelectionStatus)
{
  // Without getter
  EXPECT_FALSE (point->getSelected ());

  // With getter
  point->set_selection_status_getter ([] (const auto &) { return true; });
  EXPECT_TRUE (point->getSelected ());

  // Unset getter
  point->unset_selection_status_getter ();
  EXPECT_FALSE (point->getSelected ());
}

// Test serialization/deserialization
TEST_F (AutomationPointTest, Serialization)
{
  // Set initial state
  point->position ()->setSamples (2000);
  point->setValue (0.8f);

  // Serialize
  nlohmann::json j;
  to_json (j, *point);

  // Create new point
  auto new_point = std::make_unique<AutomationPoint> (*tempo_map, parent.get ());
  from_json (j, *new_point);

  // Verify state
  EXPECT_EQ (new_point->position ()->samples (), 2000);
  EXPECT_FLOAT_EQ (new_point->value (), 0.8f);
}

// Test copying
TEST_F (AutomationPointTest, Copying)
{
  // Set initial state
  point->position ()->setSamples (3000);
  point->setValue (0.9f);

  // Create target
  auto target = std::make_unique<AutomationPoint> (*tempo_map, parent.get ());

  // Copy
  init_from (*target, *point, utils::ObjectCloneType::Snapshot);

  // Verify state
  EXPECT_EQ (target->position ()->samples (), 3000);
  EXPECT_FLOAT_EQ (target->value (), 0.9f);
}

// Test edge cases
TEST_F (AutomationPointTest, EdgeCases)
{
  // Negative value
  point->setValue (-0.5f);
  EXPECT_FLOAT_EQ (point->value (), -0.5f);

  // Value above 1.0
  point->setValue (1.5f);
  EXPECT_FLOAT_EQ (point->value (), 1.5f);

  // Negative position
  point->position ()->setSamples (-1000);
  EXPECT_GE (point->position ()->samples (), 0); // Should clamp to 0
}

} // namespace zrythm::structure::arrangement
