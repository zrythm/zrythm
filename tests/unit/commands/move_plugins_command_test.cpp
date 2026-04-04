// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "commands/move_plugins_command.h"
#include "dsp/parameter.h"
#include "dsp/processor_base.h"
#include "plugins/plugin.h"
#include "plugins/plugin_group.h"
#include "structure/tracks/automation_tracklist.h"

#include <gtest/gtest.h>

namespace zrythm::commands
{

class MovePluginsCommandTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    source_group_ = std::make_unique<plugins::PluginGroup> (
      dsp::ProcessorBase::ProcessorBaseDependencies{
        .port_registry_ = port_registry_, .param_registry_ = param_registry_ },
      plugin_registry_, plugins::PluginGroup::DeviceGroupType::Audio,
      plugins::PluginGroup::ProcessingTypeHint::Parallel);
    target_group_ = std::make_unique<plugins::PluginGroup> (
      dsp::ProcessorBase::ProcessorBaseDependencies{
        .port_registry_ = port_registry_, .param_registry_ = param_registry_ },
      plugin_registry_, plugins::PluginGroup::DeviceGroupType::Audio,
      plugins::PluginGroup::ProcessingTypeHint::Parallel);

    source_atl_ =
      std::make_unique<structure::tracks::AutomationTracklist> (atl_deps_);
    target_atl_ =
      std::make_unique<structure::tracks::AutomationTracklist> (atl_deps_);
  }

  void TearDown () override
  {
    target_atl_.reset ();
    source_atl_.reset ();
    target_group_.reset ();
    source_group_.reset ();
  }

  plugins::PluginUuidReference
  create_and_append_plugin (plugins::PluginGroup &group)
  {
    auto ref = plugin_registry_.create_object<plugins::InternalPluginBase> (
      dsp::ProcessorBase::ProcessorBaseDependencies{
        .port_registry_ = port_registry_, .param_registry_ = param_registry_ },
      [] () { return fs::path (); }, nullptr);
    group.append_plugin (ref);
    return ref;
  }

  static auto
  get_plugin_id_at_index (const plugins::PluginGroup &group, int idx)
  {
    return group.element_at_idx (idx).value<plugins::Plugin *> ()->get_uuid ();
  }

  using PluginLocation = MovePluginsCommand::PluginLocation;

  PluginLocation source_loc ()
  {
    return { source_group_.get (), source_atl_.get () };
  }

  PluginLocation target_loc ()
  {
    return { target_group_.get (), target_atl_.get () };
  }

  dsp::PortRegistry               port_registry_;
  dsp::ProcessorParameterRegistry param_registry_{ port_registry_ };
  plugins::PluginRegistry         plugin_registry_;

  std::unique_ptr<plugins::PluginGroup> source_group_;
  std::unique_ptr<plugins::PluginGroup> target_group_;

  dsp::TempoMap                tempo_map_{ units::sample_rate (44100) };
  dsp::TempoMapWrapper         tempo_map_wrapper_{ tempo_map_ };
  dsp::FileAudioSourceRegistry file_audio_source_registry_;
  structure::arrangement::ArrangerObjectRegistry         object_registry_;
  structure::tracks::AutomationTrackHolder::Dependencies atl_deps_{
    .tempo_map_ = tempo_map_wrapper_,
    .file_audio_source_registry_ = file_audio_source_registry_,
    .port_registry_ = port_registry_,
    .param_registry_ = param_registry_,
    .object_registry_ = object_registry_
  };

  std::unique_ptr<structure::tracks::AutomationTracklist> source_atl_;
  std::unique_ptr<structure::tracks::AutomationTracklist> target_atl_;
};

// --- Single plugin tests ---

