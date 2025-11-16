// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "commands/add_plugin_command.h"
#include "plugins/internal_plugin_base.h"
#include "plugins/plugin_group.h"

#include <gtest/gtest.h>

namespace zrythm::commands
{

class AddPluginCommandTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    // Create a test plugin group
    plugin_group_ = std::make_unique<plugins::PluginGroup> (
      dsp::ProcessorBase::ProcessorBaseDependencies{
        .port_registry_ = port_registry_, .param_registry_ = param_registry_ },
      plugin_registry_, plugins::PluginGroup::DeviceGroupType::Audio,
      plugins::PluginGroup::ProcessingTypeHint::Parallel);

    // Create a test plugin
    create_test_plugin ();
  }

  void TearDown () override
  {
    // Clean up in reverse order of creation
    plugin_group_.reset ();
  }

  void create_test_plugin ()
  {
    // Create a simple test plugin
    test_plugin_ref_ = plugin_registry_.create_object<
      plugins::InternalPluginBase> (
      dsp::ProcessorBase::ProcessorBaseDependencies{
        .port_registry_ = port_registry_, .param_registry_ = param_registry_ },
      [] () { return fs::path{ "/tmp/test_state" }; }, nullptr);
  }

  static auto
  get_plugin_id_at_index (const plugins::PluginGroup &group, int idx)
  {
    return group.element_at_idx (idx).value<plugins::Plugin *> ()->get_uuid ();
  }

  dsp::PortRegistry               port_registry_;
  dsp::ProcessorParameterRegistry param_registry_{ port_registry_ };
  plugins::PluginRegistry         plugin_registry_;

  std::unique_ptr<plugins::PluginGroup> plugin_group_;
  plugins::PluginUuidReference          test_plugin_ref_{ plugin_registry_ };
};

// Test initial state after construction
TEST_F (AddPluginCommandTest, InitialState)
{
  AddPluginCommand command (*plugin_group_, test_plugin_ref_);

  // Initially, plugin should not be in group
  EXPECT_EQ (plugin_group_->rowCount (), 0);
}

// Test redo operation adds plugin to group (append)
TEST_F (AddPluginCommandTest, RedoAddsPluginAppend)
{
  AddPluginCommand command (*plugin_group_, test_plugin_ref_);

  command.redo ();

  // After redo, plugin should be in group
  EXPECT_EQ (plugin_group_->rowCount (), 1);
  EXPECT_EQ (get_plugin_id_at_index (*plugin_group_, 0), test_plugin_ref_.id ());
}

// Test redo operation adds plugin to group (at specific index)
TEST_F (AddPluginCommandTest, RedoAddsPluginAtIndex)
{
  // Add some dummy plugins first
  auto dummy_plugin1 = plugin_registry_.create_object<
    plugins::InternalPluginBase> (
    dsp::ProcessorBase::ProcessorBaseDependencies{
      .port_registry_ = port_registry_, .param_registry_ = param_registry_ },
    [] () { return fs::path{ "/tmp/test_state" }; }, nullptr);
  auto dummy_plugin2 = plugin_registry_.create_object<
    plugins::InternalPluginBase> (
    dsp::ProcessorBase::ProcessorBaseDependencies{
      .port_registry_ = port_registry_, .param_registry_ = param_registry_ },
    [] () { return fs::path{ "/tmp/test_state" }; }, nullptr);

  plugin_group_->append_plugin (dummy_plugin1);
  plugin_group_->append_plugin (dummy_plugin2);

  // Insert test plugin at index 1
  AddPluginCommand command (*plugin_group_, test_plugin_ref_, 1);

  command.redo ();

  // Plugin should be at correct position
  EXPECT_EQ (plugin_group_->rowCount (), 3);
  EXPECT_EQ (get_plugin_id_at_index (*plugin_group_, 0), dummy_plugin1.id ());
  EXPECT_EQ (get_plugin_id_at_index (*plugin_group_, 1), test_plugin_ref_.id ());
  EXPECT_EQ (get_plugin_id_at_index (*plugin_group_, 2), dummy_plugin2.id ());
}

