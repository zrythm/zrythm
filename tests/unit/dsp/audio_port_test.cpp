// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/panning.h"
#include "dsp/port_all.h"
#include "utils/icloneable.h"

#include "unit/dsp/graph_helpers.h"
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

namespace zrythm::dsp
{

class AudioPortTest : public ::testing::Test
{
protected:
  static constexpr auto SAMPLE_RATE = units::sample_rate (44100);
  static constexpr auto BLOCK_LENGTH = units::samples (256);

  void SetUp () override
  {
    // Create ports
    mono_input = std::make_unique<AudioPort> (
      u8"MonoIn", PortFlow::Input, SpeakerArrangement::mono ());
    stereo_output = std::make_unique<AudioPort> (
      u8"StereoOut", PortFlow::Output, SpeakerArrangement::stereo ());

    // Prepare for processing
    mono_input->prepare_for_processing (nullptr, SAMPLE_RATE, BLOCK_LENGTH);
    stereo_output->prepare_for_processing (nullptr, SAMPLE_RATE, BLOCK_LENGTH);

    mock_transport_ = std::make_unique<graph_test::MockTransport> ();
    tempo_map_ = std::make_unique<dsp::TempoMap> (SAMPLE_RATE);

    // Fill with test data
    for (const auto i : std::views::iota (0, BLOCK_LENGTH.in (units::samples)))
      {
        float sample =
          0.5f
          * std::sin (
            2.0f * std::numbers::pi_v<float> * 440.0f * static_cast<float> (i)
            / static_cast<float> (SAMPLE_RATE.in (units::sample_rate)));

        // For mono input, write to channel 0
        mono_input->buffers ()->setSample (0, static_cast<int> (i), sample);

        // For stereo output, initialize both channels to silence
        stereo_output->buffers ()->setSample (0, static_cast<int> (i), 0.0f);
        stereo_output->buffers ()->setSample (1, static_cast<int> (i), 0.0f);
      }
  }

