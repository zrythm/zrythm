// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/audio_input_selection.h"
#include "dsp/graph.h"
#include "structure/project/project.h"
#include "structure/project/project_graph_builder.h"
#include "structure/tracks/audio_track.h"
#include "structure/tracks/track.h"
#include "utils/app_settings.h"
#include "utils/io_utils.h"
#include "utils/object_registry.h"

#include "helpers/mock_hardware_audio_interface.h"
#include "helpers/mock_hardware_midi_interface.h"
#include "helpers/mock_settings_backend.h"
#include "helpers/scoped_juce_qapplication.h"

#include <gtest/gtest.h>
#include <juce_audio_processors/juce_audio_processors.h>

namespace zrythm::structure::project
{
using namespace ::testing;

class ProjectGraphBuilderTest
    : public ::testing::Test,
      public test_helpers::ScopedJuceQApplication
{
protected:
  void SetUp () override
  {
    temp_dir_obj_ = utils::io::make_tmp_dir ();
    project_dir_ =
      utils::Utf8String::from_qstring (temp_dir_obj_->path ()).to_path ();

    hw_interface_ =
      std::make_unique<test_helpers::MockHardwareAudioInterface> ();

    plugin_format_manager_ = std::make_shared<juce::AudioPluginFormatManager> ();
    juce::addDefaultFormatsToManager (*plugin_format_manager_);

    auto mock_backend = std::make_unique<test_helpers::MockSettingsBackend> ();
    mock_backend_ptr_ = mock_backend.get ();
    ON_CALL (*mock_backend_ptr_, value (_, _))
      .WillByDefault (Return (QVariant ()));

    app_settings_ =
      std::make_unique<utils::AppSettings> (std::move (mock_backend));

    registry_ = std::make_unique<utils::ObjectRegistry> ();
    monitor_fader_ = utils::make_qobject_unique<dsp::Fader> (
      *registry_, dsp::PortType::Audio, true, false,
      [] () -> utils::Utf8String { return u8"Test Control Room"; },
      [] (bool) { return false; });

    juce::AudioSampleBuffer emphasis_sample (2, 512);
    juce::AudioSampleBuffer normal_sample (2, 512);
    metronome_ = utils::make_qobject_unique<dsp::Metronome> (
      *registry_, emphasis_sample, normal_sample, true, 1.0f, nullptr);
  }

  void TearDown () override
  {
    metronome_.reset ();
    monitor_fader_.reset ();
    registry_.reset ();
    app_settings_.reset ();
    plugin_format_manager_.reset ();
    hw_interface_.reset ();
  }

  std::unique_ptr<Project> create_project ()
  {
    Project::ProjectDirectoryPathProvider path_provider =
      [this] (bool for_backup) {
        return for_backup ? project_dir_ / "backups" : project_dir_;
      };

    plugins::PluginHostWindowFactory window_factory =
      [] (plugins::Plugin &) -> std::unique_ptr<plugins::IPluginHostWindow> {
      return nullptr;
    };

    auto project = std::make_unique<Project> (
      *app_settings_, path_provider, *hw_interface_, midi_interface_,
      plugin_format_manager_, window_factory, *metronome_, *monitor_fader_);
    project->install_recording_callback (
      [] (
        const tracks::Track::Uuid &, units::sample_t, const dsp::ITransport &,
        std::optional<std::span<const dsp::MidiEvent>>,
        std::optional<tracks::TrackProcessor::ConstStereoPortPair>,
        units::sample_u32_t) { });
    return project;
  }

  tracks::AudioBusTrack * add_audio_bus_track (Project &project)
  {
    auto track_ref =
      project.track_factory_->create_empty_track<tracks::AudioBusTrack> ();
    auto * raw = track_ref.get_object_as<tracks::AudioBusTrack> ();
    raw->setName (u8"Audio Bus");
    project.tracklist ()->collection ()->add_track (track_ref);
    return raw;
  }

  tracks::AudioTrack * add_audio_track (Project &project)
  {
    auto track_ref =
      project.track_factory_->create_empty_track<tracks::AudioTrack> ();
    auto * raw = track_ref.get_object_as<tracks::AudioTrack> ();
    raw->setName (u8"Audio Track");
    project.tracklist ()->collection ()->add_track (track_ref);
    return raw;
  }

  bool has_audio_input_connection (
    dsp::graph::Graph &graph,
    Project           &project,
    tracks::Track     &track)
  {
    auto * audio_in_processor = project.engine ()->audio_input_processor ();
    if (audio_in_processor == nullptr)
      return false;

    auto * src_port = audio_in_processor->find_output_port (0, true);
    if (src_port == nullptr)
      return false;

    auto * src_node = graph.get_nodes ().find_node_for_processable (*src_port);
    auto * dst_node = graph.get_nodes ().find_node_for_processable (
      track.get_track_processor ()->get_stereo_in_port ());
    if (src_node == nullptr || dst_node == nullptr)
      return false;

    const auto &feeds = src_node->feeds ();
    return std::ranges::any_of (feeds, [dst_node] (const auto &ref) {
      return &ref.get () == dst_node;
    });
  }

  std::unique_ptr<QTemporaryDir>                  temp_dir_obj_;
  std::filesystem::path                           project_dir_;
  std::unique_ptr<dsp::IHardwareAudioInterface>   hw_interface_;
  test_helpers::MockHardwareMidiInterface         midi_interface_;
  std::shared_ptr<juce::AudioPluginFormatManager> plugin_format_manager_;
  test_helpers::MockSettingsBackend *             mock_backend_ptr_{};
  std::unique_ptr<utils::AppSettings>             app_settings_;
  std::unique_ptr<utils::ObjectRegistry>          registry_;
  utils::QObjectUniquePtr<dsp::Fader>             monitor_fader_;
  utils::QObjectUniquePtr<dsp::Metronome>         metronome_;
};

TEST_F (ProjectGraphBuilderTest, DefaultTracksGraphIsValidAfterFinalizeNodes)
{
  auto project = create_project ();
  project->add_default_tracks ();

  dsp::graph::Graph   graph;
  ProjectGraphBuilder builder (*project, *metronome_, *monitor_fader_);
  builder.build_graph (graph);
  graph.finalize_nodes ();

  EXPECT_TRUE (graph.is_valid ())
    << "Default tracks should produce a valid graph";
}

TEST_F (ProjectGraphBuilderTest, AudioBusTrackWithNoRouteTargetGraphIsValid)
{
  auto project = create_project ();
  project->add_default_tracks ();
  add_audio_bus_track (*project);

  dsp::graph::Graph   graph;
  ProjectGraphBuilder builder (*project, *metronome_, *monitor_fader_);
  builder.build_graph (graph);
  graph.finalize_nodes ();

  EXPECT_TRUE (graph.is_valid ())
    << "AudioBusTrack with no route target should still produce a valid graph";
}

TEST_F (ProjectGraphBuilderTest, AudioBusTrackRoutedToMasterGraphIsValid)
{
  auto project = create_project ();
  project->add_default_tracks ();
  auto * track = add_audio_bus_track (*project);

  project->tracklist ()->trackRouting ()->add_or_replace_route (
    track->get_uuid (),
    project->tracklist ()->singletonTracks ()->masterTrack ()->get_uuid ());

  dsp::graph::Graph   graph;
  ProjectGraphBuilder builder (*project, *metronome_, *monitor_fader_);
  builder.build_graph (graph);
  graph.finalize_nodes ();

  EXPECT_TRUE (graph.is_valid ())
    << "AudioBusTrack routed to master should produce a valid graph";
}

TEST_F (ProjectGraphBuilderTest, AudioInputProviderWithNoSelectionsGraphIsValid)
{
  auto project = create_project ();
  project->add_default_tracks ();
  add_audio_bus_track (*project);

  project->set_audio_input_selection_provider (
    [] (const structure::tracks::Track::Uuid &) -> dsp::AudioInputSelection * {
      return nullptr;
    });

  dsp::graph::Graph   graph;
  ProjectGraphBuilder builder (*project, *metronome_, *monitor_fader_);
  builder.build_graph (graph);
  graph.finalize_nodes ();

  EXPECT_TRUE (graph.is_valid ())
    << "Graph should be valid when provider returns nullptr for all tracks";
}

TEST_F (ProjectGraphBuilderTest, AudioInputConnectedWhenDeviceNameMatches)
{
  const auto device_name = "Test Device";
  auto * mock_hw = dynamic_cast<test_helpers::MockHardwareAudioInterface *> (
    hw_interface_.get ());
  ASSERT_NE (mock_hw, nullptr);
  mock_hw->set_device_info (
    {
      .device_name = utils::Utf8String::from_utf8_encoded_string (device_name),
      .sample_rate = units::sample_rate (48000),
      .block_length = units::samples (256),
      .input_channel_count = units::channels (2),
      .output_channel_count = units::channels (2),
    });

  auto project = create_project ();
  project->add_default_tracks ();
  auto * track = add_audio_track (*project);

  auto selection = std::make_unique<dsp::AudioInputSelection> ();
  selection->setDeviceName (QString::fromStdString (device_name));
  selection->setFirstChannel (0);
  selection->setStereo (true);
  auto * raw_sel = selection.get ();

  project->set_audio_input_selection_provider (
    [raw_sel] (const structure::tracks::Track::Uuid &)
      -> dsp::AudioInputSelection * { return raw_sel; });

  project->engine ()->activate ();

  dsp::graph::Graph   graph;
  ProjectGraphBuilder builder (*project, *metronome_, *monitor_fader_);
  builder.build_graph (graph);
  graph.finalize_nodes ();

  EXPECT_TRUE (has_audio_input_connection (graph, *project, *track));
}

TEST_F (ProjectGraphBuilderTest, AudioInputNotConnectedWhenDeviceNameDiffers)
{
  auto * mock_hw = dynamic_cast<test_helpers::MockHardwareAudioInterface *> (
    hw_interface_.get ());
  ASSERT_NE (mock_hw, nullptr);
  mock_hw->set_device_info (
    {
      .device_name =
        utils::Utf8String::from_utf8_encoded_string ("Current Device"),
      .sample_rate = units::sample_rate (48000),
      .block_length = units::samples (256),
      .input_channel_count = units::channels (2),
      .output_channel_count = units::channels (2),
    });

  auto project = create_project ();
  project->add_default_tracks ();
  auto * track = add_audio_track (*project);

  auto selection = std::make_unique<dsp::AudioInputSelection> ();
  selection->setDeviceName ("Different Device");
  selection->setFirstChannel (0);
  selection->setStereo (true);
  auto * raw_sel = selection.get ();

  project->set_audio_input_selection_provider (
    [raw_sel] (const structure::tracks::Track::Uuid &)
      -> dsp::AudioInputSelection * { return raw_sel; });

  project->engine ()->activate ();

  dsp::graph::Graph   graph;
  ProjectGraphBuilder builder (*project, *metronome_, *monitor_fader_);
  builder.build_graph (graph);
  graph.finalize_nodes ();

  EXPECT_FALSE (has_audio_input_connection (graph, *project, *track));
}

TEST_F (ProjectGraphBuilderTest, AudioInputNotConnectedWhenDeviceNameEmpty)
{
  auto * mock_hw = dynamic_cast<test_helpers::MockHardwareAudioInterface *> (
    hw_interface_.get ());
  ASSERT_NE (mock_hw, nullptr);
  mock_hw->set_device_info (
    {
      .device_name = utils::Utf8String::from_utf8_encoded_string ("Test Device"),
      .sample_rate = units::sample_rate (48000),
      .block_length = units::samples (256),
      .input_channel_count = units::channels (2),
      .output_channel_count = units::channels (2),
    });

  auto project = create_project ();
  project->add_default_tracks ();
  auto * track = add_audio_track (*project);

  auto selection = std::make_unique<dsp::AudioInputSelection> ();
  selection->setDeviceName ("");
  selection->setFirstChannel (0);
  selection->setStereo (true);
  auto * raw_sel = selection.get ();

  project->set_audio_input_selection_provider (
    [raw_sel] (const structure::tracks::Track::Uuid &)
      -> dsp::AudioInputSelection * { return raw_sel; });

  project->engine ()->activate ();

  dsp::graph::Graph   graph;
  ProjectGraphBuilder builder (*project, *metronome_, *monitor_fader_);
  builder.build_graph (graph);
  graph.finalize_nodes ();

  EXPECT_FALSE (has_audio_input_connection (graph, *project, *track));
}

} // namespace zrythm::structure::project
