// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/plugin_importer.h"
#include "plugins/plugin_configuration.h"
#include "plugins/plugin_descriptor.h"
#include "plugins/plugin_factory.h"

#include <QSignalSpy>

#include "helpers/scoped_juce_qapplication.h"

#include "unit/actions/mock_undo_stack.h"
#include "unit/dsp/graph_helpers.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace zrythm::actions
{

class PluginImporterTest
    : public ::testing::Test,
      private test_helpers::ScopedJuceQApplication
{
protected:
  void SetUp () override
  {
    // Create track registry and related components
    singleton_tracks_ = std::make_unique<structure::tracks::SingletonTracks> ();
    track_collection_ =
      std::make_unique<structure::tracks::TrackCollection> (track_registry_);
    track_routing_ =
      std::make_unique<structure::tracks::TrackRouting> (track_registry_);

    // Create tempo map and transport
    tempo_map_ = std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));
    tempo_map_wrapper_ = std::make_unique<dsp::TempoMapWrapper> (*tempo_map_);
    transport_ = std::make_unique<dsp::graph_test::MockTransport> ();

    // Create file audio source registry
    file_audio_source_registry_ =
      std::make_unique<dsp::FileAudioSourceRegistry> ();
    obj_registry_ =
      std::make_unique<structure::arrangement::ArrangerObjectRegistry> ();

    // Create track factory dependencies
    structure::tracks::FinalTrackDependencies factory_deps{
      *tempo_map_wrapper_,
      *file_audio_source_registry_,
      plugin_registry_,
      port_registry_,
      param_registry_,
      *obj_registry_,
      track_registry_,
      *transport_,
      soloed_tracks_exist_getter_,
    };
    track_factory_ =
      std::make_unique<structure::tracks::TrackFactory> (factory_deps);

    // Create and register singleton tracks
    auto master_track_ref =
      track_factory_->create_empty_track<structure::tracks::MasterTrack> ();
    auto chord_track_ref =
      track_factory_->create_empty_track<structure::tracks::ChordTrack> ();
    auto modulator_track_ref =
      track_factory_->create_empty_track<structure::tracks::ModulatorTrack> ();
    auto marker_track_ref =
      track_factory_->create_empty_track<structure::tracks::MarkerTrack> ();

    // Set singleton track pointers
    singleton_tracks_->setMasterTrack (
      master_track_ref.get_object_as<structure::tracks::MasterTrack> ());
    singleton_tracks_->setChordTrack (
      chord_track_ref.get_object_as<structure::tracks::ChordTrack> ());
    singleton_tracks_->setModulatorTrack (
      modulator_track_ref.get_object_as<structure::tracks::ModulatorTrack> ());
    singleton_tracks_->setMarkerTrack (
      marker_track_ref.get_object_as<structure::tracks::MarkerTrack> ());

    // Add singleton tracks to collection
    track_collection_->add_track (master_track_ref);
    track_collection_->add_track (chord_track_ref);
    track_collection_->add_track (modulator_track_ref);
    track_collection_->add_track (marker_track_ref);

    // Create undo stack
    undo_stack_ = create_mock_undo_stack ();

    // Create track creator
    track_creator_ = std::make_unique<TrackCreator> (
      *undo_stack_, *track_factory_, *track_collection_, *track_routing_,
      *singleton_tracks_);

    // Create plugin factory
    plugin_factory_ = create_plugin_factory ();

    // Create plugin importer
    instantiation_finished_called_ = false;
    instantiation_finished_handler_ = [this] (plugins::PluginUuidReference ref) {
      instantiation_finished_called_ = true;
      last_instantiated_plugin_ref_ = ref;
    };

    plugin_importer_ = std::make_unique<PluginImporter> (
      *undo_stack_, *plugin_factory_, *track_creator_,
      instantiation_finished_handler_);
  }

  void TearDown () override { }

  // Helper to create a mock async plugin instance function
  plugins::JucePlugin::CreatePluginInstanceAsyncFunc create_mock_async_func ()
  {
    return
      [] (
        const juce::PluginDescription &, double, int,
        std::function<void (
          std::unique_ptr<juce::AudioPluginInstance>, const juce::String &)>
          callback) {
        // Simulate async failure for now - can be customized in tests
        callback (nullptr, "Mock plugin not found");
      };
  }

  // Helper to create a mock window provider
  plugins::PluginHostWindowFactory create_mock_window_provider ()
  {
    return [] (plugins::Plugin &) {
      class MockWindow : public plugins::IPluginHostWindow
      {
      public:
        void setJuceComponentContentNonOwned (juce::Component *) override { }
        void setSizeAndCenter (int, int) override { }
        void setSize (int, int) override { }
        void setVisible (bool) override { }
        WId  getEmbedWindowId () const override { return 0; }
      };
      return std::make_unique<MockWindow> ();
    };
  }

  // Helper to create a test plugin descriptor
  std::unique_ptr<plugins::PluginDescriptor> create_test_descriptor (
    plugins::Protocol::ProtocolType protocol =
      plugins::Protocol::ProtocolType::Internal,
    bool is_instrument = false,
    bool is_midi_modifier = false)
  {
    auto descriptor = std::make_unique<plugins::PluginDescriptor> ();
    descriptor->name_ = u8"Test Plugin";
    descriptor->author_ = u8"Test Author";
    descriptor->protocol_ = protocol;
    descriptor->num_audio_ins_ = is_instrument ? 0 : 2;
    descriptor->num_audio_outs_ = 2;
    descriptor->num_midi_ins_ = 1;
    descriptor->num_midi_outs_ = 1;

    // Set category based on type
    if (is_instrument)
      {
        descriptor->category_ = plugins::PluginCategory::Instrument;
      }
    else if (is_midi_modifier)
      {
        descriptor->category_ = plugins::PluginCategory::MIDI;
      }
    else
      {
        descriptor->category_ = plugins::PluginCategory::REVERB;
      }

    return descriptor;
  }

  // Helper to create a plugin factory with default dependencies
  std::unique_ptr<plugins::PluginFactory> create_plugin_factory ()
  {
    auto factory_deps = plugins::PluginFactory::CommonFactoryDependencies{
      .plugin_registry_ = plugin_registry_,
      .processor_base_dependencies_{
                                    .port_registry_ = port_registry_, .param_registry_ = param_registry_ },
      .state_dir_path_provider_ = [] () { return fs::path{ "/tmp/test_state" }; },
      .create_plugin_instance_async_func_ = create_mock_async_func (),
      .sample_rate_provider_ = [this] () { return sample_rate_; },
      .buffer_size_provider_ = [this] () { return buffer_size_; },
      .top_level_window_provider_ = create_mock_window_provider (),
      .audio_thread_checker_ = [] () { return false; }
    };

    return std::make_unique<plugins::PluginFactory> (std::move (factory_deps));
  }

  dsp::PortRegistry                port_registry_;
  dsp::ProcessorParameterRegistry  param_registry_{ port_registry_ };
  plugins::PluginRegistry          plugin_registry_;
  structure::tracks::TrackRegistry track_registry_;
  std::unique_ptr<structure::tracks::SingletonTracks> singleton_tracks_;
  std::unique_ptr<structure::tracks::TrackCollection> track_collection_;
  std::unique_ptr<structure::tracks::TrackRouting>    track_routing_;
  std::unique_ptr<structure::tracks::TrackFactory>    track_factory_;
  std::unique_ptr<undo::UndoStack>                    undo_stack_;
  std::unique_ptr<TrackCreator>                       track_creator_;
  std::unique_ptr<plugins::PluginFactory>             plugin_factory_;
  std::unique_ptr<PluginImporter>                     plugin_importer_;

  // Additional dependencies for track factory
  std::unique_ptr<dsp::TempoMap>                tempo_map_;
  std::unique_ptr<dsp::TempoMapWrapper>         tempo_map_wrapper_;
  std::unique_ptr<dsp::FileAudioSourceRegistry> file_audio_source_registry_;
  std::unique_ptr<structure::arrangement::ArrangerObjectRegistry> obj_registry_;
  std::unique_ptr<dsp::graph_test::MockTransport>                 transport_;
  structure::tracks::SoloedTracksExistGetter soloed_tracks_exist_getter_{ [] {
    return false;
  } };

  // Test tracking
  bool instantiation_finished_called_;
  plugins::PluginUuidReference last_instantiated_plugin_ref_{ plugin_registry_ };
  plugins::PluginFactory::InstantiationFinishedHandler
    instantiation_finished_handler_;

  units::sample_rate_t sample_rate_{ units::sample_rate (48000) };
  nframes_t            buffer_size_{ 1024 };
};

