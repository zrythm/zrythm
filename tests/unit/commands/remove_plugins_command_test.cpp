// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "commands/remove_plugins_command.h"
#include "dsp/parameter.h"
#include "dsp/processor_base.h"
#include "plugins/plugin.h"
#include "plugins/plugin_group.h"
#include "structure/tracks/automation_tracklist.h"

#include <gtest/gtest.h>

namespace zrythm::commands
{

class RemovePluginsCommandTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    group_ = std::make_unique<plugins::PluginGroup> (
      dsp::ProcessorBase::ProcessorBaseDependencies{
        .port_registry_ = port_registry_, .param_registry_ = param_registry_ },
      plugin_registry_, plugins::PluginGroup::DeviceGroupType::Audio,
      plugins::PluginGroup::ProcessingTypeHint::Parallel);

    atl_ = std::make_unique<structure::tracks::AutomationTracklist> (atl_deps_);
  }

  void TearDown () override
  {
    atl_.reset ();
    group_.reset ();
  }

  plugins::PluginUuidReference
  create_and_append_plugin (plugins::PluginGroup &grp)
  {
    auto ref = plugin_registry_.create_object<plugins::InternalPluginBase> (
      dsp::ProcessorBase::ProcessorBaseDependencies{
        .port_registry_ = port_registry_, .param_registry_ = param_registry_ },
      nullptr);
    grp.append_plugin (ref);
    return ref;
  }

  static auto get_plugin_id_at_index (const plugins::PluginGroup &grp, int idx)
  {
    return grp.element_at_idx (idx).value<plugins::Plugin *> ()->get_uuid ();
  }

  dsp::PortRegistry               port_registry_;
  dsp::ProcessorParameterRegistry param_registry_{ port_registry_ };
  plugins::PluginRegistry         plugin_registry_;

  std::unique_ptr<plugins::PluginGroup> group_;

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

  std::unique_ptr<structure::tracks::AutomationTracklist> atl_;
};

// --- Basic removal ---

TEST_F (RemovePluginsCommandTest, RemoveSinglePlugin)
{
  auto ref = create_and_append_plugin (*group_);
  ASSERT_EQ (group_->rowCount (), 1);

  RemovePluginsCommand cmd (
    {
      RemovePluginsCommand::PluginRemoveInfo{
                                             .plugin_ref = ref,
                                             .source_group = group_.get (),
                                             .source_atl = nullptr }
  });

  cmd.redo ();
  EXPECT_EQ (group_->rowCount (), 0);

  cmd.undo ();
  ASSERT_EQ (group_->rowCount (), 1);
  EXPECT_EQ (get_plugin_id_at_index (*group_, 0), ref.id ());
}

TEST_F (RemovePluginsCommandTest, RemoveMultiplePlugins)
{
  auto ref0 = create_and_append_plugin (*group_);
  auto stay = create_and_append_plugin (*group_);
  auto ref2 = create_and_append_plugin (*group_);
  ASSERT_EQ (group_->rowCount (), 3);

  RemovePluginsCommand cmd (
    {
      RemovePluginsCommand::PluginRemoveInfo{
                                             .plugin_ref = ref0, .source_group = group_.get (), .source_atl = nullptr },
      RemovePluginsCommand::PluginRemoveInfo{
                                             .plugin_ref = ref2,
                                             .source_group = group_.get (),
                                             .source_atl = nullptr                                                    }
  });

  cmd.redo ();
  ASSERT_EQ (group_->rowCount (), 1);
  EXPECT_EQ (get_plugin_id_at_index (*group_, 0), stay.id ());

  cmd.undo ();
  ASSERT_EQ (group_->rowCount (), 3);
  EXPECT_EQ (get_plugin_id_at_index (*group_, 0), ref0.id ());
  EXPECT_EQ (get_plugin_id_at_index (*group_, 1), stay.id ());
  EXPECT_EQ (get_plugin_id_at_index (*group_, 2), ref2.id ());
}

TEST_F (RemovePluginsCommandTest, RemoveAllPlugins)
{
  auto ref0 = create_and_append_plugin (*group_);
  auto ref1 = create_and_append_plugin (*group_);

  RemovePluginsCommand cmd (
    {
      RemovePluginsCommand::PluginRemoveInfo{
                                             .plugin_ref = ref0, .source_group = group_.get (), .source_atl = nullptr },
      RemovePluginsCommand::PluginRemoveInfo{
                                             .plugin_ref = ref1,
                                             .source_group = group_.get (),
                                             .source_atl = nullptr                                                    }
  });

  cmd.redo ();
  EXPECT_EQ (group_->rowCount (), 0);

  cmd.undo ();
  ASSERT_EQ (group_->rowCount (), 2);
  EXPECT_EQ (get_plugin_id_at_index (*group_, 0), ref0.id ());
  EXPECT_EQ (get_plugin_id_at_index (*group_, 1), ref1.id ());
}

