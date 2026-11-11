// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/fader.h"
#include "dsp/graph.h"
#include "dsp/midi_event.h"
#include "dsp/midi_port.h"
#include "dsp/port.h"
#include "dsp/processor_base.h"
#include "plugins/faust/faust_plugin.h"
#include "plugins/internal_plugin_base.h"
#include "plugins/plugin.h"
#include "plugins/plugin_group.h"
#include "structure/tracks/channel.h"
#include "structure/tracks/channel_send.h"
#include "structure/tracks/channel_subgraph_builder.h"
#include "utils/object_registry.h"
#include "utils/registry_utils.h"
#include "utils/views.h"

#include <QObject>

#include "../../dsp/graph_helpers.h"
#include "channel_subgraph_builder_test.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace zrythm::structure::tracks
{

// Mock processor for testing track processor outputs
class MockTrackProcessor : public dsp::ProcessorBase
{
public:
  MockTrackProcessor (utils::IObjectRegistry &registry)
      : dsp::ProcessorBase (registry, u8"MockTrackProcessor")
  {
  }

  MOCK_METHOD (
    void,
    custom_process_block,
    (dsp::graph::ProcessBlockInfo, const dsp::ITransport &, const dsp::TempoMap &),
    (noexcept, override));
  MOCK_METHOD (
    void,
    custom_prepare_for_processing,
    (const dsp::graph::GraphNode *, units::sample_rate_t, units::sample_u32_t),
    (override));
  MOCK_METHOD (void, custom_release_resources, (), (override));
};

// Track processor double whose MIDI output carries a fixed note every block.
class NoteSourceTrackProcessor : public dsp::ProcessorBase
{
public:
  NoteSourceTrackProcessor (utils::IObjectRegistry &registry, midi_byte_t pitch)
      : dsp::ProcessorBase (registry, u8"NoteSourceTrackProcessor"), pitch_ (pitch)
  {
    add_output_port (
      utils::create_object<dsp::MidiPort> (
        registry, u8"Track MIDI Out", dsp::PortFlow::Output));
  }

  void custom_process_block (
    dsp::graph::ProcessBlockInfo,
    const dsp::ITransport &,
    const dsp::TempoMap &) noexcept override
  {
    auto &out_port =
      *get_all_output_ports ().front ().get_object_as<dsp::MidiPort> ();
    const auto ev =
      dsp::midi_event::make_note_on (0, pitch_, 100, units::samples (0));
    out_port.buffer_.push_back (ev.time_, ev.data ());
  }

private:
  midi_byte_t pitch_;
};

std::vector<int>
pitches_on (const dsp::MidiPort &port)
{
  std::vector<int> pitches;
  for (const auto &view : port.buffer_)
    {
      const auto bytes = view.data ();
      if (bytes.size () >= 2)
        {
          pitches.push_back (bytes[1]);
        }
    }
  return pitches;
}

class ChannelSubgraphBuilderTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    registry_ = std::make_unique<utils::ObjectRegistry> ();

    name_provider_ = [] { return u8"Test Channel"; };
    should_be_muted_cb_ = [] (bool solo_status) { return false; };

    mock_transport_ = std::make_unique<dsp::graph_test::MockTransport> ();
    tempo_map_ = std::make_unique<dsp::TempoMap> (sample_rate_);
  }

  void TearDown () override
  {
    if (audio_channel_)
      {
        audio_channel_->get_fader ().release_resources ();
      }
    if (midi_channel_)
      {
        midi_channel_->get_fader ().release_resources ();
      }
    if (instrument_channel_)
      {
        instrument_channel_->get_fader ().release_resources ();
      }
  }

  std::unique_ptr<Channel> createAudioChannel ()
  {
    return std::make_unique<Channel> (
      *registry_, dsp::PortType::Audio, dsp::PortType::Audio, name_provider_,
      false, should_be_muted_cb_);
  }

  std::unique_ptr<Channel> createMidiChannel ()
  {
    return std::make_unique<Channel> (
      *registry_, dsp::PortType::Midi, dsp::PortType::Midi, name_provider_,
      false, should_be_muted_cb_);
  }

  std::unique_ptr<Channel> createInstrumentChannel ()
  {
    return std::make_unique<Channel> (
      *registry_, dsp::PortType::Midi, dsp::PortType::Audio, name_provider_,
      false, should_be_muted_cb_);
  }

  plugins::PluginUuidReference createMockPlugin (
    dsp::PortType input_type,
    dsp::PortType output_type,
    int           num_audio_inputs = 2,
    int           num_audio_outputs = 2,
    bool          has_midi_input = false,
    bool          has_midi_output = false)
  {
    // Create a plugin descriptor
    auto descriptor = std::make_unique<plugins::PluginDescriptor> ();
    descriptor->name_ = u8"Test Plugin";
    descriptor->protocol_ = plugins::Protocol::ProtocolType::Internal;

    // Create configuration
    plugins::PluginConfiguration config;
    config.descr_ = std::move (descriptor);

    auto ref = utils::create_object<plugins::FaustPlugin> (
      *registry_, *registry_, nullptr);
    auto * pl = ref.get_object_as<plugins::FaustPlugin> ();
    pl->set_configuration (config);

    // Add appropriate ports based on types
    if (input_type == dsp::PortType::Audio)
      {
        for (int i = 0; i < num_audio_inputs; ++i)
          {
            pl->add_input_port (createMockPort (
              dsp::PortType::Audio,
              utils::Utf8String::from_utf8_encoded_string (
                fmt::format ("Audio In {}", i + 1)),
              dsp::PortFlow::Input));
          }
      }
    if (output_type == dsp::PortType::Audio)
      {
        for (int i = 0; i < num_audio_outputs; ++i)
          {
            pl->add_output_port (createMockPort (
              dsp::PortType::Audio,
              utils::Utf8String::from_utf8_encoded_string (
                fmt::format ("Audio Out {}", i + 1)),
              dsp::PortFlow::Output));
          }
      }
    if (has_midi_input)
      {
        pl->add_input_port (createMockPort (
          dsp::PortType::Midi, u8"MIDI In", dsp::PortFlow::Input));
      }
    if (has_midi_output)
      {
        pl->add_output_port (createMockPort (
          dsp::PortType::Midi, u8"MIDI Out", dsp::PortFlow::Output));
      }

    return ref;
  }

  dsp::PortUuidReference createMockPort (
    dsp::PortType            type,
    const utils::Utf8String &name,
    dsp::PortFlow            flow,
    dsp::AudioPort::Purpose  purpose = dsp::AudioPort::Purpose::Main)
  {
    if (type == dsp::PortType::Audio)
      {
        return utils::create_object<dsp::AudioPort> (
          *registry_, name, flow, dsp::SpeakerArrangement::mono (), purpose);
      }
    else if (type == dsp::PortType::Midi)
      {
        return utils::create_object<dsp::MidiPort> (*registry_, name, flow);
      }
    throw std::runtime_error ("Unsupported port type");
  }

  std::unique_ptr<utils::ObjectRegistry>          registry_;
  Channel::NameProvider                           name_provider_;
  dsp::Fader::ShouldBeMutedCallback               should_be_muted_cb_;
  dsp::graph::Graph                               graph_;
  std::unique_ptr<dsp::graph_test::MockTransport> mock_transport_;
  std::unique_ptr<dsp::TempoMap>                  tempo_map_;
  units::sample_rate_t sample_rate_{ units::sample_rate (48000) };
  units::sample_u32_t  max_block_length_{ units::samples (1024u) };

  std::unique_ptr<Channel> audio_channel_;
  std::unique_ptr<Channel> midi_channel_;
  std::unique_ptr<Channel> instrument_channel_;

  // Helper functions for connection verification
  dsp::graph::GraphNode *
  findNodeForProcessable (const dsp::graph::IProcessable &processable)
  {
    return graph_.get_nodes ().find_node_for_processable (processable);
  }

  bool hasConnection (
    const dsp::graph::IProcessable &from,
    const dsp::graph::IProcessable &to)
  {
    auto * from_node = findNodeForProcessable (from);
    auto * to_node = findNodeForProcessable (to);

    if ((from_node == nullptr) || (to_node == nullptr))
      return false;

    // Check if to_node is in from_node's childnodes_
    return std::ranges::any_of (
      from_node->feeds (),
      [to_node] (const auto &child) { return &child.get () == to_node; });
  }

  size_t countNodes () { return graph_.get_nodes ().graph_nodes_.size (); }

  std::unique_ptr<MockTrackProcessor>
  createMockTrackProcessor (dsp::PortType type, int num_channels = 2)
  {
    auto processor = std::make_unique<MockTrackProcessor> (*registry_);

    // Add appropriate output ports
    if (type == dsp::PortType::Audio)
      {
        auto port_ref = utils::create_object<dsp::AudioPort> (
          *registry_, u8"Track Out", dsp::PortFlow::Output,
          num_channels == 1
            ? dsp::SpeakerArrangement::mono ()
            : dsp::SpeakerArrangement::stereo ());
        processor->add_output_port (port_ref);
      }
    else if (type == dsp::PortType::Midi)
      {
        auto port_ref = utils::create_object<dsp::MidiPort> (
          *registry_, u8"Track MIDI Out", dsp::PortFlow::Output);
        processor->add_output_port (port_ref);
      }

    return processor;
  }

  dsp::PortUuidReference
  getProcessorOutputPort (const dsp::ProcessorBase &processor)
  {
    return processor.get_all_output_ports ().front ();
  }

  template <typename TrackProcessorT>
  void
  addTrackProcessorToGraph (TrackProcessorT &track_processor, bool connect = true)
  {
    auto track_processor_node =
      graph_.add_node_for_processable (track_processor);
    for (const auto &port_ref : track_processor.get_all_output_ports ())
      {
        auto * port = port_ref.get ();
        auto * node = graph_.add_node_for_processable (*port);
        if (connect)
          {
            track_processor_node->connect_to (*node);
          }
      }
  }

  // Prepares all nodes and processes one block in topological order,
  // mirroring what the graph scheduler does per cycle.
  void process_graph_once (dsp::graph::Graph &graph)
  {
    graph.finalize_nodes ();
    ASSERT_TRUE (graph.is_valid ());
    for (auto &node : graph.get_nodes ().graph_nodes_)
      {
        node->get_processable ().prepare_for_processing (
          node.get (), sample_rate_, max_block_length_);
      }
    graph.get_nodes ().update_latencies ();

    std::vector<dsp::graph::GraphNode *> ready;
    for (auto &node : graph.get_nodes ().graph_nodes_)
      {
        if (node->init_refcount_ == 0)
          {
            ready.push_back (node.get ());
          }
      }
    while (!ready.empty ())
      {
        auto * node = ready.back ();
        ready.pop_back ();
        node->process (
          dsp::graph::ProcessBlockInfo{
            .transport_position_ = units::samples (0),
            .buffer_offset_ = units::samples (0),
            .nframes_ = max_block_length_ },
          units::samples (0), *mock_transport_, *tempo_map_);
        for (auto &child : node->feeds ())
          {
            if (--child.get ().init_refcount_ == 0)
              {
                ready.push_back (&child.get ());
              }
          }
      }
  }

  // Releases the resources allocated by process_graph_once().
  void release_graph_resources (dsp::graph::Graph &graph)
  {
    for (auto &node : graph.get_nodes ().graph_nodes_)
      {
        node->get_processable ().release_resources ();
      }
  }

  void verifyProcessorsConnected (
    const dsp::ProcessorBase &src_processor,
    const dsp::ProcessorBase &dest_processor)
  {
    const auto src_output_ports =
      src_processor.get_all_output_ports ()
      | std::views::transform (&utils::TypedUuidReference<dsp::Port>::get);
    const auto dest_input_ports =
      dest_processor.get_all_input_ports ()
      | std::views::transform (&utils::TypedUuidReference<dsp::Port>::get);

    auto src_midi_out_ports =
      src_output_ports | utils::views::qobject_cast_and_filter<dsp::MidiPort>;
    auto dest_midi_in_ports =
      dest_input_ports | utils::views::qobject_cast_and_filter<dsp::MidiPort>;
    auto src_audio_out_ports =
      src_output_ports | utils::views::qobject_cast_and_filter<dsp::AudioPort>;
    auto dest_audio_in_ports =
      dest_input_ports | utils::views::qobject_cast_and_filter<dsp::AudioPort>;

    const auto verify_conns =
      [this, &src_processor, &dest_processor] (const auto &ports) {
        const auto &[src_port, dest_port] = ports;
        EXPECT_TRUE (hasConnection (src_processor, *src_port));
        EXPECT_TRUE (hasConnection (*src_port, *dest_port));
        EXPECT_TRUE (hasConnection (*dest_port, dest_processor));
      };

    std::ranges::for_each (
      std::views::zip (src_midi_out_ports, dest_midi_in_ports), verify_conns);
    std::ranges::for_each (
      std::views::zip (src_audio_out_ports, dest_audio_in_ports), verify_conns);
  }

  void verifyBasicAudioChannelConnections (MockTrackProcessor &track_processor)
  {
    const auto &nodes = graph_.get_nodes ().graph_nodes_;
    EXPECT_GT (nodes.size (), 0);

    // Verify we have the expected processors
    auto * prefader =
      findNodeForProcessable (audio_channel_->get_audio_pre_fader ());
    auto * fader = findNodeForProcessable (audio_channel_->get_fader ());
    auto * postfader =
      findNodeForProcessable (audio_channel_->get_audio_post_fader ());

    EXPECT_NE (prefader, nullptr);
    EXPECT_NE (fader, nullptr);
    EXPECT_NE (postfader, nullptr);

    // Verify processors are connected correctly
    verifyProcessorsConnected (
      track_processor, audio_channel_->get_audio_pre_fader ());
    verifyProcessorsConnected (
      audio_channel_->get_audio_pre_fader (), audio_channel_->get_fader ());
    verifyProcessorsConnected (
      audio_channel_->get_fader (), audio_channel_->get_audio_post_fader ());
  }

  void verifyBasicMidiChannelConnections (MockTrackProcessor &track_processor)
  {
    const auto &nodes = graph_.get_nodes ().graph_nodes_;
    EXPECT_GT (nodes.size (), 0);

    // Verify we have the expected processors
    auto * prefader =
      findNodeForProcessable (midi_channel_->get_midi_pre_fader ());
    auto * fader = findNodeForProcessable (midi_channel_->get_fader ());
    auto * postfader =
      findNodeForProcessable (midi_channel_->get_midi_post_fader ());

    EXPECT_NE (prefader, nullptr);
    EXPECT_NE (fader, nullptr);
    EXPECT_NE (postfader, nullptr);

    // Verify processors are connected correctly
    verifyProcessorsConnected (
      track_processor, midi_channel_->get_midi_pre_fader ());
    verifyProcessorsConnected (
      midi_channel_->get_midi_pre_fader (), midi_channel_->get_fader ());
    verifyProcessorsConnected (
      midi_channel_->get_fader (), midi_channel_->get_midi_post_fader ());
  }

  void
  verifyChannelToTrackConnections (const dsp::PortUuidReference &track_output)
  {
    // Verify post-fader connects to track output
    auto * postfader_node =
      findNodeForProcessable (audio_channel_->get_audio_post_fader ());
    ASSERT_NE (postfader_node, nullptr);

    // For now, just verify we have connections from post-fader
    EXPECT_FALSE (postfader_node->feeds ().empty ());
  }
};