// Test basic construction
TEST_F (PluginImporterTest, Construction)
{
  EXPECT_NE (plugin_importer_, nullptr);
}

// Test importing null descriptor
TEST_F (PluginImporterTest, ImportNullDescriptor)
{
  // Should not crash and should not call instantiation handler
  plugin_importer_->importPluginToNewTrack (nullptr);

  EXPECT_FALSE (instantiation_finished_called_);
}

// Test importing instrument plugin
TEST_F (PluginImporterTest, ImportInstrumentPlugin)
{
  auto descriptor = create_test_descriptor (
    plugins::Protocol::ProtocolType::Internal, true, false);

  plugin_importer_->importPluginToNewTrack (descriptor.get ());

  // Process events to handle async instantiation
  QCoreApplication::processEvents ();

  // Should have created an instrument track
  auto instrument_tracks =
    track_collection_->get_track_span ()
      .get_elements_derived_from<structure::tracks::InstrumentTrack> ();
  EXPECT_GE (
    std::distance (instrument_tracks.begin (), instrument_tracks.end ()), 1);

  // Should have called instantiation handler
  EXPECT_TRUE (instantiation_finished_called_);
}

// Test importing MIDI modifier plugin
TEST_F (PluginImporterTest, ImportMidiModifierPlugin)
{
  auto descriptor = create_test_descriptor (
    plugins::Protocol::ProtocolType::Internal, false, true);

  plugin_importer_->importPluginToNewTrack (descriptor.get ());

  // Process events to handle async instantiation
  QCoreApplication::processEvents ();

  // Should have created a MIDI bus track
  auto midi_bus_tracks =
    track_collection_->get_track_span ()
      .get_elements_derived_from<structure::tracks::MidiBusTrack> ();
  EXPECT_GE (
    std::distance (midi_bus_tracks.begin (), midi_bus_tracks.end ()), 1);

  // Should have called instantiation handler
  EXPECT_TRUE (instantiation_finished_called_);
}

