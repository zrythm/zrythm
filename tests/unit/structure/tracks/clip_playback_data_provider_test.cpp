// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/midi_event.h"
#include "dsp/tempo_map.h"
#include "structure/arrangement/midi_note.h"
#include "structure/arrangement/midi_region.h"
#include "structure/tracks/clip_playback_data_provider.h"
#include "utils/types.h"

#include "gtest/gtest.h"
#include <juce_wrapper.h>

namespace zrythm::structure::tracks
{

class ClipPlaybackDataProviderTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    // Create a tempo map for testing
    tempo_map_ = std::make_unique<dsp::TempoMap> (units::sample_rate (44100));

    // Create the provider
    provider_ = std::make_unique<ClipPlaybackDataProvider> (*tempo_map_);

    // Create an object registry
    obj_registry_ = std::make_unique<arrangement::ArrangerObjectRegistry> ();

    // Create a file audio source registry
    file_audio_source_registry_ =
      std::make_unique<dsp::FileAudioSourceRegistry> ();
  }

  void TearDown () override
  {
    region_refs.clear (); // Clear references first
    provider_.reset ();
    tempo_map_.reset ();
    obj_registry_.reset ();
    file_audio_source_registry_.reset ();
  }

  // Helper function to create a MIDI region with notes at specific sample
  // positions
  arrangement::MidiRegion * create_midi_region_with_sampled_notes (
    double                                        start_pos_ticks,
    double                                        end_pos_ticks,
    const std::vector<std::tuple<int, int, int>> &notes) // pitch, velocity,
                                                         // position in samples
  {
    // Create a MIDI region
    auto region_ref = obj_registry_->create_object<arrangement::MidiRegion> (
      *tempo_map_, *obj_registry_, *file_audio_source_registry_);
    auto region = region_ref.get_object_as<arrangement::MidiRegion> ();

    // Set the region's position
    region->position ()->setTicks (start_pos_ticks);
    region->bounds ()->length ()->setTicks (end_pos_ticks - start_pos_ticks);

    // Create MIDI notes
    for (const auto &[pitch, velocity, sample_pos] : notes)
      {
        auto note_ref =
          obj_registry_->create_object<arrangement::MidiNote> (*tempo_map_);
        auto midi_note = note_ref.get_object_as<arrangement::MidiNote> ();

        // Set the note's properties
        midi_note->setPitch (pitch);
        midi_note->setVelocity (velocity);
        // Convert sample position to ticks
        midi_note->position ()->setTicks (
          tempo_map_->samples_to_tick (units::samples (sample_pos))
            .in (units::ticks));
        midi_note->bounds ()->length ()->setTicks (50.0); // Note duration

        // Add the note to the region
        region->add_object (note_ref);
      }

    // Keep a reference to the region
    region_refs.push_back (std::move (region_ref));

    return region;
  }

  // Simplified helper for creating a region with a single note at position 0
  arrangement::MidiRegion *
  create_simple_region (midi_byte_t pitch = 60, midi_byte_t velocity = 64)
  {
    return create_midi_region_with_sampled_notes (
      0.0, 400.0,
      {
        { pitch, velocity, 0 }
    });
  }

  // Simplified helper for creating a region with multiple notes at position 0
  arrangement::MidiRegion * create_multi_note_region (
    const std::vector<std::pair<int, int>> &notes) // pitch, velocity pairs
  {
    std::vector<std::tuple<int, int, int>> sampled_notes;
    for (const auto &[pitch, velocity] : notes)
      {
        sampled_notes.emplace_back (pitch, velocity, 0);
      }
    return create_midi_region_with_sampled_notes (0.0, 400.0, sampled_notes);
  }

  // Helper function to find events with specific pitch
  std::vector<midi_time_t>
  find_event_timestamps (const dsp::MidiEventVector &events, midi_byte_t pitch)
  {
    std::vector<midi_time_t> timestamps;
    for (const auto &event : events)
      {
        if (
          (event.raw_buffer_[0] & 0xF0) == 0x90 // Note on
          && event.raw_buffer_[1] == pitch)     // Matching pitch
          {
            timestamps.push_back (event.time_);
          }
      }
    return timestamps;
  }

  // Helper function to check for timestamp overflow
  bool has_timestamp_overflow (const dsp::MidiEventVector &events)
  {
    return std::ranges::any_of (events, [] (const auto &ev) {
      // Check if timestamp is greater than a reasonable maximum
      // (e.g., 1 second of audio at 44100Hz = 44100 samples)
      return ev.time_ > 44100;
    });
  }

  // Helper to create EngineProcessTimeInfo
  static EngineProcessTimeInfo create_time_info (
    unsigned_frame_t g_start_frame,
    unsigned_frame_t g_start_frame_w_offset,
    unsigned_frame_t local_offset,
    nframes_t        nframes)
  {
    return {
      .g_start_frame_ = g_start_frame,
      .g_start_frame_w_offset_ = g_start_frame_w_offset,
      .local_offset_ = static_cast<nframes_t> (local_offset),
      .nframes_ = nframes
    };
  }

  // Helper function to create a simple audio region
  arrangement::AudioRegion * create_simple_audio_region (float gain = 1.0f)
  {
    // Create a simple audio buffer with test data
    auto sample_buffer = std::make_unique<utils::audio::AudioBuffer> (2, 1024);

    // Fill with test data: left channel = 0.5, right channel = -0.5
    for (int i = 0; i < sample_buffer->getNumSamples (); i++)
      {
        sample_buffer->setSample (0, i, 0.5f * gain);
        sample_buffer->setSample (1, i, -0.5f * gain);
      }

    // Create a mock audio source
    auto source_ref = file_audio_source_registry_->create_object<
      dsp::FileAudioSource> (
      *sample_buffer, utils::audio::BitDepth::BIT_DEPTH_32, 44100, 120.f,
      u8"DummySource");
    auto audio_source_object_ref =
      obj_registry_->create_object<arrangement::AudioSourceObject> (
        *tempo_map_, *file_audio_source_registry_, source_ref);

    // Create the audio region
    auto region_ref = obj_registry_->create_object<arrangement::AudioRegion> (
      *tempo_map_, *obj_registry_, *file_audio_source_registry_,
      [] () { return true; });
    auto region = region_ref.get_object_as<arrangement::AudioRegion> ();

    region->set_source (audio_source_object_ref);
    region->setGain (gain);

    // Set the region's position
    region->position ()->setTicks (0.0);
    region->bounds ()->length ()->setTicks (400.0);

    // Keep a reference to the region
    region_refs.push_back (std::move (region_ref));

    return region;
  }

  // Helper function to create a short audio region
  arrangement::AudioRegion *
  create_short_audio_region (int num_samples, float gain = 1.0f)
  {
    // Create a short audio buffer with test data
    auto sample_buffer =
      std::make_unique<utils::audio::AudioBuffer> (2, num_samples);

    // Fill with test data: left channel = 0.5, right channel = -0.5
    for (int i = 0; i < sample_buffer->getNumSamples (); i++)
      {
        sample_buffer->setSample (0, i, 0.5f * gain);
        sample_buffer->setSample (1, i, -0.5f * gain);
      }

    // Create a mock audio source
    auto source_ref = file_audio_source_registry_->create_object<
      dsp::FileAudioSource> (
      *sample_buffer, utils::audio::BitDepth::BIT_DEPTH_32, 44100, 120.f,
      u8"ShortSource");
    auto audio_source_object_ref =
      obj_registry_->create_object<arrangement::AudioSourceObject> (
        *tempo_map_, *file_audio_source_registry_, source_ref);

    // Create the audio region
    auto region_ref = obj_registry_->create_object<arrangement::AudioRegion> (
      *tempo_map_, *obj_registry_, *file_audio_source_registry_,
      [] () { return true; });
    auto region = region_ref.get_object_as<arrangement::AudioRegion> ();

    region->set_source (audio_source_object_ref);
    region->setGain (gain);

    // Set the region's position
    region->position ()->setTicks (0.0);
    region->bounds ()->length ()->setTicks (1.0); // Very short region

    // Keep a reference to the region
    region_refs.push_back (std::move (region_ref));

    return region;
  }

  // Helper function to create a sine wave audio region
  arrangement::AudioRegion *
  create_sine_wave_audio_region (int num_samples, float gain = 1.0f)
  {
    // Create a sine wave audio buffer
    auto sample_buffer =
      std::make_unique<utils::audio::AudioBuffer> (2, num_samples);
    const double sample_rate = 44100.0;
    const double frequency_hz = 441.0; // 441 Hz divides evenly into 44100 Hz
    const double phase_increment = 2.0 * M_PI * frequency_hz / sample_rate;
    const double phase_offset = M_PI / 4.0; // Start at 45 degrees to avoid zero

    for (int i = 0; i < num_samples; ++i)
      {
        const double phase = phase_offset + i * phase_increment;
        const float sample_value = static_cast<float> (std::sin (phase)) * gain;

        // Left channel: sine wave
        sample_buffer->setSample (0, i, sample_value);
        // Right channel: cosine wave (90 degrees phase shift)
        sample_buffer->setSample (
          1, i, static_cast<float> (std::cos (phase)) * gain);
      }

    // Create a mock audio source
    auto source_ref = file_audio_source_registry_->create_object<
      dsp::FileAudioSource> (
      *sample_buffer, utils::audio::BitDepth::BIT_DEPTH_32, 44100, 120.f,
      u8"SineSource");
    auto audio_source_object_ref =
      obj_registry_->create_object<arrangement::AudioSourceObject> (
        *tempo_map_, *file_audio_source_registry_, source_ref);

    // Create the audio region
    auto region_ref = obj_registry_->create_object<arrangement::AudioRegion> (
      *tempo_map_, *obj_registry_, *file_audio_source_registry_,
      [] () { return true; });
    auto region = region_ref.get_object_as<arrangement::AudioRegion> ();

    region->set_source (audio_source_object_ref);
    region->setGain (gain);

    // Set the region's position
    region->position ()->setTicks (0.0);
    region->bounds ()->length ()->setTicks (
      tempo_map_->samples_to_tick (units::samples (num_samples))
        .in (units::ticks));

    // Keep a reference to the region
    region_refs.push_back (std::move (region_ref));

    return region;
  }

  std::unique_ptr<ClipPlaybackDataProvider>            provider_;
  std::unique_ptr<dsp::TempoMap>                       tempo_map_;
  std::unique_ptr<arrangement::ArrangerObjectRegistry> obj_registry_;
  std::unique_ptr<dsp::FileAudioSourceRegistry> file_audio_source_registry_;
  std::vector<arrangement::ArrangerObjectUuidReference>
    region_refs; // Keep references
};