TEST_F (MovePluginsCommandTest, MoveSinglePlugin)
{
  auto ref = create_and_append_plugin (*source_group_);
  ASSERT_EQ (source_group_->rowCount (), 1);
  ASSERT_EQ (target_group_->rowCount (), 0);

  MovePluginsCommand cmd (
    {
      MovePluginsCommand::PluginMoveInfo{
                                         .plugin_ref = ref, .source_location = source_loc () }
  },
    target_loc ());

  cmd.redo ();
  EXPECT_EQ (source_group_->rowCount (), 0);
  EXPECT_EQ (target_group_->rowCount (), 1);
  EXPECT_EQ (get_plugin_id_at_index (*target_group_, 0), ref.id ());

  cmd.undo ();
  EXPECT_EQ (source_group_->rowCount (), 1);
  EXPECT_EQ (target_group_->rowCount (), 0);
  EXPECT_EQ (get_plugin_id_at_index (*source_group_, 0), ref.id ());
}

TEST_F (MovePluginsCommandTest, MoveSinglePluginToSpecificIndex)
{
  auto ref = create_and_append_plugin (*source_group_);
  auto dummy1 = create_and_append_plugin (*target_group_);
  auto dummy2 = create_and_append_plugin (*target_group_);

  MovePluginsCommand cmd (
    {
      MovePluginsCommand::PluginMoveInfo{
                                         .plugin_ref = ref, .source_location = source_loc () }
  },
    target_loc (), 1);

  cmd.redo ();
  ASSERT_EQ (target_group_->rowCount (), 3);
  EXPECT_EQ (get_plugin_id_at_index (*target_group_, 0), dummy1.id ());
  EXPECT_EQ (get_plugin_id_at_index (*target_group_, 1), ref.id ());
  EXPECT_EQ (get_plugin_id_at_index (*target_group_, 2), dummy2.id ());

  cmd.undo ();
  EXPECT_EQ (source_group_->rowCount (), 1);
  EXPECT_EQ (get_plugin_id_at_index (*source_group_, 0), ref.id ());
}

// --- Multi-plugin tests ---

TEST_F (MovePluginsCommandTest, MoveTwoPluginsToEmptyTarget)
{
  auto ref0 = create_and_append_plugin (*source_group_);
  auto ref1 = create_and_append_plugin (*source_group_);
  ASSERT_EQ (source_group_->rowCount (), 2);

  MovePluginsCommand cmd (
    {
      MovePluginsCommand::PluginMoveInfo{
                                         .plugin_ref = ref0, .source_location = source_loc () },
      MovePluginsCommand::PluginMoveInfo{
                                         .plugin_ref = ref1, .source_location = source_loc () }
  },
    target_loc ());

  cmd.redo ();
  EXPECT_EQ (source_group_->rowCount (), 0);
  ASSERT_EQ (target_group_->rowCount (), 2);
  // Plugins should maintain relative order
  EXPECT_EQ (get_plugin_id_at_index (*target_group_, 0), ref0.id ());
  EXPECT_EQ (get_plugin_id_at_index (*target_group_, 1), ref1.id ());

  cmd.undo ();
  EXPECT_EQ (target_group_->rowCount (), 0);
  ASSERT_EQ (source_group_->rowCount (), 2);
  EXPECT_EQ (get_plugin_id_at_index (*source_group_, 0), ref0.id ());
  EXPECT_EQ (get_plugin_id_at_index (*source_group_, 1), ref1.id ());
}

