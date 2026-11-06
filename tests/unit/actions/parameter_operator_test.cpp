// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/parameter_operator.h"
#include "dsp/parameter.h"
#include "undo/undo_stack.h"
#include "utils/object_registry.h"
#include "utils/registry_utils.h"

#include "unit/actions/mock_undo_stack.h"
#include <gtest/gtest.h>

namespace zrythm::actions
{

class ParameterOperatorTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    // Create test parameters
    param_ref_ = utils::create_object<dsp::ProcessorParameter> (
      registry_, registry_, dsp::ProcessorParameter::UniqueId (u8"test_param"),
      dsp::ParameterRange (
        dsp::ParameterRange::Type::Linear, 0.f, 1.f, 0.f, 0.5f),
      utils::Utf8String::from_utf8_encoded_string ("Test Parameter"));
    param_ = param_ref_.get_object_as<dsp::ProcessorParameter> ();
    ASSERT_NE (param_, nullptr);

    second_param_ref_ = utils::create_object<dsp::ProcessorParameter> (
      registry_, registry_, dsp::ProcessorParameter::UniqueId (u8"second_param"),
      dsp::ParameterRange (
        dsp::ParameterRange::Type::Linear, 0.f, 1.f, 0.f, 0.5f),
      utils::Utf8String::from_utf8_encoded_string ("Second Parameter"));
    second_param_ = second_param_ref_.get_object_as<dsp::ProcessorParameter> ();
    ASSERT_NE (second_param_, nullptr);

    // Create undo stack
    undo_stack_ = create_mock_undo_stack ();

    // Set initial values
    param_->setBaseValue (0.5f);
    second_param_->setBaseValue (0.5f);

    // Create operator
    operator_ =
      std::make_unique<ProcessorParameterOperator> (*undo_stack_, registry_);
  }

  void TearDown () override
  {
    operator_.reset ();
    undo_stack_.reset ();
  }

  utils::ObjectRegistry                       registry_;
  dsp::ProcessorParameterUuidReference        param_ref_{ registry_ };
  dsp::ProcessorParameter *                   param_ = nullptr;
  dsp::ProcessorParameterUuidReference        second_param_ref_{ registry_ };
  dsp::ProcessorParameter *                   second_param_ = nullptr;
  std::unique_ptr<undo::UndoStack>            undo_stack_;
  std::unique_ptr<ProcessorParameterOperator> operator_;
};

TEST_F (ParameterOperatorTest, SetValue)
{
  // Initial state
  EXPECT_FLOAT_EQ (param_->baseValue (), 0.5f);
  EXPECT_EQ (undo_stack_->count (), 0);

  // Set new value
  operator_->setValue (param_, 0.75f);
  EXPECT_FLOAT_EQ (param_->baseValue (), 0.75f);
  EXPECT_EQ (undo_stack_->count (), 1);

  // Undo
  undo_stack_->undo ();
  EXPECT_FLOAT_EQ (param_->baseValue (), 0.5f);

  // Redo
  undo_stack_->redo ();
  EXPECT_FLOAT_EQ (param_->baseValue (), 0.75f);
}

TEST_F (ParameterOperatorTest, SetValueRejectsNullParameter)
{
  operator_->setValue (nullptr, 0.5f);
  EXPECT_EQ (undo_stack_->count (), 0);
}

TEST_F (ParameterOperatorTest, SetValueSameValue)
{
  // Set same value - should still create command
  operator_->setValue (param_, 0.5f);
  EXPECT_FLOAT_EQ (param_->baseValue (), 0.5f);
  EXPECT_EQ (undo_stack_->count (), 1);
}

TEST_F (ParameterOperatorTest, SetValueBoundaryValues)
{
  // Test minimum boundary
  operator_->setValue (param_, 0.0f);
  EXPECT_FLOAT_EQ (param_->baseValue (), 0.0f);
  EXPECT_EQ (undo_stack_->count (), 1);

  // Undo and test maximum boundary
  undo_stack_->undo ();
  operator_->setValue (param_, 1.0f);
  EXPECT_FLOAT_EQ (param_->baseValue (), 1.0f);
  EXPECT_EQ (undo_stack_->count (), 1);
}

