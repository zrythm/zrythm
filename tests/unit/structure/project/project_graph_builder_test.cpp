// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/audio_input_selection.h"
#include "dsp/graph.h"
#include "structure/project/project.h"
#include "structure/project/project_graph_builder.h"
#include "structure/tracks/track.h"
#include "utils/app_settings.h"
#include "utils/io_utils.h"

#include "helpers/mock_hardware_audio_interface.h"
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

    port_registry_ = std::make_unique<dsp::PortRegistry> (nullptr);
    param_registry_ = std::make_unique<dsp::ProcessorParameterRegistry> (
      *port_registry_, nullptr);
    monitor_fader_ = utils::make_qobject_unique<dsp::Fader> (
      dsp::ProcessorBase::ProcessorBaseDependencies{
        .port_registry_ = *port_registry_,
        .param_registry_ = *param_registry_,
      },
      dsp::PortType::Audio, true, false,
      [] () -> utils::Utf8String { return u8"Test Control Room"; },
      [] (bool) { return false; });

    juce::AudioSampleBuffer emphasis_sample (2, 512);
    juce::AudioSampleBuffer normal_sample (2, 512);
    metronome_ = utils::make_qobject_unique<dsp::Metronome> (
      dsp::ProcessorBase::ProcessorBaseDependencies{
        .port_registry_ = *port_registry_,
        .param_registry_ = *param_registry_,
      },
      emphasis_sample, normal_sample, true, 1.0f, nullptr);
  }

  void TearDown () override
  {
    metronome_.reset ();
    monitor_fader_.reset ();
    param_registry_.reset ();
    port_registry_.reset ();
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

    return std::make_unique<Project> (
      *app_settings_, path_provider, *hw_interface_, plugin_format_manager_,
      window_factory, *metronome_, *monitor_fader_);
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

  std::unique_ptr<QTemporaryDir>                   temp_dir_obj_;
  std::filesystem::path                            project_dir_;
  std::unique_ptr<dsp::IHardwareAudioInterface>    hw_interface_;
  std::shared_ptr<juce::AudioPluginFormatManager>  plugin_format_manager_;
  test_helpers::MockSettingsBackend *              mock_backend_ptr_{};
  std::unique_ptr<utils::AppSettings>              app_settings_;
  std::unique_ptr<dsp::PortRegistry>               port_registry_;
  std::unique_ptr<dsp::ProcessorParameterRegistry> param_registry_;
  utils::QObjectUniquePtr<dsp::Fader>              monitor_fader_;
  utils::QObjectUniquePtr<dsp::Metronome>          metronome_;
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

} // namespace zrythm::structure::project