TEST_F (MovePluginsCommandTest, MoveThreePluginsPreservesOrder)
{
  auto ref0 = create_and_append_plugin (*source_group_);
  auto ref1 = create_and_append_plugin (*source_group_);
  auto ref2 = create_and_append_plugin (*source_group_);
  // Also add a plugin that stays behind
  auto stay = create_and_append_plugin (*source_group_);
  ASSERT_EQ (source_group_->rowCount (), 4);

  // Move plugins at indices 0, 1, 2 to target at index 0
  MovePluginsCommand cmd (
    {
      MovePluginsCommand::PluginMoveInfo{
                                         .plugin_ref = ref0, .source_location = source_loc () },
      MovePluginsCommand::PluginMoveInfo{
                                         .plugin_ref = ref1, .source_location = source_loc () },
      MovePluginsCommand::PluginMoveInfo{
                                         .plugin_ref = ref2, .source_location = source_loc () }
  },
    target_loc (), 0);

  cmd.redo ();
  EXPECT_EQ (source_group_->rowCount (), 1);
  EXPECT_EQ (get_plugin_id_at_index (*source_group_, 0), stay.id ());
  ASSERT_EQ (target_group_->rowCount (), 3);
  EXPECT_EQ (get_plugin_id_at_index (*target_group_, 0), ref0.id ());
  EXPECT_EQ (get_plugin_id_at_index (*target_group_, 1), ref1.id ());
  EXPECT_EQ (get_plugin_id_at_index (*target_group_, 2), ref2.id ());

  cmd.undo ();
  ASSERT_EQ (source_group_->rowCount (), 4);
  EXPECT_EQ (get_plugin_id_at_index (*source_group_, 0), ref0.id ());
  EXPECT_EQ (get_plugin_id_at_index (*source_group_, 1), ref1.id ());
  EXPECT_EQ (get_plugin_id_at_index (*source_group_, 2), ref2.id ());
  EXPECT_EQ (get_plugin_id_at_index (*source_group_, 3), stay.id ());
}

TEST_F (MovePluginsCommandTest, MovePluginsToSpecificIndexAmongExisting)
{
  auto ref0 = create_and_append_plugin (*source_group_);
  auto ref1 = create_and_append_plugin (*source_group_);
  auto dummy0 = create_and_append_plugin (*target_group_);
  auto dummy1 = create_and_append_plugin (*target_group_);
  auto dummy2 = create_and_append_plugin (*target_group_);

  // Insert between dummy1 (idx 1) and dummy2 (idx 2)
  MovePluginsCommand cmd (
    {
      MovePluginsCommand::PluginMoveInfo{
                                         .plugin_ref = ref0, .source_location = source_loc () },
      MovePluginsCommand::PluginMoveInfo{
                                         .plugin_ref = ref1, .source_location = source_loc () }
  },
    target_loc (), 2);

  cmd.redo ();
  ASSERT_EQ (target_group_->rowCount (), 5);
  EXPECT_EQ (get_plugin_id_at_index (*target_group_, 0), dummy0.id ());
  EXPECT_EQ (get_plugin_id_at_index (*target_group_, 1), dummy1.id ());
  EXPECT_EQ (get_plugin_id_at_index (*target_group_, 2), ref0.id ());
  EXPECT_EQ (get_plugin_id_at_index (*target_group_, 3), ref1.id ());
  EXPECT_EQ (get_plugin_id_at_index (*target_group_, 4), dummy2.id ());

  cmd.undo ();
  ASSERT_EQ (source_group_->rowCount (), 2);
  EXPECT_EQ (get_plugin_id_at_index (*source_group_, 0), ref0.id ());
  EXPECT_EQ (get_plugin_id_at_index (*source_group_, 1), ref1.id ());
  ASSERT_EQ (target_group_->rowCount (), 3);
}

// --- Reorder within same group ---

TEST_F (MovePluginsCommandTest, ReorderWithinSameGroupForward)
{
  auto ref0 = create_and_append_plugin (*source_group_);
  auto ref1 = create_and_append_plugin (*source_group_);
  auto ref2 = create_and_append_plugin (*source_group_);

  // Move plugin at index 0 to index 2 (after ref2)
  MovePluginsCommand cmd (
    {
      MovePluginsCommand::PluginMoveInfo{
                                         .plugin_ref = ref0, .source_location = source_loc () }
  },
    source_loc (), 2);

  cmd.redo ();
  ASSERT_EQ (source_group_->rowCount (), 3);
  EXPECT_EQ (get_plugin_id_at_index (*source_group_, 0), ref1.id ());
  EXPECT_EQ (get_plugin_id_at_index (*source_group_, 1), ref2.id ());
  EXPECT_EQ (get_plugin_id_at_index (*source_group_, 2), ref0.id ());

  cmd.undo ();
  ASSERT_EQ (source_group_->rowCount (), 3);
  EXPECT_EQ (get_plugin_id_at_index (*source_group_, 0), ref0.id ());
  EXPECT_EQ (get_plugin_id_at_index (*source_group_, 1), ref1.id ());
  EXPECT_EQ (get_plugin_id_at_index (*source_group_, 2), ref2.id ());
}

