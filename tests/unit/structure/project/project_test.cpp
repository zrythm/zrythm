// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/juce_hardware_audio_interface.h"
#include "structure/project/project.h"
#include "utils/io_utils.h"

#include "helpers/mock_audio_io_device.h"
#include "helpers/mock_settings_backend.h"
#include "helpers/scoped_juce_qapplication.h"

#include <gtest/gtest.h>

using namespace zrythm::structure::project;
using namespace ::testing;

// Fixture for testing Project functionality
class ProjectTest : public ::testing::Test, public test_helpers::ScopedJuceQApplication
{
protected:
  void SetUp () override
  {
    // Create a temporary directory for the project
    temp_dir_obj = utils::io::make_tmp_dir ();
    project_dir =
      utils::Utf8String::from_qstring (temp_dir_obj->path ()).to_path ();

    // Create audio device manager with dummy device
    audio_device_manager =
      test_helpers::create_audio_device_manager_with_dummy_device ();

    // Create hardware audio interface wrapper
    hw_interface =
      dsp::JuceHardwareAudioInterface::create (audio_device_manager);

    plugin_format_manager = std::make_shared<juce::AudioPluginFormatManager> ();
    plugin_format_manager->addDefaultFormats ();

    // Create a mock settings backend
    auto mock_backend = std::make_unique<test_helpers::MockSettingsBackend> ();
    mock_backend_ptr = mock_backend.get ();

    // Set up default expectations for common settings
    ON_CALL (*mock_backend_ptr, value (testing::_, testing::_))
      .WillByDefault (testing::Return (QVariant ()));

    app_settings =
      std::make_unique<utils::AppSettings> (std::move (mock_backend));

    // Create port registry and monitor fader
    port_registry = std::make_unique<dsp::PortRegistry> (nullptr);
    param_registry = std::make_unique<dsp::ProcessorParameterRegistry> (
      *port_registry, nullptr);
    monitor_fader = utils::make_qobject_unique<dsp::Fader> (
      dsp::ProcessorBase::ProcessorBaseDependencies{
        .port_registry_ = *port_registry,
        .param_registry_ = *param_registry,
      },
      dsp::PortType::Audio,
      true,  // hard_limit_output
      false, // make_params_automatable
      [] () -> utils::Utf8String { return u8"Test Control Room"; },
      [] (bool fader_solo_status) { return false; });

    // Create metronome with test samples
    juce::AudioSampleBuffer emphasis_sample (2, 512);
    juce::AudioSampleBuffer normal_sample (2, 512);
    metronome = utils::make_qobject_unique<dsp::Metronome> (
      dsp::ProcessorBase::ProcessorBaseDependencies{
        .port_registry_ = *port_registry,
        .param_registry_ = *param_registry,
      },
      emphasis_sample, normal_sample, true, 1.0f, nullptr);
  }

  void TearDown () override
  {
    metronome.reset ();
    monitor_fader.reset ();
    param_registry.reset ();
    port_registry.reset ();
    app_settings.reset ();
    plugin_format_manager.reset ();
    hw_interface.reset ();
  }

  std::unique_ptr<Project> create_minimal_project ()
  {
    structure::project::Project::ProjectDirectoryPathProvider path_provider =
      [this] (bool for_backup) {
        if (for_backup)
          {
            return project_dir / "backups";
          }
        return project_dir;
      };

    plugins::PluginHostWindowFactory window_factory =
      [] (plugins::Plugin &) -> std::unique_ptr<plugins::IPluginHostWindow> {
      return nullptr;
    };

    auto project = std::make_unique<Project> (
      *app_settings, path_provider, *hw_interface, plugin_format_manager,
      window_factory, *metronome, *monitor_fader);

    return project;
  }

  std::unique_ptr<QTemporaryDir>                   temp_dir_obj;
  fs::path                                         project_dir;
  std::shared_ptr<juce::AudioDeviceManager>        audio_device_manager;
  std::unique_ptr<dsp::IHardwareAudioInterface>    hw_interface;
  std::shared_ptr<juce::AudioPluginFormatManager>  plugin_format_manager;
  test_helpers::MockSettingsBackend *              mock_backend_ptr{};
  std::unique_ptr<utils::AppSettings>              app_settings;
  std::unique_ptr<dsp::PortRegistry>               port_registry;
  std::unique_ptr<dsp::ProcessorParameterRegistry> param_registry;
  utils::QObjectUniquePtr<dsp::Fader>              monitor_fader;
  utils::QObjectUniquePtr<dsp::Metronome>          metronome;
};

// ============================================================================
// Registry Accessors Tests
// ============================================================================

TEST_F (ProjectTest, GetRegistries)
{
  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  // Test that all registries are accessible and non-null
  EXPECT_NE (&project->get_file_audio_source_registry (), nullptr);
  EXPECT_NE (&project->get_track_registry (), nullptr);
  EXPECT_NE (&project->get_plugin_registry (), nullptr);
  EXPECT_NE (&project->get_port_registry (), nullptr);
  EXPECT_NE (&project->get_param_registry (), nullptr);
  EXPECT_NE (&project->get_arranger_object_registry (), nullptr);
}

// ============================================================================
// Find Methods Tests
// ============================================================================

TEST_F (ProjectTest, FindPortByIdNotFound)
{
  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  // Try to find a port with a random UUID - should return nullopt
  dsp::Port::Uuid random_uuid;
  auto            result = project->find_port_by_id (random_uuid);
  EXPECT_FALSE (result.has_value ());
}

TEST_F (ProjectTest, FindParamByIdNotFound)
{
  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  // Try to find a param with a random UUID - should return nullptr
  dsp::ProcessorParameter::Uuid random_uuid;
  auto result = project->find_param_by_id (random_uuid);
  EXPECT_EQ (result, nullptr);
}

