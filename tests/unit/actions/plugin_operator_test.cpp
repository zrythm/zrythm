// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/plugin_operator.h"
#include "controllers/clipboard.h"
#include "plugins/faust/faust_plugin.h"
#include "plugins/plugin_configuration.h"
#include "plugins/plugin_descriptor.h"
#include "plugins/plugin_factory.h"
#include "plugins/plugin_group.h"
#include "structure/arrangement/arranger_object_factory.h"
#include "structure/project/project_registry.h"
#include "structure/tracks/automation_tracklist.h"
#include "structure/tracks/track.h"
#include "structure/tracks/track_factory.h"
#include "undo/undo_stack.h"
#include "utils/app_settings.h"
#include "utils/object_registry.h"
#include "utils/registry_utils.h"

#include <QSignalSpy>

#include "helpers/in_memory_settings_backend.h"
#include "helpers/mock_plugin_host_window.h"
#include "helpers/scoped_qcoreapplication.h"

#include "unit/actions/mock_undo_stack.h"
#include "unit/structure/tracks/mock_track.h"
#include <gtest/gtest.h>

namespace zrythm::actions
{

class PluginOperatorTest
    : public ::testing::Test,
      private test_helpers::ScopedQCoreApplication
{
protected:
  void SetUp () override
  {
    tempo_map_ = std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));
    tempo_map_wrapper_ = std::make_unique<dsp::TempoMapWrapper> (*tempo_map_);

    undo_stack_ = create_mock_undo_stack ();
    plugin_operator_ = std::make_unique<PluginOperator> (
      *undo_stack_, registry_, clipboard_,
      [] () { return QString ("test-project-id"); });

    // Create source and target plugin groups
    source_group_ = std::make_unique<plugins::PluginGroup> (
      registry_, plugins::PluginGroup::DeviceGroupType::Audio,
      plugins::PluginGroup::ProcessingTypeHint::Parallel);
    target_group_ = std::make_unique<plugins::PluginGroup> (
      registry_, plugins::PluginGroup::DeviceGroupType::Audio,
      plugins::PluginGroup::ProcessingTypeHint::Parallel);

    // Factories needed for clipboard payload imports (paste)
    arranger_object_factory_ = std::make_unique<
      structure::arrangement::ArrangerObjectFactory> (
      structure::arrangement::ArrangerObjectFactory::Dependencies{
        .tempo_map_ = *tempo_map_wrapper_,
        .registry_ = registry_,
        .last_timeline_obj_len_provider_ = [] () { return 100.0; },
        .last_editor_obj_len_provider_ = [] () { return 50.0; },
        .automation_curve_algorithm_provider_ =
          [] () { return dsp::CurveOptions::Algorithm::Exponent; },
      },
      [] () { return units::sample_rate (44100); },
      [] () { return units::bpm (120.0); });

    const structure::tracks::FinalTrackDependencies track_deps{
      *tempo_map_wrapper_,
      registry_,
      structure::tracks::SoloedTracksExistGetter{ [] () { return false; } },
      {}
    };
    track_factory_for_paste_ = std::make_unique<structure::tracks::TrackFactory> (
      [track_deps] () { return track_deps; });

    plugin_factory_ = std::make_unique<
      plugins::PluginFactory> (plugins::PluginFactory::CommonFactoryDependencies{
      .registry = registry_,
      .create_plugin_instance_async_func_ =
        [] (
          const juce::PluginDescription &, double, int,
          juce::AudioPluginFormat::PluginCreationCallback callback) {
          callback (nullptr, "No plugin in operator tests");
        },
      .sample_rate_provider_ = [] () { return units::sample_rate (44100); },
      .buffer_size_provider_ = [] () { return units::samples (256u); },
      .top_level_window_provider_ =
        test_helpers::make_mock_plugin_host_window_factory (
          std::make_shared<test_helpers::MockPluginHostWindowState> ()),
      .main_thread_dispatcher_ = main_dispatcher_ });