TEST_F (MovePluginsCommandTest, ReorderWithinSameGroupBackward)
{
  auto ref0 = create_and_append_plugin (*source_group_);
  auto ref1 = create_and_append_plugin (*source_group_);
  auto ref2 = create_and_append_plugin (*source_group_);

  // Move plugin at index 2 to index 0
  MovePluginsCommand cmd (
    {
      MovePluginsCommand::PluginMoveInfo{
                                         .plugin_ref = ref2, .source_location = source_loc () }
  },
    source_loc (), 0);

  cmd.redo ();
  ASSERT_EQ (source_group_->rowCount (), 3);
  EXPECT_EQ (get_plugin_id_at_index (*source_group_, 0), ref2.id ());
  EXPECT_EQ (get_plugin_id_at_index (*source_group_, 1), ref0.id ());
  EXPECT_EQ (get_plugin_id_at_index (*source_group_, 2), ref1.id ());

  cmd.undo ();
  ASSERT_EQ (source_group_->rowCount (), 3);
  EXPECT_EQ (get_plugin_id_at_index (*source_group_, 0), ref0.id ());
  EXPECT_EQ (get_plugin_id_at_index (*source_group_, 1), ref1.id ());
  EXPECT_EQ (get_plugin_id_at_index (*source_group_, 2), ref2.id ());
}

// --- Undo/redo cycles ---

TEST_F (MovePluginsCommandTest, MultipleUndoRedoCycles)
{
  auto ref0 = create_and_append_plugin (*source_group_);
  auto ref1 = create_and_append_plugin (*source_group_);

  MovePluginsCommand cmd (
    {
      MovePluginsCommand::PluginMoveInfo{
                                         .plugin_ref = ref0, .source_location = source_loc () },
      MovePluginsCommand::PluginMoveInfo{
                                         .plugin_ref = ref1, .source_location = source_loc () }
  },
    target_loc ());

  for (int i = 0; i < 3; ++i)
    {
      cmd.redo ();
      EXPECT_EQ (source_group_->rowCount (), 0);
      ASSERT_EQ (target_group_->rowCount (), 2);
      EXPECT_EQ (get_plugin_id_at_index (*target_group_, 0), ref0.id ());
      EXPECT_EQ (get_plugin_id_at_index (*target_group_, 1), ref1.id ());

      cmd.undo ();
      ASSERT_EQ (source_group_->rowCount (), 2);
      EXPECT_EQ (target_group_->rowCount (), 0);
      EXPECT_EQ (get_plugin_id_at_index (*source_group_, 0), ref0.id ());
      EXPECT_EQ (get_plugin_id_at_index (*source_group_, 1), ref1.id ());
    }
}

// --- Append to end (nullopt index) ---

TEST_F (MovePluginsCommandTest, AppendToEndByDefault)
{
  auto ref = create_and_append_plugin (*source_group_);
  auto dummy0 = create_and_append_plugin (*target_group_);
  auto dummy1 = create_and_append_plugin (*target_group_);

  // No target index = append to end
  MovePluginsCommand cmd (
    {
      MovePluginsCommand::PluginMoveInfo{
                                         .plugin_ref = ref, .source_location = source_loc () }
  },
    target_loc ());

  cmd.redo ();
  ASSERT_EQ (target_group_->rowCount (), 3);
  EXPECT_EQ (get_plugin_id_at_index (*target_group_, 0), dummy0.id ());
  EXPECT_EQ (get_plugin_id_at_index (*target_group_, 1), dummy1.id ());
  EXPECT_EQ (get_plugin_id_at_index (*target_group_, 2), ref.id ());
}