  std::unique_ptr<AudioPort>                 mono_input;
  std::unique_ptr<AudioPort>                 stereo_output;
  std::unique_ptr<graph_test::MockTransport> mock_transport_;
  std::unique_ptr<dsp::TempoMap>             tempo_map_;
};

TEST_F (AudioPortTest, ArrangementChangeReleasesBufferUntilReprepare)
{
  ASSERT_NE (stereo_output->buffers ().get (), nullptr);
  EXPECT_EQ (stereo_output->num_channels (), 2);

  stereo_output->set_arrangement (SpeakerArrangement::mono ());

  // The buffer keeps no stale channel count: it is released until the next
  // prepare, so misuse trips the null-buffer assertions in the processing
  // path
  EXPECT_EQ (stereo_output->num_channels (), 1);
  EXPECT_EQ (stereo_output->buffers ().get (), nullptr);

  stereo_output->prepare_for_processing (nullptr, SAMPLE_RATE, BLOCK_LENGTH);
  ASSERT_NE (stereo_output->buffers ().get (), nullptr);
  EXPECT_EQ (stereo_output->buffers ()->getNumChannels (), 1);
}

TEST_F (AudioPortTest, BasicProperties)
{
  EXPECT_TRUE (mono_input->is_audio ());
  EXPECT_TRUE (mono_input->is_input ());
  EXPECT_EQ (mono_input->arrangement (), SpeakerArrangement::mono ());
  EXPECT_EQ (mono_input->purpose (), AudioPort::Purpose::Main);
  EXPECT_EQ (mono_input->num_channels (), 1);

  EXPECT_TRUE (stereo_output->is_audio ());
  EXPECT_TRUE (stereo_output->is_output ());
  EXPECT_EQ (stereo_output->arrangement (), SpeakerArrangement::stereo ());
  EXPECT_EQ (stereo_output->purpose (), AudioPort::Purpose::Main);
  EXPECT_EQ (stereo_output->num_channels (), 2);
}

TEST_F (AudioPortTest, ExternalPortIdSerialization)
{
  // absent by default, and absent from the JSON
  EXPECT_EQ (mono_input->external_port_id (), std::nullopt);
  nlohmann::json j;
  to_json (j, *mono_input);
  EXPECT_FALSE (j.contains ("externalPortId"));

  AudioPort deserialized (
    u8"Placeholder", PortFlow::Input, SpeakerArrangement::mono ());
  from_json (j, deserialized);
  EXPECT_EQ (deserialized.external_port_id (), std::nullopt);

  // a set id round-trips
  stereo_output->set_external_port_id (42);
  nlohmann::json j_with_id;
  to_json (j_with_id, *stereo_output);
  EXPECT_EQ (j_with_id.at ("externalPortId").get<uint32_t> (), 42);

  AudioPort round_tripped (
    u8"Placeholder", PortFlow::Output, SpeakerArrangement::stereo ());
  from_json (j_with_id, round_tripped);
  EXPECT_EQ (round_tripped.external_port_id (), 42);
}

TEST_F (AudioPortTest, ExternalPortIdSurvivesClone)
{
  stereo_output->set_external_port_id (7);
  const auto clone = utils::clone_unique (
    *stereo_output, utils::ObjectCloneType::Snapshot, u8"Clone",
    PortFlow::Output, SpeakerArrangement::stereo ());
  EXPECT_EQ (clone->external_port_id (), 7);
}

TEST_F (AudioPortTest, ResourceManagement)
{
  // Verify buffers were allocated
  EXPECT_NE (mono_input->buffers ()->getArrayOfReadPointers (), nullptr);
  EXPECT_NE (stereo_output->buffers ()->getArrayOfReadPointers (), nullptr);

  // Release resources
  mono_input->release_resources ();
  stereo_output->release_resources ();

  // Verify buffers were released
  EXPECT_EQ (mono_input->buffers (), nullptr);
  EXPECT_EQ (stereo_output->buffers (), nullptr);
}

TEST_F (AudioPortTest, StereoPortsHelper)
{
  // Test the StereoPorts::get_name_and_symbols helper function
  auto [left_name, left_symbol] =
    StereoPorts::get_name_and_symbols (true, u8"Stereo", u8"stereo");
  auto [right_name, right_symbol] =
    StereoPorts::get_name_and_symbols (false, u8"Stereo", u8"stereo");

  EXPECT_EQ (left_name.str (), "Stereo L");
  EXPECT_EQ (right_name.str (), "Stereo R");
  EXPECT_EQ (left_symbol.str (), "stereo_l");
  EXPECT_EQ (right_symbol.str (), "stereo_r");
}

TEST_F (AudioPortTest, LimitingFunctionality)
{
  auto port = std::make_unique<AudioPort> (
    u8"TestPort", PortFlow::Output, SpeakerArrangement::mono ());
  port->prepare_for_processing (nullptr, SAMPLE_RATE, BLOCK_LENGTH);

  // Initially should not require limiting
  EXPECT_FALSE (port->requires_limiting ());

  // Enable limiting
  port->mark_as_requires_limiting ();
  EXPECT_TRUE (port->requires_limiting ());
}

TEST_F (AudioPortTest, DifferentSpeakerArrangements)
{
  // Test creating ports with different bus layouts
  auto mono_port = std::make_unique<AudioPort> (
    u8"Mono", PortFlow::Output, SpeakerArrangement::mono ());
  auto stereo_port = std::make_unique<AudioPort> (
    u8"Stereo", PortFlow::Output, SpeakerArrangement::stereo ());

  EXPECT_EQ (mono_port->arrangement (), SpeakerArrangement::mono ());
  EXPECT_EQ (stereo_port->arrangement (), SpeakerArrangement::stereo ());

  EXPECT_EQ (mono_port->num_channels (), 1);
  EXPECT_EQ (stereo_port->num_channels (), 2);
}

TEST_F (AudioPortTest, DifferentPurposes)
{
  // Test creating ports with different purposes
  auto main_port = std::make_unique<AudioPort> (
    u8"Main", PortFlow::Output, SpeakerArrangement::mono (),
    AudioPort::Purpose::Main);
  auto sidechain_port = std::make_unique<AudioPort> (
    u8"Sidechain", PortFlow::Output, SpeakerArrangement::mono (),
    AudioPort::Purpose::Sidechain);

  EXPECT_EQ (main_port->purpose (), AudioPort::Purpose::Main);
  EXPECT_EQ (sidechain_port->purpose (), AudioPort::Purpose::Sidechain);
}

TEST_F (AudioPortTest, OutputPortPreservesDataThroughProcessBlock)
{
  // Write test signal to output port (simulating a processor writing output)
  for (const auto i : std::views::iota (0, BLOCK_LENGTH.in (units::samples)))
    {
      float sample = static_cast<float> (i) * 0.01f;
      stereo_output->buffers ()->setSample (0, static_cast<int> (i), sample);
      stereo_output->buffers ()->setSample (1, static_cast<int> (i), -sample);
    }

  auto time_nfo = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), BLOCK_LENGTH);
  stereo_output->process_block (time_nfo, *mock_transport_, *tempo_map_);

  // Output port should not clear — processor-written data survives
  for (const auto i : std::views::iota (0, BLOCK_LENGTH.in (units::samples)))
    {
      float expected = static_cast<float> (i) * 0.01f;
      EXPECT_NEAR (
        stereo_output->buffers ()->getSample (0, static_cast<int> (i)),
        expected, 1e-6f);
      EXPECT_NEAR (
        stereo_output->buffers ()->getSample (1, static_cast<int> (i)),
        -expected, 1e-6f);
    }
}

