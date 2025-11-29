// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/audio_sample_processor.h"
#include "dsp/metronome.h"
#include "dsp/tempo_map.h"

#include "unit/dsp/graph_helpers.h"
#include <gtest/gtest.h>

namespace zrythm::dsp
{

class MetronomeTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    port_registry_ = std::make_unique<dsp::PortRegistry> ();
    param_registry_ =
      std::make_unique<dsp::ProcessorParameterRegistry> (*port_registry_);
    tempo_map_ = std::make_unique<TempoMap> (units::sample_rate (44100.0));
    transport_ = std::make_unique<graph_test::MockTransport> ();

    // Create test samples
    emphasis_sample_ = juce::AudioSampleBuffer (2, 512);
    normal_sample_ = juce::AudioSampleBuffer (2, 512);

    // Fill samples with test data
    for (int i = 0; i < emphasis_sample_.getNumSamples (); ++i)
      {
        emphasis_sample_.setSample (0, i, 0.8f); // Left channel
        emphasis_sample_.setSample (1, i, 0.6f); // Right channel
      }

    for (int i = 0; i < normal_sample_.getNumSamples (); ++i)
      {
        normal_sample_.setSample (0, i, 0.4f); // Left channel
        normal_sample_.setSample (1, i, 0.3f); // Right channel
      }

    queued_samples_.clear ();
  }

  void TearDown () override { queued_samples_.clear (); }

  /**
   * @brief Creates a metronome instance with test samples
   */
  std::unique_ptr<Metronome> create_metronome (float initial_volume = 1.0f)
  {
    auto metronome = std::make_unique<Metronome> (
      dsp::ProcessorBase::ProcessorBaseDependencies{
        .port_registry_ = *port_registry_, .param_registry_ = *param_registry_ },
      *tempo_map_, emphasis_sample_, normal_sample_, true, initial_volume);
    metronome->set_queue_sample_callback (
      [this] (AudioSampleProcessor::PlayableSampleSingleChannel sample) {
        queued_samples_.push_back (sample);
      });
    return metronome;
  }

  /**
   * @brief Helper to verify sample was queued correctly
   */
  void verify_sample_queued (
    size_t     index,
    bool       emphasis,
    nframes_t  expected_offset,
    channels_t expected_channel,
    float      expected_volume = 1.0f)
  {
    ASSERT_LT (index, queued_samples_.size ());

    const auto &sample = queued_samples_[index];
    EXPECT_EQ (sample.channel_index_, expected_channel);
    EXPECT_EQ (sample.volume_, expected_volume);
    EXPECT_EQ (sample.start_offset_, expected_offset);

    const auto &expected_buffer = emphasis ? emphasis_sample_ : normal_sample_;
    EXPECT_EQ (sample.buf_.size (), expected_buffer.getNumSamples ());
    EXPECT_EQ (sample.buf_[0], expected_buffer.getSample (expected_channel, 0));
  }

  std::unique_ptr<dsp::PortRegistry>               port_registry_;
  std::unique_ptr<dsp::ProcessorParameterRegistry> param_registry_;
  std::unique_ptr<TempoMap>                        tempo_map_;
  std::unique_ptr<graph_test::MockTransport>       transport_;
  juce::AudioSampleBuffer                          emphasis_sample_;
  juce::AudioSampleBuffer                          normal_sample_;
  boost::container::
    static_vector<AudioSampleProcessor::PlayableSampleSingleChannel, 128>
      queued_samples_;
};

TEST_F (MetronomeTest, Construction)
{
  auto metronome = create_metronome ();

  EXPECT_EQ (metronome->volume (), 1.0f);
  EXPECT_TRUE (queued_samples_.empty ());
}

TEST_F (MetronomeTest, VolumeProperty)
{
  auto metronome = create_metronome ();

  // Test initial value
  EXPECT_EQ (metronome->volume (), 1.0f);

  // Test setting volume
  metronome->setVolume (0.5f);
  EXPECT_EQ (metronome->volume (), 0.5f);

  // Test volume change signal (QML interface)
  bool  signal_emitted = false;
  float new_volume = 0.0f;
  QObject::connect (
    metronome.get (), &Metronome::volumeChanged, metronome.get (),
    [&signal_emitted, &new_volume] (float volume) {
      signal_emitted = true;
      new_volume = volume;
    });

  metronome->setVolume (0.75f);
  EXPECT_TRUE (signal_emitted);
  EXPECT_EQ (new_volume, 0.75f);

  // Test no signal for same value
  signal_emitted = false;
  metronome->setVolume (0.75f);
  EXPECT_FALSE (signal_emitted);
}

