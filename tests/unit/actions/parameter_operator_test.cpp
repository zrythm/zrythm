// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/parameter_operator.h"
#include "dsp/parameter.h"
#include "undo/undo_stack.h"
#include "utils/object_registry.h"
#include "utils/registry_utils.h"

#include "helpers/project_fixture.h"

#include "operations/builtin_operations.h"
#include "operations/operation_dispatcher.h"
#include "operations/operation_environment.h"
#include "operations/operation_registry.h"
#include "unit/actions/mock_undo_stack.h"
#include <gtest/gtest.h>

namespace zrythm::actions
{

class ParameterOperatorTest : public test_helpers::ProjectTestFixture
{
protected:
  /** Registers a processor parameter in the project's registry. */
  dsp::ProcessorParameter * add_parameter (const char8_t * unique_id)
  {
    auto ref = utils::create_object<dsp::ProcessorParameter> (
      project_->get_registry (), project_->get_registry (),
      dsp::ProcessorParameter::UniqueId (unique_id),
      dsp::ParameterRange (
        dsp::ParameterRange::Type::Linear, 0.f, 1.f, 0.f, 0.5f),
      utils::Utf8String{ unique_id });
    parameter_refs_.push_back (ref.as_untyped ());
    return ref.get ();
  }

  void SetUp () override
  {
    ProjectTestFixture::SetUp ();
    project_ = create_minimal_project ();
    undo_stack_ = create_mock_undo_stack ();
    operations::register_builtin_operations (operation_registry_);
    environment_ = std::make_unique<operations::OperationEnvironment> (
      project_->get_registry (), *undo_stack_,
      *project_->tracklist ()->collection (), *project_->track_factory_,
      *project_->tracklist ()->trackRouting (),
      *project_->tracklist ()->channelSendRouting (),
      *project_->tracklist ()->singletonTracks (),
      *project_->port_connections_manager_, project_->getTransport ());
    dispatcher_ = std::make_unique<operations::OperationDispatcher> (
      operation_registry_, *environment_);
    operator_ = std::make_unique<ProcessorParameterOperator> (*dispatcher_);

    param_ = add_parameter (u8"test_param");
    second_param_ = add_parameter (u8"second_param");
    ASSERT_NE (param_, nullptr);
    ASSERT_NE (second_param_, nullptr);

    param_->setBaseValue (0.5f);
    second_param_->setBaseValue (0.5f);
  }

  void TearDown () override
  {
    operator_.reset ();
    dispatcher_.reset ();
    environment_.reset ();
    undo_stack_.reset ();
    // release the parameter references while the project registry they
    // point into is still alive
    parameter_refs_.clear ();
    project_.reset ();
  }