TEST_F (ChannelSubgraphBuilderTest, AddNodesForEmptyAudioChannel)
{
  audio_channel_ = createAudioChannel ();
  ASSERT_NE (audio_channel_, nullptr);

  EXPECT_NO_THROW ({
    ChannelSubgraphBuilder::add_nodes (graph_, *audio_channel_);
  });

  // Verify that nodes were added for pre-fader, fader, and post-fader
  EXPECT_GT (graph_.get_nodes ().graph_nodes_.size (), 0);
}

TEST_F (ChannelSubgraphBuilderTest, AddNodesForEmptyMidiChannel)
{
  midi_channel_ = createMidiChannel ();
  ASSERT_NE (midi_channel_, nullptr);

  EXPECT_NO_THROW ({
    ChannelSubgraphBuilder::add_nodes (graph_, *midi_channel_);
  });

  // Verify that nodes were added for pre-fader, fader, and post-fader
  EXPECT_GT (graph_.get_nodes ().graph_nodes_.size (), 0);
}

TEST_F (ChannelSubgraphBuilderTest, AddNodesWithAudioPlugins)
{
  audio_channel_ = createAudioChannel ();
  ASSERT_NE (audio_channel_, nullptr);

  // Add some audio plugins
  auto plugin1 =
    createMockPlugin (dsp::PortType::Audio, dsp::PortType::Audio, 2, 2);
  auto plugin2 =
    createMockPlugin (dsp::PortType::Audio, dsp::PortType::Audio, 2, 2);

  audio_channel_->inserts ()->append_plugin (plugin1);
  audio_channel_->inserts ()->append_plugin (plugin2);

  EXPECT_NO_THROW ({
    ChannelSubgraphBuilder::add_nodes (graph_, *audio_channel_);
  });

  // Verify nodes for plugins were added
  EXPECT_GT (
    graph_.get_nodes ().graph_nodes_.size (),
    3); // pre-fader, fader, post-fader + 2 plugins
}