TEST_F (MetronomeTest, EnabledProperty)
{
  auto metronome = create_metronome ();

  // Test initial value
  EXPECT_TRUE (metronome->enabled ());

  // Test setting enabled
  metronome->setEnabled (false);
  EXPECT_FALSE (metronome->enabled ());

  // Test enabled change signal (QML interface)
  bool signal_emitted = false;
  bool new_enabled = true;
  QObject::connect (
    metronome.get (), &Metronome::enabledChanged, metronome.get (),
    [&signal_emitted, &new_enabled] (bool enabled) {
      signal_emitted = true;
      new_enabled = enabled;
    });

  metronome->setEnabled (true);
  EXPECT_TRUE (signal_emitted);
  EXPECT_TRUE (new_enabled);

  // Test no signal for same value
  signal_emitted = false;
  metronome->setEnabled (true);
  EXPECT_FALSE (signal_emitted);
}

TEST_F (MetronomeTest, PrepareForProcessing)
{
  auto metronome = create_metronome ();

  // Should not throw
  EXPECT_NO_THROW (metronome->prepare_for_processing (
    nullptr, units::sample_rate (44100), 256));
}

TEST_F (MetronomeTest, BasicPlaybackNoTicks)
{
  auto metronome = create_metronome ();

  // Setup transport to not be rolling
  EXPECT_CALL (*transport_, get_play_state ())
    .WillRepeatedly (::testing::Return (dsp::ITransport::PlayState::Paused));

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 256
  };

  metronome->prepare_for_processing (nullptr, units::sample_rate (44100), 256);
  metronome->process_block (time_nfo, *transport_);

  // No samples should be queued when not rolling
  EXPECT_TRUE (queued_samples_.empty ());
}

TEST_F (MetronomeTest, BasicPlaybackWithTicks)
{
  auto metronome = create_metronome ();

  // Setup transport to be rolling
  EXPECT_CALL (*transport_, get_play_state ())
    .WillRepeatedly (::testing::Return (dsp::ITransport::PlayState::Rolling));

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 256
  };

  metronome->prepare_for_processing (nullptr, units::sample_rate (44100), 256);
  metronome->process_block (time_nfo, *transport_);

  // Should have queued an emphasis sample
  EXPECT_EQ (queued_samples_.size (), 2);
  verify_sample_queued (0, true, 0, 0);
  verify_sample_queued (1, true, 0, 1);
}

TEST_F (MetronomeTest, BarAndBeatTicks)
{
  auto metronome = create_metronome ();

  // Setup 4/4 time signature
  tempo_map_->add_time_signature_event (units::ticks (0), 4, 4);
  tempo_map_->add_tempo_event (
    units::ticks (0), 120.0, TempoMap::CurveType::Constant);

  // Setup transport
  EXPECT_CALL (*transport_, get_play_state ())
    .WillRepeatedly (::testing::Return (dsp::ITransport::PlayState::Rolling));

  // 120 BPM = 0.5 seconds per beat = 22050 samples per beat
  const nframes_t samples_per_beat = 22050;

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = samples_per_beat * 4 // 4 beats
  };

  metronome->prepare_for_processing (
    nullptr, units::sample_rate (44100), samples_per_beat * 4);
  metronome->process_block (time_nfo, *transport_);

  // Should have exactly 4 ticks: 1 bar tick + 3 beat ticks
  EXPECT_EQ (queued_samples_.size (), 8);

  // First tick should be emphasis (bar start)
  verify_sample_queued (0, true, 0, 0);
  verify_sample_queued (1, true, 0, 1);

  // Remaining ticks should be normal beats
  for (size_t i = 2; i < 8; i += 2)
    {
      const auto expected_offset = (i / 2) * samples_per_beat;
      verify_sample_queued (i, false, expected_offset, i % 2);
      verify_sample_queued (i + 1, false, expected_offset, (i + 1) % 2);
    }
}

