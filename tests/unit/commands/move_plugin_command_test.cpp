// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "commands/move_plugin_command.h"
#include "dsp/parameter.h"
#include "dsp/processor_base.h"
#include "plugins/plugin.h"
#include "plugins/plugin_group.h"
#include "structure/tracks/automation_tracklist.h"

#include <gtest/gtest.h>

namespace zrythm::commands
{

class MovePluginCommandTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    // Create source and target plugin lists
    source_plugin_list_ = std::make_unique<plugins::PluginGroup> (
      dsp::ProcessorBase::ProcessorBaseDependencies{
        .port_registry_ = port_registry_, .param_registry_ = param_registry_ },
      plugin_registry_, plugins::PluginGroup::DeviceGroupType::Audio,
      plugins::PluginGroup::ProcessingTypeHint::Parallel);
    target_plugin_list_ = std::make_unique<plugins::PluginGroup> (
      dsp::ProcessorBase::ProcessorBaseDependencies{
        .port_registry_ = port_registry_, .param_registry_ = param_registry_ },
      plugin_registry_, plugins::PluginGroup::DeviceGroupType::Audio,
      plugins::PluginGroup::ProcessingTypeHint::Parallel);

    // Create source and target automation tracklists
    source_automation_tracklist_ = std::make_unique<
      structure::tracks::AutomationTracklist> (automation_tracklist_deps_);
    target_automation_tracklist_ = std::make_unique<
      structure::tracks::AutomationTracklist> (automation_tracklist_deps_);

    // Create a test plugin
    create_test_plugin ();
  }

  void TearDown () override
  {
    // Clean up in reverse order of creation
    target_automation_tracklist_.reset ();
    source_automation_tracklist_.reset ();
    target_plugin_list_.reset ();
    source_plugin_list_.reset ();
  }

  void create_test_plugin ()
  {
    // Create a simple test plugin with a parameter
    auto param_ref = param_registry_.create_object<dsp::ProcessorParameter> (
      port_registry_, dsp::ProcessorParameter::UniqueId (u8"test_param"),
      dsp::ParameterRange (
        dsp::ParameterRange::Type::Linear, 0.f, 1.f, 0.f, 0.5f),
      utils::Utf8String::from_utf8_encoded_string ("Test Parameter"));

    // Create a mock plugin (simplified for testing)
    // In a real test, we would use a proper mock plugin factory
    test_plugin_ref_ = plugin_registry_.create_object<
      plugins::InternalPluginBase> (
      dsp::ProcessorBase::ProcessorBaseDependencies{
        .port_registry_ = port_registry_, .param_registry_ = param_registry_ },
      [] () { return fs::path (); }, nullptr);

    // Add the plugin to the source list
    source_plugin_list_->append_plugin (test_plugin_ref_);
  }

  static auto
  get_plugin_id_at_index (const plugins::PluginGroup &group, int idx)
  {
    return group.element_at_idx (idx).value<plugins::Plugin *> ()->get_uuid ();
  }

  dsp::PortRegistry               port_registry_;
  dsp::ProcessorParameterRegistry param_registry_{ port_registry_ };
  plugins::PluginRegistry         plugin_registry_;

  std::unique_ptr<plugins::PluginGroup> source_plugin_list_;
  std::unique_ptr<plugins::PluginGroup> target_plugin_list_;

  // Automation tracklist dependencies
  dsp::TempoMap                tempo_map_{ units::sample_rate (44100) };
  dsp::TempoMapWrapper         tempo_map_wrapper_{ tempo_map_ };
  dsp::FileAudioSourceRegistry file_audio_source_registry_;
  structure::arrangement::ArrangerObjectRegistry object_registry_;
  structure::tracks::AutomationTrackHolder::Dependencies automation_tracklist_deps_{
    .tempo_map_ = tempo_map_wrapper_,
    .file_audio_source_registry_ = file_audio_source_registry_,
    .port_registry_ = port_registry_,
    .param_registry_ = param_registry_,
    .object_registry_ = object_registry_
  };

  std::unique_ptr<structure::tracks::AutomationTracklist>
    source_automation_tracklist_;
  std::unique_ptr<structure::tracks::AutomationTracklist>
    target_automation_tracklist_;

  plugins::PluginUuidReference test_plugin_ref_{ plugin_registry_ };
};

