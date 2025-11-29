// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/graph_renderer.h"
#include "utils/audio.h"

#include "./graph_helpers.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;
using namespace zrythm::units;

namespace zrythm::dsp
{
class GraphRendererTest : public ::testing::Test
{
protected:
  class SineWaveGenerator
  {
  public:
    SineWaveGenerator (
      AudioPort &audio_port,
      float      amplitude = 0.1f,
      float      frequency = 440.0f,
      float      sample_rate = 48000.0f,
      nframes_t  latency_frames = 0)
        : audio_port_ (audio_port), amplitude_ (amplitude),
          frequency_ (frequency), sample_rate_ (sample_rate),
          original_latency_ (latency_frames), prefilled_samples_ (2, 48000)
    {
      // Prepare the samples to output (with enough samples for any test)
      for (
        int i = 0;
        i
        < prefilled_samples_.getNumSamples () - static_cast<int> (latency_frames);
        ++i)
        {
          for (int ch = 0; ch < prefilled_samples_.getNumChannels (); ++ch)
            {
              const float sample =
                amplitude_
                * std::sin (
                  2.0f * std::numbers::pi_v<float>
                  * frequency_ * static_cast<float> (i) / sample_rate_);

              // Account for latency: only generate samples after
              // latency frames
              prefilled_samples_.setSample (
                ch, static_cast<int> (latency_frames) + i,
                sample * static_cast<float> (ch + 1));
            }
        }

      // Fill the latency samples with silence
      if (latency_frames > 0)
        {
          prefilled_samples_.clear (0, static_cast<int> (latency_frames));
        }
    }

    void process_block (
      const EngineProcessTimeInfo &time_nfo,
      const ITransport            &transport)
    {
      const auto &buffers = audio_port_.buffers ();
      if (buffers && buffers->getNumChannels () > 0)
        {
          // Copy from prefilled frames
          for (int ch = 0; ch < buffers->getNumChannels (); ++ch)
            {
              buffers->copyFrom (
                ch, static_cast<int> (time_nfo.local_offset_),
                prefilled_samples_, ch,
                static_cast<int> (time_nfo.g_start_frame_w_offset_),
                static_cast<int> (time_nfo.nframes_));
            }
        }
    }

  private:
    AudioPort                       &audio_port_;
    float                            amplitude_ = 0.1f;
    float                            frequency_ = 440.0f;
    float                            sample_rate_ = 48000.0f;
    [[maybe_unused]] const nframes_t original_latency_;
    juce::AudioSampleBuffer          prefilled_samples_;
  };

  using MockProcessable = zrythm::dsp::graph_test::MockProcessable;

  void SetUp () override
  {

    // Setup default render options
    options_ = GraphRenderer::RenderOptions{
      .bit_depth_ = utils::audio::BitDepth::BIT_DEPTH_16,
      .dither_ = false,
      .sample_rate_ = units::sample_rate (48000),
      .block_length_ = units::samples (256),
      .num_threads_ = 2
    };

    // Create audio ports for testing
    audio_port_ = std::make_unique<AudioPort> (
      u8"TestAudioOut", PortFlow::Output, AudioPort::BusLayout::Stereo, 2);
    extra_audio_port_ = std::make_unique<AudioPort> (
      u8"TestAudioOut2", PortFlow::Output, AudioPort::BusLayout::Stereo, 2);
    extra_audio_port_for_summing_ = std::make_unique<AudioPort> (
      u8"TestAudioOutForSumming", PortFlow::Output,
      AudioPort::BusLayout::Stereo, 2);

    // Create mock processable with default behavior
    processable_ = std::make_unique<MockProcessable> ();
    sine_generator_ = std::make_unique<SineWaveGenerator> (*audio_port_);

    ON_CALL (*processable_, get_node_name ())
      .WillByDefault (Return (u8"test_node"));
    ON_CALL (*processable_, get_single_playback_latency ())
      .WillByDefault (Return (0));
    ON_CALL (*processable_, prepare_for_processing (_, _, _))
      .WillByDefault (Return ());
    ON_CALL (*processable_, release_resources ()).WillByDefault (Return ());
    ON_CALL (*processable_, process_block (_, _))
      .WillByDefault ([this] (auto time_nfo, const auto &transport) {
        return sine_generator_->process_block (time_nfo, transport);
      });
  }