/**
 * @brief Covers how a source port's channels are mapped onto a destination
 * port's channels for every combination of speaker arrangements.
 */
class AudioPortRoutingTest : public ::testing::Test
{
protected:
  static constexpr auto SAMPLE_RATE = units::sample_rate (44100);
  static constexpr auto BLOCK_LENGTH = units::samples (64);

  static void fill (const AudioPort &port, float value)
  {
    for (const auto ch : std::views::iota (0, int{ port.num_channels () }))
      {
        for (
          const auto i :
          std::views::iota (0, BLOCK_LENGTH.in<int> (units::samples)))
          {
            port.buffers ()->setSample (ch, i, value);
          }
      }
  }

  /** Creates a prepared port whose channel @c ch holds the constant @c ch + 1. */
  static std::unique_ptr<AudioPort> make_source (SpeakerArrangement arrangement)
  {
    auto port =
      std::make_unique<AudioPort> (u8"Src", PortFlow::Output, arrangement);
    port->prepare_for_processing (nullptr, SAMPLE_RATE, BLOCK_LENGTH);
    for (const auto ch : std::views::iota (0, int{ port->num_channels () }))
      {
        for (
          const auto i :
          std::views::iota (0, BLOCK_LENGTH.in<int> (units::samples)))
          {
            port->buffers ()->setSample (ch, i, static_cast<float> (ch + 1));
          }
      }
    return port;
  }

  static std::unique_ptr<AudioPort>
  make_destination (SpeakerArrangement arrangement)
  {
    auto port =
      std::make_unique<AudioPort> (u8"Dest", PortFlow::Input, arrangement);
    port->prepare_for_processing (nullptr, SAMPLE_RATE, BLOCK_LENGTH);
    port->buffers ()->clear ();
    return port;
  }

  static float first_sample (const AudioPort &port, int channel)
  {
    return port.buffers ()->getSample (channel, 0);
  }

  static dsp::graph::ProcessBlockInfo whole_block ()
  {
    return dsp::graph::ProcessBlockInfo::from_position_and_nframes (
      units::samples (0), BLOCK_LENGTH);
  }

  /** Gains a two-channel source is summed with when folded down to one. */
  static std::pair<float, float> downmix_gains ()
  {
    return calculate_panning (PanLaw::Minus6dB, PanAlgorithm::SquareRoot, 0.5f);
  }

  void SetUp () override
  {
    mock_transport_ = std::make_unique<graph_test::MockTransport> ();
    tempo_map_ = std::make_unique<dsp::TempoMap> (SAMPLE_RATE);
  }