// --- Null automation tracklists ---

TEST_F (MovePluginsCommandTest, NullAutomationTracklists)
{
  auto ref0 = create_and_append_plugin (*source_group_);
  auto ref1 = create_and_append_plugin (*source_group_);

  MovePluginsCommand cmd (
    {
      MovePluginsCommand::PluginMoveInfo{
                                         .plugin_ref = ref0,
                                         .source_location = PluginLocation{ source_group_.get (), nullptr } },
      MovePluginsCommand::PluginMoveInfo{
                                         .plugin_ref = ref1,
                                         .source_location = PluginLocation{ source_group_.get (), nullptr } }
  },
    PluginLocation{ target_group_.get (), nullptr });

  cmd.redo ();
  EXPECT_EQ (source_group_->rowCount (), 0);
  EXPECT_EQ (target_group_->rowCount (), 2);

  cmd.undo ();
  EXPECT_EQ (source_group_->rowCount (), 2);
  EXPECT_EQ (target_group_->rowCount (), 0);
}

// --- Command text ---

TEST_F (MovePluginsCommandTest, CommandTextSinglePlugin)
{
  auto ref = create_and_append_plugin (*source_group_);

  MovePluginsCommand cmd (
    {
      MovePluginsCommand::PluginMoveInfo{
                                         .plugin_ref = ref, .source_location = source_loc () }
  },
    target_loc ());

  EXPECT_EQ (cmd.text (), QString ("Move Plugin"));
}

TEST_F (MovePluginsCommandTest, CommandTextMultiplePlugins)
{
  auto ref0 = create_and_append_plugin (*source_group_);
  auto ref1 = create_and_append_plugin (*source_group_);

  MovePluginsCommand cmd (
    {
      MovePluginsCommand::PluginMoveInfo{
                                         .plugin_ref = ref0, .source_location = source_loc () },
      MovePluginsCommand::PluginMoveInfo{
                                         .plugin_ref = ref1, .source_location = source_loc () }
  },
    target_loc ());

  EXPECT_EQ (cmd.text (), QString ("Move 2 Plugin(s)"));
}

TEST_F (MovePluginsCommandTest, CommandTextManyPlugins)
{
  std::vector<MovePluginsCommand::PluginMoveInfo> infos;
  for (int i = 0; i < 5; ++i)
    {
      auto ref = create_and_append_plugin (*source_group_);
      infos.push_back (
        MovePluginsCommand::PluginMoveInfo{
          .plugin_ref = ref, .source_location = source_loc () });
    }

  MovePluginsCommand cmd (std::move (infos), target_loc ());
  EXPECT_EQ (cmd.text (), QString ("Move 5 Plugin(s)"));
}

// --- Move all plugins from source ---

TEST_F (MovePluginsCommandTest, MoveAllPluginsLeavesSourceEmpty)
{
  auto ref0 = create_and_append_plugin (*source_group_);
  auto ref1 = create_and_append_plugin (*source_group_);
  auto ref2 = create_and_append_plugin (*source_group_);

  MovePluginsCommand cmd (
    {
      MovePluginsCommand::PluginMoveInfo{
                                         .plugin_ref = ref0, .source_location = source_loc () },
      MovePluginsCommand::PluginMoveInfo{
                                         .plugin_ref = ref1, .source_location = source_loc () },
      MovePluginsCommand::PluginMoveInfo{
                                         .plugin_ref = ref2, .source_location = source_loc () }
  },
    target_loc ());

  cmd.redo ();
  EXPECT_EQ (source_group_->rowCount (), 0);
  ASSERT_EQ (target_group_->rowCount (), 3);

  cmd.undo ();
  ASSERT_EQ (source_group_->rowCount (), 3);
  EXPECT_EQ (target_group_->rowCount (), 0);
}