  graph::GraphNodeCollection create_simple_test_collection ()
  {
    graph::GraphNodeCollection collection;

    // Create a graph with mock processable as first node connected to audio port
    auto mock_node = std::make_unique<graph::GraphNode> (1, *processable_);
    auto audio_port_node = std::make_unique<graph::GraphNode> (2, *audio_port_);

    // Connect mock node to audio port
    mock_node->connect_to (*audio_port_node);

    collection.graph_nodes_.push_back (std::move (mock_node));
    collection.graph_nodes_.push_back (std::move (audio_port_node));

    collection.finalize_nodes ();
    return collection;
  }

  auto create_mixed_latency_test_collection (const auto latency)
  {
    graph::GraphNodeCollection collection;

    // Create two mock processables - one with latency, one without
    auto processable_no_latency = std::make_unique<MockProcessable> ();
    auto processable_with_latency = std::make_unique<MockProcessable> ();

    // Setup mock without latency
    ON_CALL (*processable_no_latency, get_node_name ())
      .WillByDefault (Return (u8"no_latency_node"));
    ON_CALL (*processable_no_latency, get_single_playback_latency ())
      .WillByDefault (Return (0));
    ON_CALL (*processable_no_latency, prepare_for_processing (_, _, _))
      .WillByDefault (Return ());
    ON_CALL (*processable_no_latency, release_resources ())
      .WillByDefault (Return ());
    auto no_latency_generator = std::make_unique<SineWaveGenerator> (
      *audio_port_, 0.05f, 440.0f, 48000.0f, 0);
    auto * no_latency_generator_ptr = no_latency_generator.get ();
    ON_CALL (*processable_no_latency, process_block (_, _))
      .WillByDefault (
        [no_latency_generator_ptr] (auto time_nfo, const auto &transport) {
          no_latency_generator_ptr->process_block (time_nfo, transport);
        });

    // Setup mock with latency
    ON_CALL (*processable_with_latency, get_node_name ())
      .WillByDefault (Return (u8"latency_node"));
    ON_CALL (*processable_with_latency, get_single_playback_latency ())
      .WillByDefault (Return (latency));
    ON_CALL (*processable_with_latency, prepare_for_processing (_, _, _))
      .WillByDefault (Return ());
    ON_CALL (*processable_with_latency, release_resources ())
      .WillByDefault (Return ());
    auto latency_generator = std::make_unique<SineWaveGenerator> (
      *extra_audio_port_, 0.05f, 660.0f, 48000.0f, latency);
    auto * latency_generator_ptr = latency_generator.get ();
    ON_CALL (*processable_with_latency, process_block (_, _))
      .WillByDefault (
        [latency_generator_ptr] (auto time_nfo, const auto &transport) {
          latency_generator_ptr->process_block (time_nfo, transport);
        });

    // Create graph nodes
    auto no_latency_node =
      std::make_unique<graph::GraphNode> (1, *processable_no_latency);
    auto latency_node =
      std::make_unique<graph::GraphNode> (2, *processable_with_latency);
    auto no_latency_audio_port_node =
      std::make_unique<graph::GraphNode> (3, *audio_port_);
    auto latency_audio_port_node =
      std::make_unique<graph::GraphNode> (4, *extra_audio_port_);
    auto dummy_final_node =
      std::make_unique<graph::GraphNode> (5, *extra_audio_port_for_summing_);

    // Create the connections
    no_latency_node->connect_to (*no_latency_audio_port_node);
    latency_node->connect_to (*latency_audio_port_node);
    no_latency_audio_port_node->connect_to (*dummy_final_node);
    latency_audio_port_node->connect_to (*dummy_final_node);

    collection.graph_nodes_.push_back (std::move (no_latency_node));
    collection.graph_nodes_.push_back (std::move (latency_node));
    collection.graph_nodes_.push_back (std::move (no_latency_audio_port_node));
    collection.graph_nodes_.push_back (std::move (latency_audio_port_node));
    collection.graph_nodes_.push_back (std::move (dummy_final_node));

    // Store the mock processables so they don't get destroyed
    mixed_latency_processables_.push_back (std::move (processable_no_latency));
    mixed_latency_processables_.push_back (std::move (processable_with_latency));

    collection.finalize_nodes ();
    return std::make_tuple (
      std::move (collection), std::move (no_latency_generator),
      std::move (latency_generator));
  }