TEST_F (MetronomeTest, LoopCrossing)
{
  auto metronome = create_metronome ();

  // Setup 4/4 time signature
  tempo_map_->add_time_signature_event (units::ticks (0), 4, 4);
  tempo_map_->add_tempo_event (
    units::ticks (0), 120.0, TempoMap::CurveType::Constant);

  // Setup transport with loop (0-1 seconds)
  EXPECT_CALL (*transport_, get_play_state ())
    .WillRepeatedly (::testing::Return (dsp::ITransport::PlayState::Rolling));
  EXPECT_CALL (*transport_, loop_enabled ())
    .WillRepeatedly (::testing::Return (true));
  EXPECT_CALL (*transport_, get_loop_range_positions ())
    .WillRepeatedly (
      ::testing::Return (
        std::make_pair (units::samples (0u), units::samples (44100u))));

  const nframes_t samples_per_beat = 22050; // 120 BPM

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 44100 - 11025, // Start just before loop
    .g_start_frame_w_offset_ = 44100 - 11025,
    .local_offset_ = 0,
    .nframes_ = samples_per_beat * 2 // 2 beats
  };

  metronome->prepare_for_processing (
    nullptr, units::sample_rate (44100), samples_per_beat * 2);
  metronome->process_block (time_nfo, *transport_);

  // Should handle loop crossing correctly

  // Check that we have the expected number of ticks (should be 2: one at loop
  // start, one at loop end)
  EXPECT_EQ (queued_samples_.size (), 4);

  // Verify tick positions are at the correct sample locations
  // Goes back to 0 after looping (bar tick)
  verify_sample_queued (0, true, 11025, 0);
  verify_sample_queued (1, true, 11025, 1);
  // Next beat after looping
  verify_sample_queued (2, false, (samples_per_beat * 2) - 11025, 0);
  verify_sample_queued (3, false, (samples_per_beat * 2) - 11025, 1);
}

TEST_F (MetronomeTest, CountinTicks)
{
  auto metronome = create_metronome ();

  // Setup 4/4 time signature
  tempo_map_->add_time_signature_event (units::ticks (0), 4, 4);
  tempo_map_->add_tempo_event (
    units::ticks (0), 120.0, TempoMap::CurveType::Constant);

  // Setup transport for countin
  EXPECT_CALL (*transport_, get_play_state ())
    .WillRepeatedly (::testing::Return (dsp::ITransport::PlayState::Rolling));
  EXPECT_CALL (*transport_, metronome_countin_frames_remaining ())
    .WillRepeatedly (
      ::testing::Return (units::samples (88200))); // 2 seconds (1 bar) countin

  const nframes_t samples_per_beat = 22050; // 120 BPM

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = samples_per_beat * 2
  };

  metronome->prepare_for_processing (
    nullptr, units::sample_rate (44100), samples_per_beat * 2);
  metronome->process_block (time_nfo, *transport_);

  // Should queue countin ticks
  EXPECT_EQ (queued_samples_.size (), 4);
  verify_sample_queued (0, true, 0, 0);
  verify_sample_queued (1, true, 0, 1);
  verify_sample_queued (2, false, samples_per_beat, 0);
  verify_sample_queued (3, false, samples_per_beat, 1);
}

TEST_F (MetronomeTest, VolumeAppliedToSamples)
{
  auto metronome = create_metronome (0.5f); // 50% volume

  // Setup 4/4 time signature
  tempo_map_->add_time_signature_event (units::ticks (0), 4, 4);
  tempo_map_->add_tempo_event (
    units::ticks (0), 120.0, TempoMap::CurveType::Constant);

  // Setup transport
  EXPECT_CALL (*transport_, get_play_state ())
    .WillRepeatedly (::testing::Return (dsp::ITransport::PlayState::Rolling));

  const nframes_t samples_per_beat = 22050;

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = samples_per_beat
  };

  metronome->prepare_for_processing (
    nullptr, units::sample_rate (44100), samples_per_beat);
  metronome->process_block (time_nfo, *transport_);

  // Should have queued samples with correct volume
  ASSERT_EQ (queued_samples_.size (), 2);
  verify_sample_queued (0, true, 0, 0, 0.5f);
  verify_sample_queued (1, true, 0, 1, 0.5f);
}