TEST_F (ClipPlaybackDataProviderTest, InitialState)
{
  // Provider should start with no events
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = create_time_info (0, 0, 0, 256);

  provider_->process_midi_events (time_info, output_buffer);
  EXPECT_EQ (output_buffer.size (), 0);
}

TEST_F (ClipPlaybackDataProviderTest, GenerateEventsWithMidiRegion)
{
  // Create a MIDI region
  auto region = create_simple_region ();

  // Generate events for the region
  provider_->generate_midi_events (
    *region, structure::tracks::ClipQuantizeOption::Immediate);

  // Test processing events that should include the note
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = create_time_info (0, 0, 0, 256);

  provider_->process_midi_events (time_info, output_buffer);

  // Should have at least one event (the note on)
  EXPECT_GE (output_buffer.size (), 1);

  // Check that the event timestamp is within the expected range
  if (!output_buffer.empty ())
    {
      const auto &event = output_buffer.front ();
      EXPECT_GE (event.time_, 0);
      EXPECT_LT (event.time_, time_info.nframes_);

      // Verify it's a note on event with the correct pitch
      EXPECT_EQ (event.raw_buffer_[0] & 0xF0, 0x90); // Note on
      EXPECT_EQ (event.raw_buffer_[1], 60);          // Pitch
    }
}

TEST_F (ClipPlaybackDataProviderTest, ClearEvents)
{
  // Create a MIDI region
  auto region = create_simple_region ();

  // Generate events for the region
  provider_->generate_midi_events (
    *region, structure::tracks::ClipQuantizeOption::Immediate);

  // Test processing events - should have events
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = create_time_info (0, 0, 0, 256);

  provider_->process_midi_events (time_info, output_buffer);
  EXPECT_GT (output_buffer.size (), 0);

  // Clear events
  provider_->clear_events ();

  // Test processing events again - should have no events
  output_buffer.clear ();
  provider_->process_midi_events (time_info, output_buffer);
  EXPECT_EQ (output_buffer.size (), 0);
}

TEST_F (ClipPlaybackDataProviderTest, QuantizeOptions)
{
  // Create a MIDI region
  auto region = create_simple_region ();

  // Test different quantize options
  std::vector<ClipQuantizeOption> quantize_options = {
    structure::tracks::ClipQuantizeOption::Immediate,
    structure::tracks::ClipQuantizeOption::NextBar,
    structure::tracks::ClipQuantizeOption::NextBeat,
    structure::tracks::ClipQuantizeOption::NextQuarterBeat,
    structure::tracks::ClipQuantizeOption::NextEighthBeat,
    structure::tracks::ClipQuantizeOption::NextSixteenthBeat,
    structure::tracks::ClipQuantizeOption::NextThirtySecondBeat
  };

  for (const auto &quantize_option : quantize_options)
    {
      // Generate events with the quantize option
      provider_->generate_midi_events (*region, quantize_option);

      // Test processing events
      dsp::MidiEventVector  output_buffer;
      EngineProcessTimeInfo time_info = create_time_info (0, 0, 0, 256);

      provider_->process_midi_events (time_info, output_buffer);

      // Should have events (quantize option doesn't affect the presence of
      // events, just when they start playing)
      if (quantize_option == structure::tracks::ClipQuantizeOption::Immediate)
        {
          EXPECT_GT (output_buffer.size (), 0);
        }
    }
}