TEST_F (RemovePluginsCommandTest, RemoveNonContiguousPlugins)
{
  auto ref0 = create_and_append_plugin (*group_);
  auto stay1 = create_and_append_plugin (*group_);
  auto ref2 = create_and_append_plugin (*group_);
  auto stay3 = create_and_append_plugin (*group_);

  RemovePluginsCommand cmd (
    {
      RemovePluginsCommand::PluginRemoveInfo{
                                             .plugin_ref = ref0, .source_group = group_.get (), .source_atl = nullptr },
      RemovePluginsCommand::PluginRemoveInfo{
                                             .plugin_ref = ref2,
                                             .source_group = group_.get (),
                                             .source_atl = nullptr                                                    }
  });

  cmd.redo ();
  ASSERT_EQ (group_->rowCount (), 2);
  EXPECT_EQ (get_plugin_id_at_index (*group_, 0), stay1.id ());
  EXPECT_EQ (get_plugin_id_at_index (*group_, 1), stay3.id ());

  cmd.undo ();
  ASSERT_EQ (group_->rowCount (), 4);
  EXPECT_EQ (get_plugin_id_at_index (*group_, 0), ref0.id ());
  EXPECT_EQ (get_plugin_id_at_index (*group_, 1), stay1.id ());
  EXPECT_EQ (get_plugin_id_at_index (*group_, 2), ref2.id ());
  EXPECT_EQ (get_plugin_id_at_index (*group_, 3), stay3.id ());
}

// --- Undo/redo cycles ---

TEST_F (RemovePluginsCommandTest, UndoSingleRemove)
{
  auto ref0 = create_and_append_plugin (*group_);
  auto ref1 = create_and_append_plugin (*group_);

  RemovePluginsCommand cmd (
    {
      RemovePluginsCommand::PluginRemoveInfo{
                                             .plugin_ref = ref0,
                                             .source_group = group_.get (),
                                             .source_atl = nullptr }
  });

  cmd.redo ();
  ASSERT_EQ (group_->rowCount (), 1);
  EXPECT_EQ (get_plugin_id_at_index (*group_, 0), ref1.id ());

  cmd.undo ();
  ASSERT_EQ (group_->rowCount (), 2);
  EXPECT_EQ (get_plugin_id_at_index (*group_, 0), ref0.id ());
  EXPECT_EQ (get_plugin_id_at_index (*group_, 1), ref1.id ());
}

TEST_F (RemovePluginsCommandTest, MultipleUndoRedoCycles)
{
  auto ref0 = create_and_append_plugin (*group_);
  auto ref1 = create_and_append_plugin (*group_);

  RemovePluginsCommand cmd (
    {
      RemovePluginsCommand::PluginRemoveInfo{
                                             .plugin_ref = ref0, .source_group = group_.get (), .source_atl = nullptr },
      RemovePluginsCommand::PluginRemoveInfo{
                                             .plugin_ref = ref1,
                                             .source_group = group_.get (),
                                             .source_atl = nullptr                                                    }
  });

  for (const auto _ : std::views::iota (0, 3))
    {
      cmd.redo ();
      EXPECT_EQ (group_->rowCount (), 0);

      cmd.undo ();
      ASSERT_EQ (group_->rowCount (), 2);
      EXPECT_EQ (get_plugin_id_at_index (*group_, 0), ref0.id ());
      EXPECT_EQ (get_plugin_id_at_index (*group_, 1), ref1.id ());
    }
}

// --- Command text ---

TEST_F (RemovePluginsCommandTest, CommandTextSinglePlugin)
{
  auto ref = create_and_append_plugin (*group_);

  RemovePluginsCommand cmd (
    {
      RemovePluginsCommand::PluginRemoveInfo{
                                             .plugin_ref = ref,
                                             .source_group = group_.get (),
                                             .source_atl = nullptr }
  });

  EXPECT_EQ (cmd.text (), QString ("Remove Plugin"));
}

TEST_F (RemovePluginsCommandTest, CommandTextMultiplePlugins)
{
  auto ref0 = create_and_append_plugin (*group_);
  auto ref1 = create_and_append_plugin (*group_);

  RemovePluginsCommand cmd (
    {
      RemovePluginsCommand::PluginRemoveInfo{
                                             .plugin_ref = ref0, .source_group = group_.get (), .source_atl = nullptr },
      RemovePluginsCommand::PluginRemoveInfo{
                                             .plugin_ref = ref1,
                                             .source_group = group_.get (),
                                             .source_atl = nullptr                                                    }
  });

  EXPECT_EQ (cmd.text (), QString ("Remove 2 Plugin(s)"));
}

