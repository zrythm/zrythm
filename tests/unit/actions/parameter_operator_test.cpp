// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/parameter_operator.h"
#include "dsp/parameter.h"
#include "undo/undo_stack.h"
#include "utils/gtest_wrapper.h"

#include "unit/actions/mock_undo_stack.h"

namespace zrythm::actions
{

class ParameterOperatorTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    // Create test parameter
    param_ref_ = param_registry_.create_object<dsp::ProcessorParameter> (
      port_registry_, dsp::ProcessorParameter::UniqueId (u8"test_param"),
      dsp::ParameterRange (
        dsp::ParameterRange::Type::Linear, 0.f, 1.f, 0.f, 0.5f),
      utils::Utf8String::from_utf8_encoded_string ("Test Parameter"));
    param_ = param_ref_.get_object_as<dsp::ProcessorParameter> ();
    ASSERT_NE (param_, nullptr);

    // Create undo stack
    undo_stack_ = create_mock_undo_stack ();

    // Set initial value
    param_->setBaseValue (0.5f);

    // Create operator
    operator_ = std::make_unique<ProcessorParameterOperator> ();
    operator_->setProcessorParameter (param_);
    operator_->setUndoStack (undo_stack_.get ());
  }

  void TearDown () override
  {
    operator_.reset ();
    undo_stack_.reset ();
  }

  dsp::PortRegistry                           port_registry_;
  dsp::ProcessorParameterRegistry             param_registry_{ port_registry_ };
  dsp::ProcessorParameterUuidReference        param_ref_{ param_registry_ };
  dsp::ProcessorParameter *                   param_ = nullptr;
  std::unique_ptr<undo::UndoStack>            undo_stack_;
  std::unique_ptr<ProcessorParameterOperator> operator_;
};

TEST_F (ParameterOperatorTest, InitialState)
{
  EXPECT_EQ (operator_->processorParameter (), param_);
  EXPECT_EQ (operator_->undoStack (), undo_stack_.get ());
}

TEST_F (ParameterOperatorTest, SetProcessorParameter)
{
  EXPECT_THROW (
    operator_->setProcessorParameter (nullptr), std::invalid_argument);
}

TEST_F (ParameterOperatorTest, SetUndoStack)
{
  EXPECT_THROW (operator_->setUndoStack (nullptr), std::invalid_argument);
}

TEST_F (ParameterOperatorTest, SetValue)
{
  // Initial state
  EXPECT_FLOAT_EQ (param_->baseValue (), 0.5f);
  EXPECT_EQ (undo_stack_->count (), 0);

  // Set new value
  operator_->setValue (0.75f);
  EXPECT_FLOAT_EQ (param_->baseValue (), 0.75f);
  EXPECT_EQ (undo_stack_->count (), 1);

  // Undo
  undo_stack_->undo ();
  EXPECT_FLOAT_EQ (param_->baseValue (), 0.5f);

  // Redo
  undo_stack_->redo ();
  EXPECT_FLOAT_EQ (param_->baseValue (), 0.75f);
}

TEST_F (ParameterOperatorTest, SetValueSameValue)
{
  // Set same value - should still create command
  operator_->setValue (0.5f);
  EXPECT_FLOAT_EQ (param_->baseValue (), 0.5f);
  EXPECT_EQ (undo_stack_->count (), 1);
}

TEST_F (ParameterOperatorTest, SetValueBoundaryValues)
{
  // Test minimum boundary
  operator_->setValue (0.0f);
  EXPECT_FLOAT_EQ (param_->baseValue (), 0.0f);
  EXPECT_EQ (undo_stack_->count (), 1);

  // Undo and test maximum boundary
  undo_stack_->undo ();
  operator_->setValue (1.0f);
  EXPECT_FLOAT_EQ (param_->baseValue (), 1.0f);
  EXPECT_EQ (undo_stack_->count (), 1);
}

TEST_F (ParameterOperatorTest, SetValueNegative)
{
  // Parameter should clamp negative values to 0
  operator_->setValue (-0.5f);
  EXPECT_FLOAT_EQ (param_->baseValue (), 0.0f);
  EXPECT_EQ (undo_stack_->count (), 1);
}

TEST_F (ParameterOperatorTest, SetValueGreaterThanOne)
{
  // Parameter should clamp values > 1 to 1
  operator_->setValue (1.5f);
  EXPECT_FLOAT_EQ (param_->baseValue (), 1.0f);
  EXPECT_EQ (undo_stack_->count (), 1);
}