TEST_F (ClipPlaybackDataProviderTest, MultipleNotesInRegion)
{
  // Create a MIDI region with multiple notes
  std::vector<std::pair<int, int>> notes = {
    { 60, 64  },
    { 64, 80  },
    { 67, 100 }
  };
  auto region = create_multi_note_region (notes);

  // Generate events for the region
  provider_->generate_midi_events (
    *region, structure::tracks::ClipQuantizeOption::Immediate);

  // Test processing events that should include all notes
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = create_time_info (0, 0, 0, 256);

  provider_->process_midi_events (time_info, output_buffer);

  // Should have at least 3 events (the note ons)
  EXPECT_GE (output_buffer.size (), 3);

  // Check that we have the expected pitches
  std::vector<int> found_pitches;
  for (const auto &event : output_buffer)
    {
      if ((event.raw_buffer_[0] & 0xF0) == 0x90) // Note on
        {
          found_pitches.push_back (event.raw_buffer_[1]);
        }
    }

  EXPECT_TRUE (std::ranges::contains (found_pitches, 60));
  EXPECT_TRUE (std::ranges::contains (found_pitches, 64));
  EXPECT_TRUE (std::ranges::contains (found_pitches, 67));
}

TEST_F (ClipPlaybackDataProviderTest, PreciseLoopingBehavior)
{
  // Create a very short MIDI region that will loop (approximately 23 samples)
  std::vector<std::tuple<int, int, int>> notes = {
    { 60, 64, 0 }  // Note at position 0
  };
  auto region = create_midi_region_with_sampled_notes (0.0, 1.0, notes);

  // Generate events for the region
  provider_->generate_midi_events (
    *region, structure::tracks::ClipQuantizeOption::Immediate);

  // Process multiple buffers to trigger looping
  std::vector<std::vector<midi_time_t>> loop_events;
  dsp::MidiEventVector                  output_buffer;

  // Process first buffer
  EngineProcessTimeInfo time_info1 = create_time_info (0, 0, 0, 1024);
  provider_->process_midi_events (time_info1, output_buffer);
  auto first_loop_events = find_event_timestamps (output_buffer, 60);
  loop_events.push_back (first_loop_events);

  // Process second buffer that should include the loop
  EngineProcessTimeInfo time_info2 = create_time_info (1024, 1024, 0, 2048);
  output_buffer.clear ();
  provider_->process_midi_events (time_info2, output_buffer);
  auto second_loop_events = find_event_timestamps (output_buffer, 60);
  loop_events.push_back (second_loop_events);

  // Process third buffer that should include another loop
  EngineProcessTimeInfo time_info3 = create_time_info (3072, 3072, 0, 2048);
  output_buffer.clear ();
  provider_->process_midi_events (time_info3, output_buffer);
  auto third_loop_events = find_event_timestamps (output_buffer, 60);
  loop_events.push_back (third_loop_events);

  // Verify that we have events in each buffer
  EXPECT_GT (first_loop_events.size (), 0);
  EXPECT_GT (second_loop_events.size (), 0);
  EXPECT_GT (third_loop_events.size (), 0);

  // Verify that the events in subsequent loops are at the correct positions
  // The second loop should have events at positions relative to the loop point
  // (accounting for buffer offset)
  if (!second_loop_events.empty ())
    {
      // The first event in the second loop should be approximately at the loop
      // point (accounting for buffer offset)
      EXPECT_LT (second_loop_events[0], time_info2.nframes_);
    }

  if (!third_loop_events.empty ())
    {
      // The first event in the third loop should also be at the correct position
      EXPECT_LT (third_loop_events[0], time_info3.nframes_);
    }

  // Check for timestamp overflow
  EXPECT_FALSE (has_timestamp_overflow (output_buffer));
}

TEST_F (ClipPlaybackDataProviderTest, PreciseEventTiming)
{
  // Create a MIDI region with notes at specific sample positions
  std::vector<std::tuple<int, int, int>> notes = {
    { 60, 64,  0    }, // Note at position 0
    { 62, 80,  1150 }, // Note at position ~1150 samples
    { 64, 100, 2295 }  // Note at position ~2295 samples
  };
  auto region = create_midi_region_with_sampled_notes (0.0, 400.0, notes);

  // Generate events for the region
  provider_->generate_midi_events (
    *region, structure::tracks::ClipQuantizeOption::Immediate);

  // Process events
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = create_time_info (0, 0, 0, 4096);

  provider_->process_midi_events (time_info, output_buffer);

  // Find events for each pitch
  auto pitch_60_events = find_event_timestamps (output_buffer, 60);
  auto pitch_62_events = find_event_timestamps (output_buffer, 62);
  auto pitch_64_events = find_event_timestamps (output_buffer, 64);

  // Verify that we have events for each pitch
  EXPECT_GT (pitch_60_events.size (), 0);
  EXPECT_GT (pitch_62_events.size (), 0);
  EXPECT_GT (pitch_64_events.size (), 0);

  // Verify that the events are at the correct positions
  // Note at position 0 should be at timestamp 0
  EXPECT_EQ (pitch_60_events[0], 0);

  // Note at position 1150 samples should be at the correct position
  EXPECT_EQ (pitch_62_events[0], 1150);

  // Note at position 2295 samples should be at the correct position
  EXPECT_EQ (pitch_64_events[0], 2295);

  // Verify that all timestamps are within the buffer range
  for (const auto &event : output_buffer)
    {
      EXPECT_GE (event.time_, 0);
      EXPECT_LT (event.time_, time_info.nframes_);
    }

  // Check for timestamp overflow
  EXPECT_FALSE (has_timestamp_overflow (output_buffer));
}

TEST_F (ClipPlaybackDataProviderTest, BarBoundaryQuantization)
{
  // Create a MIDI region
  auto region = create_simple_region ();

  // Generate events with NextBar quantization
  provider_->generate_midi_events (
    *region, structure::tracks::ClipQuantizeOption::NextBar);

  // Test processing events that should NOT include the note initially
  // because we're quantizing to the next bar
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = create_time_info (64, 64, 0, 256);

  provider_->process_midi_events (time_info, output_buffer);

  // Should have no events initially because we're waiting for the next bar
  EXPECT_EQ (output_buffer.size (), 0);

  // Process events at the next bar boundary (1 bar = 4 beats = 3840 ticks)
  const auto bar_start_samples =
    tempo_map_->tick_to_samples_rounded (units::ticks (3840.0));
  EngineProcessTimeInfo bar_time_info = create_time_info (
    static_cast<unsigned_frame_t> (bar_start_samples.in (units::sample)),
    static_cast<unsigned_frame_t> (bar_start_samples.in (units::sample)), 0,
    256);

  provider_->process_midi_events (bar_time_info, output_buffer);

  // Should now have events because we're at the next bar
  EXPECT_GT (output_buffer.size (), 0);
}

