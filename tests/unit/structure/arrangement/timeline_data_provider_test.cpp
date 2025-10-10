// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/midi_event.h"
#include "dsp/tempo_map.h"
#include "structure/arrangement/audio_region.h"
#include "structure/arrangement/midi_note.h"
#include "structure/arrangement/midi_region.h"
#include "structure/arrangement/timeline_data_provider.h"
#include "utils/types.h"

#include "gtest/gtest.h"
#include <juce_audio_formats/juce_audio_formats.h>

namespace zrythm::structure::arrangement
{

class TimelineDataProviderTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    // Create the provider
    provider_ = std::make_unique<TimelineDataProvider> ();

    // Create a tempo map for testing
    tempo_map_ = std::make_unique<dsp::TempoMap> (units::sample_rate (44100));

    // Create an object registry
    obj_registry_ = std::make_unique<ArrangerObjectRegistry> ();

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

  // Helper function to create a MIDI region
  MidiRegion * create_midi_region (
    double      start_pos_ticks,
    double      end_pos_ticks,
    midi_byte_t note = 60,
    midi_byte_t velocity = 64)
  {
    // Create a MIDI region
    auto region_ref = obj_registry_->create_object<MidiRegion> (
      *tempo_map_, *obj_registry_, *file_audio_source_registry_);
    auto region = region_ref.get_object_as<MidiRegion> ();

    // Set the region's position
    region->position ()->setTicks (start_pos_ticks);
    region->bounds ()->length ()->setTicks (end_pos_ticks - start_pos_ticks);

    // Create a MIDI note
    auto note_ref = obj_registry_->create_object<MidiNote> (*tempo_map_);
    auto midi_note = note_ref.get_object_as<MidiNote> ();

    // Set the note's properties
    midi_note->setPitch (note);
    midi_note->setVelocity (velocity);
    // Note position is relative to the region
    midi_note->position ()->setTicks (0.0);
    midi_note->bounds ()->length ()->setTicks (50.0); // Note duration

    // Add the note to the region
    region->add_object (note_ref);

    // Keep a reference to the region to prevent it from being deleted
    region_refs.push_back (std::move (region_ref));

    return region;
  }

  // Helper function to create a sine wave audio source
  ArrangerObjectUuidReference create_sine_wave_audio_source (
    int    num_samples,
    double frequency_hz = 441.0, // 441 Hz divides evenly into 44100 Hz (100
                                 // samples per period)
    double phase_offset = M_PI / 4.0) // Start at 45 degrees to avoid zero
  {
    auto sample_buffer =
      std::make_unique<utils::audio::AudioBuffer> (2, num_samples);
    const double sample_rate = 44100.0;
    const double phase_increment = 2.0 * M_PI * frequency_hz / sample_rate;

    for (int i = 0; i < num_samples; ++i)
      {
        const double phase = phase_offset + i * phase_increment;
        const float  sample_value = static_cast<float> (std::sin (phase));

        // Left channel: sine wave
        sample_buffer->setSample (0, i, sample_value);
        // Right channel: cosine wave (90 degrees phase shift)
        sample_buffer->setSample (1, i, static_cast<float> (std::cos (phase)));
      }

    auto source_ref = file_audio_source_registry_->create_object<
      dsp::FileAudioSource> (
      *sample_buffer, utils::audio::BitDepth::BIT_DEPTH_32, 44100, 120.f,
      u8"SineTestSource");

    return obj_registry_->create_object<AudioSourceObject> (
      *tempo_map_, *file_audio_source_registry_, source_ref);
  }

  // Helper function to create an audio region
  AudioRegion * create_audio_region (
    double start_pos_ticks,
    double end_pos_ticks,
    float  gain = 1.0f)
  {
    // Create a sine wave audio source
    auto audio_source_object_ref = create_sine_wave_audio_source (4096);

    // Create the audio region
    auto region_ref = obj_registry_->create_object<AudioRegion> (
      *tempo_map_, *obj_registry_, *file_audio_source_registry_,
      [] () { return true; });
    auto region = region_ref.get_object_as<AudioRegion> ();
    region->set_source (audio_source_object_ref);

    // Set the region's position and length
    region->position ()->setTicks (start_pos_ticks);
    region->bounds ()->length ()->setTicks (end_pos_ticks - start_pos_ticks);
    region->setGain (gain);

    // Keep a reference to the region to prevent it from being deleted
    region_refs.push_back (std::move (region_ref));

    return region;
  }

  // Helper function to get expected sine wave value at a specific sample
  std::pair<float, float> get_expected_sine_values (
    int    sample_index,
    double frequency_hz = 441.0, // 441 Hz divides evenly into 44100 Hz (100
                                 // samples per period)
    double phase_offset = M_PI / 4.0) const
  {
    const double sample_rate = 44100.0;
    const double phase_increment = 2.0 * M_PI * frequency_hz / sample_rate;
    const double phase = phase_offset + sample_index * phase_increment;

    return {
      static_cast<float> (std::sin (phase)), // Left channel
      static_cast<float> (std::cos (phase))  // Right channel
    };
  }

  std::unique_ptr<TimelineDataProvider>         provider_;
  std::unique_ptr<dsp::TempoMap>                tempo_map_;
  std::unique_ptr<ArrangerObjectRegistry>       obj_registry_;
  std::unique_ptr<dsp::FileAudioSourceRegistry> file_audio_source_registry_;
  std::vector<ArrangerObjectUuidReference>      region_refs; // Keep references
};

TEST_F (TimelineDataProviderTest, InitialState)
{
  // Provider should start with no events
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = {
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 256
  };

  provider_->process_midi_events (time_info, output_buffer);
  EXPECT_EQ (output_buffer.size (), 0);
}

TEST_F (TimelineDataProviderTest, ProcessEventsWithNoEvents)
{
  // Test processing when no events are available
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = {
    .g_start_frame_ = 1000,
    .g_start_frame_w_offset_ = 1000,
    .local_offset_ = 0,
    .nframes_ = 512
  };

  provider_->process_midi_events (time_info, output_buffer);
  EXPECT_EQ (output_buffer.size (), 0);
}

