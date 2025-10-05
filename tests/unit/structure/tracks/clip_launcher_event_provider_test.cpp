// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/midi_event.h"
#include "dsp/tempo_map.h"
#include "structure/arrangement/midi_note.h"
#include "structure/arrangement/midi_region.h"
#include "structure/tracks/clip_launcher_event_provider.h"
#include "utils/types.h"

#include "gtest/gtest.h"
#include <juce_wrapper.h>

namespace zrythm::structure::tracks
{

class ClipLauncherMidiEventProviderTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    // Create a tempo map for testing
    tempo_map_ = std::make_unique<dsp::TempoMap> (units::sample_rate (44100));

    // Create the provider
    provider_ = std::make_unique<ClipLauncherMidiEventProvider> (*tempo_map_);

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

  std::unique_ptr<ClipLauncherMidiEventProvider>       provider_;
  std::unique_ptr<dsp::TempoMap>                       tempo_map_;
  std::unique_ptr<arrangement::ArrangerObjectRegistry> obj_registry_;
  std::unique_ptr<dsp::FileAudioSourceRegistry> file_audio_source_registry_;
  std::vector<arrangement::ArrangerObjectUuidReference>
    region_refs; // Keep references
};

TEST_F (ClipLauncherMidiEventProviderTest, InitialState)
{
  // Provider should start with no events
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = create_time_info (0, 0, 0, 256);

  provider_->process_events (time_info, output_buffer);
  EXPECT_EQ (output_buffer.size (), 0);
}

TEST_F (ClipLauncherMidiEventProviderTest, GenerateEventsWithMidiRegion)
{
  // Create a MIDI region
  auto region = create_simple_region ();

  // Generate events for the region
  provider_->generate_events (
    *region, structure::tracks::ClipQuantizeOption::Immediate);

  // Test processing events that should include the note
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = create_time_info (0, 0, 0, 256);

  provider_->process_events (time_info, output_buffer);

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

TEST_F (ClipLauncherMidiEventProviderTest, ClearEvents)
{
  // Create a MIDI region
  auto region = create_simple_region ();

  // Generate events for the region
  provider_->generate_events (
    *region, structure::tracks::ClipQuantizeOption::Immediate);

  // Test processing events - should have events
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = create_time_info (0, 0, 0, 256);

  provider_->process_events (time_info, output_buffer);
  EXPECT_GT (output_buffer.size (), 0);

  // Clear events
  provider_->clear_events ();

  // Test processing events again - should have no events
  output_buffer.clear ();
  provider_->process_events (time_info, output_buffer);
  EXPECT_EQ (output_buffer.size (), 0);
}

TEST_F (ClipLauncherMidiEventProviderTest, QuantizeOptions)
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
      provider_->generate_events (*region, quantize_option);

      // Test processing events
      dsp::MidiEventVector  output_buffer;
      EngineProcessTimeInfo time_info = create_time_info (0, 0, 0, 256);

      provider_->process_events (time_info, output_buffer);

      // Should have events (quantize option doesn't affect the presence of
      // events, just when they start playing)
      if (quantize_option == structure::tracks::ClipQuantizeOption::Immediate)
        {
          EXPECT_GT (output_buffer.size (), 0);
        }
    }
}

