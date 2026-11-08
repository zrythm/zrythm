// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "commands/change_parameter_value_command.h"
#include "dsp/parameter.h"
#include "utils/object_registry.h"
#include "utils/registry_utils.h"

#include <gtest/gtest.h>

namespace zrythm::commands
{

class ChangeParameterValueCommandTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    param_ref_ = utils::create_object<dsp::ProcessorParameter> (
      registry_, registry_, dsp::ProcessorParameter::UniqueId (u8"test_param"),
      dsp::ParameterRange (
        dsp::ParameterRange::Type::Linear, 0.f, 1.f, 0.f, 0.5f),
      utils::Utf8String::from_utf8_encoded_string ("Test Parameter"));
    param_ = param_ref_.get ();
    ASSERT_NE (param_, nullptr);

    param_->setBaseValue (0.5f);
  }

  void TearDown () override { }

  utils::ObjectRegistry                registry_;
  dsp::ProcessorParameterUuidReference param_ref_{ registry_ };
  dsp::ProcessorParameter *            param_ = nullptr;
};

TEST_F (ChangeParameterValueCommandTest, InitialState)
{
  ChangeParameterValueCommand cmd (*param_, registry_, 0.75f);
  EXPECT_FLOAT_EQ (param_->baseValue (), 0.5f);
}

TEST_F (ChangeParameterValueCommandTest, RedoOperation)
{
  ChangeParameterValueCommand cmd (*param_, registry_, 0.75f);

  // Initial state
  EXPECT_FLOAT_EQ (param_->baseValue (), 0.5f);

  // Execute redo (first time)
  cmd.redo ();
  EXPECT_FLOAT_EQ (param_->baseValue (), 0.75f);

  // Execute redo again (should still work)
  cmd.redo ();
  EXPECT_FLOAT_EQ (param_->baseValue (), 0.75f);
}

TEST_F (ChangeParameterValueCommandTest, UndoOperation)
{
  ChangeParameterValueCommand cmd (*param_, registry_, 0.75f);

  // First execute redo to set the value
  cmd.redo ();
  EXPECT_FLOAT_EQ (param_->baseValue (), 0.75f);

  // Then undo
  cmd.undo ();
  EXPECT_FLOAT_EQ (param_->baseValue (), 0.5f);
}

TEST_F (ChangeParameterValueCommandTest, UndoRedoCycle)
{
  ChangeParameterValueCommand cmd (*param_, registry_, 0.75f);

  // Initial state
  EXPECT_FLOAT_EQ (param_->baseValue (), 0.5f);

  // Redo
  cmd.redo ();
  EXPECT_FLOAT_EQ (param_->baseValue (), 0.75f);

  // Undo
  cmd.undo ();
  EXPECT_FLOAT_EQ (param_->baseValue (), 0.5f);

  // Redo again
  cmd.redo ();
  EXPECT_FLOAT_EQ (param_->baseValue (), 0.75f);

  // Undo again
  cmd.undo ();
  EXPECT_FLOAT_EQ (param_->baseValue (), 0.5f);
}

TEST_F (ChangeParameterValueCommandTest, SameValueNoChange)
{
  ChangeParameterValueCommand cmd (*param_, registry_, 0.5f);

  // Initial state
  EXPECT_FLOAT_EQ (param_->baseValue (), 0.5f);

  // Redo with same value
  cmd.redo ();
  EXPECT_FLOAT_EQ (param_->baseValue (), 0.5f);

  // Undo should still restore to original
  cmd.undo ();
  EXPECT_FLOAT_EQ (param_->baseValue (), 0.5f);
}

TEST_F (ChangeParameterValueCommandTest, BoundaryValues)
{
  // Test minimum boundary
  {
    ChangeParameterValueCommand cmd (*param_, registry_, 0.0f);
    cmd.redo ();
    EXPECT_FLOAT_EQ (param_->baseValue (), 0.0f);
    cmd.undo ();
    EXPECT_FLOAT_EQ (param_->baseValue (), 0.5f);
  }

  // Test maximum boundary
  {
    ChangeParameterValueCommand cmd (*param_, registry_, 1.0f);
    cmd.redo ();
    EXPECT_FLOAT_EQ (param_->baseValue (), 1.0f);
    cmd.undo ();
    EXPECT_FLOAT_EQ (param_->baseValue (), 0.5f);
  }
}

TEST_F (ChangeParameterValueCommandTest, NegativeValue)
{
  ChangeParameterValueCommand cmd (*param_, registry_, -0.5f);

  // Parameter should clamp negative values to 0
  cmd.redo ();
  EXPECT_FLOAT_EQ (param_->baseValue (), 0.0f);

  cmd.undo ();
  EXPECT_FLOAT_EQ (param_->baseValue (), 0.5f);
}

TEST_F (ChangeParameterValueCommandTest, MergeWithSameType)
{
  ChangeParameterValueCommand cmd1 (*param_, registry_, 0.25f);
  ChangeParameterValueCommand cmd2 (*param_, registry_, 0.75f);

  // First command
  cmd1.redo ();
  EXPECT_FLOAT_EQ (param_->baseValue (), 0.25f);

  // Should merge with second command (within time limit)
  EXPECT_TRUE (cmd1.merge_with (cmd2));

  // Redo should use merged value
  cmd1.redo ();
  EXPECT_FLOAT_EQ (param_->baseValue (), 0.75f);
}