TEST_F (ClipPlaybackDataProviderTest, BeatBoundaryQuantization)
{
  // Create a MIDI region
  auto region = create_simple_region ();

  // Generate events with NextBeat quantization
  provider_->generate_midi_events (
    *region, structure::tracks::ClipQuantizeOption::NextBeat);

  // Test processing events that should NOT include the note initially
  // because we're quantizing to the next beat
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = create_time_info (64, 64, 0, 256);

  provider_->process_midi_events (time_info, output_buffer);

  // Should have no events initially because we're waiting for the next beat
  EXPECT_EQ (output_buffer.size (), 0);

  // Process events at the next beat boundary (1 beat = 960 ticks)
  const auto beat_start_samples =
    tempo_map_->tick_to_samples_rounded (units::ticks (960.0));
  EngineProcessTimeInfo beat_time_info = create_time_info (
    static_cast<unsigned_frame_t> (beat_start_samples.in (units::sample)),
    static_cast<unsigned_frame_t> (beat_start_samples.in (units::sample)), 0,
    256);

  provider_->process_midi_events (beat_time_info, output_buffer);

  // Should now have events because we're at the next beat
  EXPECT_GT (output_buffer.size (), 0);
}

class ClipPlaybackDataProviderBufferSizeTest
    : public ClipPlaybackDataProviderTest,
      public ::testing::WithParamInterface<nframes_t>
{
};

TEST_P (
  ClipPlaybackDataProviderBufferSizeTest,
  ProcessEventsWithDifferentBufferSizes)
{
  const auto buffer_size = GetParam ();

  // Create a MIDI region with notes spanning multiple buffer sizes
  std::vector<std::pair<int, int>> notes = {
    { 60, 64  }, // Note at position 0
    { 62, 80  }, // Note at position 0
    { 64, 100 }  // Note at position 0
  };
  auto region = create_multi_note_region (notes);

  // Generate events for the region
  provider_->generate_midi_events (
    *region, structure::tracks::ClipQuantizeOption::Immediate);

  // Test processing events with the specific buffer size
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = create_time_info (0, 0, 0, buffer_size);

  provider_->process_midi_events (time_info, output_buffer);

  // Should have events regardless of buffer size
  EXPECT_GT (output_buffer.size (), 0);

  // Verify that event timestamps are within the buffer range
  for (const auto &event : output_buffer)
    {
      EXPECT_GE (event.time_, 0);
      EXPECT_LT (event.time_, buffer_size);
    }

  // Check that we have the expected pitches
  std::vector<int> found_pitches;
  for (const auto &event : output_buffer)
    {
      if ((event.raw_buffer_[0] & 0xF0) == 0x90) // Note on
        {
          found_pitches.push_back (event.raw_buffer_[1]);
        }
    }

  EXPECT_TRUE (std::ranges::contains (found_pitches, 60));
  EXPECT_TRUE (std::ranges::contains (found_pitches, 62));
  EXPECT_TRUE (std::ranges::contains (found_pitches, 64));
}

INSTANTIATE_TEST_SUITE_P (
  ClipPlaybackDataProviderTest,
  ClipPlaybackDataProviderBufferSizeTest,
  ::testing::Values (64, 128, 256, 512, 1024, 2048));

TEST_F (ClipPlaybackDataProviderTest, ProcessEventsWithOffset)
{
  // Create a MIDI region with a note at position 0
  auto region = create_simple_region ();

  // Generate events for the region
  provider_->generate_midi_events (
    *region, structure::tracks::ClipQuantizeOption::Immediate);

  // Test processing events with a global offset
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = create_time_info (10000, 10100, 100, 256);

  provider_->process_midi_events (time_info, output_buffer);

  // Should have events since we're using immediate quantization
  EXPECT_GT (output_buffer.size (), 0);

  // Find the note on event
  auto pitch_60_events = find_event_timestamps (output_buffer, 60);
  EXPECT_GT (pitch_60_events.size (), 0);

  // The event should be offset by the difference between g_start_frame_w_offset
  // and g_start_frame_, which is 100 samples
  EXPECT_EQ (pitch_60_events[0], 100);

  // Check for timestamp overflow
  EXPECT_FALSE (has_timestamp_overflow (output_buffer));
}

TEST_F (ClipPlaybackDataProviderTest, QuantizationTimingAccuracy)
{
  // Create a MIDI region
  auto region = create_simple_region ();

  // Generate events with NextBeat quantization
  provider_->generate_midi_events (
    *region, structure::tracks::ClipQuantizeOption::NextBeat);

  // Start processing from a position that's not on a beat boundary
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = create_time_info (64, 64, 0, 256);

  provider_->process_midi_events (time_info, output_buffer);

  // Should have no events initially because we're quantizing to the next beat
  EXPECT_EQ (output_buffer.size (), 0);

  // Process events at the next beat boundary (1 beat = 960 ticks)
  const auto beat_start_samples =
    tempo_map_->tick_to_samples_rounded (units::ticks (960.0));
  EngineProcessTimeInfo beat_time_info = create_time_info (
    static_cast<unsigned_frame_t> (beat_start_samples.in (units::sample)),
    static_cast<unsigned_frame_t> (beat_start_samples.in (units::sample)), 0,
    256);

  output_buffer.clear ();
  provider_->process_midi_events (beat_time_info, output_buffer);

  // Should now have events because we're at the next beat
  EXPECT_GT (output_buffer.size (), 0);

  // Find the note on event
  auto pitch_60_events = find_event_timestamps (output_buffer, 60);
  EXPECT_GT (pitch_60_events.size (), 0);

  // The event should be at timestamp 0 since we're starting exactly at the beat
  // boundary
  EXPECT_EQ (pitch_60_events[0], 0);

  // Check for timestamp overflow
  EXPECT_FALSE (has_timestamp_overflow (output_buffer));
}