TEST_F (ParameterOperatorTest, SetValueNegative)
{
  // Parameter should clamp negative values to 0
  operator_->setValue (param_, -0.5f);
  EXPECT_FLOAT_EQ (param_->baseValue (), 0.0f);
  EXPECT_EQ (undo_stack_->count (), 1);
}

TEST_F (ParameterOperatorTest, SetValueGreaterThanOne)
{
  // Parameter should clamp values > 1 to 1
  operator_->setValue (param_, 1.5f);
  EXPECT_FLOAT_EQ (param_->baseValue (), 1.0f);
  EXPECT_EQ (undo_stack_->count (), 1);
}

// A control drag pushes one command per change; quick successive edits of
// the same parameter coalesce into a single undo step
TEST_F (ParameterOperatorTest, SameParameterEditsMerge)
{
  operator_->setValue (param_, 0.6f);
  operator_->setValue (param_, 0.8f);
  EXPECT_FLOAT_EQ (param_->baseValue (), 0.8f);
  EXPECT_EQ (undo_stack_->count (), 1);

  undo_stack_->undo ();
  EXPECT_FLOAT_EQ (param_->baseValue (), 0.5f);
}

// Quick edits of different parameters must never merge: the merged command
// would apply the other parameter's value and the edit would fall out of
// the undo stack
TEST_F (ParameterOperatorTest, DifferentParametersDoNotMerge)
{
  operator_->setValue (param_, 0.7f);
  operator_->setValue (second_param_, 0.3f);
  EXPECT_FLOAT_EQ (param_->baseValue (), 0.7f);
  EXPECT_FLOAT_EQ (second_param_->baseValue (), 0.3f);
  EXPECT_EQ (undo_stack_->count (), 2);

  undo_stack_->undo ();
  EXPECT_FLOAT_EQ (second_param_->baseValue (), 0.5f);
  EXPECT_FLOAT_EQ (param_->baseValue (), 0.7f);

  undo_stack_->undo ();
  EXPECT_FLOAT_EQ (param_->baseValue (), 0.5f);
}

// The command references the parameter through the registry, so undo/redo
// keep working after the last external reference is gone. This mirrors
// editing a parameter and then removing its plugin from the rack: the
// removal command keeps the plugin alive, and the edit command keeps the
// parameter alive independently.
TEST_F (ParameterOperatorTest, EditRemainsUndoableAfterExternalReferencesDropped)
{
  operator_->setValue (param_, 0.75f);
  ASSERT_EQ (undo_stack_->count (), 1);

  const auto param_id = param_->get_uuid ();
  param_ = nullptr;
  param_ref_ = dsp::ProcessorParameterUuidReference{ registry_ };

  dsp::ProcessorParameterUuidReference resolved (param_id, registry_);
  auto *                               param = resolved.get ();
  ASSERT_NE (param, nullptr);

  undo_stack_->undo ();
  EXPECT_FLOAT_EQ (param->baseValue (), 0.5f);

  undo_stack_->redo ();
  EXPECT_FLOAT_EQ (param->baseValue (), 0.75f);
}

// Parameters registered in another registry are refused: the command's
// registry reference would throw for an unknown UUID, and the exception
// must not escape into QML
TEST_F (ParameterOperatorTest, SetValueRefusesForeignRegistryParameter)
{
  utils::ObjectRegistry other_registry;
  auto foreign_ref = utils::create_object<dsp::ProcessorParameter> (
    other_registry, other_registry,
    dsp::ProcessorParameter::UniqueId (u8"foreign_param"),
    dsp::ParameterRange (dsp::ParameterRange::Type::Linear, 0.f, 1.f, 0.f, 0.5f),
    utils::Utf8String::from_utf8_encoded_string ("Foreign Parameter"));
  auto * foreign = foreign_ref.get_object_as<dsp::ProcessorParameter> ();
  ASSERT_NE (foreign, nullptr);

  operator_->setValue (foreign, 0.9f);
  EXPECT_FLOAT_EQ (foreign->baseValue (), 0.5f);
  EXPECT_EQ (undo_stack_->count (), 0);
}

} // namespace zrythm::actions