TEST_F (ChannelSubgraphBuilderTest, AddNodesWithMidiPlugins)
{
  midi_channel_ = createMidiChannel ();
  ASSERT_NE (midi_channel_, nullptr);

  // Add some MIDI plugins
  auto plugin1 = createMockPlugin (
    dsp::PortType::Midi, dsp::PortType::Midi, 0, 0, true, true);
  auto plugin2 = createMockPlugin (
    dsp::PortType::Midi, dsp::PortType::Midi, 0, 0, true, true);

  midi_channel_->midiFx ()->append_plugin (plugin1);
  midi_channel_->midiFx ()->append_plugin (plugin2);

  EXPECT_NO_THROW ({
    ChannelSubgraphBuilder::add_nodes (graph_, *midi_channel_);
  });

  // Verify nodes for plugins were added
  EXPECT_GT (
    graph_.get_nodes ().graph_nodes_.size (),
    3); // pre-fader, fader, post-fader + 2 plugins
}

TEST_F (ChannelSubgraphBuilderTest, AddNodesWithSends)
{
  audio_channel_ = createAudioChannel ();
  ASSERT_NE (audio_channel_, nullptr);

  // The channel already has pre and post fader sends by default
  EXPECT_NO_THROW ({
    ChannelSubgraphBuilder::add_nodes (graph_, *audio_channel_);
  });

  // Verify nodes for sends were added
  EXPECT_GT (graph_.get_nodes ().graph_nodes_.size (), 3); // pre-fader, fader,
                                                           // post-fader + sends
}

TEST_F (ChannelSubgraphBuilderTest, AddConnectionsForEmptyAudioChannel)
{
  audio_channel_ = createAudioChannel ();
  ASSERT_NE (audio_channel_, nullptr);

  // Create mock track processor
  auto track_processor = createMockTrackProcessor (dsp::PortType::Audio, 2);
  auto track_output = getProcessorOutputPort (*track_processor);

  // Add track processor outputs to the graph
  addTrackProcessorToGraph (*track_processor);

  EXPECT_NO_THROW ({
    ChannelSubgraphBuilder::add_nodes (graph_, *audio_channel_);
  });
  EXPECT_NO_THROW ({
    ChannelSubgraphBuilder::add_connections (
      graph_, *audio_channel_, track_output);
  });

  verifyBasicAudioChannelConnections (*track_processor);
}

TEST_F (ChannelSubgraphBuilderTest, AddConnectionsForEmptyMidiChannel)
{
  midi_channel_ = createMidiChannel ();
  ASSERT_NE (midi_channel_, nullptr);

  // Create mock track processor
  auto track_processor = createMockTrackProcessor (dsp::PortType::Midi, 1);
  auto track_output = getProcessorOutputPort (*track_processor);

  // Add track processor outputs to the graph
  addTrackProcessorToGraph (*track_processor);

  EXPECT_NO_THROW ({
    ChannelSubgraphBuilder::add_nodes (graph_, *midi_channel_);
  });
  EXPECT_NO_THROW ({
    ChannelSubgraphBuilder::add_connections (
      graph_, *midi_channel_, track_output);
  });

  verifyBasicMidiChannelConnections (*track_processor);
}