  std::unique_ptr<graph_test::MockTransport> mock_transport_;
  std::unique_ptr<dsp::TempoMap>             tempo_map_;
};

TEST_F (AudioPortRoutingTest, MatchingChannelCountsRouteOneToOne)
{
  auto src = make_source (SpeakerArrangement::stereo ());
  auto dest = make_destination (SpeakerArrangement::stereo ());

  dest->copy_source_rt (*src, whole_block ());

  EXPECT_FLOAT_EQ (first_sample (*dest, 0), 1.f);
  EXPECT_FLOAT_EQ (first_sample (*dest, 1), 2.f);
}

TEST_F (AudioPortRoutingTest, MonoSourceFoldsIntoFrontPairAtMinus3dB)
{
  auto src = make_source (SpeakerArrangement::mono ());
  auto dest = make_destination (SpeakerArrangement::stereo ());

  dest->copy_source_rt (*src, whole_block ());

  EXPECT_FLOAT_EQ (first_sample (*dest, 0), 0.70710678f);
  EXPECT_FLOAT_EQ (first_sample (*dest, 1), 0.70710678f);
}

TEST_F (
  AudioPortRoutingTest,
  SingleChannelSourceWithoutLayoutFillsEveryDestinationChannel)
{
  auto src = make_source (SpeakerArrangement::discrete_channels (1));
  auto dest = make_destination (SpeakerArrangement::stereo ());

  dest->copy_source_rt (*src, whole_block ());

  EXPECT_FLOAT_EQ (first_sample (*dest, 0), 1.f);
  EXPECT_FLOAT_EQ (first_sample (*dest, 1), 1.f);
}

TEST_F (AudioPortRoutingTest, StereoSourceSumsIntoSingleChannelDestination)
{
  const auto gains = downmix_gains ();
  auto       src = make_source (SpeakerArrangement::stereo ());
  auto       dest = make_destination (SpeakerArrangement::mono ());

  dest->copy_source_rt (*src, whole_block ());

  EXPECT_FLOAT_EQ (
    first_sample (*dest, 0), (gains.first * 1.f) + (gains.second * 2.f));
}

TEST_F (
  AudioPortRoutingTest,
  TwoChannelSourceWithoutLayoutSumsIntoSingleChannelDestination)
{
  const auto gains = downmix_gains ();
  auto       src = make_source (SpeakerArrangement::discrete_channels (2));
  auto dest = make_destination (SpeakerArrangement::discrete_channels (1));

  dest->copy_source_rt (*src, whole_block ());

  EXPECT_FLOAT_EQ (
    first_sample (*dest, 0), (gains.first * 1.f) + (gains.second * 2.f));
}

TEST_F (AudioPortRoutingTest, SourceChannelsBeyondDestinationCountAreDropped)
{
  auto src = make_source (SpeakerArrangement::discrete_channels (4));
  auto dest = make_destination (SpeakerArrangement::discrete_channels (2));

  dest->copy_source_rt (*src, whole_block ());

  EXPECT_FLOAT_EQ (first_sample (*dest, 0), 1.f);
  EXPECT_FLOAT_EQ (first_sample (*dest, 1), 2.f);
}

TEST_F (AudioPortRoutingTest, CopyClearsDestinationChannelsWithNoSource)
{
  auto src = make_source (SpeakerArrangement::discrete_channels (2));
  auto dest = make_destination (SpeakerArrangement::discrete_channels (4));
  fill (*dest, 9.f);

  dest->copy_source_rt (*src, whole_block ());

  EXPECT_FLOAT_EQ (first_sample (*dest, 0), 1.f);
  EXPECT_FLOAT_EQ (first_sample (*dest, 1), 2.f);
  EXPECT_FLOAT_EQ (first_sample (*dest, 2), 0.f);
  EXPECT_FLOAT_EQ (first_sample (*dest, 3), 0.f);
}

