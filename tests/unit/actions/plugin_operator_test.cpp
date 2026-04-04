// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/plugin_operator.h"
#include "dsp/processor_base.h"
#include "plugins/plugin_group.h"
#include "structure/tracks/track.h"
#include "undo/undo_stack.h"

#include "unit/actions/mock_undo_stack.h"
#include <gtest/gtest.h>

namespace zrythm::actions
{

class PluginOperatorTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    undo_stack_ = create_mock_undo_stack ();
    plugin_operator_ =
      std::make_unique<PluginOperator> (*undo_stack_, plugin_registry_);

    // Create source and target plugin groups
    auto make_deps = [&] {
      return dsp::ProcessorBase::ProcessorBaseDependencies{
        .port_registry_ = port_registry_, .param_registry_ = param_registry_
      };
    };

    source_group_ = std::make_unique<plugins::PluginGroup> (
      make_deps (), plugin_registry_,
      plugins::PluginGroup::DeviceGroupType::Audio,
      plugins::PluginGroup::ProcessingTypeHint::Parallel);
    target_group_ = std::make_unique<plugins::PluginGroup> (
      make_deps (), plugin_registry_,
      plugins::PluginGroup::DeviceGroupType::Audio,
      plugins::PluginGroup::ProcessingTypeHint::Parallel);
  }

  void TearDown () override
  {
    plugin_operator_.reset ();
    undo_stack_.reset ();
    target_group_.reset ();
    source_group_.reset ();
  }

  plugins::Plugin * create_and_append_plugin (plugins::PluginGroup &group)
  {
    auto ref = plugin_registry_.create_object<plugins::InternalPluginBase> (
      dsp::ProcessorBase::ProcessorBaseDependencies{
        .port_registry_ = port_registry_, .param_registry_ = param_registry_ },
      [] () { return fs::path (); }, nullptr);
    group.append_plugin (ref);
    return ref.get_object_as<plugins::InternalPluginBase> ();
  }

  static auto
  get_plugin_id_at_index (const plugins::PluginGroup &group, int idx)
  {
    return group.element_at_idx (idx).value<plugins::Plugin *> ()->get_uuid ();
  }

  dsp::PortRegistry               port_registry_;
  dsp::ProcessorParameterRegistry param_registry_{ port_registry_ };
  plugins::PluginRegistry         plugin_registry_;

  std::unique_ptr<undo::UndoStack> undo_stack_;
  std::unique_ptr<PluginOperator>  plugin_operator_;

  std::unique_ptr<plugins::PluginGroup> source_group_;
  std::unique_ptr<plugins::PluginGroup> target_group_;
};

// --- Basic move ---

TEST_F (PluginOperatorTest, MoveSinglePluginBetweenGroups)
{
  auto * pl = create_and_append_plugin (*source_group_);
  ASSERT_EQ (source_group_->rowCount (), 1);

  plugin_operator_->movePlugins (
    { pl }, source_group_.get (), nullptr, target_group_.get (), nullptr, -1);

  EXPECT_EQ (source_group_->rowCount (), 0);
  ASSERT_EQ (target_group_->rowCount (), 1);
  EXPECT_EQ (get_plugin_id_at_index (*target_group_, 0), pl->get_uuid ());
  EXPECT_EQ (undo_stack_->count (), 1);
}

TEST_F (PluginOperatorTest, MoveSinglePluginToSpecificIndex)
{
  auto * pl = create_and_append_plugin (*source_group_);
  auto * dummy1 = create_and_append_plugin (*target_group_);
  auto * dummy2 = create_and_append_plugin (*target_group_);

  plugin_operator_->movePlugins (
    { pl }, source_group_.get (), nullptr, target_group_.get (), nullptr, 1);

  ASSERT_EQ (target_group_->rowCount (), 3);
  EXPECT_EQ (get_plugin_id_at_index (*target_group_, 0), dummy1->get_uuid ());
  EXPECT_EQ (get_plugin_id_at_index (*target_group_, 1), pl->get_uuid ());
  EXPECT_EQ (get_plugin_id_at_index (*target_group_, 2), dummy2->get_uuid ());
}