TEST_F (ChannelSubgraphBuilderTest, AddConnectionsWithAudioPlugins)
{
  audio_channel_ = createAudioChannel ();
  ASSERT_NE (audio_channel_, nullptr);

  // Add audio plugins
  auto plugin1 =
    createMockPlugin (dsp::PortType::Audio, dsp::PortType::Audio, 2, 2);
  auto plugin2 =
    createMockPlugin (dsp::PortType::Audio, dsp::PortType::Audio, 2, 2);

  audio_channel_->inserts ()->append_plugin (plugin1);
  audio_channel_->inserts ()->append_plugin (plugin2);

  // Add nodes first
  ChannelSubgraphBuilder::add_nodes (graph_, *audio_channel_);

  // Create mock track processor
  auto track_processor = createMockTrackProcessor (dsp::PortType::Audio, 2);
  auto track_output = getProcessorOutputPort (*track_processor);

  // Add track processor outputs to the graph
  addTrackProcessorToGraph (*track_processor);

  EXPECT_NO_THROW ({
    ChannelSubgraphBuilder::add_connections (
      graph_, *audio_channel_, track_output);
  });

  // Get plugin objects
  const auto plugin_refs = audio_channel_->get_plugins ();
  EXPECT_EQ (plugin_refs.size (), 2);

  auto plugins =
    plugin_refs | std::views::transform (&plugins::PluginUuidReference::get);

  // Verify connections
  verifyProcessorsConnected (*track_processor, *plugins[0]);
  verifyProcessorsConnected (*plugins[0], *plugins[1]);
  verifyProcessorsConnected (
    *plugins.back (), audio_channel_->get_audio_pre_fader ());
  verifyProcessorsConnected (
    audio_channel_->get_audio_pre_fader (), audio_channel_->get_fader ());
  verifyProcessorsConnected (
    audio_channel_->get_fader (), audio_channel_->get_audio_post_fader ());
}

TEST_F (ChannelSubgraphBuilderTest, AddConnectionsWithMidiPlugins)
{
  midi_channel_ = createMidiChannel ();
  ASSERT_NE (midi_channel_, nullptr);

  // Add MIDI plugins
  auto plugin1 = createMockPlugin (
    dsp::PortType::Midi, dsp::PortType::Midi, 0, 0, true, true);
  auto plugin2 = createMockPlugin (
    dsp::PortType::Midi, dsp::PortType::Midi, 0, 0, true, true);

  midi_channel_->midiFx ()->append_plugin (plugin1);
  midi_channel_->midiFx ()->append_plugin (plugin2);

  // Add nodes first
  ChannelSubgraphBuilder::add_nodes (graph_, *midi_channel_);

  // Create mock track processor
  auto track_processor = createMockTrackProcessor (dsp::PortType::Midi, 1);
  auto track_output = getProcessorOutputPort (*track_processor);

  // Add track processor outputs to the graph
  addTrackProcessorToGraph (*track_processor);

  EXPECT_NO_THROW ({
    ChannelSubgraphBuilder::add_connections (
      graph_, *midi_channel_, track_output);
  });

  // Get plugin objects
  const auto plugin_refs = midi_channel_->get_plugins ();
  EXPECT_EQ (plugin_refs.size (), 2);

  auto plugins =
    plugin_refs | std::views::transform (&plugins::PluginUuidReference::get);

  // Verify connections
  verifyProcessorsConnected (*track_processor, *plugins[0]);
  verifyProcessorsConnected (*plugins[0], *plugins[1]);
  verifyProcessorsConnected (
    *plugins.back (), midi_channel_->get_midi_pre_fader ());
  verifyProcessorsConnected (
    midi_channel_->get_midi_pre_fader (), midi_channel_->get_fader ());
  verifyProcessorsConnected (
    midi_channel_->get_fader (), midi_channel_->get_midi_post_fader ());
}

TEST_F (ChannelSubgraphBuilderTest, AddConnectionsWithPreFaderSends)
{
  audio_channel_ = createAudioChannel ();
  ASSERT_NE (audio_channel_, nullptr);

  // Create a destination audio input port and set it on the send BEFORE building
  auto dest_port_ref = utils::create_object<dsp::AudioPort> (
    *registry_, u8"Destination Track Input", dsp::PortFlow::Input,
    dsp::SpeakerArrangement::stereo ());

  auto   send_ref = audio_channel_->sends ()->append_send (true);
  auto * send = send_ref.get ();
  send->set_destination_port (dest_port_ref);
  ASSERT_TRUE (send->has_destination ());

  // Add nodes first
  ChannelSubgraphBuilder::add_nodes (graph_, *audio_channel_);

  // Add destination port node to graph (required for connection)
  graph_.add_node_for_processable (*dest_port_ref.get ());

  // Create mock track processor
  auto track_processor = createMockTrackProcessor (dsp::PortType::Audio, 2);
  auto track_output = getProcessorOutputPort (*track_processor);

  // Add track processor outputs to the graph
  addTrackProcessorToGraph (*track_processor);

  EXPECT_NO_THROW ({
    ChannelSubgraphBuilder::add_connections (
      graph_, *audio_channel_, track_output);
  });

  // Verify basic connections
  verifyBasicAudioChannelConnections (*track_processor);

  // Check that pre-fader has connections to sends
  verifyProcessorsConnected (audio_channel_->get_audio_pre_fader (), *send);

  // Verify send output is connected to destination port by ChannelSubgraphBuilder
  EXPECT_TRUE (
    hasConnection (send->get_stereo_out_port (), *dest_port_ref.get ()));
}

TEST_F (ChannelSubgraphBuilderTest, AddConnectionsWithPostFaderSends)
{
  audio_channel_ = createAudioChannel ();
  ASSERT_NE (audio_channel_, nullptr);

  // Create a destination audio input port and set it on the send BEFORE building
  auto dest_port_ref = utils::create_object<dsp::AudioPort> (
    *registry_, u8"Destination Track Input", dsp::PortFlow::Input,
    dsp::SpeakerArrangement::stereo ());

  auto   send_ref = audio_channel_->sends ()->append_send (false);
  auto * send = send_ref.get ();
  send->set_destination_port (dest_port_ref);
  ASSERT_TRUE (send->has_destination ());

  // Add nodes first
  ChannelSubgraphBuilder::add_nodes (graph_, *audio_channel_);

  // Add destination port node to graph (required for connection)
  graph_.add_node_for_processable (*dest_port_ref.get ());

  // Create mock track processor
  auto track_processor = createMockTrackProcessor (dsp::PortType::Audio, 2);
  auto track_output = getProcessorOutputPort (*track_processor);

  // Add track processor outputs to the graph
  addTrackProcessorToGraph (*track_processor);

  EXPECT_NO_THROW ({
    ChannelSubgraphBuilder::add_connections (
      graph_, *audio_channel_, track_output);
  });

  // Verify basic connections
  verifyBasicAudioChannelConnections (*track_processor);

  // Check that fader has connections to sends
  verifyProcessorsConnected (audio_channel_->get_fader (), *send);

  // Verify send output is connected to destination port by ChannelSubgraphBuilder
  EXPECT_TRUE (
    hasConnection (send->get_stereo_out_port (), *dest_port_ref.get ()));
}