TEST_F (AudioPortRoutingTest, CopyReplacesExistingDestinationContents)
{
  auto src = make_source (SpeakerArrangement::stereo ());
  auto dest = make_destination (SpeakerArrangement::stereo ());
  fill (*dest, 9.f);

  dest->copy_source_rt (*src, whole_block ());

  EXPECT_FLOAT_EQ (first_sample (*dest, 0), 1.f);
  EXPECT_FLOAT_EQ (first_sample (*dest, 1), 2.f);
}

TEST_F (AudioPortRoutingTest, MultiplierScalesRoutedSignal)
{
  auto src = make_source (SpeakerArrangement::stereo ());
  auto dest = make_destination (SpeakerArrangement::stereo ());

  dest->copy_source_rt (*src, whole_block (), 0.5f);

  EXPECT_FLOAT_EQ (first_sample (*dest, 0), 0.5f);
  EXPECT_FLOAT_EQ (first_sample (*dest, 1), 1.f);
}

TEST_F (AudioPortRoutingTest, AddAccumulatesIntoDestination)
{
  auto src = make_source (SpeakerArrangement::stereo ());
  auto dest = make_destination (SpeakerArrangement::stereo ());
  fill (*dest, 10.f);

  dest->add_source_rt (*src, whole_block ());

  EXPECT_FLOAT_EQ (first_sample (*dest, 0), 11.f);
  EXPECT_FLOAT_EQ (first_sample (*dest, 1), 12.f);
}

TEST_F (AudioPortRoutingTest, AddLeavesDestinationChannelsWithNoSourceUntouched)
{
  auto src = make_source (SpeakerArrangement::discrete_channels (2));
  auto dest = make_destination (SpeakerArrangement::discrete_channels (4));
  fill (*dest, 10.f);

  dest->add_source_rt (*src, whole_block ());

  EXPECT_FLOAT_EQ (first_sample (*dest, 0), 11.f);
  EXPECT_FLOAT_EQ (first_sample (*dest, 1), 12.f);
  EXPECT_FLOAT_EQ (first_sample (*dest, 2), 10.f);
  EXPECT_FLOAT_EQ (first_sample (*dest, 3), 10.f);
}

TEST_F (AudioPortRoutingTest, ExplicitRoutingOverridesArrangements)
{
  auto src = make_source (SpeakerArrangement::stereo ());
  auto dest = make_destination (SpeakerArrangement::stereo ());

  // swap the channels, which the arrangements alone would never do
  const AudioBusChannelRouting routing{
    { { .source_channel = 1, .destination_channel = 0 },
     { .source_channel = 0, .destination_channel = 1 } }
  };
  dest->copy_source_rt (*src, routing, whole_block ());

  EXPECT_FLOAT_EQ (first_sample (*dest, 0), 2.f);
  EXPECT_FLOAT_EQ (first_sample (*dest, 1), 1.f);
}

TEST_F (AudioPortRoutingTest, ExplicitRoutingCanSumSeveralSourcesIntoOneChannel)
{
  auto src = make_source (SpeakerArrangement::discrete_channels (3));
  auto dest = make_destination (SpeakerArrangement::stereo ());

  const AudioBusChannelRouting routing{
    { { .source_channel = 0, .destination_channel = 0 },
     { .source_channel = 1, .destination_channel = 0 },
     { .source_channel = 2, .destination_channel = 0, .gain = 0.5f } }
  };
  dest->copy_source_rt (*src, routing, whole_block ());

  // 1 + 2 + (3 * 0.5)
  EXPECT_FLOAT_EQ (first_sample (*dest, 0), 4.5f);
  EXPECT_FLOAT_EQ (first_sample (*dest, 1), 0.f);
}

TEST_F (AudioPortRoutingTest, CopyWithEmptyExplicitRoutingSilencesDestination)
{
  auto src = make_source (SpeakerArrangement::stereo ());
  auto dest = make_destination (SpeakerArrangement::stereo ());
  fill (*dest, 9.f);

  dest->copy_source_rt (
    *src, AudioBusChannelRouting{ std::vector<AudioBusChannelRoute>{} },
    whole_block ());

  EXPECT_FLOAT_EQ (first_sample (*dest, 0), 0.f);
  EXPECT_FLOAT_EQ (first_sample (*dest, 1), 0.f);
}