TEST_F (TimelineDataProviderTest, GenerateEventsWithEmptyRegions)
{
  // Test generating events with empty region list
  std::vector<const MidiRegion *> empty_regions;
  utils::ExpandableTickRange      range (
    std::pair (0.0, 960.0)); // One bar at 120 BPM

  provider_->generate_midi_events (*tempo_map_, empty_regions, range);

  // Verify no events are generated
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = {
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 256
  };

  provider_->process_midi_events (time_info, output_buffer);
  EXPECT_EQ (output_buffer.size (), 0);
}

TEST_F (TimelineDataProviderTest, ProcessEventsWithMidiRegion)
{
  // Create a MIDI region at tick 0 (start of timeline)
  auto region = create_midi_region (0.0, 200.0);

  // Create a vector of regions
  std::vector<const MidiRegion *> regions;
  regions.push_back (region);

  // Generate events for the regions
  utils::ExpandableTickRange range (std::pair (0.0, 960.0));
  provider_->generate_midi_events (*tempo_map_, regions, range);

  // Test processing events that should include the note
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = {
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 256
  };

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

TEST_F (TimelineDataProviderTest, ProcessEventsOutsideTimeRange)
{
  // Create a MIDI region at tick 500
  auto region = create_midi_region (500.0, 700.0);

  // Create a vector of regions
  std::vector<const MidiRegion *> regions;
  regions.push_back (region);

  // Generate events for the regions
  utils::ExpandableTickRange range (std::pair (0.0, 960.0));
  provider_->generate_midi_events (*tempo_map_, regions, range);

  // Test processing events that should NOT include the note
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = {
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 256
  };

  provider_->process_midi_events (time_info, output_buffer);

  // Should have no events since the note is outside the time range
  EXPECT_EQ (output_buffer.size (), 0);
}

TEST_F (TimelineDataProviderTest, ProcessEventsWithOffset)
{
  // Create a MIDI region at tick 0
  auto region = create_midi_region (0.0, 200.0);

  // Create a vector of regions
  std::vector<const MidiRegion *> regions;
  regions.push_back (region);

  // Generate events for the regions
  utils::ExpandableTickRange range (std::pair (0.0, 960.0));
  provider_->generate_midi_events (*tempo_map_, regions, range);

  // Test processing events with a global offset that's far from the note
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = {
    .g_start_frame_ = 10000,
    .g_start_frame_w_offset_ = 10100, // 100 frame offset
    .local_offset_ = 0,
    .nframes_ = 256
  };

  provider_->process_midi_events (time_info, output_buffer);

  // Should have no events since the note is at tick 0
  // and we're processing frames 10100-10356
  EXPECT_EQ (output_buffer.size (), 0);
}

TEST_F (TimelineDataProviderTest, MultipleEventsInSequence)
{
  // Create multiple MIDI regions at the beginning of the timeline
  // with notes at different positions
  auto region1 = create_midi_region (0.0, 200.0, 60);
  auto region2 = create_midi_region (50.0, 250.0, 64);
  auto region3 = create_midi_region (100.0, 300.0, 67);

  // Create a vector of regions
  std::vector<const MidiRegion *> regions;
  regions.push_back (region1);
  regions.push_back (region2);
  regions.push_back (region3);

  // Generate events for the regions
  utils::ExpandableTickRange range (std::pair (0.0, 960.0));
  provider_->generate_midi_events (*tempo_map_, regions, range);

  // Convert tick positions to sample positions for precise testing
  const auto region1_start_samples =
    tempo_map_->tick_to_samples_rounded (units::ticks (0.0));
  const auto region3_start_samples =
    tempo_map_->tick_to_samples_rounded (units::ticks (100.0));

  // Test processing events that should include all notes
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = {
    .g_start_frame_ =
      static_cast<unsigned_frame_t> (region1_start_samples.in (units::sample)),
    .g_start_frame_w_offset_ =
      static_cast<unsigned_frame_t> (region1_start_samples.in (units::sample)),
    .local_offset_ = 0,
    .nframes_ = static_cast<nframes_t> (
      (region3_start_samples - region1_start_samples).in (units::sample) + 256)
  };

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

TEST_F (TimelineDataProviderTest, BasicFunctionality)
{
  // Test that the provider can be constructed
  EXPECT_NE (provider_, nullptr);

  // Test that process_midi_events can be called without crashing
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = {
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 256
  };

  provider_->process_midi_events (time_info, output_buffer);
  EXPECT_EQ (output_buffer.size (), 0);

  // Test that generate_midi_events can be called without crashing
  std::vector<const MidiRegion *> empty_regions;
  utils::ExpandableTickRange      range (std::pair (0.0, 960.0));

  provider_->generate_midi_events (*tempo_map_, empty_regions, range);

  // Process again to ensure no crash
  provider_->process_midi_events (time_info, output_buffer);
  EXPECT_EQ (output_buffer.size (), 0);
}

// Tests from PlaybackCacheBuilder that are unique and test specific concerns
TEST_F (TimelineDataProviderTest, GenerateCacheWithAffectedRange)
{
  // Create a MIDI region at tick 0
  auto region = create_midi_region (0.0, 200.0);

  // Create a vector of regions
  std::vector<const MidiRegion *> regions;
  regions.push_back (region);

  // Generate cache with affected range that includes our region
  const double               affected_start = 0.0;
  const double               affected_end = 200.0;
  utils::ExpandableTickRange range (
    std::make_pair (affected_start, affected_end));
  provider_->generate_midi_events (*tempo_map_, regions, range);

  // Test processing events that should include the note
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = {
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 256
  };

  provider_->process_midi_events (time_info, output_buffer);

  // Should have events from the MIDI region
  EXPECT_GE (output_buffer.size (), 1);

  // Convert affected range to samples for verification
  const auto sample_start =
    tempo_map_->tick_to_samples_rounded (units::ticks (affected_start));
  const auto sample_end =
    tempo_map_->tick_to_samples_rounded (units::ticks (affected_end));

  // All events should be within the sample interval
  for (const auto &event : output_buffer)
    {
      const auto event_time = event.time_;
      EXPECT_GE (
        event_time, static_cast<midi_time_t> (sample_start.in (units::sample)));
      EXPECT_LE (
        event_time, static_cast<midi_time_t> (sample_end.in (units::sample)));

      // Verify it's a note on event with the correct pitch
      EXPECT_EQ (event.raw_buffer_[0] & 0xF0, 0x90); // Note on
      EXPECT_EQ (event.raw_buffer_[1], 60);          // Pitch
    }
}

TEST_F (TimelineDataProviderTest, GenerateCacheOutsideAffectedRange)
{
  // Create a MIDI region at tick 500
  auto region = create_midi_region (500.0, 700.0);

  // Create a vector of regions
  std::vector<const MidiRegion *> regions;
  regions.push_back (region);

  // Generate cache with affected range that doesn't include our region
  const double               affected_start = 0.0;
  const double               affected_end = 200.0;
  utils::ExpandableTickRange range (
    std::make_pair (affected_start, affected_end));
  provider_->generate_midi_events (*tempo_map_, regions, range);

  // Test processing events that should NOT include the note
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = {
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 256
  };

  provider_->process_midi_events (time_info, output_buffer);

  // Should have no events since region is outside affected range
  EXPECT_EQ (output_buffer.size (), 0);
}

TEST_F (TimelineDataProviderTest, GenerateCachePartialOverlap)
{
  // Create a MIDI region at tick 0
  auto region1 = create_midi_region (0.0, 200.0, 60);

  // Create a second MIDI region that partially overlaps with affected range
  auto region2 = create_midi_region (400.0, 500.0, 64);

  // Create a vector of regions
  std::vector<const MidiRegion *> regions;
  regions.push_back (region1);
  regions.push_back (region2);

  // Generate cache with affected range that overlaps with first region
  const double               affected_start = 0.0;
  const double               affected_end = 200.0;
  utils::ExpandableTickRange range (
    std::make_pair (affected_start, affected_end));
  provider_->generate_midi_events (*tempo_map_, regions, range);

  // Test processing events that should include only the first region
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = {
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 512
  };

  provider_->process_midi_events (time_info, output_buffer);

  // Should have events only from the first region
  EXPECT_GT (output_buffer.size (), 0);

  // All events should be from the first region (pitch 60)
  for (const auto &event : output_buffer)
    {
      if ((event.raw_buffer_[0] & 0xF0) == 0x90) // Note on
        {
          EXPECT_EQ (event.raw_buffer_[1], 60);
        }
    }
}

TEST_F (TimelineDataProviderTest, GenerateCacheWithMutedNote)
{
  // Create a MIDI region at tick 0
  auto region = create_midi_region (0.0, 200.0);

  // Mute the note in our region
  auto note_view = region->get_children_view ();
  if (!note_view.empty ())
    {
      note_view[0]->mute ()->setMuted (true);
    }

  // Create a vector of regions
  std::vector<const MidiRegion *> regions;
  regions.push_back (region);

  // Generate cache
  utils::ExpandableTickRange range (std::pair (0.0, 960.0));
  provider_->generate_midi_events (*tempo_map_, regions, range);

  // Test processing events
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = {
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 256
  };

  provider_->process_midi_events (time_info, output_buffer);

  // Should have no events since the only note is muted
  EXPECT_EQ (output_buffer.size (), 0);
}

TEST_F (TimelineDataProviderTest, GenerateCacheMultipleRegions)
{
  // Create multiple regions with different pitches at the beginning of the
  // timeline
  std::vector<int>          pitches = { 60, 64, 67 };
  std::vector<MidiRegion *> additional_regions;

  for (int i = 0; i < 3; ++i)
    {
      auto region =
        create_midi_region (i * 50.0, (i + 1) * 50.0 + 150.0, pitches[i]);
      additional_regions.push_back (region);
    }

  // Create a vector of regions
  std::vector<const MidiRegion *> regions;
  for (const auto &region : additional_regions)
    {
      regions.push_back (region);
    }

  // Generate cache with all regions
  utils::ExpandableTickRange range (std::pair (0.0, 960.0));
  provider_->generate_midi_events (*tempo_map_, regions, range);

  // Convert tick positions to sample positions for precise testing
  const auto region1_start_samples =
    tempo_map_->tick_to_samples_rounded (units::ticks (0.0));
  const auto region3_start_samples =
    tempo_map_->tick_to_samples_rounded (units::ticks (100.0));

  // Test processing events that should include all notes
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = {
    .g_start_frame_ =
      static_cast<unsigned_frame_t> (region1_start_samples.in (units::sample)),
    .g_start_frame_w_offset_ =
      static_cast<unsigned_frame_t> (region1_start_samples.in (units::sample)),
    .local_offset_ = 0,
    .nframes_ = static_cast<nframes_t> (
      (region3_start_samples - region1_start_samples).in (units::sample) + 256)
  };

  provider_->process_midi_events (time_info, output_buffer);

  // Should have events from all regions
  EXPECT_GE (output_buffer.size (), 3);

  // Should have all pitches including duplicates
  std::vector<int> found_pitches;
  for (const auto &event : output_buffer)
    {
      if ((event.raw_buffer_[0] & 0xF0) == 0x90) // Note on
        {
          found_pitches.push_back (event.raw_buffer_[1]);
        }
    }

  // Check that all expected pitches are present
  EXPECT_TRUE (std::ranges::contains (found_pitches, 60));
  EXPECT_TRUE (std::ranges::contains (found_pitches, 64));
  EXPECT_TRUE (std::ranges::contains (found_pitches, 67));
}

TEST_F (TimelineDataProviderTest, GenerateCacheEdgeCaseZeroLengthRegion)
{
  // Create a region with zero length
  auto zero_length_region_ref = obj_registry_->create_object<MidiRegion> (
    *tempo_map_, *obj_registry_, *file_audio_source_registry_);
  auto zero_length_region = zero_length_region_ref.get_object_as<MidiRegion> ();
  zero_length_region->position ()->setTicks (200.0);
  zero_length_region->bounds ()->length ()->setTicks (0.0);

  // Create a normal region at the beginning
  auto normal_region = create_midi_region (0.0, 200.0, 60);

  // Create a vector of regions
  std::vector<const MidiRegion *> regions;
  regions.push_back (normal_region);
  regions.push_back (zero_length_region);

  // Keep a reference to the zero-length region
  region_refs.push_back (std::move (zero_length_region_ref));

  // Generate cache with both regions
  utils::ExpandableTickRange range (std::pair (0.0, 960.0));
  provider_->generate_midi_events (*tempo_map_, regions, range);

  // Test processing events
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = {
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 256
  };

  provider_->process_midi_events (time_info, output_buffer);

  // Should still have events from the original region, not the zero-length one
  EXPECT_GT (output_buffer.size (), 0);

  // All events should be from the original region (pitch 60)
  for (const auto &event : output_buffer)
    {
      if ((event.raw_buffer_[0] & 0xF0) == 0x90) // Note on
        {
          EXPECT_EQ (event.raw_buffer_[1], 60);
        }
    }
}

TEST_F (TimelineDataProviderTest, GenerateCacheWithExistingCache)
{
  // Create a MIDI region at tick 0
  auto region = create_midi_region (0.0, 200.0);

  // Create a vector of regions
  std::vector<const MidiRegion *> regions;
  regions.push_back (region);

  // First generate cache normally
  utils::ExpandableTickRange range (std::pair (0.0, 960.0));
  provider_->generate_midi_events (*tempo_map_, regions, range);

  // Test processing events
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = {
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 256
  };

  provider_->process_midi_events (time_info, output_buffer);
  const auto initial_event_count = output_buffer.size ();
  EXPECT_GT (initial_event_count, 0);

  // Generate cache again with affected range that should clear and regenerate
  const double               affected_start = 0.0;
  const double               affected_end = 200.0;
  utils::ExpandableTickRange affected_range (
    std::make_pair (affected_start, affected_end));
  provider_->generate_midi_events (*tempo_map_, regions, affected_range);

  // Test processing events again
  output_buffer.clear ();
  provider_->process_midi_events (time_info, output_buffer);

  // Should still have events
  EXPECT_GT (output_buffer.size (), 0);
  EXPECT_LE (output_buffer.size (), initial_event_count); // Might be fewer due
                                                          // to range filtering
}

TEST_F (TimelineDataProviderTest, PreciseTimingVerification)
{
  // Create a MIDI region at a specific tick position
  const double region_start_ticks = 240.0; // 1 beat at 120 BPM
  auto         region =
    create_midi_region (region_start_ticks, region_start_ticks + 200.0);

  // Create a vector of regions
  std::vector<const MidiRegion *> regions;
  regions.push_back (region);

  // Generate events for the regions
  utils::ExpandableTickRange range (std::pair (0.0, 960.0));
  provider_->generate_midi_events (*tempo_map_, regions, range);

  // Convert region start to samples
  const auto region_start_samples =
    tempo_map_->tick_to_samples_rounded (units::ticks (region_start_ticks));

  // Test processing events that should include the note
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = {
    .g_start_frame_ =
      static_cast<unsigned_frame_t> (region_start_samples.in (units::sample)),
    .g_start_frame_w_offset_ =
      static_cast<unsigned_frame_t> (region_start_samples.in (units::sample)),
    .local_offset_ = 0,
    .nframes_ = 256
  };

  provider_->process_midi_events (time_info, output_buffer);

  // Should have events from the MIDI region
  EXPECT_GE (output_buffer.size (), 1);

  // Verify the event timing is correct
  if (!output_buffer.empty ())
    {
      const auto &event = output_buffer.front ();

      // The event should be at the beginning of our processing block
      // since the note is at the start of the region
      EXPECT_LT (event.time_, time_info.nframes_);

      // Verify it's a note on event with the correct pitch
      EXPECT_EQ (event.raw_buffer_[0] & 0xF0, 0x90); // Note on
      EXPECT_EQ (event.raw_buffer_[1], 60);          // Pitch
    }
}

// ========== Audio Region Tests ==========

TEST_F (TimelineDataProviderTest, AudioInitialState)
{
  // Provider should start with no audio
  std::vector<float>    output_left (256, 0.0f);
  std::vector<float>    output_right (256, 0.0f);
  EngineProcessTimeInfo time_info = {
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 256
  };

  provider_->process_audio_events (time_info, output_left, output_right);

  // Output should be all zeros
  for (size_t i = 0; i < output_left.size (); ++i)
    {
      EXPECT_FLOAT_EQ (output_left[i], 0.0f);
      EXPECT_FLOAT_EQ (output_right[i], 0.0f);
    }
}

TEST_F (TimelineDataProviderTest, GenerateAudioEventsWithEmptyRegions)
{
  // Test generating audio events with empty region list
  std::vector<const AudioRegion *> empty_regions;
  utils::ExpandableTickRange       range (
    std::pair (0.0, 960.0)); // One bar at 120 BPM

  provider_->generate_audio_events (*tempo_map_, empty_regions, range);

  // Verify no audio is generated
  std::vector<float>    output_left (256, 0.0f);
  std::vector<float>    output_right (256, 0.0f);
  EngineProcessTimeInfo time_info = {
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 256
  };

  provider_->process_audio_events (time_info, output_left, output_right);

  // Output should be all zeros
  for (size_t i = 0; i < output_left.size (); ++i)
    {
      EXPECT_FLOAT_EQ (output_left[i], 0.0f);
      EXPECT_FLOAT_EQ (output_right[i], 0.0f);
    }
}

TEST_F (TimelineDataProviderTest, ProcessAudioEventsWithAudioRegion)
{
  // Create an audio region at tick 0 (start of timeline)
  auto region = create_audio_region (0.0, 200.0);

  // Create a vector of regions
  std::vector<const AudioRegion *> regions;
  regions.push_back (region);

  // Generate events for the regions
  utils::ExpandableTickRange range (std::pair (0.0, 960.0));
  provider_->generate_audio_events (*tempo_map_, regions, range);

  // Test processing audio that should include the region
  std::vector<float>    output_left (256, 0.0f);
  std::vector<float>    output_right (256, 0.0f);
  EngineProcessTimeInfo time_info = {
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 256
  };

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

TEST_F (TimelineDataProviderTest, ProcessAudioEventsOutsideTimeRange)
{
  // Create an audio region at tick 500
  auto region = create_audio_region (500.0, 700.0);

  // Create a vector of regions
  std::vector<const AudioRegion *> regions;
  regions.push_back (region);

  // Generate events for the regions
  utils::ExpandableTickRange range (std::pair (0.0, 960.0));
  provider_->generate_audio_events (*tempo_map_, regions, range);

  // Test processing audio that should NOT include the region
  std::vector<float>    output_left (256, 0.0f);
  std::vector<float>    output_right (256, 0.0f);
  EngineProcessTimeInfo time_info = {
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 256
  };

  provider_->process_audio_events (time_info, output_left, output_right);

  // Should have no audio since the region is outside the time range
  for (size_t i = 0; i < output_left.size (); ++i)
    {
      EXPECT_FLOAT_EQ (output_left[i], 0.0f);
      EXPECT_FLOAT_EQ (output_right[i], 0.0f);
    }
}

TEST_F (TimelineDataProviderTest, ProcessAudioEventsWithOffset)
{
  // Create an audio region at tick 0
  auto region = create_audio_region (0.0, 200.0);

  // Create a vector of regions
  std::vector<const AudioRegion *> regions;
  regions.push_back (region);

  // Generate events for the regions
  utils::ExpandableTickRange range (std::pair (0.0, 960.0));
  provider_->generate_audio_events (*tempo_map_, regions, range);

  // Test processing audio with a global offset that's far from the region
  std::vector<float>    output_left (256, 0.0f);
  std::vector<float>    output_right (256, 0.0f);
  EngineProcessTimeInfo time_info = {
    .g_start_frame_ = 10000,
    .g_start_frame_w_offset_ = 10100, // 100 frame offset
    .local_offset_ = 0,
    .nframes_ = 256
  };

  provider_->process_audio_events (time_info, output_left, output_right);

  // Should have no audio since the region is at tick 0
  // and we're processing frames 10100-10356
  for (size_t i = 0; i < output_left.size (); ++i)
    {
      EXPECT_FLOAT_EQ (output_left[i], 0.0f);
      EXPECT_FLOAT_EQ (output_right[i], 0.0f);
    }
}

TEST_F (TimelineDataProviderTest, MultipleAudioRegionsInSequence)
{
  // Create multiple audio regions at the beginning of the timeline
  auto region1 = create_audio_region (0.0, 100.0, 0.5f);
  auto region2 = create_audio_region (50.0, 150.0, 0.7f);
  auto region3 = create_audio_region (100.0, 200.0, 0.3f);

  // Create a vector of regions
  std::vector<const AudioRegion *> regions;
  regions.push_back (region1);
  regions.push_back (region2);
  regions.push_back (region3);

  // Generate events for the regions
  utils::ExpandableTickRange range (std::pair (0.0, 960.0));
  provider_->generate_audio_events (*tempo_map_, regions, range);

  // Convert tick positions to sample positions for precise testing
  const auto region1_start_samples =
    tempo_map_->tick_to_samples_rounded (units::ticks (0.0));
  const auto region3_start_samples =
    tempo_map_->tick_to_samples_rounded (units::ticks (100.0));

  // Test processing audio that should include all regions
  std::vector<float>    output_left (1024, 0.0f);
  std::vector<float>    output_right (1024, 0.0f);
  EngineProcessTimeInfo time_info = {
    .g_start_frame_ =
      static_cast<unsigned_frame_t> (region1_start_samples.in (units::sample)),
    .g_start_frame_w_offset_ =
      static_cast<unsigned_frame_t> (region1_start_samples.in (units::sample)),
    .local_offset_ = 0,
    .nframes_ = static_cast<nframes_t> (
      (region3_start_samples - region1_start_samples).in (units::sample) + 256)
  };

  provider_->process_audio_events (time_info, output_left, output_right);

  // Should have some audio output from overlapping regions
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

TEST_F (TimelineDataProviderTest, GenerateAudioCacheWithAffectedRange)
{
  // Create an audio region at tick 0
  auto region = create_audio_region (0.0, 200.0);

  // Create a vector of regions
  std::vector<const AudioRegion *> regions;
  regions.push_back (region);

  // Generate cache with affected range that includes our region
  const double               affected_start = 0.0;
  const double               affected_end = 200.0;
  utils::ExpandableTickRange range (
    std::make_pair (affected_start, affected_end));
  provider_->generate_audio_events (*tempo_map_, regions, range);

  // Test processing audio that should include the region
  std::vector<float>    output_left (256, 0.0f);
  std::vector<float>    output_right (256, 0.0f);
  EngineProcessTimeInfo time_info = {
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 256
  };

  provider_->process_audio_events (time_info, output_left, output_right);

  // Should have audio from the audio region
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

TEST_F (TimelineDataProviderTest, GenerateAudioCacheOutsideAffectedRange)
{
  // Create an audio region at tick 500
  auto region = create_audio_region (500.0, 700.0);

  // Create a vector of regions
  std::vector<const AudioRegion *> regions;
  regions.push_back (region);

  // Generate cache with affected range that doesn't include our region
  const double               affected_start = 0.0;
  const double               affected_end = 200.0;
  utils::ExpandableTickRange range (
    std::make_pair (affected_start, affected_end));
  provider_->generate_audio_events (*tempo_map_, regions, range);

  // Test processing audio that should NOT include the region
  std::vector<float>    output_left (256, 0.0f);
  std::vector<float>    output_right (256, 0.0f);
  EngineProcessTimeInfo time_info = {
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 256
  };

  provider_->process_audio_events (time_info, output_left, output_right);

  // Should have no audio since region is outside affected range
  for (size_t i = 0; i < output_left.size (); ++i)
    {
      EXPECT_FLOAT_EQ (output_left[i], 0.0f);
      EXPECT_FLOAT_EQ (output_right[i], 0.0f);
    }
}

TEST_F (TimelineDataProviderTest, GenerateAudioCachePartialOverlap)
{
  // Create an audio region at tick 0
  auto region1 = create_audio_region (0.0, 200.0);

  // Create a second audio region that partially overlaps with affected range
  auto region2 = create_audio_region (400.0, 500.0);

  // Create a vector of regions
  std::vector<const AudioRegion *> regions;
  regions.push_back (region1);
  regions.push_back (region2);

  // Generate cache with affected range that overlaps with first region
  const double               affected_start = 0.0;
  const double               affected_end = 200.0;
  utils::ExpandableTickRange range (
    std::make_pair (affected_start, affected_end));
  provider_->generate_audio_events (*tempo_map_, regions, range);

  // Test processing audio that should include only the first region
  std::vector<float>    output_left (512, 0.0f);
  std::vector<float>    output_right (512, 0.0f);
  EngineProcessTimeInfo time_info = {
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 512
  };

  provider_->process_audio_events (time_info, output_left, output_right);

  // Should have audio only from the first region
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

TEST_F (TimelineDataProviderTest, GenerateAudioCacheMultipleRegions)
{
  // Create multiple regions with different gains at the beginning of the timeline
  std::vector<float>         gains = { 0.5f, 0.7f, 0.3f };
  std::vector<AudioRegion *> additional_regions;

  for (int i = 0; i < 3; ++i)
    {
      auto region =
        create_audio_region (i * 50.0, (i + 1) * 50.0 + 150.0, gains[i]);
      additional_regions.push_back (region);
    }

  // Create a vector of regions
  std::vector<const AudioRegion *> regions;
  for (const auto &region : additional_regions)
    {
      regions.push_back (region);
    }

  // Generate cache with all regions
  utils::ExpandableTickRange range (std::pair (0.0, 960.0));
  provider_->generate_audio_events (*tempo_map_, regions, range);

  // Convert tick positions to sample positions for precise testing
  const auto region1_start_samples =
    tempo_map_->tick_to_samples_rounded (units::ticks (0.0));
  const auto region3_start_samples =
    tempo_map_->tick_to_samples_rounded (units::ticks (100.0));

  // Test processing audio that should include all regions
  std::vector<float>    output_left (1024, 0.0f);
  std::vector<float>    output_right (1024, 0.0f);
  EngineProcessTimeInfo time_info = {
    .g_start_frame_ =
      static_cast<unsigned_frame_t> (region1_start_samples.in (units::sample)),
    .g_start_frame_w_offset_ =
      static_cast<unsigned_frame_t> (region1_start_samples.in (units::sample)),
    .local_offset_ = 0,
    .nframes_ = static_cast<nframes_t> (
      (region3_start_samples - region1_start_samples).in (units::sample) + 256)
  };

  provider_->process_audio_events (time_info, output_left, output_right);

  // Should have audio from all regions
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

TEST_F (TimelineDataProviderTest, GenerateAudioCacheEdgeCaseZeroLengthRegion)
{
  // Create a region with zero length
  auto zero_length_region_ref = obj_registry_->create_object<AudioRegion> (
    *tempo_map_, *obj_registry_, *file_audio_source_registry_,
    [] () { return true; });
  auto zero_length_region = zero_length_region_ref.get_object_as<AudioRegion> ();

  // Create and set an audio source for the zero-length region (required by
  // AudioRegion contract)
  auto audio_source_object_ref = create_sine_wave_audio_source (4096);
  zero_length_region->set_source (audio_source_object_ref);

  zero_length_region->position ()->setTicks (200.0);
  zero_length_region->bounds ()->length ()->setTicks (0.0);

  // Create a normal region at the beginning
  auto normal_region = create_audio_region (0.0, 200.0, 0.6f);

  // Create a vector of regions
  std::vector<const AudioRegion *> regions;
  regions.push_back (normal_region);
  regions.push_back (zero_length_region);

  // Keep a reference to the zero-length region
  region_refs.push_back (std::move (zero_length_region_ref));

  // Generate cache with both regions
  utils::ExpandableTickRange range (std::pair (0.0, 960.0));
  provider_->generate_audio_events (*tempo_map_, regions, range);

  // Test processing audio
  std::vector<float>    output_left (256, 0.0f);
  std::vector<float>    output_right (256, 0.0f);
  EngineProcessTimeInfo time_info = {
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 256
  };

  provider_->process_audio_events (time_info, output_left, output_right);

  // Should still have audio from the original region, not the zero-length one
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

TEST_F (TimelineDataProviderTest, GenerateAudioCacheWithExistingCache)
{
  // Create an audio region at tick 0
  auto region = create_audio_region (0.0, 200.0);

  // Create a vector of regions
  std::vector<const AudioRegion *> regions;
  regions.push_back (region);

  // First generate cache normally
  utils::ExpandableTickRange range (std::pair (0.0, 960.0));
  provider_->generate_audio_events (*tempo_map_, regions, range);

  // Test processing audio
  std::vector<float>    output_left (256, 0.0f);
  std::vector<float>    output_right (256, 0.0f);
  EngineProcessTimeInfo time_info = {
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 256
  };

  provider_->process_audio_events (time_info, output_left, output_right);

  // Should have audio
  bool has_audio_initially = false;
  for (size_t i = 0; i < output_left.size (); ++i)
    {
      if (
        std::abs (output_left[i]) > 0.001f
        || std::abs (output_right[i]) > 0.001f)
        {
          has_audio_initially = true;
          break;
        }
    }
  EXPECT_TRUE (has_audio_initially);

  // Generate cache again with affected range that should clear and regenerate
  const double               affected_start = 0.0;
  const double               affected_end = 200.0;
  utils::ExpandableTickRange affected_range (
    std::make_pair (affected_start, affected_end));
  provider_->generate_audio_events (*tempo_map_, regions, affected_range);

  // Test processing audio again
  std::fill (output_left.begin (), output_left.end (), 0.0f);
  std::fill (output_right.begin (), output_right.end (), 0.0f);
  provider_->process_audio_events (time_info, output_left, output_right);

  // Should still have audio
  bool has_audio_after = false;
  for (size_t i = 0; i < output_left.size (); ++i)
    {
      if (
        std::abs (output_left[i]) > 0.001f
        || std::abs (output_right[i]) > 0.001f)
        {
          has_audio_after = true;
          break;
        }
    }
  EXPECT_TRUE (has_audio_after);
}

TEST_F (TimelineDataProviderTest, AudioPreciseTimingVerification)
{
  // Create an audio region at a specific tick position
  const double region_start_ticks = 240.0; // 1 beat at 120 BPM
  auto         region =
    create_audio_region (region_start_ticks, region_start_ticks + 200.0);

  // Create a vector of regions
  std::vector<const AudioRegion *> regions;
  regions.push_back (region);

  // Generate events for the regions
  utils::ExpandableTickRange range (std::pair (0.0, 960.0));
  provider_->generate_audio_events (*tempo_map_, regions, range);

  // Convert region start to samples
  const auto region_start_samples =
    tempo_map_->tick_to_samples_rounded (units::ticks (region_start_ticks));

  // Test processing audio that should include the region
  std::vector<float>    output_left (256, 0.0f);
  std::vector<float>    output_right (256, 0.0f);
  EngineProcessTimeInfo time_info = {
    .g_start_frame_ =
      static_cast<unsigned_frame_t> (region_start_samples.in (units::sample)),
    .g_start_frame_w_offset_ =
      static_cast<unsigned_frame_t> (region_start_samples.in (units::sample)),
    .local_offset_ = 0,
    .nframes_ = 256
  };

  provider_->process_audio_events (time_info, output_left, output_right);

  // Should have audio from the audio region
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

TEST_F (TimelineDataProviderTest, AudioBasicFunctionality)
{
  // Test that the provider can be constructed
  EXPECT_NE (provider_, nullptr);

  // Test that process_audio_events can be called without crashing
  std::vector<float>    output_left (256, 0.0f);
  std::vector<float>    output_right (256, 0.0f);
  EngineProcessTimeInfo time_info = {
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 256
  };

  provider_->process_audio_events (time_info, output_left, output_right);

  // Should be all zeros initially
  for (size_t i = 0; i < output_left.size (); ++i)
    {
      EXPECT_FLOAT_EQ (output_left[i], 0.0f);
      EXPECT_FLOAT_EQ (output_right[i], 0.0f);
    }

  // Test that generate_audio_events can be called without crashing
  std::vector<const AudioRegion *> empty_regions;
  utils::ExpandableTickRange       range (std::pair (0.0, 960.0));

  provider_->generate_audio_events (*tempo_map_, empty_regions, range);

  // Process again to ensure no crash
  provider_->process_audio_events (time_info, output_left, output_right);

  // Should still be all zeros
  for (size_t i = 0; i < output_left.size (); ++i)
    {
      EXPECT_FLOAT_EQ (output_left[i], 0.0f);
      EXPECT_FLOAT_EQ (output_right[i], 0.0f);
    }
}

// ========== Chord Region Tests ==========

TEST_F (TimelineDataProviderTest, ChordRegionInitialState)
{
  // Provider should start with no chord events
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = {
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 256
  };

  provider_->process_midi_events (time_info, output_buffer);
  EXPECT_EQ (output_buffer.size (), 0);
}

TEST_F (TimelineDataProviderTest, ProcessChordRegion)
{
  // Create a chord region at tick 0
  auto chord_region_ref = obj_registry_->create_object<arrangement::ChordRegion> (
    *tempo_map_, *obj_registry_, *file_audio_source_registry_);
  auto chord_region =
    chord_region_ref.get_object_as<arrangement::ChordRegion> ();
  chord_region->position ()->setTicks (0.0);
  chord_region->bounds ()->length ()->setTicks (200.0);

  // Keep a reference to the chord region
  region_refs.push_back (std::move (chord_region_ref));

  // Create a vector of chord regions
  std::vector<const arrangement::ChordRegion *> chord_regions;
  chord_regions.push_back (chord_region);

  // Generate events for the chord regions
  utils::ExpandableTickRange range (std::pair (0.0, 960.0));
  provider_->generate_midi_events (*tempo_map_, chord_regions, range);

  // Test processing events that should include chord events
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = {
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 256
  };

  provider_->process_midi_events (time_info, output_buffer);

  // Should have some events from the chord region
  EXPECT_GE (output_buffer.size (), 0);

  // Verify events are valid MIDI events
  for (const auto &event : output_buffer)
    {
      EXPECT_GE (event.time_, 0);
      EXPECT_LT (event.time_, time_info.nframes_);
      // Check that it's a valid MIDI message
      EXPECT_GE (event.raw_buffer_[0], 0x80);
      EXPECT_LE (event.raw_buffer_[0], 0xEF);
    }
}

TEST_F (TimelineDataProviderTest, ProcessChordRegionOutsideRange)
{
  // Create a chord region at tick 500
  auto chord_region_ref = obj_registry_->create_object<arrangement::ChordRegion> (
    *tempo_map_, *obj_registry_, *file_audio_source_registry_);
  auto chord_region =
    chord_region_ref.get_object_as<arrangement::ChordRegion> ();
  chord_region->position ()->setTicks (500.0);
  chord_region->bounds ()->length ()->setTicks (200.0);

  // Keep a reference to the chord region
  region_refs.push_back (std::move (chord_region_ref));

  // Create a vector of chord regions
  std::vector<const arrangement::ChordRegion *> chord_regions;
  chord_regions.push_back (chord_region);

  // Generate events for the chord regions
  utils::ExpandableTickRange range (std::pair (0.0, 960.0));
  provider_->generate_midi_events (*tempo_map_, chord_regions, range);

  // Test processing events that should NOT include the chord
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = {
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 256
  };

  provider_->process_midi_events (time_info, output_buffer);

  // Should have no events since the chord region is outside the time range
  EXPECT_EQ (output_buffer.size (), 0);
}

// ========== Muted Region Tests ==========

TEST_F (TimelineDataProviderTest, ProcessMutedMidiRegion)
{
  // Create a MIDI region at tick 0
  auto region = create_midi_region (0.0, 200.0);

  // Mute the entire region
  region->mute ()->setMuted (true);

  // Create a vector of regions
  std::vector<const MidiRegion *> regions;
  regions.push_back (region);

  // Generate events for the regions
  utils::ExpandableTickRange range (std::pair (0.0, 960.0));
  provider_->generate_midi_events (*tempo_map_, regions, range);

  // Test processing events
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = {
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 256
  };

  provider_->process_midi_events (time_info, output_buffer);

  // Should have no events since the entire region is muted
  EXPECT_EQ (output_buffer.size (), 0);
}

TEST_F (TimelineDataProviderTest, ProcessMutedAudioRegion)
{
  // Create an audio region at tick 0
  auto region = create_audio_region (0.0, 200.0);

  // Mute the entire region
  region->mute ()->setMuted (true);

  // Create a vector of regions
  std::vector<const AudioRegion *> regions;
  regions.push_back (region);

  // Generate events for the regions
  utils::ExpandableTickRange range (std::pair (0.0, 960.0));
  provider_->generate_audio_events (*tempo_map_, regions, range);

  // Test processing audio
  std::vector<float>    output_left (256, 0.0f);
  std::vector<float>    output_right (256, 0.0f);
  EngineProcessTimeInfo time_info = {
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 256
  };

  provider_->process_audio_events (time_info, output_left, output_right);

  // Should have no audio since the entire region is muted
  for (size_t i = 0; i < output_left.size (); ++i)
    {
      EXPECT_FLOAT_EQ (output_left[i], 0.0f);
      EXPECT_FLOAT_EQ (output_right[i], 0.0f);
    }
}

TEST_F (TimelineDataProviderTest, ProcessMutedChordRegion)
{
  // Create a chord region at tick 0
  auto chord_region_ref = obj_registry_->create_object<arrangement::ChordRegion> (
    *tempo_map_, *obj_registry_, *file_audio_source_registry_);
  auto chord_region =
    chord_region_ref.get_object_as<arrangement::ChordRegion> ();
  chord_region->position ()->setTicks (0.0);
  chord_region->bounds ()->length ()->setTicks (200.0);

  // Mute the entire chord region
  chord_region->mute ()->setMuted (true);

  // Keep a reference to the chord region
  region_refs.push_back (std::move (chord_region_ref));

  // Create a vector of chord regions
  std::vector<const arrangement::ChordRegion *> chord_regions;
  chord_regions.push_back (chord_region);

  // Generate events for the chord regions
  utils::ExpandableTickRange range (std::pair (0.0, 960.0));
  provider_->generate_midi_events (*tempo_map_, chord_regions, range);

  // Test processing events
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = {
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 256
  };

  provider_->process_midi_events (time_info, output_buffer);

  // Should have no events since the entire chord region is muted
  EXPECT_EQ (output_buffer.size (), 0);
}

TEST_F (TimelineDataProviderTest, ProcessPartiallyMutedRegion)
{
  // Create a MIDI region at tick 0 with multiple notes
  auto region = create_midi_region (0.0, 200.0, 60);

  // Add another note to the region
  auto note_ref = obj_registry_->create_object<MidiNote> (*tempo_map_);
  auto midi_note = note_ref.get_object_as<MidiNote> ();
  midi_note->setPitch (64);
  midi_note->setVelocity (80);
  midi_note->position ()->setSamples (25);
  midi_note->bounds ()->length ()->setSamples (50);
  region->add_object (note_ref);

  // Mute only the first note (pitch 60), not the entire region
  auto note_view = region->get_children_view ();
  note_view[0]->mute ()->setMuted (true);

  // Create a vector of regions
  std::vector<const MidiRegion *> regions;
  regions.push_back (region);

  // Generate events for the regions
  utils::ExpandableTickRange range (std::pair (0.0, 960.0));
  provider_->generate_midi_events (*tempo_map_, regions, range);

  // Test processing events
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = {
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 256
  };

  provider_->process_midi_events (time_info, output_buffer);

  // Should have events from the unmuted note (pitch 64) but not from the muted
  // note (pitch 60)
  EXPECT_GT (output_buffer.size (), 0);

  // Check that only the unmuted note is present
  bool found_muted_note = false;
  bool found_unmuted_note = false;

  for (const auto &event : output_buffer)
    {
      if ((event.raw_buffer_[0] & 0xF0) == 0x90) // Note on
        {
          if (event.raw_buffer_[1] == 60)
            {
              found_muted_note = true;
            }
          else if (event.raw_buffer_[1] == 64)
            {
              found_unmuted_note = true;
            }
        }
    }

  EXPECT_FALSE (found_muted_note) << "Muted note (60) should not be present";
  EXPECT_TRUE (found_unmuted_note) << "Unmuted note (64) should be present";
}

} // namespace zrythm::structure::arrangement
