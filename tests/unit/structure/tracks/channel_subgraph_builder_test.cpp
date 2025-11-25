// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/fader.h"
#include "dsp/graph.h"
#include "dsp/port.h"
#include "dsp/processor_base.h"
#include "plugins/internal_plugin_base.h"
#include "plugins/plugin.h"
#include "plugins/plugin_group.h"
#include "structure/tracks/channel.h"
#include "structure/tracks/channel_send.h"
#include "structure/tracks/channel_subgraph_builder.h"
#include "utils/gtest_wrapper.h"

#include <QObject>

#include "../../dsp/graph_helpers.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace zrythm::structure::tracks
{

// Mock processor for testing track processor outputs
class MockTrackProcessor : public dsp::ProcessorBase
{
public:
  MockTrackProcessor (dsp::ProcessorBase::ProcessorBaseDependencies dependencies)
      : dsp::ProcessorBase (dependencies, u8"MockTrackProcessor")
  {
  }

  MOCK_METHOD (
    void,
    custom_process_block,
    (EngineProcessTimeInfo, const dsp::ITransport &),
    (noexcept, override));
  MOCK_METHOD (
    void,
    custom_prepare_for_processing,
    (const dsp::graph::GraphNode *, sample_rate_t, nframes_t),
    (override));
  MOCK_METHOD (void, custom_release_resources, (), (override));
};

class ChannelSubgraphBuilderTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    port_registry_ = std::make_unique<dsp::PortRegistry> ();
    param_registry_ =
      std::make_unique<dsp::ProcessorParameterRegistry> (*port_registry_);
    plugin_registry_ = std::make_unique<plugins::PluginRegistry> ();

    name_provider_ = [] { return u8"Test Channel"; };
    should_be_muted_cb_ = [] (bool solo_status) { return false; };
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
  }

  std::unique_ptr<Channel> createAudioChannel ()
  {
    return std::make_unique<Channel> (
      *plugin_registry_,
      dsp::ProcessorBase::ProcessorBaseDependencies{
        .port_registry_ = *port_registry_, .param_registry_ = *param_registry_ },
      dsp::PortType::Audio, name_provider_, false, should_be_muted_cb_);
  }

  std::unique_ptr<Channel> createMidiChannel ()
  {
    return std::make_unique<Channel> (
      *plugin_registry_,
      dsp::ProcessorBase::ProcessorBaseDependencies{
        .port_registry_ = *port_registry_, .param_registry_ = *param_registry_ },
      dsp::PortType::Midi, name_provider_, false, should_be_muted_cb_);
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

    auto ref = plugin_registry_->create_object<plugins::InternalPluginBase> (
      dsp::ProcessorBase::ProcessorBaseDependencies{
        .port_registry_ = *port_registry_, .param_registry_ = *param_registry_ },
      [] { return fs::path ("/tmp"); }, nullptr);
    auto * pl = ref.get_object_as<plugins::InternalPluginBase> ();
    pl->set_configuration (config);

    // Add appropriate ports based on types
    if (input_type == dsp::PortType::Audio)
      {
        for (int i = 0; i < num_audio_inputs; ++i)
          {
            pl->add_input_port (createMockPort (
              dsp::PortType::Audio,
              utils::Utf8String ::from_utf8_encoded_string (
                fmt::format ("Audio In {}", i + 1))));
          }
      }
    if (output_type == dsp::PortType::Audio)
      {
        for (int i = 0; i < num_audio_outputs; ++i)
          {
            pl->add_output_port (createMockPort (
              dsp::PortType::Audio,
              utils::Utf8String ::from_utf8_encoded_string (
                fmt::format ("Audio Out {}", i + 1))));
          }
      }
    if (has_midi_input)
      {
        pl->add_input_port (createMockPort (dsp::PortType::Midi, u8"MIDI In"));
      }
    if (has_midi_output)
      {
        pl->add_output_port (createMockPort (dsp::PortType::Midi, u8"MIDI Out"));
      }

    return ref;
  }

  dsp::PortUuidReference
  createMockPort (dsp::PortType type, const utils::Utf8String &name)
  {
    if (type == dsp::PortType::Audio)
      {
        return port_registry_->create_object<dsp::AudioPort> (
          name, dsp::PortFlow::Output, dsp::AudioPort::BusLayout::Mono, 1);
      }
    else if (type == dsp::PortType::Midi)
      {
        return port_registry_->create_object<dsp::MidiPort> (
          name, dsp::PortFlow::Output);
      }
    throw std::runtime_error ("Unsupported port type");
  }

  std::unique_ptr<dsp::PortRegistry>               port_registry_;
  std::unique_ptr<dsp::ProcessorParameterRegistry> param_registry_;
  std::unique_ptr<plugins::PluginRegistry>         plugin_registry_;
  Channel::NameProvider                            name_provider_;
  dsp::Fader::ShouldBeMutedCallback                should_be_muted_cb_;
  dsp::graph::Graph                                graph_;
  sample_rate_t                                    sample_rate_{ 48000 };
  nframes_t                                        max_block_length_{ 1024 };

  std::unique_ptr<Channel> audio_channel_;
  std::unique_ptr<Channel> midi_channel_;

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
    auto processor = std::make_unique<
      MockTrackProcessor> (dsp::ProcessorBase::ProcessorBaseDependencies{
      .port_registry_ = *port_registry_, .param_registry_ = *param_registry_ });

    // Add appropriate output ports
    if (type == dsp::PortType::Audio)
      {
        auto port_ref = port_registry_->create_object<dsp::AudioPort> (
          u8"Track Out", dsp::PortFlow::Output,
          num_channels == 1
            ? dsp::AudioPort::BusLayout::Mono
            : dsp::AudioPort::BusLayout::Stereo,
          num_channels);
        processor->add_output_port (port_ref);
      }
    else if (type == dsp::PortType::Midi)
      {
        auto port_ref = port_registry_->create_object<dsp::MidiPort> (
          u8"Track MIDI Out", dsp::PortFlow::Output);
        processor->add_output_port (port_ref);
      }

    return processor;
  }

  dsp::PortUuidReference
  getProcessorOutputPort (const dsp::ProcessorBase &processor)
  {
    return processor.get_output_ports ().front ();
  }

  void addTrackProcessorToGraph (
    MockTrackProcessor &track_processor,
    bool                connect = true)
  {
    auto track_processor_node =
      graph_.add_node_for_processable (track_processor);
    for (const auto &port_ref : track_processor.get_output_ports ())
      {
        std::visit (
          [&] (auto &&port) {
            auto * node = graph_.add_node_for_processable (*port);
            if (connect)
              {
                track_processor_node->connect_to (*node);
              }
          },
          port_ref.get_object ());
      }
  }

  void verifyProcessorsConnected (
    const dsp::ProcessorBase &src_processor,
    const dsp::ProcessorBase &dest_processor)
  {
    using ObjectView = utils::UuidIdentifiableObjectView<dsp::PortRegistry>;
    const auto object_getter = [] (auto &&port_ref) {
      return port_ref.get_object ();
    };
    const auto src_output_ports =
      src_processor.get_output_ports () | std::views::transform (object_getter);
    const auto dest_input_ports =
      dest_processor.get_input_ports () | std::views::transform (object_getter);

    auto src_midi_out_ports =
      src_output_ports
      | std::views::filter (ObjectView::type_projection<dsp::MidiPort>)
      | std::views::transform (ObjectView::type_transformation<dsp::MidiPort>);
    auto dest_midi_in_ports =
      dest_input_ports
      | std::views::filter (ObjectView::type_projection<dsp::MidiPort>)
      | std::views::transform (ObjectView::type_transformation<dsp::MidiPort>);
    auto src_audio_out_ports =
      src_output_ports
      | std::views::filter (ObjectView::type_projection<dsp::AudioPort>)
      | std::views::transform (ObjectView::type_transformation<dsp::AudioPort>);
    auto dest_audio_in_ports =
      dest_input_ports
      | std::views::filter (ObjectView::type_projection<dsp::AudioPort>)
      | std::views::transform (ObjectView::type_transformation<dsp::AudioPort>);

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

    EXPECT_NONNULL (prefader);
    EXPECT_NONNULL (fader);
    EXPECT_NONNULL (postfader);

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

    EXPECT_NONNULL (prefader);
    EXPECT_NONNULL (fader);
    EXPECT_NONNULL (postfader);

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
      graph_, *port_registry_, *audio_channel_, track_output);
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
      graph_, *port_registry_, *midi_channel_, track_output);
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
      graph_, *port_registry_, *audio_channel_, track_output);
  });

  // Get plugin objects
  std::vector<zrythm::plugins::PluginPtrVariant> plugin_vars;
  audio_channel_->get_plugins (plugin_vars);
  EXPECT_EQ (plugin_vars.size (), 2);

  auto plugins =
    plugin_vars | std::views::transform (&plugins::plugin_ptr_variant_to_base);

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
      graph_, *port_registry_, *midi_channel_, track_output);
  });

  // Get plugin objects
  std::vector<zrythm::plugins::PluginPtrVariant> plugin_vars;
  midi_channel_->get_plugins (plugin_vars);
  EXPECT_EQ (plugin_vars.size (), 2);

  auto plugins =
    plugin_vars | std::views::transform (&plugins::plugin_ptr_variant_to_base);

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

  // Add nodes first
  ChannelSubgraphBuilder::add_nodes (graph_, *audio_channel_);

  // Create mock track processor
  auto track_processor = createMockTrackProcessor (dsp::PortType::Audio, 2);
  auto track_output = getProcessorOutputPort (*track_processor);

  // Add track processor outputs to the graph
  addTrackProcessorToGraph (*track_processor);

  EXPECT_NO_THROW ({
    ChannelSubgraphBuilder::add_connections (
      graph_, *port_registry_, *audio_channel_, track_output);
  });

  // Verify basic connections
  verifyBasicAudioChannelConnections (*track_processor);

  // Check also that pre-fader has connections to sends
  verifyProcessorsConnected (
    audio_channel_->get_audio_pre_fader (),
    *audio_channel_->pre_fader_sends ().front ());
}

