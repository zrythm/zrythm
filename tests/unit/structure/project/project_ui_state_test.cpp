// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/project/project.h"
#include "structure/project/project_ui_state.h"
#include "structure/tracks/track.h"
#include "utils/app_settings.h"
#include "utils/io_utils.h"
#include "utils/object_registry.h"
#include "utils/qt.h"
#include "utils/registry_utils.h"

#include <QSignalSpy>

#include "helpers/mock_hardware_audio_interface.h"
#include "helpers/mock_hardware_midi_interface.h"
#include "helpers/mock_settings_backend.h"
#include "helpers/scoped_juce_qapplication.h"

#include <gtest/gtest.h>
#include <juce_audio_processors/juce_audio_processors.h>

namespace zrythm::structure::project
{
using namespace ::testing;

class ProjectUiStateTest
    : public ::testing::Test,
      public test_helpers::ScopedJuceQApplication
{
protected:
  void SetUp () override
  {
    temp_dir_obj = utils::io::make_tmp_dir ();
    project_dir =
      utils::Utf8String::from_qstring (temp_dir_obj->path ()).to_path ();

    hw_interface = std::make_unique<test_helpers::MockHardwareAudioInterface> ();

    plugin_format_manager = std::make_shared<juce::AudioPluginFormatManager> ();
    juce::addDefaultFormatsToManager (*plugin_format_manager);

    auto mock_backend = std::make_unique<test_helpers::MockSettingsBackend> ();
    mock_backend_ptr = mock_backend.get ();
    ON_CALL (*mock_backend_ptr, value (testing::_, testing::_))
      .WillByDefault (testing::Return (QVariant ()));

    app_settings =
      std::make_unique<utils::AppSettings> (std::move (mock_backend));

    registry_ = std::make_unique<utils::ObjectRegistry> ();
    monitor_fader = utils::make_qobject_unique<dsp::Fader> (
      *registry_, dsp::PortType::Audio, true, false,
      [] () -> utils::Utf8String { return u8"Test Control Room"; },
      [] (bool) { return false; });

    juce::AudioSampleBuffer emphasis_sample (2, 512);
    juce::AudioSampleBuffer normal_sample (2, 512);
    metronome = utils::make_qobject_unique<dsp::Metronome> (
      *registry_, emphasis_sample, normal_sample, true, 1.0f, nullptr);
  }

  void TearDown () override
  {
    ui_state.reset ();
    project.reset ();
    metronome.reset ();
    monitor_fader.reset ();
    registry_.reset ();
    app_settings.reset ();
    plugin_format_manager.reset ();
    hw_interface.reset ();
  }

  std::unique_ptr<Project> create_minimal_project ()
  {
    Project::ProjectDirectoryPathProvider path_provider =
      [this] (bool for_backup) {
        if (for_backup)
          return project_dir / "backups";
        return project_dir;
      };

    plugins::PluginHostWindowFactory window_factory =
      [] (plugins::Plugin &) -> std::unique_ptr<plugins::IPluginHostWindow> {
      return nullptr;
    };

    auto proj = std::make_unique<Project> (
      *app_settings, path_provider, *hw_interface, midi_interface_,
      plugin_format_manager, window_factory, *metronome, *monitor_fader);
    proj->install_recording_callback (
      [] (
        const tracks::Track::Uuid &, units::sample_t, const dsp::ITransport &,
        std::optional<std::span<const dsp::MidiEvent>>,
        std::optional<tracks::TrackProcessor::ConstStereoPortPair>,
        units::sample_u32_t) { });
    return proj;
  }

  // Note: add_default_tracks() creates Chord/Modulator/Marker/Master tracks,
  // but no Audio track. audioInputSelectionForTrack() works on any track type
  // so we just grab the first one.
  static structure::tracks::Track * get_first_track (Project &project)
  {
    auto &tracks = project.tracklist ()->collection ()->tracks ();
    return tracks.empty () ? nullptr : tracks.front ().get ();
  }

  std::unique_ptr<QTemporaryDir>                  temp_dir_obj;
  std::filesystem::path                           project_dir;
  std::unique_ptr<dsp::IHardwareAudioInterface>   hw_interface;
  test_helpers::MockHardwareMidiInterface         midi_interface_;
  std::shared_ptr<juce::AudioPluginFormatManager> plugin_format_manager;
  test_helpers::MockSettingsBackend *             mock_backend_ptr{};
  std::unique_ptr<utils::AppSettings>             app_settings;
  std::unique_ptr<utils::ObjectRegistry>          registry_;
  utils::QObjectUniquePtr<dsp::Fader>             monitor_fader;
  utils::QObjectUniquePtr<dsp::Metronome>         metronome;
  std::unique_ptr<Project>                        project;
  utils::QObjectUniquePtr<ProjectUiState>         ui_state;
};

TEST_F (ProjectUiStateTest, Construction)
{
  project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  ui_state =
    utils::make_qobject_unique<ProjectUiState> (*project, *app_settings);
  ASSERT_NE (ui_state.get (), nullptr);
}

TEST_F (ProjectUiStateTest, ToolAccessor)
{
  project = create_minimal_project ();
  ui_state =
    utils::make_qobject_unique<ProjectUiState> (*project, *app_settings);

  auto * tool = ui_state->tool ();
  ASSERT_NE (tool, nullptr);
}

TEST_F (ProjectUiStateTest, ClipEditorAccessor)
{
  project = create_minimal_project ();
  ui_state =
    utils::make_qobject_unique<ProjectUiState> (*project, *app_settings);

  auto * clip_editor = ui_state->clipEditor ();
  ASSERT_NE (clip_editor, nullptr);
}

TEST_F (ProjectUiStateTest, TimelineAccessor)
{
  project = create_minimal_project ();
  ui_state =
    utils::make_qobject_unique<ProjectUiState> (*project, *app_settings);

  auto * timeline = ui_state->timeline ();
  ASSERT_NE (timeline, nullptr);
}

TEST_F (ProjectUiStateTest, SnapGridTimelineAccessor)
{
  project = create_minimal_project ();
  ui_state =
    utils::make_qobject_unique<ProjectUiState> (*project, *app_settings);

  auto * snap_grid = ui_state->snapGridTimeline ();
  ASSERT_NE (snap_grid, nullptr);
}

TEST_F (ProjectUiStateTest, SnapGridEditorAccessor)
{
  project = create_minimal_project ();
  ui_state =
    utils::make_qobject_unique<ProjectUiState> (*project, *app_settings);

  auto * snap_grid = ui_state->snapGridEditor ();
  ASSERT_NE (snap_grid, nullptr);
}

TEST_F (ProjectUiStateTest, AllAccessorsNonNull)
{
  project = create_minimal_project ();
  project->add_default_tracks ();
  ui_state =
    utils::make_qobject_unique<ProjectUiState> (*project, *app_settings);

  // Verify all accessors return non-null
  EXPECT_NE (ui_state->tool (), nullptr);
  EXPECT_NE (ui_state->clipEditor (), nullptr);
  EXPECT_NE (ui_state->timeline (), nullptr);
  EXPECT_NE (ui_state->snapGridTimeline (), nullptr);
  EXPECT_NE (ui_state->snapGridEditor (), nullptr);
}

// ========================================================================
// AudioInputSelection tests
// ========================================================================

TEST_F (ProjectUiStateTest, AudioInputSelectionNullTrackReturnsNull)
{
  project = create_minimal_project ();
  project->add_default_tracks ();
  ui_state =
    utils::make_qobject_unique<ProjectUiState> (*project, *app_settings);

  auto * sel = ui_state->audioInputSelectionForTrack (nullptr);
  EXPECT_EQ (sel, nullptr);
}

TEST_F (ProjectUiStateTest, AudioInputSelectionDefaultValues)
{
  project = create_minimal_project ();
  project->add_default_tracks ();
  ui_state =
    utils::make_qobject_unique<ProjectUiState> (*project, *app_settings);

  auto * track = get_first_track (*project);
  ASSERT_NE (track, nullptr);

  auto * sel = ui_state->audioInputSelectionForTrack (track);
  ASSERT_NE (sel, nullptr);
  EXPECT_EQ (sel->deviceName (), QString ());
  EXPECT_EQ (sel->firstChannel (), 0);
  EXPECT_TRUE (sel->stereo ());
}

TEST_F (ProjectUiStateTest, AudioInputSelectionLazyCreationReturnsSameObject)
{
  project = create_minimal_project ();
  project->add_default_tracks ();
  ui_state =
    utils::make_qobject_unique<ProjectUiState> (*project, *app_settings);

  auto * track = get_first_track (*project);
  auto * first = ui_state->audioInputSelectionForTrack (track);
  auto * second = ui_state->audioInputSelectionForTrack (track);
  EXPECT_EQ (first, second);
}

TEST_F (ProjectUiStateTest, AudioInputSelectionSetDeviceName)
{
  project = create_minimal_project ();
  project->add_default_tracks ();
  ui_state =
    utils::make_qobject_unique<ProjectUiState> (*project, *app_settings);

  auto *     track = get_first_track (*project);
  auto *     sel = ui_state->audioInputSelectionForTrack (track);
  QSignalSpy spy (sel, &dsp::AudioInputSelection::deviceNameChanged);
  sel->setDeviceName (u"Test Device"_s);
  EXPECT_EQ (spy.count (), 1);
  EXPECT_EQ (sel->deviceName (), u"Test Device"_s);
}

TEST_F (ProjectUiStateTest, AudioInputSelectionSetDeviceNameNoDuplicateSignal)
{
  project = create_minimal_project ();
  project->add_default_tracks ();
  ui_state =
    utils::make_qobject_unique<ProjectUiState> (*project, *app_settings);

  auto * track = get_first_track (*project);
  auto * sel = ui_state->audioInputSelectionForTrack (track);
  sel->setDeviceName (u"Test Device"_s);
  QSignalSpy spy (sel, &dsp::AudioInputSelection::deviceNameChanged);
  sel->setDeviceName (u"Test Device"_s);
  EXPECT_EQ (spy.count (), 0);
}

TEST_F (ProjectUiStateTest, AudioInputSelectionSerializationPrunesStaleEntries)
{
  project = create_minimal_project ();
  project->add_default_tracks ();
  ui_state =
    utils::make_qobject_unique<ProjectUiState> (*project, *app_settings);

  auto * track = get_first_track (*project);
  auto * sel = ui_state->audioInputSelectionForTrack (track);
  sel->setDeviceName (u"LiveDevice"_s);

  nlohmann::json j;
  to_json (j, *ui_state);

  ASSERT_TRUE (j.contains (ProjectUiState::kAudioInputSelectionsKey));
  const auto &selections = j[ProjectUiState::kAudioInputSelectionsKey];
  ASSERT_TRUE (selections.is_array ());
  for (const auto &entry : selections)
    {
      ASSERT_EQ (entry.size (), 2);
      structure::tracks::Track::Uuid uuid;
      entry[0].get_to (uuid);
      EXPECT_TRUE (utils::contains (project->get_registry (), uuid));
    }
}

TEST_F (ProjectUiStateTest, AudioInputSelectionSerializationRoundtrip)
{
  project = create_minimal_project ();
  project->add_default_tracks ();
  ui_state =
    utils::make_qobject_unique<ProjectUiState> (*project, *app_settings);

  auto * track = get_first_track (*project);
  auto * sel = ui_state->audioInputSelectionForTrack (track);
  sel->setDeviceName (u"My Device"_s);
  sel->setFirstChannel (4);
  sel->setStereo (false);

  nlohmann::json j;
  to_json (j, *ui_state);

  // Use the same project (same track UUIDs) — in real usage the project
  // would be deserialized first, restoring original UUIDs.
  auto restored_state =
    utils::make_qobject_unique<ProjectUiState> (*project, *app_settings);
  from_json (j, *restored_state);

  auto * restored_sel = restored_state->audioInputSelectionForTrack (track);
  ASSERT_NE (restored_sel, nullptr);
  EXPECT_EQ (restored_sel->deviceName (), u"My Device"_s);
  EXPECT_EQ (restored_sel->firstChannel (), 4);
  EXPECT_FALSE (restored_sel->stereo ());
}
}