  std::unique_ptr<structure::project::Project>      project_;
  std::unique_ptr<undo::UndoStack>                  undo_stack_;
  operations::OperationRegistry                     operation_registry_;
  std::unique_ptr<operations::OperationEnvironment> environment_;
  std::unique_ptr<operations::OperationDispatcher>  dispatcher_;
  std::unique_ptr<ProcessorParameterOperator>       operator_;
  dsp::ProcessorParameter *                         param_ = nullptr;
  dsp::ProcessorParameter *                         second_param_ = nullptr;
  std::vector<utils::UuidReference>                 parameter_refs_;
};

TEST_F (ParameterOperatorTest, SetValue)
{
  // Initial state
  EXPECT_FLOAT_EQ (param_->baseValue (), 0.5f);
  EXPECT_EQ (undo_stack_->count (), 0);

  // Set new value
  EXPECT_TRUE (operator_->setValue (param_, 0.75f).isEmpty ());
  EXPECT_FLOAT_EQ (param_->baseValue (), 0.75f);
  EXPECT_EQ (undo_stack_->count (), 1);

  // Undo
  undo_stack_->undo ();
  EXPECT_FLOAT_EQ (param_->baseValue (), 0.5f);

  // Redo
  undo_stack_->redo ();
  EXPECT_FLOAT_EQ (param_->baseValue (), 0.75f);
}

TEST_F (ParameterOperatorTest, SetValueRefusesNullParameter)
{
  EXPECT_FALSE (operator_->setValue (nullptr, 0.5f).isEmpty ());
  EXPECT_EQ (undo_stack_->count (), 0);
}

// Setting the current value applies without growing the undo stack
TEST_F (ParameterOperatorTest, SetValueElidesSameValue)
{
  EXPECT_TRUE (operator_->setValue (param_, 0.5f).isEmpty ());
  EXPECT_FLOAT_EQ (param_->baseValue (), 0.5f);
  EXPECT_EQ (undo_stack_->count (), 0);
}

TEST_F (ParameterOperatorTest, SetValueBoundaryValues)
{
  // Test minimum boundary
  EXPECT_TRUE (operator_->setValue (param_, 0.0f).isEmpty ());
  EXPECT_FLOAT_EQ (param_->baseValue (), 0.0f);
  EXPECT_EQ (undo_stack_->count (), 1);

  // Undo and test maximum boundary
  undo_stack_->undo ();
  EXPECT_TRUE (operator_->setValue (param_, 1.0f).isEmpty ());
  EXPECT_FLOAT_EQ (param_->baseValue (), 1.0f);
  EXPECT_EQ (undo_stack_->count (), 1);
}

// Values outside the normalized 0.0-1.0 range are refused
TEST_F (ParameterOperatorTest, SetValueRefusesOutOfRangeValues)
{
  EXPECT_FALSE (operator_->setValue (param_, -0.5f).isEmpty ());
  EXPECT_FLOAT_EQ (param_->baseValue (), 0.5f);
  EXPECT_FALSE (operator_->setValue (param_, 1.5f).isEmpty ());
  EXPECT_FLOAT_EQ (param_->baseValue (), 0.5f);
  EXPECT_EQ (undo_stack_->count (), 0);
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
  EXPECT_TRUE (operator_->setValue (param_, 0.75f).isEmpty ());
  ASSERT_EQ (undo_stack_->count (), 1);

  const auto param_id = param_->get_uuid ();
  param_ = nullptr;
  parameter_refs_.clear ();

  dsp::ProcessorParameterUuidReference resolved (
    param_id, project_->get_registry ());
  auto * param = resolved.get ();
  ASSERT_NE (param, nullptr);

  undo_stack_->undo ();
  EXPECT_FLOAT_EQ (param->baseValue (), 0.5f);

  undo_stack_->redo ();
  EXPECT_FLOAT_EQ (param->baseValue (), 0.75f);
}

// Parameters registered in another registry are refused
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
  foreign->setBaseValue (0.5f);

  EXPECT_FALSE (operator_->setValue (foreign, 0.9f).isEmpty ());
  EXPECT_FLOAT_EQ (foreign->baseValue (), 0.5f);
  EXPECT_EQ (undo_stack_->count (), 0);
}

// Live values apply immediately and never appear on the undo stack
TEST_F (ParameterOperatorTest, SetLiveValueAppliesWithoutUndoEntry)
{
  EXPECT_TRUE (operator_->setLiveValue (param_, 1.0).isEmpty ());
  EXPECT_FLOAT_EQ (param_->baseValue (), 1.f);
  EXPECT_EQ (undo_stack_->count (), 0);

  undo_stack_->undo ();
  EXPECT_FLOAT_EQ (param_->baseValue (), 1.f);
}

TEST_F (ParameterOperatorTest, SetLiveValueRefusesNullParameter)
{
  EXPECT_FALSE (operator_->setLiveValue (nullptr, 0.5f).isEmpty ());
  EXPECT_EQ (undo_stack_->count (), 0);
}

// Parameters registered in another registry are refused, so stale
// references into a closed project cannot be written
TEST_F (ParameterOperatorTest, SetLiveValueRefusesForeignRegistryParameter)
{
  utils::ObjectRegistry other_registry;
  auto foreign_ref = utils::create_object<dsp::ProcessorParameter> (
    other_registry, other_registry,
    dsp::ProcessorParameter::UniqueId (u8"foreign_live_param"),
    dsp::ParameterRange (dsp::ParameterRange::Type::Linear, 0.f, 1.f, 0.f, 0.5f),
    utils::Utf8String::from_utf8_encoded_string ("Foreign Live Parameter"));
  auto * foreign = foreign_ref.get_object_as<dsp::ProcessorParameter> ();
  ASSERT_NE (foreign, nullptr);
  foreign->setBaseValue (0.5f);

  EXPECT_FALSE (operator_->setLiveValue (foreign, 0.9f).isEmpty ());
  EXPECT_FLOAT_EQ (foreign->baseValue (), 0.5f);
}

} // namespace zrythm::actions