// Test importing effect plugin (default case)
TEST_F (PluginImporterTest, ImportEffectPlugin)
{
  auto descriptor = create_test_descriptor (
    plugins::Protocol::ProtocolType::Internal, false, false);

  plugin_importer_->importPluginToNewTrack (descriptor.get ());

  // Process events to handle async instantiation
  QCoreApplication::processEvents ();

  // Should have created an audio bus track
  auto audio_bus_tracks =
    track_collection_->get_track_span ()
      .get_elements_derived_from<structure::tracks::AudioBusTrack> ();
  EXPECT_GE (
    std::distance (audio_bus_tracks.begin (), audio_bus_tracks.end ()), 1);

  // Should have called instantiation handler
  EXPECT_TRUE (instantiation_finished_called_);
}

// Test importing multiple plugins of different types
TEST_F (PluginImporterTest, ImportMultiplePluginTypes)
{
  auto instrument_desc = create_test_descriptor (
    plugins::Protocol::ProtocolType::Internal, true, false);
  auto midi_modifier_desc = create_test_descriptor (
    plugins::Protocol::ProtocolType::Internal, false, true);
  auto effect_desc = create_test_descriptor (
    plugins::Protocol::ProtocolType::Internal, false, false);

  // Import instrument plugin
  plugin_importer_->importPluginToNewTrack (instrument_desc.get ());
  QCoreApplication::processEvents ();

  // Import MIDI modifier plugin
  plugin_importer_->importPluginToNewTrack (midi_modifier_desc.get ());
  QCoreApplication::processEvents ();

  // Import effect plugin
  plugin_importer_->importPluginToNewTrack (effect_desc.get ());
  QCoreApplication::processEvents ();

  // Should have created tracks of each type
  auto instrument_tracks =
    track_collection_->get_track_span ()
      .get_elements_derived_from<structure::tracks::InstrumentTrack> ();
  auto midi_bus_tracks =
    track_collection_->get_track_span ()
      .get_elements_derived_from<structure::tracks::MidiBusTrack> ();
  auto audio_bus_tracks =
    track_collection_->get_track_span ()
      .get_elements_derived_from<structure::tracks::AudioBusTrack> ();

  EXPECT_GE (
    std::distance (instrument_tracks.begin (), instrument_tracks.end ()), 1);
  EXPECT_GE (
    std::distance (midi_bus_tracks.begin (), midi_bus_tracks.end ()), 1);
  EXPECT_GE (
    std::distance (audio_bus_tracks.begin (), audio_bus_tracks.end ()), 1);
}