// --- Null automation tracklist ---

TEST_F (RemovePluginsCommandTest, NullAutomationTracklist)
{
  auto ref0 = create_and_append_plugin (*group_);
  auto ref1 = create_and_append_plugin (*group_);

  RemovePluginsCommand cmd (
    {
      RemovePluginsCommand::PluginRemoveInfo{
                                             .plugin_ref = ref0, .source_group = group_.get (), .source_atl = nullptr },
      RemovePluginsCommand::PluginRemoveInfo{
                                             .plugin_ref = ref1,
                                             .source_group = group_.get (),
                                             .source_atl = nullptr                                                    }
  });

  cmd.redo ();
  EXPECT_EQ (group_->rowCount (), 0);

  cmd.undo ();
  ASSERT_EQ (group_->rowCount (), 2);
  EXPECT_EQ (get_plugin_id_at_index (*group_, 0), ref0.id ());
  EXPECT_EQ (get_plugin_id_at_index (*group_, 1), ref1.id ());
}

// --- Automation track handling ---

TEST_F (RemovePluginsCommandTest, RemovePluginWithAutomationTrack)
{
  auto   ref = create_and_append_plugin (*group_);
  auto * plugin = plugins::plugin_ptr_variant_to_base (ref.get_object ());

  auto param_ref = param_registry_.create_object<dsp::ProcessorParameter> (
    port_registry_, dsp::ProcessorParameter::UniqueId (u8"test_param"),
    dsp::ParameterRange (dsp::ParameterRange::Type::Linear, 0.0f, 1.0f),
    u8"Test Param");
  plugin->add_parameter (param_ref);

  auto at = utils::make_qobject_unique<structure::tracks::AutomationTrack> (
    tempo_map_wrapper_, file_audio_source_registry_, object_registry_,
    param_ref);
  atl_->add_automation_track (std::move (at));
  ASSERT_EQ (atl_->rowCount (), 1);

  RemovePluginsCommand cmd (
    {
      RemovePluginsCommand::PluginRemoveInfo{
                                             .plugin_ref = ref,
                                             .source_group = group_.get (),
                                             .source_atl = atl_.get () }
  });

  cmd.redo ();
  EXPECT_EQ (group_->rowCount (), 0);
  EXPECT_EQ (atl_->rowCount (), 0);

  cmd.undo ();
  ASSERT_EQ (group_->rowCount (), 1);
  EXPECT_EQ (get_plugin_id_at_index (*group_, 0), ref.id ());
  EXPECT_EQ (atl_->rowCount (), 1);
  EXPECT_EQ (
    atl_->automation_track_at (0)->parameter (),
    param_ref.get_object_as<dsp::ProcessorParameter> ());
}

TEST_F (RemovePluginsCommandTest, RemovePluginWithMultipleAutomationTracks)
{
  auto   ref = create_and_append_plugin (*group_);
  auto * plugin = plugins::plugin_ptr_variant_to_base (ref.get_object ());

  auto param0 = param_registry_.create_object<dsp::ProcessorParameter> (
    port_registry_, dsp::ProcessorParameter::UniqueId (u8"param_0"),
    dsp::ParameterRange (dsp::ParameterRange::Type::Linear, 0.0f, 1.0f),
    u8"Param 0");
  auto param1 = param_registry_.create_object<dsp::ProcessorParameter> (
    port_registry_, dsp::ProcessorParameter::UniqueId (u8"param_1"),
    dsp::ParameterRange (dsp::ParameterRange::Type::Linear, 0.0f, 1.0f),
    u8"Param 1");
  plugin->add_parameter (param0);
  plugin->add_parameter (param1);

  atl_->add_automation_track (
    utils::make_qobject_unique<structure::tracks::AutomationTrack> (
      tempo_map_wrapper_, file_audio_source_registry_, object_registry_, param0));
  atl_->add_automation_track (
    utils::make_qobject_unique<structure::tracks::AutomationTrack> (
      tempo_map_wrapper_, file_audio_source_registry_, object_registry_, param1));
  ASSERT_EQ (atl_->rowCount (), 2);

  RemovePluginsCommand cmd (
    {
      RemovePluginsCommand::PluginRemoveInfo{
                                             .plugin_ref = ref,
                                             .source_group = group_.get (),
                                             .source_atl = atl_.get () }
  });

  cmd.redo ();
  EXPECT_EQ (group_->rowCount (), 0);
  EXPECT_EQ (atl_->rowCount (), 0);

  cmd.undo ();
  ASSERT_EQ (group_->rowCount (), 1);
  EXPECT_EQ (atl_->rowCount (), 2);
}