    registry_.set_deserialization_dependencies (
      { *track_factory_for_paste_, *arranger_object_factory_, *plugin_factory_ });
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
    auto ref = utils::create_object<plugins::FaustPlugin> (
      registry_, registry_, nullptr);
    group.append_plugin (ref);
    return ref.get_object_as<plugins::FaustPlugin> ();
  }

  /**
   * @brief Creates a registered internal-protocol plugin whose
   * descriptor matches the given categories, for clipboard tests.
   *
   * The returned reference owns the plugin's registry reference: keep it
   * alive for as long as the plugin is used.
   */
  plugins::PluginUuidReference
  create_configured_plugin (bool is_instrument, bool is_midi_modifier)
  {
    auto descriptor = std::make_unique<plugins::PluginDescriptor> ();
    descriptor->name_ = u8"Test Plugin";
    descriptor->author_ = u8"Test Author";
    descriptor->protocol_ = plugins::Protocol::ProtocolType::Internal;
    descriptor->num_audio_ins_ = is_instrument ? 0 : 2;
    descriptor->num_audio_outs_ = 2;
    descriptor->num_midi_ins_ = 1;
    descriptor->num_midi_outs_ = 1;
    descriptor->category_ =
      is_instrument
        ? plugins::PluginCategory::Instrument
        : (is_midi_modifier
             ? plugins::PluginCategory::MIDI
             : plugins::PluginCategory::REVERB);

    auto config = std::make_unique<plugins::PluginConfiguration> ();
    config->descr_ = std::move (descriptor);

    return plugin_factory_->create_plugin_from_setting (
      *config,
      plugins::PluginFactory::InstantiationFinishOptions{
        .handler_ = [] (plugins::PluginUuidReference, bool, const QString &) { },
        .handler_context_ = nullptr });
  }

  static auto
  get_plugin_id_at_index (const plugins::PluginGroup &group, int idx)
  {
    return group.element_at_idx (idx).value<plugins::Plugin *> ()->get_uuid ();
  }

  structure::project::ProjectRegistry registry_;

  // Declared before plugin_operator_ so destruction (reverse order)
  // destroys the operator while the clipboard it references still exists
  controllers::Clipboard clipboard_{
    [] () { return QString (); }, [] (const QString &) { }
  };

  std::unique_ptr<undo::UndoStack> undo_stack_;
  std::unique_ptr<PluginOperator>  plugin_operator_;

  std::unique_ptr<plugins::PluginGroup> source_group_;
  std::unique_ptr<plugins::PluginGroup> target_group_;

  // Factories for clipboard payload imports (paste)
  std::unique_ptr<dsp::TempoMap>        tempo_map_;
  std::unique_ptr<dsp::TempoMapWrapper> tempo_map_wrapper_;
  std::unique_ptr<structure::arrangement::ArrangerObjectFactory>
                                                   arranger_object_factory_;
  std::unique_ptr<structure::tracks::TrackFactory> track_factory_for_paste_;
  std::unique_ptr<plugins::PluginFactory>          plugin_factory_;
  QObject                                          dispatcher_context_;
  utils::MainThreadClosureDispatcher               main_dispatcher_{
    dispatcher_context_, std::chrono::milliseconds{ 10 }
  };

  // For cross-track automation tests
  structure::tracks::MockTrackFactory track_factory_;

  structure::tracks::AutomationTrackHolder::Dependencies make_atl_deps ()
  {
    return { .tempo_map_ = *tempo_map_wrapper_, .registry_ = registry_ };
  }
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

// ================================================
// Remove tests
// ================================================

// --- Basic remove ---

TEST_F (PluginOperatorTest, RemoveSinglePlugin)
{
  auto * pl = create_and_append_plugin (*source_group_);
  ASSERT_EQ (source_group_->rowCount (), 1);

  plugin_operator_->removePlugins ({ pl }, source_group_.get (), nullptr);

  EXPECT_EQ (source_group_->rowCount (), 0);
  EXPECT_EQ (undo_stack_->count (), 1);
}

TEST_F (PluginOperatorTest, RemoveMultiplePlugins)
{
  auto * pl0 = create_and_append_plugin (*source_group_);
  auto * stay = create_and_append_plugin (*source_group_);
  auto * pl2 = create_and_append_plugin (*source_group_);

  plugin_operator_->removePlugins ({ pl0, pl2 }, source_group_.get (), nullptr);

  ASSERT_EQ (source_group_->rowCount (), 1);
  EXPECT_EQ (get_plugin_id_at_index (*source_group_, 0), stay->get_uuid ());
  EXPECT_EQ (undo_stack_->count (), 1);
}

// --- Undo/Redo ---

TEST_F (PluginOperatorTest, UndoRemovePlugin)
{
  auto * pl = create_and_append_plugin (*source_group_);

  plugin_operator_->removePlugins ({ pl }, source_group_.get (), nullptr);
  ASSERT_EQ (source_group_->rowCount (), 0);

  undo_stack_->undo ();

  ASSERT_EQ (source_group_->rowCount (), 1);
  EXPECT_EQ (get_plugin_id_at_index (*source_group_, 0), pl->get_uuid ());
}