TEST_F (ClipPlaybackDataProviderTest, StateManagementAcrossBuffers)
{
  // Create a MIDI region with notes at different positions
  std::vector<std::tuple<int, int, int>> notes = {
    { 60, 64, 0    }, // Note at position 0
    { 62, 80, 1150 }  // Note at position ~1150 samples
  };
  auto region = create_midi_region_with_sampled_notes (0.0, 400.0, notes);

  // Generate events for the region
  provider_->generate_midi_events (
    *region, structure::tracks::ClipQuantizeOption::Immediate);

  // Process first buffer
  dsp::MidiEventVector  output_buffer1;
  EngineProcessTimeInfo time_info1 = create_time_info (0, 0, 0, 128);

  provider_->process_midi_events (time_info1, output_buffer1);

  // Should have the first note but not the second
  auto pitch_60_events1 = find_event_timestamps (output_buffer1, 60);
  auto pitch_62_events1 = find_event_timestamps (output_buffer1, 62);
  EXPECT_GT (pitch_60_events1.size (), 0);
  EXPECT_EQ (pitch_62_events1.size (), 0);

  // Process second buffer continuing from where we left off
  dsp::MidiEventVector  output_buffer2;
  EngineProcessTimeInfo time_info2 = create_time_info (128, 128, 0, 2048);

  provider_->process_midi_events (time_info2, output_buffer2);

  // Should have the second note
  auto pitch_62_events2 = find_event_timestamps (output_buffer2, 62);
  EXPECT_GT (pitch_62_events2.size (), 0);

  // The second note should be at the correct position relative to the buffer
  EXPECT_EQ (pitch_62_events2[0], 1150 - 128);

  // Check for timestamp overflow
  EXPECT_FALSE (has_timestamp_overflow (output_buffer2));
}

TEST_F (ClipPlaybackDataProviderTest, BasicFunctionality)
{
  // Test that the provider can be constructed
  EXPECT_NE (provider_, nullptr);

  // Test that process_midi_events can be called without crashing
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = create_time_info (0, 0, 0, 256);

  provider_->process_midi_events (time_info, output_buffer);
  EXPECT_EQ (output_buffer.size (), 0);

  // Test that generate_midi_events can be called without crashing
  auto region = create_simple_region ();
  provider_->generate_midi_events (
    *region, structure::tracks::ClipQuantizeOption::Immediate);

  // Process again to ensure no crash
  provider_->process_midi_events (time_info, output_buffer);
  EXPECT_GT (output_buffer.size (), 0);
}

TEST_F (ClipPlaybackDataProviderTest, GenerateEventsWithMutedNote)
{
  // Create a MIDI region
  auto region = create_simple_region ();

  // Mute the note in our region
  auto note_view = region->get_children_view ();
  if (!note_view.empty ())
    {
      note_view[0]->mute ()->setMuted (true);
    }

  // Generate events for the region
  provider_->generate_midi_events (
    *region, structure::tracks::ClipQuantizeOption::Immediate);

  // Test processing events
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = create_time_info (0, 0, 0, 256);

  provider_->process_midi_events (time_info, output_buffer);

  // Should have no events since the only note is muted
  EXPECT_EQ (output_buffer.size (), 0);
}

TEST_F (ClipPlaybackDataProviderTest, LoopingEdgeCases)
{
  // Create a very short MIDI region (23 samples) to test edge cases
  std::vector<std::tuple<int, int, int>> notes = {
    { 60, 64, 0 }  // Note at position 0
  };
  auto region = create_midi_region_with_sampled_notes (0.0, 1.0, notes);

  // Generate events for the region
  provider_->generate_midi_events (
    *region, structure::tracks::ClipQuantizeOption::Immediate);

  // Process multiple buffers to verify looping
  std::vector<std::vector<midi_time_t>> loop_events;
  dsp::MidiEventVector                  output_buffer;

  // Process several buffers
  for (int i = 0; i < 5; ++i)
    {
      EngineProcessTimeInfo time_info =
        create_time_info (i * 256, i * 256, 0, 256);

      output_buffer.clear ();
      provider_->process_midi_events (time_info, output_buffer);
      auto events = find_event_timestamps (output_buffer, 60);
      loop_events.push_back (events);
    }

  // Should have events in each buffer due to looping
  for (const auto &events : loop_events)
    {
      EXPECT_GT (events.size (), 0);
    }

  // Verify that events are at the correct positions in each buffer
  // The first buffer should have the event at position 0
  EXPECT_EQ (loop_events[0][0], 0);

  // Subsequent buffers should have events at positions that depend on where
  // the loop point falls within the buffer
  for (size_t i = 1; i < loop_events.size (); ++i)
    {
      // Events should be within the buffer range
      EXPECT_LT (loop_events[i][0], 256);
    }

  // Check for timestamp overflow
  EXPECT_FALSE (has_timestamp_overflow (output_buffer));
}

TEST_F (ClipPlaybackDataProviderTest, EventsOnBufferBoundaries)
{
  // Create a MIDI region with a note exactly at a buffer boundary
  std::vector<std::tuple<int, int, int>> notes = {
    { 60, 64, 128 }  // Note exactly at buffer boundary
  };
  auto region = create_midi_region_with_sampled_notes (0.0, 400.0, notes);

  // Generate events for the region
  provider_->generate_midi_events (
    *region, structure::tracks::ClipQuantizeOption::Immediate);

  // Process first buffer
  dsp::MidiEventVector  output_buffer1;
  EngineProcessTimeInfo time_info1 = create_time_info (0, 0, 0, 128);

  provider_->process_midi_events (time_info1, output_buffer1);

  // Should have no events in the first buffer since the note is at the boundary
  auto pitch_60_events1 = find_event_timestamps (output_buffer1, 60);
  EXPECT_EQ (pitch_60_events1.size (), 0);

  // Process second buffer starting at the boundary
  dsp::MidiEventVector  output_buffer2;
  EngineProcessTimeInfo time_info2 = create_time_info (128, 128, 0, 128);

  provider_->process_midi_events (time_info2, output_buffer2);

  // Should have the event in the second buffer at position 0
  auto pitch_60_events2 = find_event_timestamps (output_buffer2, 60);
  EXPECT_GT (pitch_60_events2.size (), 0);
  EXPECT_EQ (pitch_60_events2[0], 0);

  // Check for timestamp overflow
  EXPECT_FALSE (has_timestamp_overflow (output_buffer2));
}

TEST_F (ClipPlaybackDataProviderTest, BackwardPlayheadMovement)
{
  // Create a MIDI region with notes at different positions
  std::vector<std::tuple<int, int, int>> notes = {
    { 60, 64, 0    }, // Note at position 0
    { 62, 80, 1150 }  // Note at position ~1150 samples
  };
  auto region = create_midi_region_with_sampled_notes (0.0, 400.0, notes);

  // Generate events for the region
  provider_->generate_midi_events (
    *region, structure::tracks::ClipQuantizeOption::Immediate);

  // Process first buffer starting at frame 0
  dsp::MidiEventVector  output_buffer1;
  EngineProcessTimeInfo time_info1 = create_time_info (0, 0, 0, 256);

  provider_->process_midi_events (time_info1, output_buffer1);

  // Should have the first note
  auto pitch_60_events1 = find_event_timestamps (output_buffer1, 60);
  EXPECT_GT (pitch_60_events1.size (), 0);

  // Process second buffer continuing forward
  dsp::MidiEventVector  output_buffer2;
  EngineProcessTimeInfo time_info2 = create_time_info (256, 256, 0, 256);

  provider_->process_midi_events (time_info2, output_buffer2);

  // Should NOT have the second note yet because it's at position 1150
  // and this buffer only covers positions 256-511
  auto pitch_62_events2 = find_event_timestamps (output_buffer2, 62);
  EXPECT_EQ (pitch_62_events2.size (), 0);

  // Now simulate transport looping back to frame 0
  // This is the critical test - when playhead moves backwards
  dsp::MidiEventVector  output_buffer3;
  EngineProcessTimeInfo time_info3 = create_time_info (0, 0, 0, 256);

  provider_->process_midi_events (time_info3, output_buffer3);

  // Should have the first note again since we looped back
  auto pitch_60_events3 = find_event_timestamps (output_buffer3, 60);
  EXPECT_GT (pitch_60_events3.size (), 0);

  // The event should be at timestamp 0 since we're starting at frame 0
  EXPECT_EQ (pitch_60_events3[0], 0);

  // Check for timestamp overflow
  EXPECT_FALSE (has_timestamp_overflow (output_buffer3));
}