  auto create_test_range (const auto start, const auto end)
  {
    return std::make_pair (units::samples (start), units::samples (end));
  }
  void verify_sine_wave_samples (
    const juce::AudioSampleBuffer &result,
    float                          sample_rate = 48000.0f,
    float                          frequency = 440.0f,
    float                          amplitude = 0.1f,
    int                            start_sample = 0,
    int                            num_samples_to_check = -1)
  {
    const auto samples_to_check =
      (num_samples_to_check == -1)
        ? result.getNumSamples ()
        : std::min (num_samples_to_check, result.getNumSamples ());

    for (int ch = 0; ch < result.getNumChannels (); ++ch)
      {
        const auto channel_multiplier = static_cast<float> (ch + 1);

        for (int i = 0; i < samples_to_check; ++i)
          {
            const auto expected_sample =
              amplitude * channel_multiplier
              * std::sin (
                2.0f * std::numbers::pi_v<float>
                * frequency * static_cast<float> (i) / sample_rate);

            const auto actual_sample = result.getSample (ch, i + start_sample);

            // Allow for small floating-point differences
            EXPECT_NEAR (actual_sample, expected_sample, 1e-6f)
              << "Channel " << ch << ", Sample " << i + start_sample
              << ": expected " << expected_sample << ", got " << actual_sample;
          }
      }
  }

  void verify_samples_are_zero (
    const juce::AudioSampleBuffer &result,
    int                            start_sample = 0,
    int                            num_samples = -1)
  {
    const auto samples_to_check =
      (num_samples == -1)
        ? result.getNumSamples ()
        : std::min (num_samples, result.getNumSamples () - start_sample);

    for (int ch = 0; ch < result.getNumChannels (); ++ch)
      {
        for (int i = start_sample; i < start_sample + samples_to_check; ++i)
          {
            EXPECT_FLOAT_EQ (result.getSample (ch, i), 0.0f)
              << "Channel " << ch << ", Sample " << i << " should be zero";
          }
      }
  }