TEST_F (PluginOperatorTest, MoveMultiplePlugins)
{
  auto * pl0 = create_and_append_plugin (*source_group_);
  auto * pl1 = create_and_append_plugin (*source_group_);
  auto * pl2 = create_and_append_plugin (*source_group_);

  plugin_operator_->movePlugins (
    { pl0, pl1, pl2 }, source_group_.get (), nullptr, target_group_.get (),
    nullptr, -1);

  EXPECT_EQ (source_group_->rowCount (), 0);
  ASSERT_EQ (target_group_->rowCount (), 3);
  EXPECT_EQ (get_plugin_id_at_index (*target_group_, 0), pl0->get_uuid ());
  EXPECT_EQ (get_plugin_id_at_index (*target_group_, 1), pl1->get_uuid ());
  EXPECT_EQ (get_plugin_id_at_index (*target_group_, 2), pl2->get_uuid ());
}

// --- Undo/Redo ---

TEST_F (PluginOperatorTest, UndoSingleMove)
{
  auto * pl = create_and_append_plugin (*source_group_);

  plugin_operator_->movePlugins (
    { pl }, source_group_.get (), nullptr, target_group_.get (), nullptr, -1);
  ASSERT_EQ (target_group_->rowCount (), 1);

  undo_stack_->undo ();

  EXPECT_EQ (target_group_->rowCount (), 0);
  ASSERT_EQ (source_group_->rowCount (), 1);
  EXPECT_EQ (get_plugin_id_at_index (*source_group_, 0), pl->get_uuid ());
}

TEST_F (PluginOperatorTest, RedoSingleMove)
{
  auto * pl = create_and_append_plugin (*source_group_);

  plugin_operator_->movePlugins (
    { pl }, source_group_.get (), nullptr, target_group_.get (), nullptr, -1);
  undo_stack_->undo ();
  undo_stack_->redo ();

  EXPECT_EQ (source_group_->rowCount (), 0);
  ASSERT_EQ (target_group_->rowCount (), 1);
  EXPECT_EQ (get_plugin_id_at_index (*target_group_, 0), pl->get_uuid ());
}

TEST_F (PluginOperatorTest, UndoRedoMultiplePlugins)
{
  auto * pl0 = create_and_append_plugin (*source_group_);
  auto * pl1 = create_and_append_plugin (*source_group_);
  auto * stay = create_and_append_plugin (*source_group_);

  plugin_operator_->movePlugins (
    { pl0, pl1 }, source_group_.get (), nullptr, target_group_.get (), nullptr,
    -1);

  // Verify redo state
  ASSERT_EQ (source_group_->rowCount (), 1);
  EXPECT_EQ (get_plugin_id_at_index (*source_group_, 0), stay->get_uuid ());
  ASSERT_EQ (target_group_->rowCount (), 2);

  // Undo
  undo_stack_->undo ();
  ASSERT_EQ (source_group_->rowCount (), 3);
  EXPECT_EQ (get_plugin_id_at_index (*source_group_, 0), pl0->get_uuid ());
  EXPECT_EQ (get_plugin_id_at_index (*source_group_, 1), pl1->get_uuid ());
  EXPECT_EQ (get_plugin_id_at_index (*source_group_, 2), stay->get_uuid ());
  EXPECT_EQ (target_group_->rowCount (), 0);

  // Redo
  undo_stack_->redo ();
  ASSERT_EQ (source_group_->rowCount (), 1);
  ASSERT_EQ (target_group_->rowCount (), 2);
  EXPECT_EQ (get_plugin_id_at_index (*target_group_, 0), pl0->get_uuid ());
  EXPECT_EQ (get_plugin_id_at_index (*target_group_, 1), pl1->get_uuid ());
}