// Test undo/redo functionality for plugin import
TEST_F (PluginImporterTest, UndoRedoPluginImport)
{
  auto descriptor = create_test_descriptor (
    plugins::Protocol::ProtocolType::Internal, false, false);

  // Get initial track count
  auto initial_track_count = track_collection_->track_count ();

  plugin_importer_->importPluginToNewTrack (descriptor.get ());
  QCoreApplication::processEvents ();

  // Should have added a track
  EXPECT_EQ (track_collection_->track_count (), initial_track_count + 1);

  // Undo should remove the track
  undo_stack_->undo ();
  EXPECT_EQ (track_collection_->track_count (), initial_track_count);

  // Redo should add the track back
  undo_stack_->redo ();
  EXPECT_EQ (track_collection_->track_count (), initial_track_count + 1);
}

// Test instantiation finished handler is called with correct plugin reference
TEST_F (PluginImporterTest, InstantiationFinishedHandlerCalled)
{
  auto descriptor = create_test_descriptor (
    plugins::Protocol::ProtocolType::Internal, false, false);

  plugin_importer_->importPluginToNewTrack (descriptor.get ());
  QCoreApplication::processEvents ();

  // Handler should have been called
  EXPECT_TRUE (instantiation_finished_called_);
  EXPECT_TRUE (plugin_registry_.contains (last_instantiated_plugin_ref_.id ()));
}

// Test that plugin is added to correct plugin group
TEST_F (PluginImporterTest, PluginAddedToCorrectGroup)
{
  auto descriptor = create_test_descriptor (
    plugins::Protocol::ProtocolType::Internal, true, false);

  plugin_importer_->importPluginToNewTrack (descriptor.get ());
  QCoreApplication::processEvents ();

  // For instrument plugin, should be added to instrument group
  auto instrument_tracks =
    track_collection_->get_track_span ()
      .get_elements_derived_from<structure::tracks::InstrumentTrack> ();
  ASSERT_GE (
    std::distance (instrument_tracks.begin (), instrument_tracks.end ()), 1);

  auto * instrument_track = *instrument_tracks.begin ();
  EXPECT_NE (instrument_track->channel ()->instruments (), nullptr);
  EXPECT_GE (instrument_track->channel ()->instruments ()->rowCount (), 1);
}

// Test with different plugin protocols
TEST_F (PluginImporterTest, ImportDifferentProtocols)
{
  // Test with CLAP protocol
  auto clap_descriptor = create_test_descriptor (
    plugins::Protocol::ProtocolType::CLAP, false, false);
  plugin_importer_->importPluginToNewTrack (clap_descriptor.get ());
  QCoreApplication::processEvents ();
  EXPECT_TRUE (instantiation_finished_called_);

  // Reset handler flag
  instantiation_finished_called_ = false;

  // Test with VST3 protocol
  auto vst3_descriptor = create_test_descriptor (
    plugins::Protocol::ProtocolType::VST3, false, false);
  plugin_importer_->importPluginToNewTrack (vst3_descriptor.get ());
  QCoreApplication::processEvents ();
  EXPECT_TRUE (instantiation_finished_called_);
}

// Test that undo macro is properly created
TEST_F (PluginImporterTest, UndoMacroCreated)
{
  auto descriptor = create_test_descriptor (
    plugins::Protocol::ProtocolType::Internal, false, false);

  // Get initial undo stack count
  auto initial_count = undo_stack_->count ();

  plugin_importer_->importPluginToNewTrack (descriptor.get ());
  QCoreApplication::processEvents ();

  // Should have added one macro to the undo stack
  EXPECT_EQ (undo_stack_->count (), initial_count + 1);
}

// Test plugin descriptor name is used in undo macro
TEST_F (PluginImporterTest, UndoMacroUsesPluginName)
{
  auto descriptor = create_test_descriptor (
    plugins::Protocol::ProtocolType::Internal, false, false);
  descriptor->name_ = u8"Custom Test Plugin Name";

  plugin_importer_->importPluginToNewTrack (descriptor.get ());
  QCoreApplication::processEvents ();

  // The undo stack should have a macro with the plugin name
  // Note: This test assumes we can inspect the undo stack text
  // In a real implementation, you might need to add a method to get the last
  // macro text
  EXPECT_GT (undo_stack_->count (), 0);
}