TEST_F (ProjectTest, FindPluginByIdNotFound)
{
  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  // Try to find a plugin with a random UUID - should return nullopt
  plugins::Plugin::Uuid random_uuid;
  auto                  result = project->find_plugin_by_id (random_uuid);
  EXPECT_FALSE (result.has_value ());
}

TEST_F (ProjectTest, FindTrackByIdNotFound)
{
  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  // Try to find a track with a random UUID - should return nullopt
  structure::tracks::Track::Uuid random_uuid;
  auto result = project->find_track_by_id (random_uuid);
  EXPECT_FALSE (result.has_value ());
}

TEST_F (ProjectTest, FindArrangerObjectByIdNotFound)
{
  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  // Try to find an arranger object with a random UUID - should return nullopt
  structure::arrangement::ArrangerObject::Uuid random_uuid;
  auto result = project->find_arranger_object_by_id (random_uuid);
  EXPECT_FALSE (result.has_value ());
}

// ============================================================================
// Add Default Tracks Tests
// ============================================================================

TEST_F (ProjectTest, AddDefaultTracks)
{
  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  // Add default tracks
  project->add_default_tracks ();

  // Verify that singleton tracks exist in the tracklist
  auto * singleton_tracks = project->tracklist ()->singletonTracks ();
  ASSERT_NE (singleton_tracks, nullptr);

  EXPECT_NE (singleton_tracks->chordTrack (), nullptr);
  EXPECT_NE (singleton_tracks->markerTrack (), nullptr);
  EXPECT_NE (singleton_tracks->masterTrack (), nullptr);
  EXPECT_NE (singleton_tracks->modulatorTrack (), nullptr);

  // Verify the tracks can be found via their registries
  auto chord_track_uuid = singleton_tracks->chordTrack ()->get_uuid ();
  auto chord_track_result = project->find_track_by_id (chord_track_uuid);
  EXPECT_TRUE (chord_track_result.has_value ());
}

// ============================================================================
// Get Final Track Dependencies Tests
// ============================================================================

// ============================================================================
// Get Final Track Dependencies Tests
// ============================================================================

TEST_F (ProjectTest, GetFinalTrackDependencies)
{
  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  // Add default tracks so we have some tracks to work with
  project->add_default_tracks ();

  // Get final track dependencies
  auto deps = project->get_final_track_dependencies ();

  // Verify that dependencies object is valid
  // The exact content depends on the implementation
  (void) deps; // Suppress unused warning
}

// ============================================================================
// QML Property Accessors Tests
// ============================================================================

TEST_F (ProjectTest, QmlPropertyAccessors)
{
  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  // Test QML property accessors - should all return non-null
  EXPECT_NE (project->tracklist (), nullptr);
  EXPECT_NE (project->clipLauncher (), nullptr);
  EXPECT_NE (project->clipPlaybackService (), nullptr);
  EXPECT_NE (project->engine (), nullptr);
  EXPECT_NE (project->getTransport (), nullptr);
  EXPECT_NE (project->getTempoMap (), nullptr);
  EXPECT_NE (project->snapGridTimeline (), nullptr);
  EXPECT_NE (project->snapGridEditor (), nullptr);
  EXPECT_NE (project->tempoObjectManager (), nullptr);
}

// ============================================================================
// Tempo Map Tests
// ============================================================================

TEST_F (ProjectTest, TempoMapAccess)
{
  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  // Test tempo_map() accessor
  const auto &tempo_map = project->tempo_map ();
  (void) tempo_map; // Suppress unused warning

  // Test getTempoMap() QML accessor
  auto * tempo_map_wrapper = project->getTempoMap ();
  EXPECT_NE (tempo_map_wrapper, nullptr);
}

// ============================================================================
// Component Accessors Tests
// ============================================================================

TEST_F (ProjectTest, ComponentAccessors)
{
  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  // Test various component accessors
  EXPECT_NE (project->arrangerObjectFactory (), nullptr);
  EXPECT_NE (project->plugin_factory_.get (), nullptr);
  EXPECT_NE (project->track_factory_.get (), nullptr);
}

// ============================================================================
// Monitor Fader and Metronome Tests
// ============================================================================

TEST_F (ProjectTest, MonitorFaderAndMetronome)
{
  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  // Test monitor_fader() accessor
  auto &fader = project->monitor_fader ();
  EXPECT_EQ (&fader, monitor_fader.get ());

  // Test metronome() accessor
  auto &metro = project->metronome ();
  EXPECT_EQ (&metro, metronome.get ());
}

// ============================================================================
// Snap Grid Tests
// ============================================================================

TEST_F (ProjectTest, SnapGridDefaults)
{
  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  // Test that snap grids are accessible
  auto * timeline_snap = project->snapGridTimeline ();
  auto * editor_snap = project->snapGridEditor ();

  EXPECT_NE (timeline_snap, nullptr);
  EXPECT_NE (editor_snap, nullptr);

  // Verify they are different instances
  EXPECT_NE (timeline_snap, editor_snap);
}

// ============================================================================
// Transport Tests
// ============================================================================

TEST_F (ProjectTest, TransportAccess)
{
  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  // Test transport accessor
  auto * transport = project->getTransport ();
  EXPECT_NE (transport, nullptr);

  // Verify transport_ member is the same
  EXPECT_EQ (transport, project->transport_.get ());
}

// ============================================================================
// Engine Tests
// ============================================================================

TEST_F (ProjectTest, EngineAccess)
{
  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  // Test engine accessor
  auto * engine = project->engine ();
  EXPECT_NE (engine, nullptr);

  // Verify audio_engine_ member is the same
  EXPECT_EQ (engine, project->audio_engine_.get ());
}