TEST_F (ChannelSubgraphBuilderTest, AddConnectionsWithSingleMonoAudioPlugin)
{
  audio_channel_ = createAudioChannel ();
  ASSERT_NE (audio_channel_, nullptr);

  // Add a mono audio plugin (1 input, 1 output)
  auto plugin =
    createMockPlugin (dsp::PortType::Audio, dsp::PortType::Audio, 1, 1);
  audio_channel_->inserts ()->append_plugin (plugin);

  // Add nodes first
  ChannelSubgraphBuilder::add_nodes (graph_, *audio_channel_);

  // Create mock track processor
  auto track_processor = createMockTrackProcessor (dsp::PortType::Audio, 1);
  auto track_output = getProcessorOutputPort (*track_processor);

  // Add track processor outputs to the graph
  addTrackProcessorToGraph (*track_processor);

  EXPECT_NO_THROW ({
    ChannelSubgraphBuilder::add_connections (
      graph_, *audio_channel_, track_output);
  });

  // Get plugin objects
  const auto plugin_refs = audio_channel_->get_plugins ();
  EXPECT_EQ (plugin_refs.size (), 1);

  auto plugins =
    plugin_refs | std::views::transform (&plugins::PluginUuidReference::get);

  // Check that we have connections between components
  verifyProcessorsConnected (*track_processor, *plugins.front ());
  verifyProcessorsConnected (
    *plugins.front (), audio_channel_->get_audio_pre_fader ());
  verifyProcessorsConnected (
    audio_channel_->get_audio_pre_fader (), audio_channel_->get_fader ());
  verifyProcessorsConnected (
    audio_channel_->get_fader (), audio_channel_->get_audio_post_fader ());
}

TEST_F (ChannelSubgraphBuilderTest, AddConnectionsWithAsymmetricAudioPlugins)
{
  audio_channel_ = createAudioChannel ();
  ASSERT_NE (audio_channel_, nullptr);

  // Add plugins with asymmetric I/O
  auto plugin1 = createMockPlugin (
    dsp::PortType::Audio, dsp::PortType::Audio, 1, 2); // Mono to stereo
  auto plugin2 = createMockPlugin (
    dsp::PortType::Audio, dsp::PortType::Audio, 2, 1); // Stereo to mono

  audio_channel_->inserts ()->append_plugin (plugin1);
  audio_channel_->inserts ()->append_plugin (plugin2);

  // Add nodes first
  ChannelSubgraphBuilder::add_nodes (graph_, *audio_channel_);

  // Create mock track processor
  auto track_processor = createMockTrackProcessor (dsp::PortType::Audio, 2);
  auto track_output = getProcessorOutputPort (*track_processor);

  // Add track processor outputs to the graph
  addTrackProcessorToGraph (*track_processor);

  EXPECT_NO_THROW ({
    ChannelSubgraphBuilder::add_connections (
      graph_, *audio_channel_, track_output);
  });

  // Get plugin objects
  const auto plugin_refs = audio_channel_->get_plugins ();
  EXPECT_EQ (plugin_refs.size (), 2);

  auto plugins =
    plugin_refs | std::views::transform (&plugins::PluginUuidReference::get);

  // Verify connections handle asymmetric I/O correctly
  verifyProcessorsConnected (*track_processor, *plugins.front ());
  verifyProcessorsConnected (*plugins[0], *plugins[1]);
  EXPECT_TRUE (hasConnection (
    *plugins[0]->get_all_output_ports ().front ().get_object_as<dsp::AudioPort> (),
    *plugins[1]->get_all_input_ports ().front ().get_object_as<dsp::AudioPort> ()));
  EXPECT_TRUE (hasConnection (
    *plugins[0]->get_all_output_ports ().front ().get_object_as<dsp::AudioPort> (),
    *plugins[1]->get_all_input_ports ().front ().get_object_as<dsp::AudioPort> ()));
  EXPECT_FALSE (hasConnection (
    *plugins[0]->get_all_output_ports ().at (0).get_object_as<dsp::AudioPort> (),
    *plugins[1]->get_all_input_ports ().at (1).get_object_as<dsp::AudioPort> ()));
  verifyProcessorsConnected (
    *plugins[1], audio_channel_->get_audio_pre_fader ());
  verifyProcessorsConnected (
    audio_channel_->get_audio_pre_fader (), audio_channel_->get_fader ());
  verifyProcessorsConnected (
    audio_channel_->get_fader (), audio_channel_->get_audio_post_fader ());
}

TEST_F (ChannelSubgraphBuilderTest, AddConnectionsWithInstrumentPlugin)
{
  audio_channel_ = createAudioChannel ();
  ASSERT_NE (audio_channel_, nullptr);

  // Add a plugin that takes MIDI input and produces audio output
  auto plugin = createMockPlugin (
    dsp::PortType::Midi, dsp::PortType::Audio, 0, 2, true, false);
  audio_channel_->instruments ()->append_plugin (plugin);

  // Add nodes first
  ChannelSubgraphBuilder::add_nodes (graph_, *audio_channel_);

  // Create mock track processor (MIDI)
  auto track_processor = createMockTrackProcessor (dsp::PortType::Midi, 1);
  auto track_output = getProcessorOutputPort (*track_processor);

  // Add track processor outputs to the graph
  addTrackProcessorToGraph (*track_processor);

  EXPECT_NO_THROW ({
    ChannelSubgraphBuilder::add_connections (
      graph_, *audio_channel_, track_output);
  });

  // Get plugin objects
  const auto plugin_refs = audio_channel_->get_plugins ();
  EXPECT_EQ (plugin_refs.size (), 1);

  auto plugins =
    plugin_refs | std::views::transform ([] (auto &ref) { return ref.get (); });

  // Verify MIDI to audio conversion path
  verifyProcessorsConnected (*track_processor, *plugins.front ());
  verifyProcessorsConnected (
    *plugins.front (), audio_channel_->get_audio_pre_fader ());
  verifyProcessorsConnected (
    audio_channel_->get_audio_pre_fader (), audio_channel_->get_fader ());
  verifyProcessorsConnected (
    audio_channel_->get_fader (), audio_channel_->get_audio_post_fader ());
}

TEST_F (ChannelSubgraphBuilderTest, AddConnectionsWithEmptyTrackProcessorOutputs)
{
  audio_channel_ = createAudioChannel ();
  ASSERT_NE (audio_channel_, nullptr);

  // Add nodes first
  ChannelSubgraphBuilder::add_nodes (graph_, *audio_channel_);

  // Create mock track processor with no outputs
  auto track_processor = createMockTrackProcessor (dsp::PortType::Audio, 0);
  auto track_output = getProcessorOutputPort (*track_processor);

  // Add track processor outputs to the graph
  addTrackProcessorToGraph (*track_processor);

  // This should not crash, though it won't make useful connections
  EXPECT_NO_THROW ({
    ChannelSubgraphBuilder::add_connections (
      graph_, *audio_channel_, track_output);
  });
}

TEST_F (
  ChannelSubgraphBuilderTest,
  AddConnectionsForAudioChannelWithMidiTrackProcessorOutputNoPlugins)
{
  audio_channel_ = createAudioChannel ();
  ASSERT_NE (audio_channel_, nullptr);

  ChannelSubgraphBuilder::add_nodes (graph_, *audio_channel_);

  auto track_processor = createMockTrackProcessor (dsp::PortType::Midi, 1);
  auto track_output = getProcessorOutputPort (*track_processor);
  addTrackProcessorToGraph (*track_processor);

  EXPECT_NO_THROW ({
    ChannelSubgraphBuilder::add_connections (
      graph_, *audio_channel_, track_output);
  });
}