// Test initial state after construction
TEST_F (MovePluginCommandTest, InitialState)
{
  MovePluginCommand::PluginLocation source_location = {
    source_plugin_list_.get (), source_automation_tracklist_.get ()
  };
  MovePluginCommand::PluginLocation target_location = {
    target_plugin_list_.get (), target_automation_tracklist_.get ()
  };

  MovePluginCommand command (test_plugin_ref_, source_location, target_location);

  // Verify plugin is still in source list
  EXPECT_EQ (source_plugin_list_->rowCount (), 1);
  EXPECT_EQ (target_plugin_list_->rowCount (), 0);
  EXPECT_EQ (
    get_plugin_id_at_index (*source_plugin_list_, 0), test_plugin_ref_.id ());
}

// Test redo operation (move plugin from source to target)
TEST_F (MovePluginCommandTest, RedoOperation)
{
  MovePluginCommand::PluginLocation source_location = {
    source_plugin_list_.get (), source_automation_tracklist_.get ()
  };
  MovePluginCommand::PluginLocation target_location = {
    target_plugin_list_.get (), target_automation_tracklist_.get ()
  };

  MovePluginCommand command (test_plugin_ref_, source_location, target_location);

  // Initial state
  EXPECT_EQ (source_plugin_list_->rowCount (), 1);
  EXPECT_EQ (target_plugin_list_->rowCount (), 0);

  // Execute redo
  command.redo ();

  // Plugin should be moved to target list
  EXPECT_EQ (source_plugin_list_->rowCount (), 0);
  EXPECT_EQ (target_plugin_list_->rowCount (), 1);
  EXPECT_EQ (
    get_plugin_id_at_index (*target_plugin_list_, 0), test_plugin_ref_.id ());
}

// Test undo operation (move plugin back to source)
TEST_F (MovePluginCommandTest, UndoOperation)
{
  MovePluginCommand::PluginLocation source_location = {
    source_plugin_list_.get (), source_automation_tracklist_.get ()
  };
  MovePluginCommand::PluginLocation target_location = {
    target_plugin_list_.get (), target_automation_tracklist_.get ()
  };

  MovePluginCommand command (test_plugin_ref_, source_location, target_location);

  // First execute redo to move the plugin
  command.redo ();
  EXPECT_EQ (source_plugin_list_->rowCount (), 0);
  EXPECT_EQ (target_plugin_list_->rowCount (), 1);

  // Then undo
  command.undo ();

  // Plugin should be back in source list
  EXPECT_EQ (source_plugin_list_->rowCount (), 1);
  EXPECT_EQ (target_plugin_list_->rowCount (), 0);
  EXPECT_EQ (
    get_plugin_id_at_index (*source_plugin_list_, 0), test_plugin_ref_.id ());
}

// Test undo/redo cycle
TEST_F (MovePluginCommandTest, UndoRedoCycle)
{
  MovePluginCommand::PluginLocation source_location = {
    source_plugin_list_.get (), source_automation_tracklist_.get ()
  };
  MovePluginCommand::PluginLocation target_location = {
    target_plugin_list_.get (), target_automation_tracklist_.get ()
  };

  MovePluginCommand command (test_plugin_ref_, source_location, target_location);

  // Initial state
  EXPECT_EQ (source_plugin_list_->rowCount (), 1);
  EXPECT_EQ (target_plugin_list_->rowCount (), 0);

  // Redo
  command.redo ();
  EXPECT_EQ (source_plugin_list_->rowCount (), 0);
  EXPECT_EQ (target_plugin_list_->rowCount (), 1);

  // Undo
  command.undo ();
  EXPECT_EQ (source_plugin_list_->rowCount (), 1);
  EXPECT_EQ (target_plugin_list_->rowCount (), 0);

  // Redo again
  command.redo ();
  EXPECT_EQ (source_plugin_list_->rowCount (), 0);
  EXPECT_EQ (target_plugin_list_->rowCount (), 1);

  // Undo again
  command.undo ();
  EXPECT_EQ (source_plugin_list_->rowCount (), 1);
  EXPECT_EQ (target_plugin_list_->rowCount (), 0);
}