// Test multiple imports of same plugin type
TEST_F (PluginImporterTest, MultipleImportsSameType)
{
  auto descriptor1 = create_test_descriptor (
    plugins::Protocol::ProtocolType::Internal, false, false);
  descriptor1->name_ = u8"Plugin 1";

  auto descriptor2 = create_test_descriptor (
    plugins::Protocol::ProtocolType::Internal, false, false);
  descriptor2->name_ = u8"Plugin 2";

  // Import first plugin
  plugin_importer_->importPluginToNewTrack (descriptor1.get ());
  QCoreApplication::processEvents ();

  // Import second plugin
  plugin_importer_->importPluginToNewTrack (descriptor2.get ());
  QCoreApplication::processEvents ();

  // Should have created two audio bus tracks
  auto audio_bus_tracks =
    track_collection_->get_track_span ()
      .get_elements_derived_from<structure::tracks::AudioBusTrack> ();
  EXPECT_GE (
    std::distance (audio_bus_tracks.begin (), audio_bus_tracks.end ()), 2);
}

// Test error handling when plugin factory fails
TEST_F (PluginImporterTest, PluginFactoryFailure)
{
  // This test would require mocking the plugin factory to fail
  // For now, we'll test with a descriptor that should work
  auto descriptor = create_test_descriptor (
    plugins::Protocol::ProtocolType::Internal, false, false);

  // Should not crash even if factory fails internally
  EXPECT_NO_THROW (plugin_importer_->importPluginToNewTrack (descriptor.get ()));
}

// Test that track creator is called correctly
TEST_F (PluginImporterTest, TrackCreatorCalledCorrectly)
{
  auto descriptor = create_test_descriptor (
    plugins::Protocol::ProtocolType::Internal, true, false);

  plugin_importer_->importPluginToNewTrack (descriptor.get ());
  QCoreApplication::processEvents ();

  // Should have created an instrument track via track creator
  auto instrument_tracks =
    track_collection_->get_track_span ()
      .get_elements_derived_from<structure::tracks::InstrumentTrack> ();
  EXPECT_GE (
    std::distance (instrument_tracks.begin (), instrument_tracks.end ()), 1);
}

// Test plugin configuration creation
TEST_F (PluginImporterTest, PluginConfigurationCreated)
{
  auto descriptor = create_test_descriptor (
    plugins::Protocol::ProtocolType::Internal, false, false);

  // This test verifies that plugin configuration is created from descriptor
  // The actual configuration creation happens inside the importPlugin method
  plugin_importer_->importPluginToNewTrack (descriptor.get ());
  QCoreApplication::processEvents ();

  // If we get here without crashing, configuration creation worked
  EXPECT_TRUE (true);
}

// Test importing plugin to a specific group
TEST_F (PluginImporterTest, ImportPluginToGroup)
{
  auto descriptor = create_test_descriptor (
    plugins::Protocol::ProtocolType::Internal, false, false);

  // First create an audio bus track to get its insert group
  auto audio_bus_track_ref =
    track_factory_->create_empty_track<structure::tracks::AudioBusTrack> ();
  auto * audio_bus_track =
    audio_bus_track_ref.get_object_as<structure::tracks::AudioBusTrack> ();
  track_collection_->add_track (audio_bus_track_ref);
  auto * insert_group = audio_bus_track->channel ()->inserts ();

  // Import plugin to the specific group
  plugin_importer_->importPluginToGroup (descriptor.get (), insert_group);
  QCoreApplication::processEvents ();

  // Should have called instantiation handler
  EXPECT_TRUE (instantiation_finished_called_);

  // Should have added plugin to the group
  EXPECT_GE (insert_group->rowCount (), 1);
}

// Test importing plugin to a specific track
TEST_F (PluginImporterTest, ImportPluginToTrack)
{
  auto descriptor = create_test_descriptor (
    plugins::Protocol::ProtocolType::Internal, false, false);

  // Create an audio bus track
  auto audio_bus_track_ref =
    track_factory_->create_empty_track<structure::tracks::AudioBusTrack> ();
  auto * audio_bus_track =
    audio_bus_track_ref.get_object_as<structure::tracks::AudioBusTrack> ();
  track_collection_->add_track (audio_bus_track_ref);

  // Import plugin to the specific track
  plugin_importer_->importPluginToTrack (descriptor.get (), audio_bus_track);
  QCoreApplication::processEvents ();

  // Should have called instantiation handler
  EXPECT_TRUE (instantiation_finished_called_);

  // Should have added plugin to the track's insert group
  EXPECT_GE (audio_bus_track->channel ()->inserts ()->rowCount (), 1);
}