TEST_F (ChannelSubgraphBuilderTest, AddConnectionsThrowsWhenOutputsNotInGraph)
{
  audio_channel_ = createAudioChannel ();
  ASSERT_NE (audio_channel_, nullptr);

  // Add nodes first
  ChannelSubgraphBuilder::add_nodes (graph_, *audio_channel_);

  // Create mock track processor but DON'T add its outputs to the graph
  auto track_processor = createMockTrackProcessor (dsp::PortType::Audio, 2);
  auto track_output = getProcessorOutputPort (*track_processor);
  // Note: NOT adding the ports to the graph

  // This should throw std::invalid_argument
  EXPECT_THROW (
    {
      ChannelSubgraphBuilder::add_connections (
        graph_, *audio_channel_, track_output);
    },
    std::invalid_argument);
}

TEST_F (ChannelSubgraphBuilderTest, FirstPluginSidechainInputIsNotAutoConnected)
{
  audio_channel_ = createAudioChannel ();
  ASSERT_NE (audio_channel_, nullptr);

  // Plugin declares its sidechain bus before the main bus
  auto plugin_ref =
    createMockPlugin (dsp::PortType::Audio, dsp::PortType::Audio, 0, 1);
  auto * pl = plugin_ref.get_object_as<plugins::FaustPlugin> ();
  auto   sidechain_in_ref = createMockPort (
    dsp::PortType::Audio, u8"Sidechain In", dsp::PortFlow::Input,
    dsp::AudioPort::Purpose::Sidechain);
  auto main_in_ref =
    createMockPort (dsp::PortType::Audio, u8"Main In", dsp::PortFlow::Input);
  pl->add_input_port (sidechain_in_ref);
  pl->add_input_port (main_in_ref);
  audio_channel_->inserts ()->append_plugin (plugin_ref);

  ChannelSubgraphBuilder::add_nodes (graph_, *audio_channel_);

  auto track_processor = createMockTrackProcessor (dsp::PortType::Audio, 2);
  auto track_output = getProcessorOutputPort (*track_processor);
  addTrackProcessorToGraph (*track_processor);

  ChannelSubgraphBuilder::add_connections (
    graph_, *audio_channel_, track_output);

  auto &track_out_port = *track_output.get_object_as<dsp::AudioPort> ();
  auto &main_in_port = *main_in_ref.get_object_as<dsp::AudioPort> ();
  auto &sidechain_in_port = *sidechain_in_ref.get_object_as<dsp::AudioPort> ();

  EXPECT_TRUE (hasConnection (track_out_port, main_in_port));
  EXPECT_FALSE (hasConnection (track_out_port, sidechain_in_port));
}

TEST_F (
  ChannelSubgraphBuilderTest,
  PluginToPluginSidechainInputIsNotAutoConnected)
{
  audio_channel_ = createAudioChannel ();
  ASSERT_NE (audio_channel_, nullptr);

  auto plugin1_ref =
    createMockPlugin (dsp::PortType::Audio, dsp::PortType::Audio, 1, 1);
  auto plugin2_ref =
    createMockPlugin (dsp::PortType::Audio, dsp::PortType::Audio, 0, 1);
  auto * pl2 = plugin2_ref.get_object_as<plugins::FaustPlugin> ();
  auto   sidechain_in_ref = createMockPort (
    dsp::PortType::Audio, u8"Sidechain In", dsp::PortFlow::Input,
    dsp::AudioPort::Purpose::Sidechain);
  auto main_in_ref =
    createMockPort (dsp::PortType::Audio, u8"Main In", dsp::PortFlow::Input);
  pl2->add_input_port (sidechain_in_ref);
  pl2->add_input_port (main_in_ref);

  audio_channel_->inserts ()->append_plugin (plugin1_ref);
  audio_channel_->inserts ()->append_plugin (plugin2_ref);

  ChannelSubgraphBuilder::add_nodes (graph_, *audio_channel_);

  auto track_processor = createMockTrackProcessor (dsp::PortType::Audio, 2);
  auto track_output = getProcessorOutputPort (*track_processor);
  addTrackProcessorToGraph (*track_processor);

  ChannelSubgraphBuilder::add_connections (
    graph_, *audio_channel_, track_output);

  auto &pl1_main_out =
    *plugin1_ref.get ()
       ->get_all_output_ports ()
       .front ()
       .get_object_as<dsp::AudioPort> ();
  auto &main_in_port = *main_in_ref.get_object_as<dsp::AudioPort> ();
  auto &sidechain_in_port = *sidechain_in_ref.get_object_as<dsp::AudioPort> ();

  EXPECT_TRUE (hasConnection (pl1_main_out, main_in_port));
  EXPECT_FALSE (hasConnection (pl1_main_out, sidechain_in_port));
}

TEST_F (
  ChannelSubgraphBuilderTest,
  PluginToPluginSidechainOutputIsNotAutoConnected)
{
  audio_channel_ = createAudioChannel ();
  ASSERT_NE (audio_channel_, nullptr);

  // First plugin declares a sidechain (aux) output before its main output
  auto plugin1_ref =
    createMockPlugin (dsp::PortType::Audio, dsp::PortType::Audio, 1, 0);
  auto * pl1 = plugin1_ref.get_object_as<plugins::FaustPlugin> ();
  auto   sidechain_out_ref = createMockPort (
    dsp::PortType::Audio, u8"Sidechain Out", dsp::PortFlow::Output,
    dsp::AudioPort::Purpose::Sidechain);
  auto main_out_ref =
    createMockPort (dsp::PortType::Audio, u8"Main Out", dsp::PortFlow::Output);
  pl1->add_output_port (sidechain_out_ref);
  pl1->add_output_port (main_out_ref);

  auto plugin2_ref =
    createMockPlugin (dsp::PortType::Audio, dsp::PortType::Audio, 1, 1);

  audio_channel_->inserts ()->append_plugin (plugin1_ref);
  audio_channel_->inserts ()->append_plugin (plugin2_ref);

  ChannelSubgraphBuilder::add_nodes (graph_, *audio_channel_);

  auto track_processor = createMockTrackProcessor (dsp::PortType::Audio, 2);
  auto track_output = getProcessorOutputPort (*track_processor);
  addTrackProcessorToGraph (*track_processor);

  ChannelSubgraphBuilder::add_connections (
    graph_, *audio_channel_, track_output);

  auto &pl2_main_in =
    *plugin2_ref.get ()
       ->get_all_input_ports ()
       .front ()
       .get_object_as<dsp::AudioPort> ();
  auto &main_out_port = *main_out_ref.get_object_as<dsp::AudioPort> ();
  auto &sidechain_out_port = *sidechain_out_ref.get_object_as<dsp::AudioPort> ();

  EXPECT_TRUE (hasConnection (main_out_port, pl2_main_in));
  EXPECT_FALSE (hasConnection (sidechain_out_port, pl2_main_in));
}