TEST_F (ClipPlaybackDataProviderTest, BackwardPlayheadMovementWithLoop)
{
  // Create a very short MIDI region that will loop multiple times
  std::vector<std::tuple<int, int, int>> notes = {
    { 60, 64, 0 }  // Note at position 0
  };
  auto region = create_midi_region_with_sampled_notes (0.0, 1.0, notes);

  // Generate events for the region
  provider_->generate_midi_events (
    *region, structure::tracks::ClipQuantizeOption::Immediate);

  // Process several buffers to establish a playback position
  for (int i = 0; i < 3; ++i)
    {
      dsp::MidiEventVector  output_buffer;
      EngineProcessTimeInfo time_info =
        create_time_info (i * 256, i * 256, 0, 256);
      provider_->process_midi_events (time_info, output_buffer);

      // Should have events in each buffer
      auto events = find_event_timestamps (output_buffer, 60);
      EXPECT_GT (events.size (), 0);
    }

  // Now simulate transport looping back to frame 0
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = create_time_info (0, 0, 0, 256);

  provider_->process_midi_events (time_info, output_buffer);

  // Should have events even after the backward jump
  auto events = find_event_timestamps (output_buffer, 60);
  EXPECT_GT (events.size (), 0);

  // The event should be at timestamp 0 since we're starting at frame 0
  EXPECT_EQ (events[0], 0);

  // Check for timestamp overflow
  EXPECT_FALSE (has_timestamp_overflow (output_buffer));
}

// ========== Audio Tests ==========

TEST_F (ClipPlaybackDataProviderTest, AudioInitialState)
{
  // Provider should start with no audio
  std::vector<float>    output_left (256, 0.0f);
  std::vector<float>    output_right (256, 0.0f);
  EngineProcessTimeInfo time_info = create_time_info (0, 0, 0, 256);

  provider_->process_audio_events (time_info, output_left, output_right);

  // Output should be all zeros
  for (size_t i = 0; i < output_left.size (); ++i)
    {
      EXPECT_FLOAT_EQ (output_left[i], 0.0f);
      EXPECT_FLOAT_EQ (output_right[i], 0.0f);
    }
}

TEST_F (ClipPlaybackDataProviderTest, GenerateAudioEventsWithAudioRegion)
{
  // Create a simple audio region
  auto region = create_simple_audio_region ();

  // Generate events for the region
  provider_->generate_audio_events (
    *region, structure::tracks::ClipQuantizeOption::Immediate);

  // Test processing audio that should include the region
  std::vector<float>    output_left (256, 0.0f);
  std::vector<float>    output_right (256, 0.0f);
  EngineProcessTimeInfo time_info = create_time_info (0, 0, 0, 256);

  provider_->process_audio_events (time_info, output_left, output_right);

  // Should have some audio output (not all zeros)
  bool has_audio = false;
  for (size_t i = 0; i < output_left.size (); ++i)
    {
      if (
        std::abs (output_left[i]) > 0.001f
        || std::abs (output_right[i]) > 0.001f)
        {
          has_audio = true;
          break;
        }
    }
  EXPECT_TRUE (has_audio);
}

TEST_F (ClipPlaybackDataProviderTest, AudioClearEvents)
{
  // Create a simple audio region
  auto region = create_simple_audio_region ();

  // Generate events for the region
  provider_->generate_audio_events (
    *region, structure::tracks::ClipQuantizeOption::Immediate);

  // Test processing audio - should have audio
  std::vector<float>    output_left (256, 0.0f);
  std::vector<float>    output_right (256, 0.0f);
  EngineProcessTimeInfo time_info = create_time_info (0, 0, 0, 256);

  provider_->process_audio_events (time_info, output_left, output_right);

  bool has_audio_before = false;
  for (size_t i = 0; i < output_left.size (); ++i)
    {
      if (
        std::abs (output_left[i]) > 0.001f
        || std::abs (output_right[i]) > 0.001f)
        {
          has_audio_before = true;
          break;
        }
    }
  EXPECT_TRUE (has_audio_before);

  // Clear events
  provider_->clear_events ();

  // Test processing audio again - should have no audio
  std::ranges::fill (output_left, 0.0f);
  std::ranges::fill (output_right, 0.0f);
  provider_->process_audio_events (time_info, output_left, output_right);

  for (size_t i = 0; i < output_left.size (); ++i)
    {
      EXPECT_FLOAT_EQ (output_left[i], 0.0f);
      EXPECT_FLOAT_EQ (output_right[i], 0.0f);
    }
}

TEST_F (ClipPlaybackDataProviderTest, AudioQuantizeOptions)
{
  // Create a simple audio region
  auto region = create_simple_audio_region ();

  // Test different quantize options
  std::vector<ClipQuantizeOption> quantize_options = {
    structure::tracks::ClipQuantizeOption::Immediate,
    structure::tracks::ClipQuantizeOption::NextBar,
    structure::tracks::ClipQuantizeOption::NextBeat,
    structure::tracks::ClipQuantizeOption::NextQuarterBeat,
    structure::tracks::ClipQuantizeOption::NextEighthBeat,
    structure::tracks::ClipQuantizeOption::NextSixteenthBeat,
    structure::tracks::ClipQuantizeOption::NextThirtySecondBeat
  };

  for (const auto &quantize_option : quantize_options)
    {
      // Generate events with the quantize option
      provider_->generate_audio_events (*region, quantize_option);

      // Test processing audio
      std::vector<float>    output_left (256, 0.0f);
      std::vector<float>    output_right (256, 0.0f);
      EngineProcessTimeInfo time_info = create_time_info (0, 0, 0, 256);

      provider_->process_audio_events (time_info, output_left, output_right);

      // Should have audio for immediate quantization
      if (quantize_option == structure::tracks::ClipQuantizeOption::Immediate)
        {
          bool has_audio = false;
          for (size_t i = 0; i < output_left.size (); ++i)
            {
              if (
                std::abs (output_left[i]) > 0.001f
                || std::abs (output_right[i]) > 0.001f)
                {
                  has_audio = true;
                  break;
                }
            }
          EXPECT_TRUE (has_audio);
        }
    }
}