// --- Move non-contiguous plugins ---

TEST_F (MovePluginsCommandTest, MoveNonContiguousPlugins)
{
  auto ref0 = create_and_append_plugin (*source_group_);
  auto stay = create_and_append_plugin (*source_group_);
  auto ref2 = create_and_append_plugin (*source_group_);
  auto stay2 = create_and_append_plugin (*source_group_);

  // Move plugins at indices 0 and 2 (skipping 1 and 3)
  MovePluginsCommand cmd (
    {
      MovePluginsCommand::PluginMoveInfo{
                                         .plugin_ref = ref0, .source_location = source_loc () },
      MovePluginsCommand::PluginMoveInfo{
                                         .plugin_ref = ref2, .source_location = source_loc () }
  },
    target_loc ());

  cmd.redo ();
  ASSERT_EQ (source_group_->rowCount (), 2);
  EXPECT_EQ (get_plugin_id_at_index (*source_group_, 0), stay.id ());
  EXPECT_EQ (get_plugin_id_at_index (*source_group_, 1), stay2.id ());
  ASSERT_EQ (target_group_->rowCount (), 2);
  EXPECT_EQ (get_plugin_id_at_index (*target_group_, 0), ref0.id ());
  EXPECT_EQ (get_plugin_id_at_index (*target_group_, 1), ref2.id ());

  cmd.undo ();
  ASSERT_EQ (source_group_->rowCount (), 4);
  EXPECT_EQ (get_plugin_id_at_index (*source_group_, 0), ref0.id ());
  EXPECT_EQ (get_plugin_id_at_index (*source_group_, 1), stay.id ());
  EXPECT_EQ (get_plugin_id_at_index (*source_group_, 2), ref2.id ());
  EXPECT_EQ (get_plugin_id_at_index (*source_group_, 3), stay2.id ());
}

// --- Move to append position (nullopt) with existing plugins ---

TEST_F (MovePluginsCommandTest, AppendMultipleToExistingTarget)
{
  auto ref0 = create_and_append_plugin (*source_group_);
  auto ref1 = create_and_append_plugin (*source_group_);
  auto dummy = create_and_append_plugin (*target_group_);

  MovePluginsCommand cmd (
    {
      MovePluginsCommand::PluginMoveInfo{
                                         .plugin_ref = ref0, .source_location = source_loc () },
      MovePluginsCommand::PluginMoveInfo{
                                         .plugin_ref = ref1, .source_location = source_loc () }
  },
    target_loc ());

  cmd.redo ();
  ASSERT_EQ (target_group_->rowCount (), 3);
  EXPECT_EQ (get_plugin_id_at_index (*target_group_, 0), dummy.id ());
  EXPECT_EQ (get_plugin_id_at_index (*target_group_, 1), ref0.id ());
  EXPECT_EQ (get_plugin_id_at_index (*target_group_, 2), ref1.id ());
}

// --- Out-of-bounds target index ---

TEST_F (MovePluginsCommandTest, MoveWithIndexOutOfBoundsThrows)
{
  auto ref = create_and_append_plugin (*source_group_);

  EXPECT_THROW (
    MovePluginsCommand (
      {
        MovePluginsCommand::PluginMoveInfo{
                                           .plugin_ref = ref, .source_location = source_loc () }
  },
      target_loc (), 999),
    std::invalid_argument);
}

// --- Plugin not in source ---

TEST_F (MovePluginsCommandTest, MovePluginNotInSourceThrows)
{
  auto ref = create_and_append_plugin (*source_group_);
  source_group_->remove_plugin (ref.id ());

  EXPECT_THROW (
    MovePluginsCommand (
      {
        MovePluginsCommand::PluginMoveInfo{
                                           .plugin_ref = ref, .source_location = source_loc () }
  },
      target_loc ()),
    std::invalid_argument);
}

// --- Automation track movement ---