// Test undo operation removes plugin from group
TEST_F (AddPluginCommandTest, UndoRemovesPlugin)
{
  AddPluginCommand command (*plugin_group_, test_plugin_ref_);

  // First add plugin
  command.redo ();
  EXPECT_EQ (plugin_group_->rowCount (), 1);
  EXPECT_EQ (get_plugin_id_at_index (*plugin_group_, 0), test_plugin_ref_.id ());

  // Then undo should remove it
  command.undo ();
  EXPECT_EQ (plugin_group_->rowCount (), 0);
}

// Test multiple undo/redo cycles
TEST_F (AddPluginCommandTest, MultipleUndoRedoCycles)
{
  AddPluginCommand command (*plugin_group_, test_plugin_ref_);

  // First cycle
  command.redo ();
  EXPECT_EQ (plugin_group_->rowCount (), 1);
  EXPECT_EQ (get_plugin_id_at_index (*plugin_group_, 0), test_plugin_ref_.id ());

  command.undo ();
  EXPECT_EQ (plugin_group_->rowCount (), 0);

  // Second cycle
  command.redo ();
  EXPECT_EQ (plugin_group_->rowCount (), 1);
  EXPECT_EQ (get_plugin_id_at_index (*plugin_group_, 0), test_plugin_ref_.id ());

  command.undo ();
  EXPECT_EQ (plugin_group_->rowCount (), 0);
}

// Test command text
TEST_F (AddPluginCommandTest, CommandText)
{
  AddPluginCommand command (*plugin_group_, test_plugin_ref_);

  // The command should have default text "Add Plugin"
  EXPECT_EQ (command.text (), QString ("Add Plugin"));
}

// Test command ID
TEST_F (AddPluginCommandTest, CommandId)
{
  AddPluginCommand command (*plugin_group_, test_plugin_ref_);

  // The command should have the correct ID
  EXPECT_EQ (command.id (), AddPluginCommand::CommandId);
}

// Test adding multiple plugins to same group
TEST_F (AddPluginCommandTest, MultiplePluginsSameGroup)
{
  // Create second plugin
  auto second_plugin_ref = plugin_registry_.create_object<
    plugins::InternalPluginBase> (
    dsp::ProcessorBase::ProcessorBaseDependencies{
      .port_registry_ = port_registry_, .param_registry_ = param_registry_ },
    [] () { return fs::path{ "/tmp/test_state" }; }, nullptr);

  AddPluginCommand command1 (*plugin_group_, test_plugin_ref_);
  AddPluginCommand command2 (*plugin_group_, second_plugin_ref);

  // Add first plugin
  command1.redo ();
  EXPECT_EQ (plugin_group_->rowCount (), 1);
  EXPECT_EQ (get_plugin_id_at_index (*plugin_group_, 0), test_plugin_ref_.id ());

  // Add second plugin
  command2.redo ();
  EXPECT_EQ (plugin_group_->rowCount (), 2);
  EXPECT_EQ (get_plugin_id_at_index (*plugin_group_, 0), test_plugin_ref_.id ());
  EXPECT_EQ (
    get_plugin_id_at_index (*plugin_group_, 1), second_plugin_ref.id ());

  // Undo second plugin
  command2.undo ();
  EXPECT_EQ (plugin_group_->rowCount (), 1);
  EXPECT_EQ (get_plugin_id_at_index (*plugin_group_, 0), test_plugin_ref_.id ());

  // Undo first plugin
  command1.undo ();
  EXPECT_EQ (plugin_group_->rowCount (), 0);
}

// Test adding plugin at index beyond end (should append)
TEST_F (AddPluginCommandTest, AddPluginAtIndexBeyondEnd)
{
  // Add a dummy plugin first
  auto dummy_plugin = plugin_registry_.create_object<plugins::InternalPluginBase> (
    dsp::ProcessorBase::ProcessorBaseDependencies{
      .port_registry_ = port_registry_, .param_registry_ = param_registry_ },
    [] () { return fs::path{ "/tmp/test_state" }; }, nullptr);

  plugin_group_->append_plugin (dummy_plugin);

  // Try to insert at index 999 (should append to end)
  AddPluginCommand command (*plugin_group_, test_plugin_ref_, 999);

  command.redo ();

  // Plugin should be at end
  EXPECT_EQ (plugin_group_->rowCount (), 2);
  EXPECT_EQ (get_plugin_id_at_index (*plugin_group_, 0), dummy_plugin.id ());
  EXPECT_EQ (get_plugin_id_at_index (*plugin_group_, 1), test_plugin_ref_.id ());
}

