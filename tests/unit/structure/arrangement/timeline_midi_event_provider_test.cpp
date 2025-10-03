// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/midi_event.h"
#include "dsp/tempo_map.h"
#include "structure/arrangement/midi_note.h"
#include "structure/arrangement/midi_region.h"
#include "structure/arrangement/timeline_midi_event_provider.h"
#include "utils/types.h"

#include "gtest/gtest.h"
#include <juce_audio_formats/juce_audio_formats.h>

namespace zrythm::structure::arrangement
{

class TimelineMidiEventProviderTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    // Create the provider
    provider_ = std::make_unique<TimelineMidiEventProvider> ();

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

  std::unique_ptr<TimelineMidiEventProvider>    provider_;
  std::unique_ptr<dsp::TempoMap>                tempo_map_;
  std::unique_ptr<ArrangerObjectRegistry>       obj_registry_;
  std::unique_ptr<dsp::FileAudioSourceRegistry> file_audio_source_registry_;
  std::vector<ArrangerObjectUuidReference>      region_refs; // Keep references
};

TEST_F (TimelineMidiEventProviderTest, InitialState)
{
  // Provider should start with no events
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = {
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 256
  };

  provider_->process_events (time_info, output_buffer);
  EXPECT_EQ (output_buffer.size (), 0);
}

TEST_F (TimelineMidiEventProviderTest, ProcessEventsWithNoEvents)
{
  // Test processing when no events are available
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = {
    .g_start_frame_ = 1000,
    .g_start_frame_w_offset_ = 1000,
    .local_offset_ = 0,
    .nframes_ = 512
  };

  provider_->process_events (time_info, output_buffer);
  EXPECT_EQ (output_buffer.size (), 0);
}

TEST_F (TimelineMidiEventProviderTest, GenerateEventsWithEmptyRegions)
{
  // Test generating events with empty region list
  std::vector<const MidiRegion *> empty_regions;
  utils::ExpandableTickRange      range (
    std::pair (0.0, 960.0)); // One bar at 120 BPM

  provider_->generate_events (*tempo_map_, empty_regions, range);

  // Verify no events are generated
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = {
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 256
  };

  provider_->process_events (time_info, output_buffer);
  EXPECT_EQ (output_buffer.size (), 0);
}

TEST_F (TimelineMidiEventProviderTest, ProcessEventsWithMidiRegion)
{
  // Create a MIDI region at tick 0 (start of timeline)
  auto region = create_midi_region (0.0, 200.0);

  // Create a vector of regions
  std::vector<const MidiRegion *> regions;
  regions.push_back (region);

  // Generate events for the regions
  utils::ExpandableTickRange range (std::pair (0.0, 960.0));
  provider_->generate_events (*tempo_map_, regions, range);

  // Test processing events that should include the note
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = {
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 256
  };

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

TEST_F (TimelineMidiEventProviderTest, ProcessEventsOutsideTimeRange)
{
  // Create a MIDI region at tick 500
  auto region = create_midi_region (500.0, 700.0);

  // Create a vector of regions
  std::vector<const MidiRegion *> regions;
  regions.push_back (region);

  // Generate events for the regions
  utils::ExpandableTickRange range (std::pair (0.0, 960.0));
  provider_->generate_events (*tempo_map_, regions, range);

  // Test processing events that should NOT include the note
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = {
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 256
  };

  provider_->process_events (time_info, output_buffer);

  // Should have no events since the note is outside the time range
  EXPECT_EQ (output_buffer.size (), 0);
}

TEST_F (TimelineMidiEventProviderTest, ProcessEventsWithOffset)
{
  // Create a MIDI region at tick 0
  auto region = create_midi_region (0.0, 200.0);

  // Create a vector of regions
  std::vector<const MidiRegion *> regions;
  regions.push_back (region);

  // Generate events for the regions
  utils::ExpandableTickRange range (std::pair (0.0, 960.0));
  provider_->generate_events (*tempo_map_, regions, range);

  // Test processing events with a global offset that's far from the note
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = {
    .g_start_frame_ = 10000,
    .g_start_frame_w_offset_ = 10100, // 100 frame offset
    .local_offset_ = 0,
    .nframes_ = 256
  };

  provider_->process_events (time_info, output_buffer);

  // Should have no events since the note is at tick 0
  // and we're processing frames 10100-10356
  EXPECT_EQ (output_buffer.size (), 0);
}

TEST_F (TimelineMidiEventProviderTest, MultipleEventsInSequence)
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
  provider_->generate_events (*tempo_map_, regions, range);

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

TEST_F (TimelineMidiEventProviderTest, BasicFunctionality)
{
  // Test that the provider can be constructed
  EXPECT_NE (provider_, nullptr);

  // Test that process_events can be called without crashing
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = {
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 256
  };

  provider_->process_events (time_info, output_buffer);
  EXPECT_EQ (output_buffer.size (), 0);

  // Test that generate_events can be called without crashing
  std::vector<const MidiRegion *> empty_regions;
  utils::ExpandableTickRange      range (std::pair (0.0, 960.0));

  provider_->generate_events (*tempo_map_, empty_regions, range);

  // Process again to ensure no crash
  provider_->process_events (time_info, output_buffer);
  EXPECT_EQ (output_buffer.size (), 0);
}

// Tests from PlaybackCacheBuilder that are unique and test specific concerns
TEST_F (TimelineMidiEventProviderTest, GenerateCacheWithAffectedRange)
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
  provider_->generate_events (*tempo_map_, regions, range);

  // Test processing events that should include the note
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = {
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 256
  };

  provider_->process_events (time_info, output_buffer);

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

TEST_F (TimelineMidiEventProviderTest, GenerateCacheOutsideAffectedRange)
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
  provider_->generate_events (*tempo_map_, regions, range);

  // Test processing events that should NOT include the note
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = {
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 256
  };

  provider_->process_events (time_info, output_buffer);

  // Should have no events since region is outside affected range
  EXPECT_EQ (output_buffer.size (), 0);
}