TEST_F (ClipLauncherMidiEventProviderTest, MultipleNotesInRegion)
{
  // Create a MIDI region with multiple notes
  std::vector<std::pair<int, int>> notes = {
    { 60, 64  },
    { 64, 80  },
    { 67, 100 }
  };
  auto region = create_multi_note_region (notes);

  // Generate events for the region
  provider_->generate_events (
    *region, structure::tracks::ClipQuantizeOption::Immediate);

  // Test processing events that should include all notes
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = create_time_info (0, 0, 0, 256);

  provider_->process_events (time_info, output_buffer);

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

TEST_F (ClipLauncherMidiEventProviderTest, PreciseLoopingBehavior)
{
  // Create a very short MIDI region that will loop (approximately 23 samples)
  std::vector<std::tuple<int, int, int>> notes = {
    { 60, 64, 0 }  // Note at position 0
  };
  auto region = create_midi_region_with_sampled_notes (0.0, 1.0, notes);

  // Generate events for the region
  provider_->generate_events (
    *region, structure::tracks::ClipQuantizeOption::Immediate);

  // Process multiple buffers to trigger looping
  std::vector<std::vector<midi_time_t>> loop_events;
  dsp::MidiEventVector                  output_buffer;

  // Process first buffer
  EngineProcessTimeInfo time_info1 = create_time_info (0, 0, 0, 1024);
  provider_->process_events (time_info1, output_buffer);
  auto first_loop_events = find_event_timestamps (output_buffer, 60);
  loop_events.push_back (first_loop_events);

  // Process second buffer that should include the loop
  EngineProcessTimeInfo time_info2 = create_time_info (1024, 1024, 0, 2048);
  output_buffer.clear ();
  provider_->process_events (time_info2, output_buffer);
  auto second_loop_events = find_event_timestamps (output_buffer, 60);
  loop_events.push_back (second_loop_events);

  // Process third buffer that should include another loop
  EngineProcessTimeInfo time_info3 = create_time_info (3072, 3072, 0, 2048);
  output_buffer.clear ();
  provider_->process_events (time_info3, output_buffer);
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

TEST_F (ClipLauncherMidiEventProviderTest, PreciseEventTiming)
{
  // Create a MIDI region with notes at specific sample positions
  std::vector<std::tuple<int, int, int>> notes = {
    { 60, 64,  0    }, // Note at position 0
    { 62, 80,  1150 }, // Note at position ~1150 samples
    { 64, 100, 2295 }  // Note at position ~2295 samples
  };
  auto region = create_midi_region_with_sampled_notes (0.0, 400.0, notes);

  // Generate events for the region
  provider_->generate_events (
    *region, structure::tracks::ClipQuantizeOption::Immediate);

  // Process events
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = create_time_info (0, 0, 0, 4096);

  provider_->process_events (time_info, output_buffer);

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

TEST_F (ClipLauncherMidiEventProviderTest, BarBoundaryQuantization)
{
  // Create a MIDI region
  auto region = create_simple_region ();

  // Generate events with NextBar quantization
  provider_->generate_events (
    *region, structure::tracks::ClipQuantizeOption::NextBar);

  // Test processing events that should NOT include the note initially
  // because we're quantizing to the next bar
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = create_time_info (64, 64, 0, 256);

  provider_->process_events (time_info, output_buffer);

  // Should have no events initially because we're waiting for the next bar
  EXPECT_EQ (output_buffer.size (), 0);

  // Process events at the next bar boundary (1 bar = 4 beats = 3840 ticks)
  const auto bar_start_samples =
    tempo_map_->tick_to_samples_rounded (units::ticks (3840.0));
  EngineProcessTimeInfo bar_time_info = create_time_info (
    static_cast<unsigned_frame_t> (bar_start_samples.in (units::sample)),
    static_cast<unsigned_frame_t> (bar_start_samples.in (units::sample)), 0,
    256);

  provider_->process_events (bar_time_info, output_buffer);

  // Should now have events because we're at the next bar
  EXPECT_GT (output_buffer.size (), 0);
}

TEST_F (ClipLauncherMidiEventProviderTest, BeatBoundaryQuantization)
{
  // Create a MIDI region
  auto region = create_simple_region ();

  // Generate events with NextBeat quantization
  provider_->generate_events (
    *region, structure::tracks::ClipQuantizeOption::NextBeat);

  // Test processing events that should NOT include the note initially
  // because we're quantizing to the next beat
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = create_time_info (64, 64, 0, 256);

  provider_->process_events (time_info, output_buffer);

  // Should have no events initially because we're waiting for the next beat
  EXPECT_EQ (output_buffer.size (), 0);

  // Process events at the next beat boundary (1 beat = 960 ticks)
  const auto beat_start_samples =
    tempo_map_->tick_to_samples_rounded (units::ticks (960.0));
  EngineProcessTimeInfo beat_time_info = create_time_info (
    static_cast<unsigned_frame_t> (beat_start_samples.in (units::sample)),
    static_cast<unsigned_frame_t> (beat_start_samples.in (units::sample)), 0,
    256);

  provider_->process_events (beat_time_info, output_buffer);

  // Should now have events because we're at the next beat
  EXPECT_GT (output_buffer.size (), 0);
}

class ClipLauncherMidiEventProviderBufferSizeTest
    : public ClipLauncherMidiEventProviderTest,
      public ::testing::WithParamInterface<nframes_t>
{
};

TEST_P (
  ClipLauncherMidiEventProviderBufferSizeTest,
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
  provider_->generate_events (
    *region, structure::tracks::ClipQuantizeOption::Immediate);

  // Test processing events with the specific buffer size
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = create_time_info (0, 0, 0, buffer_size);

  provider_->process_events (time_info, output_buffer);

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
  ClipLauncherMidiEventProviderTest,
  ClipLauncherMidiEventProviderBufferSizeTest,
  ::testing::Values (64, 128, 256, 512, 1024, 2048));

TEST_F (ClipLauncherMidiEventProviderTest, ProcessEventsWithOffset)
{
  // Create a MIDI region with a note at position 0
  auto region = create_simple_region ();

  // Generate events for the region
  provider_->generate_events (
    *region, structure::tracks::ClipQuantizeOption::Immediate);

  // Test processing events with a global offset
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = create_time_info (10000, 10100, 100, 256);

  provider_->process_events (time_info, output_buffer);

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

TEST_F (ClipLauncherMidiEventProviderTest, QuantizationTimingAccuracy)
{
  // Create a MIDI region
  auto region = create_simple_region ();

  // Generate events with NextBeat quantization
  provider_->generate_events (
    *region, structure::tracks::ClipQuantizeOption::NextBeat);

  // Start processing from a position that's not on a beat boundary
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = create_time_info (64, 64, 0, 256);

  provider_->process_events (time_info, output_buffer);

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
  provider_->process_events (beat_time_info, output_buffer);

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

TEST_F (ClipLauncherMidiEventProviderTest, StateManagementAcrossBuffers)
{
  // Create a MIDI region with notes at different positions
  std::vector<std::tuple<int, int, int>> notes = {
    { 60, 64, 0    }, // Note at position 0
    { 62, 80, 1150 }  // Note at position ~1150 samples
  };
  auto region = create_midi_region_with_sampled_notes (0.0, 400.0, notes);

  // Generate events for the region
  provider_->generate_events (
    *region, structure::tracks::ClipQuantizeOption::Immediate);

  // Process first buffer
  dsp::MidiEventVector  output_buffer1;
  EngineProcessTimeInfo time_info1 = create_time_info (0, 0, 0, 128);

  provider_->process_events (time_info1, output_buffer1);

  // Should have the first note but not the second
  auto pitch_60_events1 = find_event_timestamps (output_buffer1, 60);
  auto pitch_62_events1 = find_event_timestamps (output_buffer1, 62);
  EXPECT_GT (pitch_60_events1.size (), 0);
  EXPECT_EQ (pitch_62_events1.size (), 0);

  // Process second buffer continuing from where we left off
  dsp::MidiEventVector  output_buffer2;
  EngineProcessTimeInfo time_info2 = create_time_info (128, 128, 0, 2048);

  provider_->process_events (time_info2, output_buffer2);

  // Should have the second note
  auto pitch_62_events2 = find_event_timestamps (output_buffer2, 62);
  EXPECT_GT (pitch_62_events2.size (), 0);

  // The second note should be at the correct position relative to the buffer
  EXPECT_EQ (pitch_62_events2[0], 1150 - 128);

  // Check for timestamp overflow
  EXPECT_FALSE (has_timestamp_overflow (output_buffer2));
}

TEST_F (ClipLauncherMidiEventProviderTest, BasicFunctionality)
{
  // Test that the provider can be constructed
  EXPECT_NE (provider_, nullptr);

  // Test that process_events can be called without crashing
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = create_time_info (0, 0, 0, 256);

  provider_->process_events (time_info, output_buffer);
  EXPECT_EQ (output_buffer.size (), 0);

  // Test that generate_events can be called without crashing
  auto region = create_simple_region ();
  provider_->generate_events (
    *region, structure::tracks::ClipQuantizeOption::Immediate);

  // Process again to ensure no crash
  provider_->process_events (time_info, output_buffer);
  EXPECT_GT (output_buffer.size (), 0);
}

TEST_F (ClipLauncherMidiEventProviderTest, GenerateEventsWithMutedNote)
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
  provider_->generate_events (
    *region, structure::tracks::ClipQuantizeOption::Immediate);

  // Test processing events
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = create_time_info (0, 0, 0, 256);

  provider_->process_events (time_info, output_buffer);

  // Should have no events since the only note is muted
  EXPECT_EQ (output_buffer.size (), 0);
}

TEST_F (ClipLauncherMidiEventProviderTest, LoopingEdgeCases)
{
  // Create a very short MIDI region (23 samples) to test edge cases
  std::vector<std::tuple<int, int, int>> notes = {
    { 60, 64, 0 }  // Note at position 0
  };
  auto region = create_midi_region_with_sampled_notes (0.0, 1.0, notes);

  // Generate events for the region
  provider_->generate_events (
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
      provider_->process_events (time_info, output_buffer);
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

TEST_F (ClipLauncherMidiEventProviderTest, EventsOnBufferBoundaries)
{
  // Create a MIDI region with a note exactly at a buffer boundary
  std::vector<std::tuple<int, int, int>> notes = {
    { 60, 64, 128 }  // Note exactly at buffer boundary
  };
  auto region = create_midi_region_with_sampled_notes (0.0, 400.0, notes);

  // Generate events for the region
  provider_->generate_events (
    *region, structure::tracks::ClipQuantizeOption::Immediate);

  // Process first buffer
  dsp::MidiEventVector  output_buffer1;
  EngineProcessTimeInfo time_info1 = create_time_info (0, 0, 0, 128);

  provider_->process_events (time_info1, output_buffer1);

  // Should have no events in the first buffer since the note is at the boundary
  auto pitch_60_events1 = find_event_timestamps (output_buffer1, 60);
  EXPECT_EQ (pitch_60_events1.size (), 0);

  // Process second buffer starting at the boundary
  dsp::MidiEventVector  output_buffer2;
  EngineProcessTimeInfo time_info2 = create_time_info (128, 128, 0, 128);

  provider_->process_events (time_info2, output_buffer2);

  // Should have the event in the second buffer at position 0
  auto pitch_60_events2 = find_event_timestamps (output_buffer2, 60);
  EXPECT_GT (pitch_60_events2.size (), 0);
  EXPECT_EQ (pitch_60_events2[0], 0);

  // Check for timestamp overflow
  EXPECT_FALSE (has_timestamp_overflow (output_buffer2));
}

TEST_F (ClipLauncherMidiEventProviderTest, BackwardPlayheadMovement)
{
  // Create a MIDI region with notes at different positions
  std::vector<std::tuple<int, int, int>> notes = {
    { 60, 64, 0    }, // Note at position 0
    { 62, 80, 1150 }  // Note at position ~1150 samples
  };
  auto region = create_midi_region_with_sampled_notes (0.0, 400.0, notes);

  // Generate events for the region
  provider_->generate_events (
    *region, structure::tracks::ClipQuantizeOption::Immediate);

  // Process first buffer starting at frame 0
  dsp::MidiEventVector  output_buffer1;
  EngineProcessTimeInfo time_info1 = create_time_info (0, 0, 0, 256);

  provider_->process_events (time_info1, output_buffer1);

  // Should have the first note
  auto pitch_60_events1 = find_event_timestamps (output_buffer1, 60);
  EXPECT_GT (pitch_60_events1.size (), 0);

  // Process second buffer continuing forward
  dsp::MidiEventVector  output_buffer2;
  EngineProcessTimeInfo time_info2 = create_time_info (256, 256, 0, 256);

  provider_->process_events (time_info2, output_buffer2);

  // Should NOT have the second note yet because it's at position 1150
  // and this buffer only covers positions 256-511
  auto pitch_62_events2 = find_event_timestamps (output_buffer2, 62);
  EXPECT_EQ (pitch_62_events2.size (), 0);

  // Now simulate transport looping back to frame 0
  // This is the critical test - when playhead moves backwards
  dsp::MidiEventVector  output_buffer3;
  EngineProcessTimeInfo time_info3 = create_time_info (0, 0, 0, 256);

  provider_->process_events (time_info3, output_buffer3);

  // Should have the first note again since we looped back
  auto pitch_60_events3 = find_event_timestamps (output_buffer3, 60);
  EXPECT_GT (pitch_60_events3.size (), 0);

  // The event should be at timestamp 0 since we're starting at frame 0
  EXPECT_EQ (pitch_60_events3[0], 0);

  // Check for timestamp overflow
  EXPECT_FALSE (has_timestamp_overflow (output_buffer3));
}

TEST_F (ClipLauncherMidiEventProviderTest, BackwardPlayheadMovementWithLoop)
{
  // Create a very short MIDI region that will loop multiple times
  std::vector<std::tuple<int, int, int>> notes = {
    { 60, 64, 0 }  // Note at position 0
  };
  auto region = create_midi_region_with_sampled_notes (0.0, 1.0, notes);

  // Generate events for the region
  provider_->generate_events (
    *region, structure::tracks::ClipQuantizeOption::Immediate);

  // Process several buffers to establish a playback position
  for (int i = 0; i < 3; ++i)
    {
      dsp::MidiEventVector  output_buffer;
      EngineProcessTimeInfo time_info =
        create_time_info (i * 256, i * 256, 0, 256);
      provider_->process_events (time_info, output_buffer);

      // Should have events in each buffer
      auto events = find_event_timestamps (output_buffer, 60);
      EXPECT_GT (events.size (), 0);
    }

  // Now simulate transport looping back to frame 0
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = create_time_info (0, 0, 0, 256);

  provider_->process_events (time_info, output_buffer);

  // Should have events even after the backward jump
  auto events = find_event_timestamps (output_buffer, 60);
  EXPECT_GT (events.size (), 0);

  // The event should be at timestamp 0 since we're starting at frame 0
  EXPECT_EQ (events[0], 0);

  // Check for timestamp overflow
  EXPECT_FALSE (has_timestamp_overflow (output_buffer));
}

} // namespace zrythm::structure::scenes
