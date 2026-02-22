// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/project/arranger_tool.h"

#include "helpers/scoped_qcoreapplication.h"

#include <gtest/gtest.h>

using namespace zrythm::structure::project;

class ArrangerToolTest
    : public ::testing::Test,
      public test_helpers::ScopedQCoreApplication
{
protected:
  void SetUp () override
  {
    arranger_tool_ = std::make_unique<ArrangerTool> (nullptr);
  }

  std::unique_ptr<ArrangerTool> arranger_tool_;
};

// ============================================================================
// Construction Tests
// ============================================================================

TEST_F (ArrangerToolTest, Construction)
{
  EXPECT_NE (arranger_tool_, nullptr);
  EXPECT_EQ (
    arranger_tool_->toolValue (),
    static_cast<int> (ArrangerTool::ToolType::Select));
}

// ============================================================================
// Tool Type Tests
// ============================================================================

TEST_F (ArrangerToolTest, DefaultToolIsSelect)
{
  EXPECT_EQ (
    arranger_tool_->toolValue (),
    static_cast<int> (ArrangerTool::ToolType::Select));
}

TEST_F (ArrangerToolTest, SetToolValue)
{
  arranger_tool_->setToolValue (static_cast<int> (ArrangerTool::ToolType::Edit));
  EXPECT_EQ (
    arranger_tool_->toolValue (),
    static_cast<int> (ArrangerTool::ToolType::Edit));
}

TEST_F (ArrangerToolTest, SetToolValueEmitsSignal)
{
  bool signal_emitted = false;
  int  new_value = -1;

  QObject::connect (
    arranger_tool_.get (), &ArrangerTool::toolValueChanged,
    arranger_tool_.get (), [&] (int value) {
      signal_emitted = true;
      new_value = value;
    });

  arranger_tool_->setToolValue (static_cast<int> (ArrangerTool::ToolType::Cut));

  EXPECT_TRUE (signal_emitted);
  EXPECT_EQ (new_value, static_cast<int> (ArrangerTool::ToolType::Cut));
}

TEST_F (ArrangerToolTest, SetSameToolValueDoesNotEmitSignal)
{
  bool signal_emitted = false;

  QObject::connect (
    arranger_tool_.get (), &ArrangerTool::toolValueChanged,
    arranger_tool_.get (), [&] (int) { signal_emitted = true; });

  // Set to Edit first
  arranger_tool_->setToolValue (static_cast<int> (ArrangerTool::ToolType::Edit));
  signal_emitted = false;

  // Set to Edit again - should not emit
  arranger_tool_->setToolValue (static_cast<int> (ArrangerTool::ToolType::Edit));

  EXPECT_FALSE (signal_emitted);
}

TEST_F (ArrangerToolTest, AllToolTypes)
{
  // Test setting all tool types
  arranger_tool_->setToolValue (
    static_cast<int> (ArrangerTool::ToolType::Select));
  EXPECT_EQ (
    arranger_tool_->toolValue (),
    static_cast<int> (ArrangerTool::ToolType::Select));

  arranger_tool_->setToolValue (static_cast<int> (ArrangerTool::ToolType::Edit));
  EXPECT_EQ (
    arranger_tool_->toolValue (),
    static_cast<int> (ArrangerTool::ToolType::Edit));

  arranger_tool_->setToolValue (static_cast<int> (ArrangerTool::ToolType::Cut));
  EXPECT_EQ (
    arranger_tool_->toolValue (),
    static_cast<int> (ArrangerTool::ToolType::Cut));

  arranger_tool_->setToolValue (
    static_cast<int> (ArrangerTool::ToolType::Eraser));
  EXPECT_EQ (
    arranger_tool_->toolValue (),
    static_cast<int> (ArrangerTool::ToolType::Eraser));

  arranger_tool_->setToolValue (static_cast<int> (ArrangerTool::ToolType::Ramp));
  EXPECT_EQ (
    arranger_tool_->toolValue (),
    static_cast<int> (ArrangerTool::ToolType::Ramp));

  arranger_tool_->setToolValue (
    static_cast<int> (ArrangerTool::ToolType::Audition));
  EXPECT_EQ (
    arranger_tool_->toolValue (),
    static_cast<int> (ArrangerTool::ToolType::Audition));
}

// ============================================================================
// Serialization Tests
// ============================================================================

TEST_F (ArrangerToolTest, JsonSerializationRoundTrip)
{
  // Set to a non-default value
  arranger_tool_->setToolValue (static_cast<int> (ArrangerTool::ToolType::Ramp));

  // Serialize to JSON
  nlohmann::json j = *arranger_tool_;

  // Create a new ArrangerTool and deserialize
  auto deserialized_tool = std::make_unique<ArrangerTool> (nullptr);
  j.get_to (*deserialized_tool);

  // Verify the value was preserved
  EXPECT_EQ (
    deserialized_tool->toolValue (),
    static_cast<int> (ArrangerTool::ToolType::Ramp));
}

TEST_F (ArrangerToolTest, JsonSerializationSelectTool)
{
  arranger_tool_->setToolValue (
    static_cast<int> (ArrangerTool::ToolType::Select));

  nlohmann::json j = *arranger_tool_;

  auto deserialized_tool = std::make_unique<ArrangerTool> (nullptr);
  j.get_to (*deserialized_tool);

  EXPECT_EQ (
    deserialized_tool->toolValue (),
    static_cast<int> (ArrangerTool::ToolType::Select));
}

TEST_F (ArrangerToolTest, JsonSerializationEditTool)
{
  arranger_tool_->setToolValue (static_cast<int> (ArrangerTool::ToolType::Edit));

  nlohmann::json j = *arranger_tool_;

  auto deserialized_tool = std::make_unique<ArrangerTool> (nullptr);
  j.get_to (*deserialized_tool);

  EXPECT_EQ (
    deserialized_tool->toolValue (),
    static_cast<int> (ArrangerTool::ToolType::Edit));
}

// ============================================================================
// Clone Tests
// ============================================================================

TEST_F (ArrangerToolTest, InitFrom)
{
  arranger_tool_->setToolValue (
    static_cast<int> (ArrangerTool::ToolType::Audition));

  auto cloned_tool = std::make_unique<ArrangerTool> (nullptr);
  init_from (*cloned_tool, *arranger_tool_, utils::ObjectCloneType::Snapshot);

  EXPECT_EQ (
    cloned_tool->toolValue (),
    static_cast<int> (ArrangerTool::ToolType::Audition));
}