TEST_F (
  ChannelSubgraphBuilderTest,
  LastPluginSidechainOutputIsNotConnectedToPrefader)
{
  audio_channel_ = createAudioChannel ();
  ASSERT_NE (audio_channel_, nullptr);

  // Last plugin declares a sidechain (aux) output before its main output
  auto plugin_ref =
    createMockPlugin (dsp::PortType::Audio, dsp::PortType::Audio, 1, 0);
  auto * pl = plugin_ref.get_object_as<plugins::FaustPlugin> ();
  auto   sidechain_out_ref = createMockPort (
    dsp::PortType::Audio, u8"Sidechain Out", dsp::PortFlow::Output,
    dsp::AudioPort::Purpose::Sidechain);
  auto main_out_ref =
    createMockPort (dsp::PortType::Audio, u8"Main Out", dsp::PortFlow::Output);
  pl->add_output_port (sidechain_out_ref);
  pl->add_output_port (main_out_ref);
  audio_channel_->inserts ()->append_plugin (plugin_ref);

  ChannelSubgraphBuilder::add_nodes (graph_, *audio_channel_);

  auto track_processor = createMockTrackProcessor (dsp::PortType::Audio, 2);
  auto track_output = getProcessorOutputPort (*track_processor);
  addTrackProcessorToGraph (*track_processor);

  ChannelSubgraphBuilder::add_connections (
    graph_, *audio_channel_, track_output);

  auto &prefader_in =
    audio_channel_->get_audio_pre_fader ().get_audio_in_port ();
  auto &main_out_port = *main_out_ref.get_object_as<dsp::AudioPort> ();
  auto &sidechain_out_port = *sidechain_out_ref.get_object_as<dsp::AudioPort> ();

  EXPECT_TRUE (hasConnection (main_out_port, prefader_in));
  EXPECT_FALSE (hasConnection (sidechain_out_port, prefader_in));
}

TEST_F (ChannelSubgraphBuilderTest, SidechainOnlyFirstPluginGetsNoAudioConnection)
{
  audio_channel_ = createAudioChannel ();
  ASSERT_NE (audio_channel_, nullptr);

  // Plugin whose only audio input is a sidechain bus: nothing may be
  // auto-wired into it
  auto plugin_ref =
    createMockPlugin (dsp::PortType::Audio, dsp::PortType::Audio, 0, 1);
  auto * pl = plugin_ref.get_object_as<plugins::FaustPlugin> ();
  auto   sidechain_in_ref = createMockPort (
    dsp::PortType::Audio, u8"Sidechain In", dsp::PortFlow::Input,
    dsp::AudioPort::Purpose::Sidechain);
  pl->add_input_port (sidechain_in_ref);
  audio_channel_->inserts ()->append_plugin (plugin_ref);

  ChannelSubgraphBuilder::add_nodes (graph_, *audio_channel_);

  auto track_processor = createMockTrackProcessor (dsp::PortType::Audio, 2);
  auto track_output = getProcessorOutputPort (*track_processor);
  addTrackProcessorToGraph (*track_processor);

  ChannelSubgraphBuilder::add_connections (
    graph_, *audio_channel_, track_output);

  auto &track_out_port = *track_output.get_object_as<dsp::AudioPort> ();
  auto &sidechain_in_port = *sidechain_in_ref.get_object_as<dsp::AudioPort> ();

  EXPECT_FALSE (hasConnection (track_out_port, sidechain_in_port));
  // with no port connection made, the plugin still gets a scheduling edge
  EXPECT_TRUE (hasConnection (track_out_port, *pl));
}

TEST_F (ChannelSubgraphBuilderTest, InstrumentChannelRoutesMidiThroughEventTap)
{
  instrument_channel_ = createInstrumentChannel ();
  ASSERT_NE (instrument_channel_, nullptr);

  auto midi_fx_ref = createMockPlugin (
    dsp::PortType::Midi, dsp::PortType::Midi, 0, 0, true, true);
  auto instrument_ref = createMockPlugin (
    dsp::PortType::Midi, dsp::PortType::Audio, 0, 2, true, false);
  instrument_channel_->midiFx ()->append_plugin (midi_fx_ref);
  instrument_channel_->instruments ()->append_plugin (instrument_ref);

  ChannelSubgraphBuilder::add_nodes (graph_, *instrument_channel_);

  auto track_processor = createMockTrackProcessor (dsp::PortType::Midi, 1);
  auto track_output = getProcessorOutputPort (*track_processor);
  addTrackProcessorToGraph (*track_processor);

  EXPECT_NO_THROW ({
    ChannelSubgraphBuilder::add_connections (
      graph_, *instrument_channel_, track_output);
  });

  const auto plugin_refs = instrument_channel_->get_plugins ();
  ASSERT_EQ (plugin_refs.size (), 2);
  auto * midi_fx = plugin_refs[0].get ();
  auto * instrument = plugin_refs[1].get ();

  // track processor -> MIDI FX -> MIDI event tap -> instrument -> audio
  // pre-fader -> fader -> audio post-fader
  verifyProcessorsConnected (*track_processor, *midi_fx);
  verifyProcessorsConnected (
    *midi_fx, instrument_channel_->get_midi_event_tap ());
  verifyProcessorsConnected (
    instrument_channel_->get_midi_event_tap (), *instrument);
  verifyProcessorsConnected (
    *instrument, instrument_channel_->get_audio_pre_fader ());
  verifyProcessorsConnected (
    instrument_channel_->get_audio_pre_fader (),
    instrument_channel_->get_fader ());
  verifyProcessorsConnected (
    instrument_channel_->get_fader (),
    instrument_channel_->get_audio_post_fader ());
}

TEST_F (
  ChannelSubgraphBuilderTest,
  InstrumentChannelWithoutMidiFxRoutesThroughEventTap)
{
  instrument_channel_ = createInstrumentChannel ();
  ASSERT_NE (instrument_channel_, nullptr);

  auto instrument_ref = createMockPlugin (
    dsp::PortType::Midi, dsp::PortType::Audio, 0, 2, true, false);
  instrument_channel_->instruments ()->append_plugin (instrument_ref);

  ChannelSubgraphBuilder::add_nodes (graph_, *instrument_channel_);

  auto track_processor = createMockTrackProcessor (dsp::PortType::Midi, 1);
  auto track_output = getProcessorOutputPort (*track_processor);
  addTrackProcessorToGraph (*track_processor);

  EXPECT_NO_THROW ({
    ChannelSubgraphBuilder::add_connections (
      graph_, *instrument_channel_, track_output);
  });

  verifyProcessorsConnected (
    *track_processor, instrument_channel_->get_midi_event_tap ());
  verifyProcessorsConnected (
    instrument_channel_->get_midi_event_tap (), *instrument_ref.get ());
  verifyProcessorsConnected (
    *instrument_ref.get (), instrument_channel_->get_audio_pre_fader ());
}