TEST_F (MetronomeTest, EmptyRangeHandling)
{
  auto metronome = create_metronome ();

  // Setup transport
  EXPECT_CALL (*transport_, get_play_state ())
    .WillRepeatedly (::testing::Return (dsp::ITransport::PlayState::Rolling));

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 0 // Empty range
  };

  metronome->prepare_for_processing (nullptr, units::sample_rate (44100), 256);
  metronome->process_block (time_nfo, *transport_);

  // Should handle empty range gracefully
  EXPECT_TRUE (queued_samples_.empty ());
}

TEST_F (MetronomeTest, DifferentTimeSignatures)
{
  auto metronome = create_metronome ();

  // Setup 3/4 time signature
  tempo_map_->add_time_signature_event (units::ticks (0), 3, 4);
  tempo_map_->add_tempo_event (
    units::ticks (0), 120.0, TempoMap::CurveType::Constant);

  // Setup transport
  EXPECT_CALL (*transport_, get_play_state ())
    .WillRepeatedly (::testing::Return (dsp::ITransport::PlayState::Rolling));

  const nframes_t samples_per_beat = 22050;

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = samples_per_beat * 3 // 3 beats in 3/4
  };

  metronome->prepare_for_processing (
    nullptr, units::sample_rate (44100), samples_per_beat * 3);
  metronome->process_block (time_nfo, *transport_);

  // Should have 3 ticks: 1 bar tick + 2 beat ticks
  EXPECT_EQ (queued_samples_.size (), 6);
}

TEST_F (MetronomeTest, HighTempo)
{
  auto metronome = create_metronome ();

  // Setup 4/4 time signature with high tempo
  tempo_map_->add_time_signature_event (units::ticks (0), 4, 4);
  tempo_map_->add_tempo_event (
    units::ticks (0), 240.0, TempoMap::CurveType::Constant); // 240 BPM

  // Setup transport
  EXPECT_CALL (*transport_, get_play_state ())
    .WillRepeatedly (::testing::Return (dsp::ITransport::PlayState::Rolling));

  const nframes_t samples_per_beat = 11025; // 240 BPM = 0.25s per beat

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = samples_per_beat * 4
  };

  metronome->prepare_for_processing (
    nullptr, units::sample_rate (44100), samples_per_beat * 4);
  metronome->process_block (time_nfo, *transport_);

  // Should have 4 ticks at high tempo
  EXPECT_EQ (queued_samples_.size (), 8);
  verify_sample_queued (0, true, 0, 0);
  verify_sample_queued (1, true, 0, 1);
  verify_sample_queued (2, false, samples_per_beat, 0);
  verify_sample_queued (3, false, samples_per_beat, 1);
  verify_sample_queued (4, false, samples_per_beat * 2, 0);
  verify_sample_queued (5, false, samples_per_beat * 2, 1);
  verify_sample_queued (6, false, samples_per_beat * 3, 0);
  verify_sample_queued (7, false, samples_per_beat * 3, 1);
}

TEST_F (MetronomeTest, EnabledFalsePreventsTicks)
{
  auto metronome = create_metronome ();

  // Disable metronome
  metronome->setEnabled (false);

  // Setup 4/4 time signature
  tempo_map_->add_time_signature_event (units::ticks (0), 4, 4);
  tempo_map_->add_tempo_event (
    units::ticks (0), 120.0, TempoMap::CurveType::Constant);

  // Setup transport
  EXPECT_CALL (*transport_, get_play_state ())
    .WillRepeatedly (::testing::Return (dsp::ITransport::PlayState::Rolling));

  const nframes_t samples_per_beat = 22050; // 120 BPM

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = samples_per_beat * 4
  };

  metronome->prepare_for_processing (
    nullptr, units::sample_rate (44100), samples_per_beat * 4);
  metronome->process_block (time_nfo, *transport_);

  // Should have no ticks when disabled
  EXPECT_TRUE (queued_samples_.empty ());
}