TEST_F (ClipPlaybackDataProviderTest, AudioPreciseLoopingBehavior)
{
  // Create a very short audio region that will loop
  auto region = create_short_audio_region (23); // 23 samples

  // Generate events for the region
  provider_->generate_audio_events (
    *region, structure::tracks::ClipQuantizeOption::Immediate);

  // Process multiple buffers to trigger looping
  std::vector<std::vector<float>> loop_audio_left;
  std::vector<std::vector<float>> loop_audio_right;
  std::vector<float>              output_left (256, 0.0f);
  std::vector<float>              output_right (256, 0.0f);

  // Process first buffer
  EngineProcessTimeInfo time_info1 = create_time_info (0, 0, 0, 1024);
  provider_->process_audio_events (time_info1, output_left, output_right);
  loop_audio_left.push_back (output_left);
  loop_audio_right.push_back (output_right);

  // Process second buffer that should include the loop
  output_left.assign (256, 0.0f);
  output_right.assign (256, 0.0f);
  EngineProcessTimeInfo time_info2 = create_time_info (1024, 1024, 0, 2048);
  provider_->process_audio_events (time_info2, output_left, output_right);
  loop_audio_left.push_back (output_left);
  loop_audio_right.push_back (output_right);

  // Process third buffer that should include another loop
  output_left.assign (256, 0.0f);
  output_right.assign (256, 0.0f);
  EngineProcessTimeInfo time_info3 = create_time_info (3072, 3072, 0, 2048);
  provider_->process_audio_events (time_info3, output_left, output_right);
  loop_audio_left.push_back (output_left);
  loop_audio_right.push_back (output_right);

  // Verify that we have audio in each buffer
  for (const auto &audio_buffer : loop_audio_left)
    {
      bool has_audio = false;
      for (size_t i = 0; i < audio_buffer.size (); ++i)
        {
          if (std::abs (audio_buffer[i]) > 0.001f)
            {
              has_audio = true;
              break;
            }
        }
      EXPECT_TRUE (has_audio);
    }
}

TEST_F (ClipPlaybackDataProviderTest, AudioPreciseEventTiming)
{
  // Create an audio region with specific content
  auto region = create_sine_wave_audio_region (4410); // 0.1 seconds

  // Generate events for the region
  provider_->generate_audio_events (
    *region, structure::tracks::ClipQuantizeOption::Immediate);

  // Process audio
  std::vector<float>    output_left (4096, 0.0f);
  std::vector<float>    output_right (4096, 0.0f);
  EngineProcessTimeInfo time_info = create_time_info (0, 0, 0, 4096);

  provider_->process_audio_events (time_info, output_left, output_right);

  // Should have audio output
  bool has_audio = false;
  for (size_t i = 0; i < output_left.size (); ++i)
    {
      if (
        std::abs (output_left[i]) > 0.001f
        || std::abs (output_right[i]) > 0.001f)
        {
          has_audio = true;
          break;
        }
    }
  EXPECT_TRUE (has_audio);

  // First sample will be zero due to built-in fade in (starts from 0.0)
  EXPECT_FLOAT_EQ (output_left[0], 0.0f);
  EXPECT_FLOAT_EQ (output_right[0], 0.0f);

  // But we should have non-zero audio after the built-in fade in
  EXPECT_GT (
    std::abs (output_left[arrangement::AudioRegion::BUILTIN_FADE_FRAMES]),
    0.001f);
  EXPECT_GT (
    std::abs (output_right[arrangement::AudioRegion::BUILTIN_FADE_FRAMES]),
    0.001f);
}

TEST_F (ClipPlaybackDataProviderTest, AudioBarBoundaryQuantization)
{
  // Create a simple audio region
  auto region = create_simple_audio_region ();

  // Generate events with NextBar quantization
  provider_->generate_audio_events (
    *region, structure::tracks::ClipQuantizeOption::NextBar);

  // Test processing audio that should NOT include the audio initially
  // because we're quantizing to the next bar
  std::vector<float>    output_left (256, 0.0f);
  std::vector<float>    output_right (256, 0.0f);
  EngineProcessTimeInfo time_info = create_time_info (64, 64, 0, 256);

  provider_->process_audio_events (time_info, output_left, output_right);

  // Should have no audio initially because we're waiting for the next bar
  for (size_t i = 0; i < output_left.size (); ++i)
    {
      EXPECT_FLOAT_EQ (output_left[i], 0.0f);
      EXPECT_FLOAT_EQ (output_right[i], 0.0f);
    }

  // Process audio at the next bar boundary (1 bar = 4 beats = 3840 ticks)
  const auto bar_start_samples =
    tempo_map_->tick_to_samples_rounded (units::ticks (3840.0));
  EngineProcessTimeInfo bar_time_info = create_time_info (
    static_cast<unsigned_frame_t> (bar_start_samples.in (units::sample)),
    static_cast<unsigned_frame_t> (bar_start_samples.in (units::sample)), 0,
    256);

  std::ranges::fill (output_left, 0.0f);
  std::ranges::fill (output_right, 0.0f);
  provider_->process_audio_events (bar_time_info, output_left, output_right);

  // Should now have audio because we're at the next bar
  bool has_audio = false;
  for (size_t i = 0; i < output_left.size (); ++i)
    {
      if (
        std::abs (output_left[i]) > 0.001f
        || std::abs (output_right[i]) > 0.001f)
        {
          has_audio = true;
          break;
        }
    }
  EXPECT_TRUE (has_audio);
}

TEST_F (ClipPlaybackDataProviderTest, AudioBeatBoundaryQuantization)
{
  // Create a simple audio region
  auto region = create_simple_audio_region ();

  // Generate events with NextBeat quantization
  provider_->generate_audio_events (
    *region, structure::tracks::ClipQuantizeOption::NextBeat);

  // Test processing audio that should NOT include the audio initially
  // because we're quantizing to the next beat
  std::vector<float>    output_left (256, 0.0f);
  std::vector<float>    output_right (256, 0.0f);
  EngineProcessTimeInfo time_info = create_time_info (64, 64, 0, 256);

  provider_->process_audio_events (time_info, output_left, output_right);

  // Should have no audio initially because we're waiting for the next beat
  for (size_t i = 0; i < output_left.size (); ++i)
    {
      EXPECT_FLOAT_EQ (output_left[i], 0.0f);
      EXPECT_FLOAT_EQ (output_right[i], 0.0f);
    }

  // Process audio at the next beat boundary (1 beat = 960 ticks)
  const auto beat_start_samples =
    tempo_map_->tick_to_samples_rounded (units::ticks (960.0));
  EngineProcessTimeInfo beat_time_info = create_time_info (
    static_cast<unsigned_frame_t> (beat_start_samples.in (units::sample)),
    static_cast<unsigned_frame_t> (beat_start_samples.in (units::sample)), 0,
    256);

  std::ranges::fill (output_left, 0.0f);
  std::ranges::fill (output_right, 0.0f);
  provider_->process_audio_events (beat_time_info, output_left, output_right);

  // Should now have audio because we're at the next beat
  bool has_audio = false;
  for (size_t i = 0; i < output_left.size (); ++i)
    {
      if (
        std::abs (output_left[i]) > 0.001f
        || std::abs (output_right[i]) > 0.001f)
        {
          has_audio = true;
          break;
        }
    }
  EXPECT_TRUE (has_audio);
}