  GraphRenderer::RenderOptions                  options_;
  std::unique_ptr<MockProcessable>              processable_;
  std::unique_ptr<AudioPort>                    audio_port_;
  std::unique_ptr<AudioPort>                    extra_audio_port_;
  std::unique_ptr<AudioPort>                    extra_audio_port_for_summing_;
  std::vector<std::unique_ptr<MockProcessable>> mixed_latency_processables_;
  std::unique_ptr<SineWaveGenerator>            sine_generator_;
};

TEST_F (GraphRendererTest, RenderEmptyRange)
{
  auto collection = create_simple_test_collection ();
  auto range = create_test_range (0, 0);

  auto future =
    GraphRenderer::render_run_async (options_, std::move (collection), range);

  auto result = future.result ();
  future.waitForFinished ();

  EXPECT_EQ (result.getNumChannels (), 2);
  EXPECT_EQ (result.getNumSamples (), 0);
}

TEST_F (GraphRendererTest, RenderSingleBlock)
{
  auto collection = create_simple_test_collection ();
  auto range = create_test_range (0, 256);

  // Expect processing to be called once
  EXPECT_CALL (*processable_, process_block (_, _)).Times (1);

  auto future =
    GraphRenderer::render_run_async (options_, std::move (collection), range);

  auto result = future.result ();
  future.waitForFinished ();

  EXPECT_EQ (result.getNumChannels (), 2);
  EXPECT_EQ (result.getNumSamples (), 256);

  // Verify the sine wave samples are correct
  verify_sine_wave_samples (result);
}

TEST_F (GraphRendererTest, RenderMultipleBlocks)
{
  auto collection = create_simple_test_collection ();
  auto range = create_test_range (0, 512); // 2 blocks

  // Expect processing to be called twice
  EXPECT_CALL (*processable_, process_block (_, _)).Times (2);

  auto future =
    GraphRenderer::render_run_async (options_, std::move (collection), range);

  auto result = future.result ();
  future.waitForFinished ();

  EXPECT_EQ (result.getNumChannels (), 2);
  EXPECT_EQ (result.getNumSamples (), 512);

  // Verify the sine wave samples are correct across multiple blocks
  verify_sine_wave_samples (result);
}

TEST_F (GraphRendererTest, RenderWithLatency)
{
  // Setup processable with latency
  ON_CALL (*processable_, get_single_playback_latency ())
    .WillByDefault (Return (128));

  auto collection = create_simple_test_collection ();
  auto range = create_test_range (0, 256);

  // Expect processing with latency compensation
  SineWaveGenerator generator{ *audio_port_, 0.1f, 440.0f, 48000.0f, 128 };
  ON_CALL (*processable_, process_block (_, _))
    .WillByDefault ([&generator] (auto time_nfo, const auto &transport) {
      generator.process_block (time_nfo, transport);
    });
  EXPECT_CALL (*processable_, process_block (_, _)).Times (2);

  auto future =
    GraphRenderer::render_run_async (options_, std::move (collection), range);

  auto result = future.result ();
  future.waitForFinished ();

  // With latency compensation, max latency (128) should be added to range
  // So expected samples should be 256 + 128 = 384
  EXPECT_EQ (result.getNumChannels (), 2);
  ASSERT_EQ (result.getNumSamples (), 384);

  // With latency compensation, first 128 samples should be silent
  // (latency preroll), then sine wave should start from sample 128
  verify_samples_are_zero (result, 0, 128);

  // Verify sine wave starts after latency compensation (from sample 128
  // onwards) The sine wave should start at the correct phase, accounting for
  // latency offset
  verify_sine_wave_samples (result, 48000.0f, 440.0f, 0.1f, 128, 256);
}

TEST_F (GraphRendererTest, RenderWithMixedLatency)
{
  constexpr auto latency = 64;
  auto [collection, no_latency_generator, latency_generator] =
    create_mixed_latency_test_collection (latency);
  auto range = create_test_range (0, 256);

  // Expect processing for both nodes
  EXPECT_CALL (*mixed_latency_processables_[0], process_block (_, _)).Times (1);
  EXPECT_CALL (*mixed_latency_processables_[1], process_block (_, _)).Times (2);

  auto future =
    GraphRenderer::render_run_async (options_, std::move (collection), range);

  auto result = future.result ();
  future.waitForFinished ();

  // With mixed latency, max latency (64) should be added to range
  // So expected samples should be 256 + 64 = 320
  EXPECT_EQ (result.getNumChannels (), 2);
  EXPECT_EQ (result.getNumSamples (), 320);

  // With mixed latency, max latency (64) should be compensated for
  // First 64 samples should be silent (latency preroll)
  verify_samples_are_zero (result, 0, latency);

  // After latency compensation, both signals should be present
  // The no-latency signal should start at sample 64 with 660Hz frequency
  // The latency signal should also start at sample 64 with 440Hz frequency
  // Combined amplitude should be 0.1f (0.05f + 0.05f)
  for (int ch = 0; ch < result.getNumChannels (); ++ch)
    {
      const auto channel_multiplier = static_cast<float> (ch + 1);

      for (int i = latency; i < result.getNumSamples (); ++i)
        {
          // Expected combined signal: both sine waves at half amplitude each
          const auto sample_440 =
            0.05f * channel_multiplier
            * std::sin (
              2.0f * std::numbers::pi_v<float>
              * 440.0f * static_cast<float> (i - latency) / 48000.0f);
          const auto sample_660 =
            0.05f * channel_multiplier
            * std::sin (
              2.0f * std::numbers::pi_v<float>
              * 660.0f * static_cast<float> (i - latency) / 48000.0f);
          const auto expected_sample = sample_440 + sample_660;

          const auto actual_sample = result.getSample (ch, i);

          EXPECT_NEAR (actual_sample, expected_sample, 1e-5f)
            << "Channel " << ch << ", Sample " << i
            << ": expected combined signal " << expected_sample << ", got "
            << actual_sample;
        }
    }
}

TEST_F (GraphRendererTest, RenderWithDithering)
{
  auto dither_options = options_;
  dither_options.dither_ = true;
  dither_options.bit_depth_ = utils::audio::BitDepth::BIT_DEPTH_16;

  auto collection = create_simple_test_collection ();
  auto range = create_test_range (0, 256);

  EXPECT_CALL (*processable_, process_block (_, _)).Times (1);

  auto future = GraphRenderer::render_run_async (
    dither_options, std::move (collection), range);

  auto result = future.result ();
  future.waitForFinished ();

  EXPECT_EQ (result.getNumChannels (), 2);
  EXPECT_EQ (result.getNumSamples (), 256);

  // Verify that dithering is applied - samples should be quantized to 16-bit
  // levels but still follow the sine wave pattern approximately
  for (int ch = 0; ch < result.getNumChannels (); ++ch)
    {
      const auto channel_multiplier = static_cast<float> (ch + 1);

      // Check a few samples to verify they're quantized to 16-bit levels
      for (int i = 0; i < std::min (16, result.getNumSamples ()); ++i)
        {
          const auto expected_sample =
            0.1f * channel_multiplier
            * std::sin (
              2.0f * std::numbers::pi_v<float>
              * 440.0f * static_cast<float> (i) / 48000.0f);

          const auto actual_sample = result.getSample (ch, i);

          // 16-bit quantization step size is approximately 1/32768
          constexpr auto quantization_step =
            (1.0f / 32768.0f) * 2.f; // make less precise

          // The actual sample should be close to a quantized version of the
          // expected
          const auto quantized_expected =
            std::round (expected_sample / quantization_step) * quantization_step;

          EXPECT_NEAR (actual_sample, quantized_expected, quantization_step)
            << "Channel " << ch << ", Sample " << i
            << ": dithered sample should be quantized to 16-bit levels";
        }
    }
}

TEST_F (GraphRendererTest, RenderDifferentBlockLengths)
{
  auto block_options = options_;
  block_options.block_length_ = units::samples (128);

  auto collection = create_simple_test_collection ();
  auto range = create_test_range (0, 256); // 2 blocks of 128

  EXPECT_CALL (*processable_, process_block (_, _)).Times (2);

  auto future = GraphRenderer::render_run_async (
    block_options, std::move (collection), range);

  auto result = future.result ();
  future.waitForFinished ();

  EXPECT_EQ (result.getNumChannels (), 2);
  EXPECT_EQ (result.getNumSamples (), 256);

  // Verify the sine wave samples are correct with different block lengths
  verify_sine_wave_samples (result);
}

TEST_F (GraphRendererTest, ResourceManagement)
{
  auto collection = create_simple_test_collection ();
  auto range = create_test_range (0, 256);

  // Expect resource management calls
  EXPECT_CALL (
    *processable_, prepare_for_processing (_, units::sample_rate (48000), 256))
    .Times (1);
  EXPECT_CALL (*processable_, release_resources ()).Times (1);

  auto future =
    GraphRenderer::render_run_async (options_, std::move (collection), range);

  auto result = future.result ();
  future.waitForFinished ();

  EXPECT_EQ (result.getNumChannels (), 2);
  EXPECT_EQ (result.getNumSamples (), 256);

  // Verify the sine wave samples are correct
  verify_sine_wave_samples (result);
}

TEST_F (GraphRendererTest, AudioPortCollection)
{
  auto collection = create_simple_test_collection ();
  auto range = create_test_range (0, 256);

  EXPECT_CALL (*processable_, process_block (_, _)).Times (1);

  auto future =
    GraphRenderer::render_run_async (options_, std::move (collection), range);

  auto result = future.result ();
  future.waitForFinished ();

  EXPECT_EQ (result.getNumChannels (), 2);
  EXPECT_EQ (result.getNumSamples (), 256);

  // Verify the actual sine wave samples using the helper method
  verify_sine_wave_samples (result);
}

TEST_F (GraphRendererTest, LargeRangeRendering)
{
  auto collection = create_simple_test_collection ();
  auto range = create_test_range (0, 48000); // 1 second at 48kHz

  // Should process in multiple blocks
  EXPECT_CALL (*processable_, process_block (_, _))
    .Times (static_cast<int> (std::ceil (48000.0 / 256.0))); // 188 blocks

  auto future =
    GraphRenderer::render_run_async (options_, std::move (collection), range);

  auto result = future.result ();
  future.waitForFinished ();

  EXPECT_EQ (result.getNumChannels (), 2);
  EXPECT_EQ (result.getNumSamples (), 48000);

  // Verify a few samples from the beginning to ensure correctness
  verify_sine_wave_samples (result, 48000.0f, 440.0f, 0.1f, 0, 16);
}

TEST_F (GraphRendererTest, DifferentBitDepths)
{
  std::vector<utils::audio::BitDepth> bit_depths = {
    utils::audio::BitDepth::BIT_DEPTH_8, utils::audio::BitDepth::BIT_DEPTH_16,
    utils::audio::BitDepth::BIT_DEPTH_24, utils::audio::BitDepth::BIT_DEPTH_32
  };

  for (auto bit_depth : bit_depths)
    {
      auto depth_options = options_;
      depth_options.bit_depth_ = bit_depth;

      auto collection = create_simple_test_collection ();
      auto range = create_test_range (0, 256);

      EXPECT_CALL (*processable_, process_block (_, _)).Times (1);

      auto future = GraphRenderer::render_run_async (
        depth_options, std::move (collection), range);

      auto result = future.result ();
      future.waitForFinished ();

      EXPECT_EQ (result.getNumChannels (), 2);
      EXPECT_EQ (result.getNumSamples (), 256);

      // Verify the sine wave samples are correct regardless of bit depth
      verify_sine_wave_samples (result);

      // Reset mock for next iteration
      Mock::VerifyAndClearExpectations (processable_.get ());
    }
}

TEST_F (GraphRendererTest, RenderWithCancellation)
{
  auto collection = create_simple_test_collection ();
  auto range = create_test_range (0, 512); // Multiple blocks

  auto future =
    GraphRenderer::render_run_async (options_, std::move (collection), range);

  // Cancel immediately
  future.cancel ();
  future.waitForFinished ();

  // Should be canceled
  EXPECT_TRUE (future.isCanceled ());
  EXPECT_FALSE (future.isValid ());
}

TEST_F (GraphRendererTest, RenderWithProgressReporting)
{
  auto collection = create_simple_test_collection ();
  auto range = create_test_range (0, 512); // Multiple blocks

  EXPECT_CALL (*processable_, process_block (_, _)).Times (2);

  auto future =
    GraphRenderer::render_run_async (options_, std::move (collection), range);

  // No progress yet
  EXPECT_EQ (future.progressValue (), 0);

  // Wait for completion
  auto result = future.result ();
  future.waitForFinished ();

  EXPECT_EQ (result.getNumChannels (), 2);
  EXPECT_EQ (result.getNumSamples (), 512);
  EXPECT_EQ (future.progressValue (), 512);

  // Verify the sine wave samples are correct
  verify_sine_wave_samples (result);
}
} // namespace zrythm::dsp