// This test requires making the mergeWith() threshold in the command class
// configurable
#if 0
TEST_F (ParameterOperatorTest, MultipleSetValueOperations)
{
  // First operation
  operator_->setValue (0.25f);
  EXPECT_FLOAT_EQ (param_->baseValue (), 0.25f);
  EXPECT_EQ (undo_stack_->undoStack ()->count (), 1);

  // Second operation
  operator_->setValue (0.75f);
  EXPECT_FLOAT_EQ (param_->baseValue (), 0.75f);
  EXPECT_EQ (undo_stack_->undoStack ()->count (), 2);

  // Undo second operation
  undo_stack_->undo ();
  EXPECT_FLOAT_EQ (param_->baseValue (), 0.25f);

  // Undo first operation
  undo_stack_->undo ();
  EXPECT_FLOAT_EQ (param_->baseValue (), 0.5f);
}
#endif

TEST_F (ParameterOperatorTest, VerySmallValueChanges)
{
  operator_->setValue (0.50001f);
  EXPECT_NEAR (param_->baseValue (), 0.50001f, 1e-5f);
  EXPECT_EQ (undo_stack_->count (), 1);
}

TEST_F (ParameterOperatorTest, LargeValueChanges)
{
  operator_->setValue (0.99999f);
  EXPECT_NEAR (param_->baseValue (), 0.99999f, 1e-5f);
  EXPECT_EQ (undo_stack_->count (), 1);
}

TEST_F (ParameterOperatorTest, ZeroValueChange)
{
  operator_->setValue (0.0f);
  EXPECT_FLOAT_EQ (param_->baseValue (), 0.0f);
  EXPECT_EQ (undo_stack_->count (), 1);
}

TEST_F (ParameterOperatorTest, OneValueChange)
{
  operator_->setValue (1.0f);
  EXPECT_FLOAT_EQ (param_->baseValue (), 1.0f);
  EXPECT_EQ (undo_stack_->count (), 1);
}

TEST_F (ParameterOperatorTest, DifferentParameterTypes)
{
  // Create parameter with different range
  auto param_ref2 = param_registry_.create_object<dsp::ProcessorParameter> (
    port_registry_, dsp::ProcessorParameter::UniqueId (u8"volume_param"),
    dsp::ParameterRange (
      dsp::ParameterRange::Type::GainAmplitude, 0.f, 2.f, 0.f, 1.f),
    utils::Utf8String::from_utf8_encoded_string ("Volume"));
  auto * param2 = param_ref2.get_object_as<dsp::ProcessorParameter> ();
  ASSERT_NE (param2, nullptr);

  // Set initial value
  param2->setBaseValue (0.5f);

  // Create new operator
  auto operator2 = std::make_unique<ProcessorParameterOperator> ();
  operator2->setProcessorParameter (param2);
  operator2->setUndoStack (undo_stack_.get ());

  // Test with gain parameter
  operator2->setValue (1.0f);
  EXPECT_FLOAT_EQ (param2->baseValue (), 1.0f);
  EXPECT_EQ (undo_stack_->count (), 1);
}

TEST_F (ParameterOperatorTest, SignalEmissions)
{
  // Test processorParameterChanged signal
  bool param_changed = false;
  QObject::connect (
    operator_.get (), &ProcessorParameterOperator::processorParameterChanged,
    operator_.get (), [&] () { param_changed = true; });

  auto param_ref2 = param_registry_.create_object<dsp::ProcessorParameter> (
    port_registry_, dsp::ProcessorParameter::UniqueId (u8"new_param"),
    dsp::ParameterRange (dsp::ParameterRange::Type::Linear, 0.f, 1.f, 0.f, 0.5f),
    utils::Utf8String::from_utf8_encoded_string ("New Parameter"));
  auto * param2 = param_ref2.get_object_as<dsp::ProcessorParameter> ();
  operator_->setProcessorParameter (param2);
  EXPECT_TRUE (param_changed);

  // Test undoStackChanged signal
  bool stack_changed = false;
  QObject::connect (
    operator_.get (), &ProcessorParameterOperator::undoStackChanged,
    operator_.get (), [&] () { stack_changed = true; });

  auto new_stack = create_mock_undo_stack ();
  operator_->setUndoStack (new_stack.get ());
  EXPECT_TRUE (stack_changed);
}

TEST_F (ParameterOperatorTest, RapidValueChanges)
{
  // Test that rapid successive changes may be merged into a single command
  // Note: The actual merging depends on the 1-second timeout in mergeWith()
  operator_->setValue (0.25f);
  operator_->setValue (0.60f);
  operator_->setValue (0.75f);

  // Final value should be the last one set
  EXPECT_FLOAT_EQ (param_->baseValue (), 0.75f);
  EXPECT_EQ (undo_stack_->count (), 1);

  // After undo, we should return to the original value
  undo_stack_->undo ();
  EXPECT_FLOAT_EQ (param_->baseValue (), 0.5f);
}

} // namespace zrythm::actions