TEST_F (ChannelSubgraphBuilderTest, InstrumentChannelWithOnlyMidiFxFeedsEventTap)
{
  instrument_channel_ = createInstrumentChannel ();
  ASSERT_NE (instrument_channel_, nullptr);

  auto midi_fx_ref = createMockPlugin (
    dsp::PortType::Midi, dsp::PortType::Midi, 0, 0, true, true);
  instrument_channel_->midiFx ()->append_plugin (midi_fx_ref);

  ChannelSubgraphBuilder::add_nodes (graph_, *instrument_channel_);

  auto track_processor = createMockTrackProcessor (dsp::PortType::Midi, 1);
  auto track_output = getProcessorOutputPort (*track_processor);
  addTrackProcessorToGraph (*track_processor);

  EXPECT_NO_THROW ({
    ChannelSubgraphBuilder::add_connections (
      graph_, *instrument_channel_, track_output);
  });

  verifyProcessorsConnected (*track_processor, *midi_fx_ref.get ());
  verifyProcessorsConnected (
    *midi_fx_ref.get (), instrument_channel_->get_midi_event_tap ());
}

TEST_F (
  ChannelSubgraphBuilderTest,
  InstrumentChannelWithoutPluginsFeedsEventTapFromProcessor)
{
  instrument_channel_ = createInstrumentChannel ();
  ASSERT_NE (instrument_channel_, nullptr);

  ChannelSubgraphBuilder::add_nodes (graph_, *instrument_channel_);

  auto track_processor = createMockTrackProcessor (dsp::PortType::Midi, 1);
  auto track_output = getProcessorOutputPort (*track_processor);
  addTrackProcessorToGraph (*track_processor);

  EXPECT_NO_THROW ({
    ChannelSubgraphBuilder::add_connections (
      graph_, *instrument_channel_, track_output);
  });

  verifyProcessorsConnected (
    *track_processor, instrument_channel_->get_midi_event_tap ());
}

TEST_F (ChannelSubgraphBuilderTest, PostMidiFxPortStableAcrossGraphRebuilds)
{
  instrument_channel_ = createInstrumentChannel ();
  ASSERT_NE (instrument_channel_, nullptr);

  const auto * tap = instrument_channel_->postMidiFxPort ();
  ASSERT_NE (tap, nullptr);

  {
    auto track_processor = createMockTrackProcessor (dsp::PortType::Midi, 1);
    auto track_output = getProcessorOutputPort (*track_processor);
    addTrackProcessorToGraph (*track_processor);
    ChannelSubgraphBuilder::add_nodes (graph_, *instrument_channel_);
    ChannelSubgraphBuilder::add_connections (
      graph_, *instrument_channel_, track_output);
    EXPECT_NE (graph_.get_nodes ().find_node_for_processable (*tap), nullptr);
  }

  // A graph rebuild re-adds nodes and connections without touching the
  // channel-owned port.
  dsp::graph::Graph second_graph;
  {
    auto track_processor = createMockTrackProcessor (dsp::PortType::Midi, 1);
    auto track_output = getProcessorOutputPort (*track_processor);
    auto track_processor_node =
      second_graph.add_node_for_processable (*track_processor);
    for (const auto &port_ref : track_processor->get_all_output_ports ())
      {
        auto * node = second_graph.add_node_for_processable (*port_ref.get ());
        track_processor_node->connect_to (*node);
      }
    ChannelSubgraphBuilder::add_nodes (second_graph, *instrument_channel_);
    ChannelSubgraphBuilder::add_connections (
      second_graph, *instrument_channel_, track_output);
    EXPECT_NE (
      second_graph.get_nodes ().find_node_for_processable (*tap), nullptr);
  }

  EXPECT_EQ (instrument_channel_->postMidiFxPort (), tap);
}

TEST_F (ChannelSubgraphBuilderTest, PostMidiFxPortCarriesSourceAndGeneratedNotes)
{
  instrument_channel_ = createInstrumentChannel ();
  ASSERT_NE (instrument_channel_, nullptr);

  auto generator_ref = utils::create_object<NoteGeneratorPlugin> (
    *registry_, *registry_, midi_byte_t{ 96 });
  instrument_channel_->midiFx ()->append_plugin (generator_ref);
  auto instrument_ref =
    utils::create_object<InstrumentSinkPlugin> (*registry_, *registry_);
  instrument_channel_->instruments ()->append_plugin (instrument_ref);

  NoteSourceTrackProcessor track_processor (*registry_, 60);
  auto track_output = getProcessorOutputPort (track_processor);
  addTrackProcessorToGraph (track_processor);
  ChannelSubgraphBuilder::add_nodes (graph_, *instrument_channel_);
  ChannelSubgraphBuilder::add_connections (
    graph_, *instrument_channel_, track_output);

  process_graph_once (graph_);

  const auto * tap = instrument_channel_->postMidiFxPort ();
  ASSERT_NE (tap, nullptr);
  EXPECT_THAT (pitches_on (*tap), ::testing::UnorderedElementsAre (60, 96));

  release_graph_resources (graph_);
}

TEST_F (
  ChannelSubgraphBuilderTest,
  BypassedMidiFxDropsGeneratedNotesFromPostMidiFxPort)
{
  instrument_channel_ = createInstrumentChannel ();
  ASSERT_NE (instrument_channel_, nullptr);

  auto generator_ref = utils::create_object<NoteGeneratorPlugin> (
    *registry_, *registry_, midi_byte_t{ 96 });
  instrument_channel_->midiFx ()->append_plugin (generator_ref);

  NoteSourceTrackProcessor track_processor (*registry_, 60);
  auto track_output = getProcessorOutputPort (track_processor);
  addTrackProcessorToGraph (track_processor);
  ChannelSubgraphBuilder::add_nodes (graph_, *instrument_channel_);
  ChannelSubgraphBuilder::add_connections (
    graph_, *instrument_channel_, track_output);

  generator_ref.get_object_as<NoteGeneratorPlugin> ()->setBypassed (true);

  process_graph_once (graph_);

  const auto * tap = instrument_channel_->postMidiFxPort ();
  ASSERT_NE (tap, nullptr);
  // bypass passes the source note through without the generated one
  EXPECT_THAT (pitches_on (*tap), ::testing::ElementsAre (60));

  release_graph_resources (graph_);
}

TEST_F (ChannelSubgraphBuilderTest, MutedChannelKeepsEventsOnPostMidiFxPort)
{
  // channel-muted fader: the event tap sits before the fader/mute stage
  instrument_channel_ = std::make_unique<Channel> (
    *registry_, dsp::PortType::Midi, dsp::PortType::Audio, name_provider_,
    false, [] (bool) { return true; });
  ASSERT_NE (instrument_channel_, nullptr);

  auto generator_ref = utils::create_object<NoteGeneratorPlugin> (
    *registry_, *registry_, midi_byte_t{ 96 });
  instrument_channel_->midiFx ()->append_plugin (generator_ref);

  NoteSourceTrackProcessor track_processor (*registry_, 60);
  auto track_output = getProcessorOutputPort (track_processor);
  addTrackProcessorToGraph (track_processor);
  ChannelSubgraphBuilder::add_nodes (graph_, *instrument_channel_);
  ChannelSubgraphBuilder::add_connections (
    graph_, *instrument_channel_, track_output);

  process_graph_once (graph_);

  const auto * tap = instrument_channel_->postMidiFxPort ();
  ASSERT_NE (tap, nullptr);
  EXPECT_THAT (pitches_on (*tap), ::testing::UnorderedElementsAre (60, 96));

  release_graph_resources (graph_);
}

} // namespace zrythm::structure::tracks