// Test move to specific index
TEST_F (MovePluginCommandTest, MoveToSpecificIndex)
{
  // Add some dummy plugins to target list
  auto dummy_plugin1 = plugin_registry_.create_object<
    plugins::InternalPluginBase> (
    dsp::ProcessorBase::ProcessorBaseDependencies{
      .port_registry_ = port_registry_, .param_registry_ = param_registry_ },
    [] () { return fs::path (); }, nullptr);
  auto dummy_plugin2 = plugin_registry_.create_object<
    plugins::InternalPluginBase> (
    dsp::ProcessorBase::ProcessorBaseDependencies{
      .port_registry_ = port_registry_, .param_registry_ = param_registry_ },
    [] () { return fs::path (); }, nullptr);

  target_plugin_list_->append_plugin (dummy_plugin1);
  target_plugin_list_->append_plugin (dummy_plugin2);

  MovePluginCommand::PluginLocation source_location = {
    source_plugin_list_.get (), source_automation_tracklist_.get ()
  };
  MovePluginCommand::PluginLocation target_location = {
    target_plugin_list_.get (), target_automation_tracklist_.get ()
  };

  // Move to index 1 (between the two dummy plugins)
  MovePluginCommand command (
    test_plugin_ref_, source_location, target_location, 1);

  command.redo ();

  // Verify plugin is at correct position
  EXPECT_EQ (target_plugin_list_->rowCount (), 3);
  EXPECT_EQ (
    get_plugin_id_at_index (*target_plugin_list_, 0), dummy_plugin1.id ());
  EXPECT_EQ (
    get_plugin_id_at_index (*target_plugin_list_, 1), test_plugin_ref_.id ());
  EXPECT_EQ (
    get_plugin_id_at_index (*target_plugin_list_, 2), dummy_plugin2.id ());
}

// Test move with null automation tracklists
TEST_F (MovePluginCommandTest, MoveWithNullAutomationTracklists)
{
  MovePluginCommand::PluginLocation source_location = {
    source_plugin_list_.get (), nullptr
  };
  MovePluginCommand::PluginLocation target_location = {
    target_plugin_list_.get (), nullptr
  };

  MovePluginCommand command (test_plugin_ref_, source_location, target_location);

  // Should not crash with null automation tracklists
  command.redo ();
  EXPECT_EQ (source_plugin_list_->rowCount (), 0);
  EXPECT_EQ (target_plugin_list_->rowCount (), 1);

  command.undo ();
  EXPECT_EQ (source_plugin_list_->rowCount (), 1);
  EXPECT_EQ (target_plugin_list_->rowCount (), 0);
}

// Test automation track movement (simplified test)
TEST_F (MovePluginCommandTest, AutomationTrackMovement)
{
  // This is a simplified test since creating proper automation tracks
  // would require more complex setup
  MovePluginCommand::PluginLocation source_location = {
    source_plugin_list_.get (), source_automation_tracklist_.get ()
  };
  MovePluginCommand::PluginLocation target_location = {
    target_plugin_list_.get (), target_automation_tracklist_.get ()
  };

  MovePluginCommand command (test_plugin_ref_, source_location, target_location);

  // Should not crash when automation tracklists are present
  command.redo ();
  EXPECT_EQ (source_plugin_list_->rowCount (), 0);
  EXPECT_EQ (target_plugin_list_->rowCount (), 1);

  command.undo ();
  EXPECT_EQ (source_plugin_list_->rowCount (), 1);
  EXPECT_EQ (target_plugin_list_->rowCount (), 0);
}