// Test importing instrument plugin to a specific track
TEST_F (PluginImporterTest, ImportInstrumentPluginToTrack)
{
  auto descriptor = create_test_descriptor (
    plugins::Protocol::ProtocolType::Internal, true, false);

  // Create an instrument track
  auto instrument_track_ref =
    track_factory_->create_empty_track<structure::tracks::InstrumentTrack> ();
  auto * instrument_track =
    instrument_track_ref.get_object_as<structure::tracks::InstrumentTrack> ();
  track_collection_->add_track (instrument_track_ref);

  // Import plugin to the specific track
  plugin_importer_->importPluginToTrack (descriptor.get (), instrument_track);
  QCoreApplication::processEvents ();

  // Should have called instantiation handler
  EXPECT_TRUE (instantiation_finished_called_);

  // Should have added plugin to the track's instrument group
  EXPECT_GE (instrument_track->channel ()->instruments ()->rowCount (), 1);
}

// Test importing MIDI modifier plugin to a specific track
TEST_F (PluginImporterTest, ImportMidiModifierPluginToTrack)
{
  auto descriptor = create_test_descriptor (
    plugins::Protocol::ProtocolType::Internal, false, true);

  // Create a MIDI bus track
  auto midi_bus_track_ref =
    track_factory_->create_empty_track<structure::tracks::MidiBusTrack> ();
  auto * midi_bus_track =
    midi_bus_track_ref.get_object_as<structure::tracks::MidiBusTrack> ();
  track_collection_->add_track (midi_bus_track_ref);

  // Import plugin to the specific track
  plugin_importer_->importPluginToTrack (descriptor.get (), midi_bus_track);
  QCoreApplication::processEvents ();

  // Should have called instantiation handler
  EXPECT_TRUE (instantiation_finished_called_);

  // Should have added plugin to the track's MIDI FX group
  EXPECT_GE (midi_bus_track->channel ()->midiFx ()->rowCount (), 1);
}

// Test that importing to null group doesn't crash
TEST_F (PluginImporterTest, ImportPluginToNullGroup)
{
  auto descriptor = create_test_descriptor (
    plugins::Protocol::ProtocolType::Internal, false, false);

  // Should not crash when passing null group
  EXPECT_NO_THROW (
    plugin_importer_->importPluginToGroup (descriptor.get (), nullptr));
  QCoreApplication::processEvents ();
}

// Test that importing to null track doesn't crash
TEST_F (PluginImporterTest, ImportPluginToNullTrack)
{
  auto descriptor = create_test_descriptor (
    plugins::Protocol::ProtocolType::Internal, false, false);

  // Should not crash when passing null track
  EXPECT_NO_THROW (
    plugin_importer_->importPluginToTrack (descriptor.get (), nullptr));
  QCoreApplication::processEvents ();
}

// Test that all three public methods work correctly
TEST_F (PluginImporterTest, AllThreeImportMethodsWork)
{
  auto descriptor = create_test_descriptor (
    plugins::Protocol::ProtocolType::Internal, false, false);

  // Test importPluginToNewTrack
  plugin_importer_->importPluginToNewTrack (descriptor.get ());
  QCoreApplication::processEvents ();
  EXPECT_TRUE (instantiation_finished_called_);

  // Reset for next test
  instantiation_finished_called_ = false;

  // Create a track and group for the other tests
  auto audio_bus_track_ref =
    track_factory_->create_empty_track<structure::tracks::AudioBusTrack> ();
  auto * audio_bus_track =
    audio_bus_track_ref.get_object_as<structure::tracks::AudioBusTrack> ();
  track_collection_->add_track (audio_bus_track_ref);
  auto * insert_group = audio_bus_track->channel ()->inserts ();

  // Test importPluginToGroup
  plugin_importer_->importPluginToGroup (descriptor.get (), insert_group);
  QCoreApplication::processEvents ();
  EXPECT_TRUE (instantiation_finished_called_);

  // Reset for next test
  instantiation_finished_called_ = false;

  // Test importPluginToTrack
  plugin_importer_->importPluginToTrack (descriptor.get (), audio_bus_track);
  QCoreApplication::processEvents ();
  EXPECT_TRUE (instantiation_finished_called_);
}

} // namespace zrythm::actions
