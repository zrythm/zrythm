// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 * Integration tests for AudioInputProcessor wiring in the DSP graph.
 *
 * Verifies that the graph builder correctly adds the AudioInputProcessor node
 * and that the graph remains valid when audio input selections are configured
 * for tracks.
 */

#include "dsp/audio_input_processor.h"
#include "dsp/audio_port.h"
#include "dsp/graph.h"
#include "structure/project/project.h"
#include "structure/project/project_graph_builder.h"
#include "structure/project/project_ui_state.h"
#include "structure/tracks/track.h"

#include "helpers/project_fixture.h"

#include <gtest/gtest.h>

namespace zrythm
{

class AudioInputGraphTest : public test_helpers::ProjectTestFixture
{
protected:
  void SetUp () override
  {
    ProjectTestFixture::SetUp ();
    project_ = create_minimal_project ();
    project_->add_default_tracks ();
    ui_state_ = utils::make_qobject_unique<structure::project::ProjectUiState> (
      *project_, *app_settings_);
  }

  void TearDown () override
  {
    ui_state_.reset ();
    project_.reset ();
    ProjectTestFixture::TearDown ();
  }

  structure::tracks::AudioBusTrack * add_audio_bus_track ()
  {
    auto track_ref = project_->track_factory_->create_empty_track<
      structure::tracks::AudioBusTrack> ();
    auto * raw = track_ref.get_object_as<structure::tracks::AudioBusTrack> ();
    raw->setName (u8"Audio Bus");
    project_->tracklist ()->collection ()->add_track (track_ref);
    return raw;
  }

  void set_audio_input_selection (
    const structure::tracks::Track &track,
    const QString                  &device_name,
    int                             first_channel,
    bool                            stereo)
  {
    auto * sel = ui_state_->audioInputSelectionForTrack (&track);
    sel->setDeviceName (device_name);
    sel->setFirstChannel (first_channel);
    sel->setStereo (stereo);
  }

  void set_provider ()
  {
    project_->set_audio_input_selection_provider (
      [this] (const structure::tracks::Track::Uuid &uuid)
        -> dsp::AudioInputSelection * {
        return ui_state_->find_audio_input_selection (uuid);
      });
  }

  std::unique_ptr<structure::project::Project>                project_;
  utils::QObjectUniquePtr<structure::project::ProjectUiState> ui_state_;
};

TEST_F (AudioInputGraphTest, GraphContainsAudioInputProcessorNode)
{
  set_provider ();

  project_->engine ()->activate ();

  dsp::graph::Graph                       graph;
  structure::project::ProjectGraphBuilder builder (
    *project_, *metronome_, *monitor_fader_);
  builder.build_graph (graph);

  auto * engine = project_->engine ();
  ASSERT_NE (engine, nullptr);
  auto * aip = engine->audio_input_processor ();
  ASSERT_NE (aip, nullptr);

  auto * aip_node = graph.get_nodes ().find_node_for_processable (*aip);
  ASSERT_NE (aip_node, nullptr) << "AudioInputProcessor should be a graph node";
}

TEST_F (AudioInputGraphTest, GraphIsValidWithAudioInputSelection)
{
  auto * track = add_audio_bus_track ();
  ASSERT_NE (track, nullptr);

  set_audio_input_selection (*track, u"Test Device"_s, 0, true);
  set_provider ();

  project_->engine ()->activate ();

  dsp::graph::Graph                       graph;
  structure::project::ProjectGraphBuilder builder (
    *project_, *metronome_, *monitor_fader_);
  builder.build_graph (graph);
  graph.finalize_nodes ();

  EXPECT_TRUE (graph.is_valid ())
    << "Graph should remain acyclic with audio input connections";
}

TEST_F (AudioInputGraphTest, NoAudioInputConnectionWithEmptyDeviceName)
{
  auto * track = add_audio_bus_track ();
  ASSERT_NE (track, nullptr);

  set_audio_input_selection (*track, u""_s, 0, true);
  set_provider ();

  project_->engine ()->activate ();

  dsp::graph::Graph                       graph;
  structure::project::ProjectGraphBuilder builder (
    *project_, *metronome_, *monitor_fader_);
  builder.build_graph (graph);
  graph.finalize_nodes ();

  EXPECT_TRUE (graph.is_valid ());

  auto * aip = project_->engine ()->audio_input_processor ();
  ASSERT_NE (aip, nullptr);
  auto * stereo_in_port = &track->get_track_processor ()->get_stereo_in_port ();
  auto * dst_node =
    graph.get_nodes ().find_node_for_processable (*stereo_in_port);
  ASSERT_NE (dst_node, nullptr);

  if (auto * src_port = aip->find_output_port (0, true))
    {
      auto * src_node = graph.get_nodes ().find_node_for_processable (*src_port);
      if (src_node)
        {
          auto * aip_node = graph.get_nodes ().find_node_for_processable (*aip);
          bool   connected = false;
          for (auto &child : aip_node->feeds ())
            {
              if (&child.get () == dst_node)
                connected = true;
            }
          EXPECT_FALSE (connected)
            << "No connection should exist when device name is empty";
        }
    }
}

TEST_F (AudioInputGraphTest, AudioInputPortConnectedToTrackWithSelection)
{
  auto * track = add_audio_bus_track ();
  ASSERT_NE (track, nullptr);

  set_audio_input_selection (*track, u"Test Device"_s, 0, true);
  set_provider ();

  project_->engine ()->activate ();

  dsp::graph::Graph                       graph;
  structure::project::ProjectGraphBuilder builder (
    *project_, *metronome_, *monitor_fader_);
  builder.build_graph (graph);
  graph.finalize_nodes ();

  EXPECT_TRUE (graph.is_valid ());

  auto * aip = project_->engine ()->audio_input_processor ();
  ASSERT_NE (aip, nullptr);
  auto * src_port = aip->find_output_port (0, true);
  ASSERT_NE (src_port, nullptr)
    << "AudioInputProcessor should have stereo port for channels 0-1";

  auto * src_node = graph.get_nodes ().find_node_for_processable (*src_port);
  ASSERT_NE (src_node, nullptr);

  auto  &stereo_in = track->get_track_processor ()->get_stereo_in_port ();
  auto * dst_node = graph.get_nodes ().find_node_for_processable (stereo_in);
  ASSERT_NE (dst_node, nullptr);

  bool found_connection = false;
  for (auto &child : src_node->feeds ())
    {
      if (&child.get () == dst_node)
        found_connection = true;
    }
  EXPECT_TRUE (found_connection)
    << "AudioInputProcessor output port should be connected to track stereo in";
}

TEST_F (AudioInputGraphTest, ProcessingDoesNotCrashWithAudioInput)
{
  auto * track = add_audio_bus_track ();
  ASSERT_NE (track, nullptr);
  set_audio_input_selection (*track, u"Test Device"_s, 0, true);
  set_provider ();

  auto * mock_hw = dynamic_cast<
    test_helpers::ThreadedMockHardwareAudioInterface *> (hw_interface_.get ());
  project_->engine ()->activate ();
  project_->engine ()->graph_dispatcher ().recalc_graph (false);
  project_->engine ()->set_running (true);

  const auto initial_count = mock_hw->process_call_count ();
  process_events_until_timeout ();
  project_->engine ()->set_running (false);
  project_->engine ()->deactivate ();

  EXPECT_GT (mock_hw->process_call_count (), initial_count)
    << "At least one processing cycle should have run";
}

} // namespace zrythm