TEST_F (MovePluginsCommandTest, MovePluginWithSingleAutomationTrack)
{
  auto   ref = create_and_append_plugin (*source_group_);
  auto * plugin = plugins::plugin_ptr_variant_to_base (ref.get_object ());

  // Create a parameter and automation track for it
  auto param_ref = param_registry_.create_object<dsp::ProcessorParameter> (
    port_registry_, dsp::ProcessorParameter::UniqueId (u8"test_param"),
    dsp::ParameterRange (dsp::ParameterRange::Type::Linear, 0.0f, 1.0f),
    u8"Test Param");
  plugin->add_parameter (param_ref);

  auto at = utils::make_qobject_unique<structure::tracks::AutomationTrack> (
    tempo_map_wrapper_, file_audio_source_registry_, object_registry_,
    param_ref);
  source_atl_->add_automation_track (std::move (at));
  ASSERT_EQ (source_atl_->rowCount (), 1);
  ASSERT_EQ (target_atl_->rowCount (), 0);

  MovePluginsCommand cmd (
    {
      MovePluginsCommand::PluginMoveInfo{
                                         .plugin_ref = ref, .source_location = source_loc () }
  },
    target_loc ());

  cmd.redo ();
  EXPECT_EQ (source_atl_->rowCount (), 0);
  EXPECT_EQ (target_atl_->rowCount (), 1);
  EXPECT_EQ (
    target_atl_->automation_track_at (0)->parameter (),
    param_ref.get_object_as<dsp::ProcessorParameter> ());

  cmd.undo ();
  EXPECT_EQ (source_atl_->rowCount (), 1);
  EXPECT_EQ (target_atl_->rowCount (), 0);
  EXPECT_EQ (
    source_atl_->automation_track_at (0)->parameter (),
    param_ref.get_object_as<dsp::ProcessorParameter> ());
}

TEST_F (MovePluginsCommandTest, MovePluginWithMultipleAutomationTracks)
{
  auto   ref = create_and_append_plugin (*source_group_);
  auto * plugin = plugins::plugin_ptr_variant_to_base (ref.get_object ());

  // Create 3 parameters with automation tracks
  auto param0 = param_registry_.create_object<dsp::ProcessorParameter> (
    port_registry_, dsp::ProcessorParameter::UniqueId (u8"param_0"),
    dsp::ParameterRange (dsp::ParameterRange::Type::Linear, 0.0f, 1.0f),
    u8"Param 0");
  auto param1 = param_registry_.create_object<dsp::ProcessorParameter> (
    port_registry_, dsp::ProcessorParameter::UniqueId (u8"param_1"),
    dsp::ParameterRange (dsp::ParameterRange::Type::Linear, 0.0f, 1.0f),
    u8"Param 1");
  auto param2 = param_registry_.create_object<dsp::ProcessorParameter> (
    port_registry_, dsp::ProcessorParameter::UniqueId (u8"param_2"),
    dsp::ParameterRange (dsp::ParameterRange::Type::Linear, 0.0f, 1.0f),
    u8"Param 2");
  plugin->add_parameter (param0);
  plugin->add_parameter (param1);
  plugin->add_parameter (param2);

  source_atl_->add_automation_track (
    utils::make_qobject_unique<structure::tracks::AutomationTrack> (
      tempo_map_wrapper_, file_audio_source_registry_, object_registry_, param0));
  source_atl_->add_automation_track (
    utils::make_qobject_unique<structure::tracks::AutomationTrack> (
      tempo_map_wrapper_, file_audio_source_registry_, object_registry_, param1));
  source_atl_->add_automation_track (
    utils::make_qobject_unique<structure::tracks::AutomationTrack> (
      tempo_map_wrapper_, file_audio_source_registry_, object_registry_, param2));
  ASSERT_EQ (source_atl_->rowCount (), 3);

  MovePluginsCommand cmd (
    {
      MovePluginsCommand::PluginMoveInfo{
                                         .plugin_ref = ref, .source_location = source_loc () }
  },
    target_loc ());

  cmd.redo ();
  EXPECT_EQ (source_atl_->rowCount (), 0);
  EXPECT_EQ (target_atl_->rowCount (), 3);

  cmd.undo ();
  EXPECT_EQ (source_atl_->rowCount (), 3);
  EXPECT_EQ (target_atl_->rowCount (), 0);
}