TEST_F (PluginOperatorTest, RedoRemovePlugin)
{
  auto * pl = create_and_append_plugin (*source_group_);

  plugin_operator_->removePlugins ({ pl }, source_group_.get (), nullptr);
  undo_stack_->undo ();
  undo_stack_->redo ();

  EXPECT_EQ (source_group_->rowCount (), 0);
}

// --- Empty/null handling ---

TEST_F (PluginOperatorTest, RemoveEmptyPluginListDoesNothing)
{
  plugin_operator_->removePlugins ({}, source_group_.get (), nullptr);
  EXPECT_EQ (undo_stack_->count (), 0);
}

TEST_F (PluginOperatorTest, RemoveNullGroupDoesNothing)
{
  auto * pl = create_and_append_plugin (*source_group_);
  plugin_operator_->removePlugins ({ pl }, nullptr, nullptr);
  EXPECT_EQ (undo_stack_->count (), 0);
}

// --- Command text ---

TEST_F (PluginOperatorTest, RemoveCommandTextSinglePlugin)
{
  auto * pl = create_and_append_plugin (*source_group_);

  plugin_operator_->removePlugins ({ pl }, source_group_.get (), nullptr);

  EXPECT_EQ (undo_stack_->text (0), QString ("Remove Plugin"));
}

TEST_F (PluginOperatorTest, RemoveCommandTextMultiplePlugins)
{
  auto * pl0 = create_and_append_plugin (*source_group_);
  auto * pl1 = create_and_append_plugin (*source_group_);

  plugin_operator_->removePlugins ({ pl0, pl1 }, source_group_.get (), nullptr);

  EXPECT_EQ (undo_stack_->text (0), QString ("Remove 2 Plugin(s)"));
}

// ================================================
// Cross-track automation tests
// ================================================

TEST_F (PluginOperatorTest, MovePluginWithAutomationBetweenTracks)
{
  using namespace structure::tracks;

  auto   source_track = track_factory_.createMockTrack (Track::Type::Audio);
  auto   target_track = track_factory_.createMockTrack (Track::Type::Audio);
  auto * source_atl = source_track->automationTracklist ();
  auto * target_atl = target_track->automationTracklist ();

  // Create plugin with a parameter
  auto * pl = create_and_append_plugin (*source_group_);

  auto param_ref = utils::create_object<dsp::ProcessorParameter> (
    registry_, registry_, dsp::ProcessorParameter::UniqueId (u8"test_param"),
    dsp::ParameterRange (dsp::ParameterRange::Type::Linear, 0.0f, 1.0f),
    u8"Test Param");
  pl->add_parameter (param_ref);

  // Track auto-generates some ATL entries (e.g., Fader), so capture baseline
  // counts
  const auto source_atl_count_before = source_atl->rowCount ();
  const auto target_atl_count_before = target_atl->rowCount ();

  // Add automation track for the parameter on the source track
  source_atl->add_automation_track (
    utils::make_qobject_unique<AutomationTrack> (
      *tempo_map_wrapper_, registry_, param_ref));
  ASSERT_EQ (source_atl->rowCount (), source_atl_count_before + 1);
  ASSERT_EQ (target_atl->rowCount (), target_atl_count_before);

  // Move plugin between tracks (with automation)
  plugin_operator_->movePlugins (
    { pl }, source_group_.get (), source_track.get (), target_group_.get (),
    target_track.get (), -1);

  // Plugin moved
  EXPECT_EQ (source_group_->rowCount (), 0);
  ASSERT_EQ (target_group_->rowCount (), 1);

  // Automation moved
  EXPECT_EQ (source_atl->rowCount (), source_atl_count_before);
  EXPECT_EQ (target_atl->rowCount (), target_atl_count_before + 1);

  // Undo
  undo_stack_->undo ();
  EXPECT_EQ (source_group_->rowCount (), 1);
  EXPECT_EQ (target_group_->rowCount (), 0);
  EXPECT_EQ (source_atl->rowCount (), source_atl_count_before + 1);
  EXPECT_EQ (target_atl->rowCount (), target_atl_count_before);

  // Redo
  undo_stack_->redo ();
  EXPECT_EQ (source_group_->rowCount (), 0);
  ASSERT_EQ (target_group_->rowCount (), 1);
  EXPECT_EQ (source_atl->rowCount (), source_atl_count_before);
  EXPECT_EQ (target_atl->rowCount (), target_atl_count_before + 1);
}