TEST_F (TimelineMidiEventProviderTest, GenerateCachePartialOverlap)
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
  provider_->generate_events (*tempo_map_, regions, range);

  // Test processing events that should include only the first region
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = {
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 512
  };

  provider_->process_events (time_info, output_buffer);

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

TEST_F (TimelineMidiEventProviderTest, GenerateCacheWithMutedNote)
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
  provider_->generate_events (*tempo_map_, regions, range);

  // Test processing events
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = {
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 256
  };

  provider_->process_events (time_info, output_buffer);

  // Should have no events since the only note is muted
  EXPECT_EQ (output_buffer.size (), 0);
}

TEST_F (TimelineMidiEventProviderTest, GenerateCacheMultipleRegions)
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
  provider_->generate_events (*tempo_map_, regions, range);

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

  provider_->process_events (time_info, output_buffer);

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

TEST_F (TimelineMidiEventProviderTest, GenerateCacheEdgeCaseZeroLengthRegion)
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
  provider_->generate_events (*tempo_map_, regions, range);

  // Test processing events
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = {
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 256
  };

  provider_->process_events (time_info, output_buffer);

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

TEST_F (TimelineMidiEventProviderTest, GenerateCacheWithExistingCache)
{
  // Create a MIDI region at tick 0
  auto region = create_midi_region (0.0, 200.0);

  // Create a vector of regions
  std::vector<const MidiRegion *> regions;
  regions.push_back (region);

  // First generate cache normally
  utils::ExpandableTickRange range (std::pair (0.0, 960.0));
  provider_->generate_events (*tempo_map_, regions, range);

  // Test processing events
  dsp::MidiEventVector  output_buffer;
  EngineProcessTimeInfo time_info = {
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 256
  };

  provider_->process_events (time_info, output_buffer);
  const auto initial_event_count = output_buffer.size ();
  EXPECT_GT (initial_event_count, 0);

  // Generate cache again with affected range that should clear and regenerate
  const double               affected_start = 0.0;
  const double               affected_end = 200.0;
  utils::ExpandableTickRange affected_range (
    std::make_pair (affected_start, affected_end));
  provider_->generate_events (*tempo_map_, regions, affected_range);

  // Test processing events again
  output_buffer.clear ();
  provider_->process_events (time_info, output_buffer);

  // Should still have events
  EXPECT_GT (output_buffer.size (), 0);
  EXPECT_LE (output_buffer.size (), initial_event_count); // Might be fewer due
                                                          // to range filtering
}

TEST_F (TimelineMidiEventProviderTest, PreciseTimingVerification)
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
  provider_->generate_events (*tempo_map_, regions, range);

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

  provider_->process_events (time_info, output_buffer);

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
}