TEST_F (ChangeParameterValueCommandTest, MergeWithDifferentType)
{
  ChangeParameterValueCommand cmd1 (*param_, registry_, 0.25f);

  // Create a mock command of a different type
  class MockCommand : public commands::Command
  {
  public:
    MockCommand () : Command (QStringLiteral ("Mock"), EngineRequirement::None)
    {
    }
    void undo () override { }
    void redo () override { }
  } mockCmd;

  // Should not merge with different command type
  EXPECT_FALSE (cmd1.merge_with (mockCmd));
}

TEST_F (ChangeParameterValueCommandTest, ValueGreaterThanOne)
{
  ChangeParameterValueCommand cmd (*param_, registry_, 1.5f);

  // Parameter should clamp values > 1 to 1
  cmd.redo ();
  EXPECT_FLOAT_EQ (param_->baseValue (), 1.0f);

  cmd.undo ();
  EXPECT_FLOAT_EQ (param_->baseValue (), 0.5f);
}

TEST_F (ChangeParameterValueCommandTest, MultipleCommands)
{
  // Create first command
  ChangeParameterValueCommand cmd1 (*param_, registry_, 0.25f);
  cmd1.redo ();
  EXPECT_FLOAT_EQ (param_->baseValue (), 0.25f);

  // Create second command with new value
  ChangeParameterValueCommand cmd2 (*param_, registry_, 0.75f);
  cmd2.redo ();
  EXPECT_FLOAT_EQ (param_->baseValue (), 0.75f);

  // Undo second command
  cmd2.undo ();
  EXPECT_FLOAT_EQ (param_->baseValue (), 0.25f);

  // Undo first command
  cmd1.undo ();
  EXPECT_FLOAT_EQ (param_->baseValue (), 0.5f);
}

TEST_F (ChangeParameterValueCommandTest, CommandText)
{
  ChangeParameterValueCommand cmd (*param_, registry_, 0.75f);

  // Check that the command text is properly formatted
  QString expected_text =
    QObject::tr ("Change '%1' value to %2").arg (param_->label ()).arg (0.75f);
  EXPECT_EQ (cmd.text (), expected_text);
}

TEST_F (ChangeParameterValueCommandTest, DifferentParameterTypes)
{
  // Create parameter with different range
  auto param_ref2 = std::optional (
    utils::create_object<dsp::ProcessorParameter> (
      registry_, registry_, dsp::ProcessorParameter::UniqueId (u8"volume_param"),
      dsp::ParameterRange (
        dsp::ParameterRange::Type::GainAmplitude, 0.f, 2.f, 0.f, 1.f),
      utils::Utf8String::from_utf8_encoded_string ("Volume")));
  auto * param2 = param_ref2.value ().get ();
  ASSERT_NE (param2, nullptr);

  // Set initial value
  param2->setBaseValue (0.5f);

  // Test with gain parameter
  ChangeParameterValueCommand cmd (*param2, registry_, 1.0f);
  cmd.redo ();
  EXPECT_FLOAT_EQ (param2->baseValue (), 1.0f);

  cmd.undo ();
  EXPECT_FLOAT_EQ (param2->baseValue (), 0.5f);
}

TEST_F (ChangeParameterValueCommandTest, VerySmallValueChanges)
{
  ChangeParameterValueCommand cmd (*param_, registry_, 0.50001f);

  // Test very small changes
  cmd.redo ();
  EXPECT_NEAR (param_->baseValue (), 0.50001f, 1e-5f);

  cmd.undo ();
  EXPECT_NEAR (param_->baseValue (), 0.5f, 1e-5f);
}

TEST_F (ChangeParameterValueCommandTest, LargeValueChanges)
{
  ChangeParameterValueCommand cmd (*param_, registry_, 0.99999f);

  // Test large changes
  cmd.redo ();
  EXPECT_NEAR (param_->baseValue (), 0.99999f, 1e-5f);

  cmd.undo ();
  EXPECT_NEAR (param_->baseValue (), 0.5f, 1e-5f);
}

TEST_F (ChangeParameterValueCommandTest, ZeroValueChange)
{
  ChangeParameterValueCommand cmd (*param_, registry_, 0.0f);

  // Test zero value
  cmd.redo ();
  EXPECT_FLOAT_EQ (param_->baseValue (), 0.0f);

  cmd.undo ();
  EXPECT_FLOAT_EQ (param_->baseValue (), 0.5f);
}

TEST_F (ChangeParameterValueCommandTest, OneValueChange)
{
  ChangeParameterValueCommand cmd (*param_, registry_, 1.0f);

  // Test maximum value
  cmd.redo ();
  EXPECT_FLOAT_EQ (param_->baseValue (), 1.0f);

  cmd.undo ();
  EXPECT_FLOAT_EQ (param_->baseValue (), 0.5f);
}

} // namespace zrythm::commands