TEST_F (AudioPortRoutingTest, AddSumsStereoSourceIntoSingleChannelDestination)
{
  const auto gains = downmix_gains ();
  auto       src = make_source (SpeakerArrangement::stereo ());
  auto       dest = make_destination (SpeakerArrangement::mono ());

  dest->add_source_rt (*src, whole_block ());

  EXPECT_FLOAT_EQ (
    first_sample (*dest, 0), (gains.first * 1.f) + (gains.second * 2.f));
}

TEST_F (AudioPortRoutingTest, CachedConnectionMultiplierIsApplied)
{
  auto src = make_source (SpeakerArrangement::stereo ());
  auto dest = make_destination (SpeakerArrangement::stereo ());

  graph::GraphNode src_node{ 0, *src };
  graph::GraphNode dest_node{ 1, *dest };
  src_node.connect_to (
    dest_node, graph::ConnectionConfig{ .multiplier_ = 0.5f });
  dest->prepare_for_processing (&dest_node, SAMPLE_RATE, BLOCK_LENGTH);

  dest->process_block (whole_block (), *mock_transport_, *tempo_map_);

  EXPECT_FLOAT_EQ (first_sample (*dest, 0), 0.5f);
  EXPECT_FLOAT_EQ (first_sample (*dest, 1), 1.f);
}

TEST_F (AudioPortRoutingTest, DisabledCachedConnectionSkipsSource)
{
  auto src = make_source (SpeakerArrangement::stereo ());
  auto dest = make_destination (SpeakerArrangement::stereo ());

  graph::GraphNode src_node{ 0, *src };
  graph::GraphNode dest_node{ 1, *dest };
  src_node.connect_to (dest_node, graph::ConnectionConfig{ .enabled_ = false });
  dest->prepare_for_processing (&dest_node, SAMPLE_RATE, BLOCK_LENGTH);

  dest->process_block (whole_block (), *mock_transport_, *tempo_map_);

  EXPECT_FLOAT_EQ (first_sample (*dest, 0), 0.f);
  EXPECT_FLOAT_EQ (first_sample (*dest, 1), 0.f);
}

TEST_F (AudioPortRoutingTest, CachedExplicitRoutingOverridesArrangements)
{
  auto src = make_source (SpeakerArrangement::stereo ());
  auto dest = make_destination (SpeakerArrangement::stereo ());

  // swap the channels, which the arrangements alone would never do
  graph::ConnectionConfig config;
  config.routing_ = AudioBusChannelRouting{
    { { .source_channel = 1, .destination_channel = 0 },
     { .source_channel = 0, .destination_channel = 1 } }
  };

  graph::GraphNode src_node{ 0, *src };
  graph::GraphNode dest_node{ 1, *dest };
  src_node.connect_to (dest_node, config);
  dest->prepare_for_processing (&dest_node, SAMPLE_RATE, BLOCK_LENGTH);

  dest->process_block (whole_block (), *mock_transport_, *tempo_map_);

  EXPECT_FLOAT_EQ (first_sample (*dest, 0), 2.f);
  EXPECT_FLOAT_EQ (first_sample (*dest, 1), 1.f);
}

TEST_F (AudioPortRoutingTest, EdgeWithoutConnectionUsesUnityDefaults)
{
  auto src = make_source (SpeakerArrangement::stereo ());
  auto dest = make_destination (SpeakerArrangement::stereo ());

  graph::GraphNode src_node{ 0, *src };
  graph::GraphNode dest_node{ 1, *dest };
  src_node.connect_to (dest_node);
  dest->prepare_for_processing (&dest_node, SAMPLE_RATE, BLOCK_LENGTH);

  dest->process_block (whole_block (), *mock_transport_, *tempo_map_);

  EXPECT_FLOAT_EQ (first_sample (*dest, 0), 1.f);
  EXPECT_FLOAT_EQ (first_sample (*dest, 1), 2.f);
}

} // namespace zrythm::dsp