TEST_F (MetronomeTest, EnabledTrueAllowsTicks)
{
  auto metronome = create_metronome ();

  // Explicitly enable metronome (default is true)
  metronome->setEnabled (true);

  // Setup 4/4 time signature
  tempo_map_->add_time_signature_event (units::ticks (0), 4, 4);
  tempo_map_->add_tempo_event (
    units::ticks (0), 120.0, TempoMap::CurveType::Constant);

  // Setup transport
  EXPECT_CALL (*transport_, get_play_state ())
    .WillRepeatedly (::testing::Return (dsp::ITransport::PlayState::Rolling));

  const nframes_t samples_per_beat = 22050; // 120 BPM

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = samples_per_beat * 4
  };

  metronome->prepare_for_processing (
    nullptr, units::sample_rate (44100), samples_per_beat * 4);
  metronome->process_block (time_nfo, *transport_);

  // Should have ticks when enabled
  EXPECT_EQ (queued_samples_.size (), 8);
  verify_sample_queued (0, true, 0, 0);
  verify_sample_queued (1, true, 0, 1);
}

TEST_F (MetronomeTest, EnabledToggleDuringProcessing)
{
  auto metronome = create_metronome ();

  // Setup 4/4 time signature
  tempo_map_->add_time_signature_event (units::ticks (0), 4, 4);
  tempo_map_->add_tempo_event (
    units::ticks (0), 120.0, TempoMap::CurveType::Constant);

  // Setup transport
  EXPECT_CALL (*transport_, get_play_state ())
    .WillRepeatedly (::testing::Return (dsp::ITransport::PlayState::Rolling));

  const nframes_t samples_per_beat = 22050; // 120 BPM

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = samples_per_beat * 2
  };

  // First process with enabled=true (default)
  metronome->prepare_for_processing (
    nullptr, units::sample_rate (44100), samples_per_beat * 2);
  metronome->process_block (time_nfo, *transport_);

  // Should have ticks
  EXPECT_EQ (queued_samples_.size (), 4);

  // Clear samples
  queued_samples_.clear ();

  // Disable metronome
  metronome->setEnabled (false);

  // Process again
  metronome->process_block (time_nfo, *transport_);

  // Should have no ticks when disabled
  EXPECT_TRUE (queued_samples_.empty ());
}

TEST_F (MetronomeTest, DisabledDuringPlaybackClearsBuffer)
{
  auto metronome = create_metronome ();

  // Setup 4/4 time signature
  tempo_map_->add_time_signature_event (units::ticks (0), 4, 4);
  tempo_map_->add_tempo_event (
    units::ticks (0), 120.0, TempoMap::CurveType::Constant);

  // Setup transport
  EXPECT_CALL (*transport_, get_play_state ())
    .WillRepeatedly (::testing::Return (dsp::ITransport::PlayState::Rolling));

  constexpr nframes_t samples_per_beat = 22050; // 120 BPM

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = samples_per_beat
  };

  // First, enable metronome and process to queue samples
  metronome->prepare_for_processing (
    nullptr, units::sample_rate (44100), samples_per_beat);
  metronome->process_block (time_nfo, *transport_);

  // Should have queued samples
  EXPECT_EQ (queued_samples_.size (), 2);

  // Get the output port to check buffer contents
  auto out_port = metronome->get_output_audio_port_non_rt ();
  ASSERT_NE (out_port, nullptr);

  // Manually add some audio data to the output buffer to simulate
  // a partially processed sample that would be stuck if not cleared
  auto * out_l = out_port->buffers ()->getWritePointer (0);
  auto * out_r = out_port->buffers ()->getWritePointer (1);

  // Fill with non-zero data to simulate stuck audio
  std::fill_n (out_l, time_nfo.nframes_, 0.5f);
  std::fill_n (out_r, time_nfo.nframes_, 0.5f);

  // Now disable metronome
  metronome->setEnabled (false);

  // Process again - this should clear the buffer even though disabled
  metronome->process_block (time_nfo, *transport_);

  // Verify the buffer was cleared (should be all zeros)
  for (nframes_t i = 0; i < time_nfo.nframes_; ++i)
    {
      EXPECT_FLOAT_EQ (out_l[i], 0.0f)
        << "Left channel not cleared at sample " << i;
      EXPECT_FLOAT_EQ (out_r[i], 0.0f)
        << "Right channel not cleared at sample " << i;
    }

  // No new samples should be queued when disabled
  EXPECT_EQ (queued_samples_.size (), 2); // Still only the original samples
}

} // namespace zrythm::dsp