class ClipPlaybackDataProviderAudioBufferSizeTest
    : public ClipPlaybackDataProviderTest,
      public ::testing::WithParamInterface<nframes_t>
{
};

TEST_P (
  ClipPlaybackDataProviderAudioBufferSizeTest,
  ProcessAudioEventsWithDifferentBufferSizes)
{
  const auto buffer_size = GetParam ();

  // Create a simple audio region
  auto region = create_simple_audio_region ();

  // Generate events for the region
  provider_->generate_audio_events (
    *region, structure::tracks::ClipQuantizeOption::Immediate);

  // Test processing audio with the specific buffer size
  std::vector<float>    output_left (buffer_size, 0.0f);
  std::vector<float>    output_right (buffer_size, 0.0f);
  EngineProcessTimeInfo time_info = create_time_info (0, 0, 0, buffer_size);

  provider_->process_audio_events (time_info, output_left, output_right);

  // Should have audio regardless of buffer size
  bool has_audio = false;
  for (size_t i = 0; i < output_left.size (); ++i)
    {
      if (
        std::abs (output_left[i]) > 0.001f
        || std::abs (output_right[i]) > 0.001f)
        {
          has_audio = true;
          break;
        }
    }
  EXPECT_TRUE (has_audio);
}

INSTANTIATE_TEST_SUITE_P (
  ClipPlaybackDataProviderTest,
  ClipPlaybackDataProviderAudioBufferSizeTest,
  ::testing::Values (64, 128, 256, 512, 1024, 2048));

TEST_F (ClipPlaybackDataProviderTest, ProcessAudioEventsWithOffset)
{
  // Create a simple audio region
  auto region = create_simple_audio_region ();

  // Generate events for the region
  provider_->generate_audio_events (
    *region, structure::tracks::ClipQuantizeOption::Immediate);

  // Test processing audio with a global offset
  std::vector<float>    output_left (256, 0.0f);
  std::vector<float>    output_right (256, 0.0f);
  EngineProcessTimeInfo time_info = create_time_info (10000, 10100, 100, 256);

  provider_->process_audio_events (time_info, output_left, output_right);

  // Should have audio since we're using immediate quantization
  bool has_audio = false;
  for (size_t i = 0; i < output_left.size (); ++i)
    {
      if (
        std::abs (output_left[i]) > 0.001f
        || std::abs (output_right[i]) > 0.001f)
        {
          has_audio = true;
          break;
        }
    }
  EXPECT_TRUE (has_audio);

  // The audio should be offset by the difference between g_start_frame_w_offset
  // and g_start_frame_, which is 100 samples
  // So the first 100 samples should be zero
  for (size_t i = 0; i < 100; ++i)
    {
      EXPECT_FLOAT_EQ (output_left[i], 0.0f);
      EXPECT_FLOAT_EQ (output_right[i], 0.0f);
    }

  // But there should be audio after the offset
  for (size_t i = 100; i < output_left.size (); ++i)
    {
      if (
        std::abs (output_left[i]) > 0.001f
        || std::abs (output_right[i]) > 0.001f)
        {
          has_audio = true;
          break;
        }
    }
  EXPECT_TRUE (has_audio);
}

TEST_F (ClipPlaybackDataProviderTest, AudioStateManagementAcrossBuffers)
{
  // Create a short audio region
  auto region = create_short_audio_region (128);

  // Generate events for the region
  provider_->generate_audio_events (
    *region, structure::tracks::ClipQuantizeOption::Immediate);

  // Process first buffer
  std::vector<float>    output_left1 (128, 0.0f);
  std::vector<float>    output_right1 (128, 0.0f);
  EngineProcessTimeInfo time_info1 = create_time_info (0, 0, 0, 128);

  provider_->process_audio_events (time_info1, output_left1, output_right1);

  // Should have audio in the first buffer
  bool has_audio1 = false;
  for (size_t i = 0; i < output_left1.size (); ++i)
    {
      if (
        std::abs (output_left1[i]) > 0.001f
        || std::abs (output_right1[i]) > 0.001f)
        {
          has_audio1 = true;
          break;
        }
    }
  EXPECT_TRUE (has_audio1);

  // Process second buffer continuing from where we left off
  std::vector<float>    output_left2 (2048, 0.0f);
  std::vector<float>    output_right2 (2048, 0.0f);
  EngineProcessTimeInfo time_info2 = create_time_info (128, 128, 0, 2048);

  provider_->process_audio_events (time_info2, output_left2, output_right2);

  // Should have audio in the second buffer due to looping
  bool has_audio2 = false;
  for (size_t i = 0; i < output_left2.size (); ++i)
    {
      if (
        std::abs (output_left2[i]) > 0.001f
        || std::abs (output_right2[i]) > 0.001f)
        {
          has_audio2 = true;
          break;
        }
    }
  EXPECT_TRUE (has_audio2);
}

TEST_F (ClipPlaybackDataProviderTest, AudioBasicFunctionality)
{
  // Test that the provider can be constructed
  EXPECT_NE (provider_, nullptr);

  // Test that process_audio_events can be called without crashing
  std::vector<float>    output_left (256, 0.0f);
  std::vector<float>    output_right (256, 0.0f);
  EngineProcessTimeInfo time_info = create_time_info (0, 0, 0, 256);

  provider_->process_audio_events (time_info, output_left, output_right);

  // Should be all zeros initially
  for (size_t i = 0; i < output_left.size (); ++i)
    {
      EXPECT_FLOAT_EQ (output_left[i], 0.0f);
      EXPECT_FLOAT_EQ (output_right[i], 0.0f);
    }

  // Test that generate_audio_events can be called without crashing
  auto region = create_simple_audio_region ();
  provider_->generate_audio_events (
    *region, structure::tracks::ClipQuantizeOption::Immediate);

  // Process again to ensure no crash
  provider_->process_audio_events (time_info, output_left, output_right);

  // Should now have audio
  bool has_audio = false;
  for (size_t i = 0; i < output_left.size (); ++i)
    {
      if (
        std::abs (output_left[i]) > 0.001f
        || std::abs (output_right[i]) > 0.001f)
        {
          has_audio = true;
          break;
        }
    }
  EXPECT_TRUE (has_audio);
}

} // namespace zrythm::structure::scenes