TEST_F (ChannelSubgraphBuilderTest, AddConnectionsWithPostFaderSends)
{
  audio_channel_ = createAudioChannel ();
  ASSERT_NE (audio_channel_, nullptr);

  // Add nodes first
  ChannelSubgraphBuilder::add_nodes (graph_, *audio_channel_);

  // Create mock track processor
  auto track_processor = createMockTrackProcessor (dsp::PortType::Audio, 2);
  auto track_output = getProcessorOutputPort (*track_processor);

  // Add track processor outputs to the graph
  addTrackProcessorToGraph (*track_processor);

  EXPECT_NO_THROW ({
    ChannelSubgraphBuilder::add_connections (
      graph_, *port_registry_, *audio_channel_, track_output);
  });

  // Verify basic connections
  verifyBasicAudioChannelConnections (*track_processor);

  // Check that fader has connections to sends
  verifyProcessorsConnected (
    audio_channel_->get_fader (), *audio_channel_->post_fader_sends ().front ());
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
      graph_, *port_registry_, *audio_channel_, track_output);
  });

  // Get plugin objects
  std::vector<zrythm::plugins::PluginPtrVariant> plugin_vars;
  audio_channel_->get_plugins (plugin_vars);
  EXPECT_EQ (plugin_vars.size (), 1);

  auto plugins =
    plugin_vars | std::views::transform (&plugins::plugin_ptr_variant_to_base);

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
      graph_, *port_registry_, *audio_channel_, track_output);
  });

  // Get plugin objects
  std::vector<zrythm::plugins::PluginPtrVariant> plugin_vars;
  audio_channel_->get_plugins (plugin_vars);
  EXPECT_EQ (plugin_vars.size (), 2);

  auto plugins =
    plugin_vars | std::views::transform (&plugins::plugin_ptr_variant_to_base);

  // Verify connections handle asymmetric I/O correctly
  verifyProcessorsConnected (*track_processor, *plugins.front ());
  verifyProcessorsConnected (*plugins[0], *plugins[1]);
  EXPECT_TRUE (hasConnection (
    *plugins[0]->get_output_ports ().front ().get_object_as<dsp::AudioPort> (),
    *plugins[1]->get_input_ports ().front ().get_object_as<dsp::AudioPort> ()));
  EXPECT_TRUE (hasConnection (
    *plugins[0]->get_output_ports ().front ().get_object_as<dsp::AudioPort> (),
    *plugins[1]->get_input_ports ().front ().get_object_as<dsp::AudioPort> ()));
  EXPECT_FALSE (hasConnection (
    *plugins[0]->get_output_ports ().at (0).get_object_as<dsp::AudioPort> (),
    *plugins[1]->get_input_ports ().at (1).get_object_as<dsp::AudioPort> ()));
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
      graph_, *port_registry_, *audio_channel_, track_output);
  });

  // Get plugin objects
  std::vector<zrythm::plugins::PluginPtrVariant> plugin_vars;
  audio_channel_->get_plugins (plugin_vars);
  EXPECT_EQ (plugin_vars.size (), 1);

  auto plugins =
    plugin_vars | std::views::transform (&plugins::plugin_ptr_variant_to_base);

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
      graph_, *port_registry_, *audio_channel_, track_output);
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
        graph_, *port_registry_, *audio_channel_, track_output);
    },
    std::invalid_argument);
}

} // namespace zrythm::structure::tracks