// ================================================
// Clipboard tests
// ================================================

TEST_F (PluginOperatorTest, CanPastePluginsFollowsClipboard)
{
  EXPECT_FALSE (plugin_operator_->canPastePlugins ());

  auto plugin_ref = create_configured_plugin (false, false);
  ASSERT_TRUE (plugin_operator_->copyPlugins ({ plugin_ref.get () }));
  EXPECT_TRUE (plugin_operator_->canPastePlugins ());
}

TEST_F (PluginOperatorTest, CopyEmptySelectionRefused)
{
  EXPECT_FALSE (plugin_operator_->copyPlugins ({}));

  plugins::Plugin * null_plugin = nullptr;
  EXPECT_FALSE (plugin_operator_->copyPlugins ({ null_plugin }));
  EXPECT_FALSE (clipboard_.hasPlugins ());
}

TEST_F (PluginOperatorTest, PasteWithNoClipboardPayloadReturnsNothing)
{
  const auto pasted = plugin_operator_->pastePlugins (target_group_.get ());
  EXPECT_TRUE (pasted.isEmpty ());
  EXPECT_EQ (target_group_->rowCount (), 0);
  EXPECT_EQ (undo_stack_->count (), 0);
}

TEST_F (PluginOperatorTest, CopyAndPasteRoundTrip)
{
  auto plugin_ref = create_configured_plugin (
    /*is_instrument=*/false,
    /*is_midi_modifier=*/false);
  auto * pl = plugin_ref.get ();
  source_group_->append_plugin (
    plugins::PluginUuidReference (pl->get_uuid (), registry_));

  ASSERT_TRUE (plugin_operator_->copyPlugins ({ pl }));
  EXPECT_TRUE (clipboard_.hasPlugins ());

  const auto pasted = plugin_operator_->pastePlugins (target_group_.get ());
  ASSERT_EQ (pasted.size (), 1);
  // The pasted plugin is a new object with a fresh UUID
  EXPECT_NE (
    pasted.front ().toString (),
    type_safe::get (pl->get_uuid ()).toString (QUuid::WithoutBraces));

  // The pasted plugin is a new object in the target group
  ASSERT_EQ (target_group_->rowCount (), 1);
  EXPECT_NE (get_plugin_id_at_index (*target_group_, 0), pl->get_uuid ());
  // The source group still holds the original
  EXPECT_EQ (source_group_->rowCount (), 1);
}

TEST_F (PluginOperatorTest, PasteInsertsAtGivenIndex)
{
  auto   existing_ref = create_configured_plugin (false, false);
  auto * existing = existing_ref.get ();
  target_group_->append_plugin (
    plugins::PluginUuidReference (existing->get_uuid (), registry_));

  auto   plugin_ref = create_configured_plugin (false, false);
  auto * pl = plugin_ref.get ();
  source_group_->append_plugin (
    plugins::PluginUuidReference (pl->get_uuid (), registry_));
  ASSERT_TRUE (plugin_operator_->copyPlugins ({ pl }));

  const auto pasted = plugin_operator_->pastePlugins (target_group_.get (), 0);
  ASSERT_EQ (pasted.size (), 1);

  ASSERT_EQ (target_group_->rowCount (), 2);
  EXPECT_NE (get_plugin_id_at_index (*target_group_, 0), existing->get_uuid ());
  EXPECT_EQ (get_plugin_id_at_index (*target_group_, 1), existing->get_uuid ());
}

TEST_F (PluginOperatorTest, PasteRefusesCategoryMismatch)
{
  auto instrument_ref = create_configured_plugin (/*is_instrument=*/true, false);
  auto * instrument = instrument_ref.get ();
  source_group_->append_plugin (
    plugins::PluginUuidReference (instrument->get_uuid (), registry_));
  ASSERT_TRUE (plugin_operator_->copyPlugins ({ instrument }));

  QSignalSpy refusal_spy (
    plugin_operator_.get (), &PluginOperator::operationRefused);
  const auto pasted = plugin_operator_->pastePlugins (target_group_.get ());
  EXPECT_TRUE (pasted.isEmpty ());
  EXPECT_EQ (target_group_->rowCount (), 0);
  EXPECT_EQ (undo_stack_->count (), 0);
  EXPECT_EQ (refusal_spy.count (), 1);
}