TEST_F (PluginOperatorTest, MultipleUndoRedoCycles)
{
  auto * pl = create_and_append_plugin (*source_group_);

  plugin_operator_->movePlugins (
    { pl }, source_group_.get (), nullptr, target_group_.get (), nullptr, -1);

  for (int i = 0; i < 3; ++i)
    {
      undo_stack_->undo ();
      EXPECT_EQ (source_group_->rowCount (), 1);
      EXPECT_EQ (target_group_->rowCount (), 0);

      undo_stack_->redo ();
      EXPECT_EQ (source_group_->rowCount (), 0);
      EXPECT_EQ (target_group_->rowCount (), 1);
    }
}

// --- Append to end ---

TEST_F (PluginOperatorTest, AppendToEndWithNegativeIndex)
{
  auto * pl = create_and_append_plugin (*source_group_);
  auto * dummy = create_and_append_plugin (*target_group_);

  plugin_operator_->movePlugins (
    { pl }, source_group_.get (), nullptr, target_group_.get (), nullptr, -1);

  ASSERT_EQ (target_group_->rowCount (), 2);
  EXPECT_EQ (get_plugin_id_at_index (*target_group_, 0), dummy->get_uuid ());
  EXPECT_EQ (get_plugin_id_at_index (*target_group_, 1), pl->get_uuid ());
}

// --- Command text ---

TEST_F (PluginOperatorTest, CommandTextSinglePlugin)
{
  auto * pl = create_and_append_plugin (*source_group_);

  plugin_operator_->movePlugins (
    { pl }, source_group_.get (), nullptr, target_group_.get (), nullptr, -1);

  EXPECT_EQ (undo_stack_->text (0), QString ("Move Plugin"));
}

TEST_F (PluginOperatorTest, CommandTextMultiplePlugins)
{
  auto * pl0 = create_and_append_plugin (*source_group_);
  auto * pl1 = create_and_append_plugin (*source_group_);

  plugin_operator_->movePlugins (
    { pl0, pl1 }, source_group_.get (), nullptr, target_group_.get (), nullptr,
    -1);

  EXPECT_EQ (undo_stack_->text (0), QString ("Move 2 Plugin(s)"));
}

// --- Empty/null handling ---

TEST_F (PluginOperatorTest, EmptyPluginListDoesNothing)
{
  plugin_operator_->movePlugins (
    {}, source_group_.get (), nullptr, target_group_.get (), nullptr, -1);

  EXPECT_EQ (undo_stack_->count (), 0);
}

// --- Sequential moves ---

TEST_F (PluginOperatorTest, SequentialMovesWithUndo)
{
  auto * pl0 = create_and_append_plugin (*source_group_);
  auto * pl1 = create_and_append_plugin (*source_group_);

  // First move
  plugin_operator_->movePlugins (
    { pl0 }, source_group_.get (), nullptr, target_group_.get (), nullptr, -1);
  EXPECT_EQ (undo_stack_->count (), 1);

  // Second move
  plugin_operator_->movePlugins (
    { pl1 }, source_group_.get (), nullptr, target_group_.get (), nullptr, -1);
  EXPECT_EQ (undo_stack_->count (), 2);

  // Undo second
  undo_stack_->undo ();
  ASSERT_EQ (source_group_->rowCount (), 1);
  EXPECT_EQ (get_plugin_id_at_index (*source_group_, 0), pl1->get_uuid ());
  ASSERT_EQ (target_group_->rowCount (), 1);
  EXPECT_EQ (get_plugin_id_at_index (*target_group_, 0), pl0->get_uuid ());

  // Undo first
  undo_stack_->undo ();
  ASSERT_EQ (source_group_->rowCount (), 2);
  EXPECT_EQ (target_group_->rowCount (), 0);
}

} // namespace zrythm::actions