// Test adding plugin at index 0 (should prepend)
TEST_F (AddPluginCommandTest, AddPluginAtStart)
{
  // Add a dummy plugin first
  auto dummy_plugin = plugin_registry_.create_object<plugins::InternalPluginBase> (
    dsp::ProcessorBase::ProcessorBaseDependencies{
      .port_registry_ = port_registry_, .param_registry_ = param_registry_ },
    [] () { return fs::path{ "/tmp/test_state" }; }, nullptr);

  plugin_group_->append_plugin (dummy_plugin);

  // Insert at index 0 (should prepend)
  AddPluginCommand command (*plugin_group_, test_plugin_ref_, 0);

  command.redo ();

  // Test plugin should be at start
  EXPECT_EQ (plugin_group_->rowCount (), 2);
  EXPECT_EQ (get_plugin_id_at_index (*plugin_group_, 0), test_plugin_ref_.id ());
  EXPECT_EQ (get_plugin_id_at_index (*plugin_group_, 1), dummy_plugin.id ());
}

// Test with different plugin group types
TEST_F (AddPluginCommandTest, DifferentPluginGroupTypes)
{
  // Test with MIDI plugin group
  auto midi_plugin_group = std::make_unique<plugins::PluginGroup> (
    dsp::ProcessorBase::ProcessorBaseDependencies{
      .port_registry_ = port_registry_, .param_registry_ = param_registry_ },
    plugin_registry_, plugins::PluginGroup::DeviceGroupType::MIDI,
    plugins::PluginGroup::ProcessingTypeHint::Serial);

  AddPluginCommand midi_command (*midi_plugin_group, test_plugin_ref_);

  midi_command.redo ();
  EXPECT_EQ (midi_plugin_group->rowCount (), 1);
  EXPECT_EQ (
    get_plugin_id_at_index (*midi_plugin_group, 0), test_plugin_ref_.id ());

  midi_command.undo ();
  EXPECT_EQ (midi_plugin_group->rowCount (), 0);

  // Test with Instrument plugin group
  auto instrument_plugin_group = std::make_unique<plugins::PluginGroup> (
    dsp::ProcessorBase::ProcessorBaseDependencies{
      .port_registry_ = port_registry_, .param_registry_ = param_registry_ },
    plugin_registry_, plugins::PluginGroup::DeviceGroupType::Instrument,
    plugins::PluginGroup::ProcessingTypeHint::Serial);

  AddPluginCommand instrument_command (
    *instrument_plugin_group, test_plugin_ref_);

  instrument_command.redo ();
  EXPECT_EQ (instrument_plugin_group->rowCount (), 1);
  EXPECT_EQ (
    get_plugin_id_at_index (*instrument_plugin_group, 0),
    test_plugin_ref_.id ());

  instrument_command.undo ();
  EXPECT_EQ (instrument_plugin_group->rowCount (), 0);
}

// Test adding plugin to empty group with specific index
TEST_F (AddPluginCommandTest, AddToEmptyGroupWithIndex)
{
  // Insert at index 5 in empty group (should just append)
  AddPluginCommand command (*plugin_group_, test_plugin_ref_, 5);

  command.redo ();

  // Plugin should be added at index 0 (only position available)
  EXPECT_EQ (plugin_group_->rowCount (), 1);
  EXPECT_EQ (get_plugin_id_at_index (*plugin_group_, 0), test_plugin_ref_.id ());
}

// Test plugin reference is properly moved
TEST_F (AddPluginCommandTest, PluginReferenceMoved)
{
  auto plugin_id = test_plugin_ref_.id ();

  AddPluginCommand command (*plugin_group_, std::move (test_plugin_ref_));

  command.redo ();

  // Plugin should still be accessible via the group
  EXPECT_EQ (plugin_group_->rowCount (), 1);
  EXPECT_EQ (get_plugin_id_at_index (*plugin_group_, 0), plugin_id);
}

} // namespace zrythm::commands