// Test move from empty source list (should fail gracefully)
TEST_F (MovePluginCommandTest, MoveFromEmptyList)
{
  // Remove the test plugin first
  source_plugin_list_->remove_plugin (test_plugin_ref_.id ());

  MovePluginCommand::PluginLocation source_location = {
    source_plugin_list_.get (), source_automation_tracklist_.get ()
  };
  MovePluginCommand::PluginLocation target_location = {
    target_plugin_list_.get (), target_automation_tracklist_.get ()
  };

  // This should throw an exception since the plugin is not in the source list
  EXPECT_THROW (
    MovePluginCommand command (
      test_plugin_ref_, source_location, target_location),
    std::invalid_argument);
}

// Test move to same list (should be no-op)
TEST_F (MovePluginCommandTest, MoveToSameList)
{
  MovePluginCommand::PluginLocation source_location = {
    source_plugin_list_.get (), source_automation_tracklist_.get ()
  };
  MovePluginCommand::PluginLocation target_location = {
    source_plugin_list_.get (), source_automation_tracklist_.get ()
  };

  MovePluginCommand command (test_plugin_ref_, source_location, target_location);

  // Initial state
  EXPECT_EQ (source_plugin_list_->rowCount (), 1);

  // Redo should not change anything
  command.redo ();
  EXPECT_EQ (source_plugin_list_->rowCount (), 1);
  EXPECT_EQ (
    get_plugin_id_at_index (*source_plugin_list_, 0), test_plugin_ref_.id ());

  // Undo should also not change anything
  command.undo ();
  EXPECT_EQ (source_plugin_list_->rowCount (), 1);
  EXPECT_EQ (
    get_plugin_id_at_index (*source_plugin_list_, 0), test_plugin_ref_.id ());
}

// Test move with index out of bounds
TEST_F (MovePluginCommandTest, MoveWithIndexOutOfBounds)
{
  MovePluginCommand::PluginLocation source_location = {
    source_plugin_list_.get (), source_automation_tracklist_.get ()
  };
  MovePluginCommand::PluginLocation target_location = {
    target_plugin_list_.get (), target_automation_tracklist_.get ()
  };

  // Move to index 999 (should append to end)
  MovePluginCommand command (
    test_plugin_ref_, source_location, target_location, 999);

  command.redo ();

  // Plugin should be at the end
  EXPECT_EQ (target_plugin_list_->rowCount (), 1);
  EXPECT_EQ (
    get_plugin_id_at_index (*target_plugin_list_, 0), test_plugin_ref_.id ());
}

// Test command text
TEST_F (MovePluginCommandTest, CommandText)
{
  MovePluginCommand::PluginLocation source_location = {
    source_plugin_list_.get (), source_automation_tracklist_.get ()
  };
  MovePluginCommand::PluginLocation target_location = {
    target_plugin_list_.get (), target_automation_tracklist_.get ()
  };

  MovePluginCommand command (test_plugin_ref_, source_location, target_location);

  // The command should have the text "Move Plugin" for display in undo stack
  EXPECT_EQ (command.text (), QString ("Move Plugin"));
}

// Test multiple move operations
TEST_F (MovePluginCommandTest, MultipleMoveOperations)
{
  MovePluginCommand::PluginLocation source_location = {
    source_plugin_list_.get (), source_automation_tracklist_.get ()
  };
  MovePluginCommand::PluginLocation target_location = {
    target_plugin_list_.get (), target_automation_tracklist_.get ()
  };

  // First move
  MovePluginCommand command1 (
    test_plugin_ref_, source_location, target_location);
  command1.redo ();
  EXPECT_EQ (source_plugin_list_->rowCount (), 0);
  EXPECT_EQ (target_plugin_list_->rowCount (), 1);

  // Move back to source
  MovePluginCommand command2 (
    test_plugin_ref_, target_location, source_location);
  command2.redo ();
  EXPECT_EQ (source_plugin_list_->rowCount (), 1);
  EXPECT_EQ (target_plugin_list_->rowCount (), 0);

  // Undo second move
  command2.undo ();
  EXPECT_EQ (source_plugin_list_->rowCount (), 0);
  EXPECT_EQ (target_plugin_list_->rowCount (), 1);

  // Undo first move
  command1.undo ();
  EXPECT_EQ (source_plugin_list_->rowCount (), 1);
  EXPECT_EQ (target_plugin_list_->rowCount (), 0);
}

} // namespace zrythm::commands