TEST_F (
  RemovePluginsCommandTest,
  RemovePluginWithAutomationMultipleUndoRedoCycles)
{
  auto   ref = create_and_append_plugin (*group_);
  auto * plugin = plugins::plugin_ptr_variant_to_base (ref.get_object ());

  auto param0 = param_registry_.create_object<dsp::ProcessorParameter> (
    port_registry_, dsp::ProcessorParameter::UniqueId (u8"param_0"),
    dsp::ParameterRange (dsp::ParameterRange::Type::Linear, 0.0f, 1.0f),
    u8"Param 0");
  auto param1 = param_registry_.create_object<dsp::ProcessorParameter> (
    port_registry_, dsp::ProcessorParameter::UniqueId (u8"param_1"),
    dsp::ParameterRange (dsp::ParameterRange::Type::Linear, 0.0f, 1.0f),
    u8"Param 1");
  plugin->add_parameter (param0);
  plugin->add_parameter (param1);

  atl_->add_automation_track (
    utils::make_qobject_unique<structure::tracks::AutomationTrack> (
      tempo_map_wrapper_, file_audio_source_registry_, object_registry_, param0));
  atl_->add_automation_track (
    utils::make_qobject_unique<structure::tracks::AutomationTrack> (
      tempo_map_wrapper_, file_audio_source_registry_, object_registry_, param1));
  ASSERT_EQ (atl_->rowCount (), 2);

  RemovePluginsCommand cmd (
    {
      RemovePluginsCommand::PluginRemoveInfo{
                                             .plugin_ref = ref,
                                             .source_group = group_.get (),
                                             .source_atl = atl_.get () }
  });

  for (const auto _ : std::views::iota (0, 3))
    {
      cmd.redo ();
      EXPECT_EQ (group_->rowCount (), 0);
      EXPECT_EQ (atl_->rowCount (), 0);

      cmd.undo ();
      ASSERT_EQ (group_->rowCount (), 1);
      EXPECT_EQ (get_plugin_id_at_index (*group_, 0), ref.id ());
      ASSERT_EQ (atl_->rowCount (), 2);
      EXPECT_EQ (
        atl_->automation_track_at (0)->parameter (),
        param0.get_object_as<dsp::ProcessorParameter> ());
      EXPECT_EQ (
        atl_->automation_track_at (1)->parameter (),
        param1.get_object_as<dsp::ProcessorParameter> ());
    }
}

TEST_F (RemovePluginsCommandTest, RedoClosesPluginWindow)
{
  auto   ref = create_and_append_plugin (*group_);
  auto * plugin = plugins::plugin_ptr_variant_to_base (ref.get_object ());
  plugin->setUiVisible (true);
  ASSERT_TRUE (plugin->uiVisible ());

  RemovePluginsCommand cmd (
    {
      RemovePluginsCommand::PluginRemoveInfo{
                                             .plugin_ref = ref,
                                             .source_group = group_.get (),
                                             .source_atl = nullptr }
  });

  cmd.redo ();
  EXPECT_FALSE (plugin->uiVisible ());
}

TEST_F (RemovePluginsCommandTest, RedoAfterReopenClosesPluginWindow)
{
  auto   ref = create_and_append_plugin (*group_);
  auto * plugin = plugins::plugin_ptr_variant_to_base (ref.get_object ());

  RemovePluginsCommand cmd (
    {
      RemovePluginsCommand::PluginRemoveInfo{
                                             .plugin_ref = ref,
                                             .source_group = group_.get (),
                                             .source_atl = nullptr }
  });

  cmd.redo ();
  EXPECT_FALSE (plugin->uiVisible ());

  cmd.undo ();
  EXPECT_FALSE (plugin->uiVisible ());

  plugin->setUiVisible (true);
  ASSERT_TRUE (plugin->uiVisible ());

  cmd.redo ();
  EXPECT_FALSE (plugin->uiVisible ());
}

TEST_F (RemovePluginsCommandTest, RedoSkipsAlreadyClosedWindows)
{
  auto   ref = create_and_append_plugin (*group_);
  auto * plugin = plugins::plugin_ptr_variant_to_base (ref.get_object ());
  ASSERT_FALSE (plugin->uiVisible ());

  RemovePluginsCommand cmd (
    {
      RemovePluginsCommand::PluginRemoveInfo{
                                             .plugin_ref = ref,
                                             .source_group = group_.get (),
                                             .source_atl = nullptr }
  });

  cmd.redo ();
  EXPECT_FALSE (plugin->uiVisible ());
  EXPECT_EQ (group_->rowCount (), 0);
}

} // namespace zrythm::commands