TEST_F (PluginOperatorTest, PasteInstrumentIntoInstrumentGroup)
{
  auto instrument_ref = create_configured_plugin (/*is_instrument=*/true, false);
  auto * instrument = instrument_ref.get ();
  ASSERT_TRUE (plugin_operator_->copyPlugins ({ instrument }));

  plugins::PluginGroup instrument_group (
    registry_, plugins::PluginGroup::DeviceGroupType::Instrument,
    plugins::PluginGroup::ProcessingTypeHint::Parallel);

  const auto pasted = plugin_operator_->pastePlugins (&instrument_group);
  ASSERT_EQ (pasted.size (), 1);
  EXPECT_EQ (instrument_group.rowCount (), 1);
}

TEST_F (PluginOperatorTest, CutRemovesAndPasteRestores)
{
  auto   plugin_ref = create_configured_plugin (false, false);
  auto * pl = plugin_ref.get ();
  source_group_->append_plugin (
    plugins::PluginUuidReference (pl->get_uuid (), registry_));

  ASSERT_TRUE (
    plugin_operator_->cutPlugins ({ pl }, source_group_.get (), nullptr));
  EXPECT_EQ (source_group_->rowCount (), 0);
  EXPECT_TRUE (clipboard_.hasPlugins ());

  const auto pasted = plugin_operator_->pastePlugins (target_group_.get ());
  ASSERT_EQ (pasted.size (), 1);
  EXPECT_EQ (target_group_->rowCount (), 1);
}

TEST_F (PluginOperatorTest, DuplicateKeepsClipboardPayload)
{
  auto   copied_ref = create_configured_plugin (false, false);
  auto * copied = copied_ref.get ();
  ASSERT_TRUE (plugin_operator_->copyPlugins ({ copied }));
  ASSERT_TRUE (clipboard_.payload ().has_value ());
  const auto copied_root = clipboard_.payload ()->roots ().front ();

  auto   duplicated_ref = create_configured_plugin (false, false);
  auto * duplicated = duplicated_ref.get ();
  source_group_->append_plugin (
    plugins::PluginUuidReference (duplicated->get_uuid (), registry_));

  ASSERT_EQ (
    plugin_operator_->duplicatePlugins ({ duplicated }, source_group_.get ())
      .size (),
    1);

  // The clipboard still holds the previously copied plugin, not the
  // duplication source
  ASSERT_TRUE (clipboard_.payload ().has_value ());
  ASSERT_EQ (clipboard_.payload ()->roots ().size (), 1);
  EXPECT_EQ (clipboard_.payload ()->roots ().front (), copied_root);
}

TEST_F (PluginOperatorTest, DuplicateInsertsAfterLastSource)
{
  auto   a_ref = create_configured_plugin (false, false);
  auto * a = a_ref.get ();
  auto   b_ref = create_configured_plugin (false, false);
  auto * b = b_ref.get ();
  source_group_->append_plugin (
    plugins::PluginUuidReference (a->get_uuid (), registry_));
  source_group_->append_plugin (
    plugins::PluginUuidReference (b->get_uuid (), registry_));

  const auto pasted =
    plugin_operator_->duplicatePlugins ({ a }, source_group_.get ());
  ASSERT_EQ (pasted.size (), 1);

  ASSERT_EQ (source_group_->rowCount (), 3);
  EXPECT_EQ (get_plugin_id_at_index (*source_group_, 0), a->get_uuid ());
  EXPECT_NE (get_plugin_id_at_index (*source_group_, 1), a->get_uuid ());
  EXPECT_NE (get_plugin_id_at_index (*source_group_, 1), b->get_uuid ());
  EXPECT_EQ (get_plugin_id_at_index (*source_group_, 2), b->get_uuid ());
}

TEST_F (PluginOperatorTest, UndoPasteRemovesPastedPlugin)
{
  auto   plugin_ref = create_configured_plugin (false, false);
  auto * pl = plugin_ref.get ();
  ASSERT_TRUE (plugin_operator_->copyPlugins ({ pl }));

  ASSERT_EQ (plugin_operator_->pastePlugins (target_group_.get ()).size (), 1);
  ASSERT_EQ (target_group_->rowCount (), 1);

  undo_stack_->undo ();
  EXPECT_EQ (target_group_->rowCount (), 0);

  undo_stack_->redo ();
  EXPECT_EQ (target_group_->rowCount (), 1);
}

} // namespace zrythm::actions