// --- Multi-plugin same-group reorder ---

TEST_F (MovePluginsCommandTest, ReorderTwoPluginsWithinSameGroup)
{
  auto ref0 = create_and_append_plugin (*source_group_);
  auto stay = create_and_append_plugin (*source_group_);
  auto ref2 = create_and_append_plugin (*source_group_);

  // Move plugins at indices 0 and 2 to index 1 within same group
  MovePluginsCommand cmd (
    {
      MovePluginsCommand::PluginMoveInfo{
                                         .plugin_ref = ref0, .source_location = source_loc () },
      MovePluginsCommand::PluginMoveInfo{
                                         .plugin_ref = ref2, .source_location = source_loc () }
  },
    source_loc (), 1);

  cmd.redo ();
  ASSERT_EQ (source_group_->rowCount (), 3);
  EXPECT_EQ (get_plugin_id_at_index (*source_group_, 0), stay.id ());
  EXPECT_EQ (get_plugin_id_at_index (*source_group_, 1), ref0.id ());
  EXPECT_EQ (get_plugin_id_at_index (*source_group_, 2), ref2.id ());

  cmd.undo ();
  ASSERT_EQ (source_group_->rowCount (), 3);
  EXPECT_EQ (get_plugin_id_at_index (*source_group_, 0), ref0.id ());
  EXPECT_EQ (get_plugin_id_at_index (*source_group_, 1), stay.id ());
  EXPECT_EQ (get_plugin_id_at_index (*source_group_, 2), ref2.id ());
}

TEST_F (MovePluginsCommandTest, ReorderNonContiguousWithinSameGroupWithUndo)
{
  auto ref0 = create_and_append_plugin (*source_group_);
  auto ref1 = create_and_append_plugin (*source_group_);
  auto ref2 = create_and_append_plugin (*source_group_);
  auto ref3 = create_and_append_plugin (*source_group_);

  // Move plugins at indices 0 and 3 to index 1
  MovePluginsCommand cmd (
    {
      MovePluginsCommand::PluginMoveInfo{
                                         .plugin_ref = ref0, .source_location = source_loc () },
      MovePluginsCommand::PluginMoveInfo{
                                         .plugin_ref = ref3, .source_location = source_loc () }
  },
    source_loc (), 1);

  cmd.redo ();
  ASSERT_EQ (source_group_->rowCount (), 4);
  // After removing 0 and 3 (descending: 3 first, then 0), remaining: [ref1,
  // ref2] Insert at 1: ref0 at 1, ref3 at 2 => [ref1, ref0, ref3, ref2]
  EXPECT_EQ (get_plugin_id_at_index (*source_group_, 0), ref1.id ());
  EXPECT_EQ (get_plugin_id_at_index (*source_group_, 1), ref0.id ());
  EXPECT_EQ (get_plugin_id_at_index (*source_group_, 2), ref3.id ());
  EXPECT_EQ (get_plugin_id_at_index (*source_group_, 3), ref2.id ());

  cmd.undo ();
  ASSERT_EQ (source_group_->rowCount (), 4);
  EXPECT_EQ (get_plugin_id_at_index (*source_group_, 0), ref0.id ());
  EXPECT_EQ (get_plugin_id_at_index (*source_group_, 1), ref1.id ());
  EXPECT_EQ (get_plugin_id_at_index (*source_group_, 2), ref2.id ());
  EXPECT_EQ (get_plugin_id_at_index (*source_group_, 3), ref3.id ());
}

} // namespace zrythm::commands
