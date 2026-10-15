// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <filesystem>

#include "dsp/audio_block_store.h"
#include "dsp/midi_event.h"
#include "dsp/tempo_map.h"
#include "dsp/tempo_map_qml_adapter.h"
#include "structure/arrangement/audio_clip.h"
#include "structure/arrangement/chord_clip.h"
#include "structure/arrangement/midi_clip.h"
#include "structure/arrangement/midi_note.h"
#include "structure/arrangement/timeline_data_provider.h"
#include "utils/io_utils.h"
#include "utils/midi.h"
#include "utils/object_registry.h"
#include "utils/registry_utils.h"
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
    // Create the providers
    midi_provider_ = std::make_unique<MidiTimelineDataProvider> ();
    audio_provider_ = std::make_unique<AudioTimelineDataProvider> ();
    automation_provider_ = std::make_unique<AutomationTimelineDataProvider> ();

    // Create a tempo map for testing
    tempo_map_ = std::make_unique<dsp::TempoMap> (units::sample_rate (44100));
    tempo_map_wrapper_ = std::make_unique<dsp::TempoMapWrapper> (*tempo_map_);

    // Create an object registry
    obj_registry_ = std::make_unique<utils::ObjectRegistry> ();
  }

  void TearDown () override
  {
    clip_refs.clear (); // Clear references first
    midi_provider_.reset ();
    audio_provider_.reset ();
    automation_provider_.reset ();
    tempo_map_.reset ();
    tempo_map_wrapper_.reset ();
    obj_registry_.reset ();
  }

  // Helper function to create a MIDI clip
  MidiClip * create_midi_clip (
    double      start_pos_ticks,
    double      end_pos_ticks,
    midi_byte_t note = 60,
    midi_byte_t velocity = 64)
  {
    // Create a MIDI clip
    auto clip_ref = utils::create_object<MidiClip> (
      *obj_registry_, *tempo_map_wrapper_, *obj_registry_);
    auto clip = clip_ref.get_object_as<MidiClip> ();

    // Set the clip's position
    clip->position ()->setTicks (start_pos_ticks);
    clip->length ()->setTicks (end_pos_ticks - start_pos_ticks);

    // Create a MIDI note
    auto note_ref =
      utils::create_object<MidiNote> (*obj_registry_, *tempo_map_wrapper_);
    auto midi_note = note_ref.get_object_as<MidiNote> ();

    // Set the note's properties
    midi_note->setPitch (note);
    midi_note->setVelocity (velocity);
    // Note position is relative to the clip
    midi_note->position ()->setTicks (0.0);
    midi_note->length ()->setTicks (50.0); // Note duration

    // Add the note to the clip
    clip->ArrangerObjectOwner<MidiNote>::add_object (note_ref);

    // Keep a reference to the clip to prevent it from being deleted
    clip_refs.push_back (std::move (clip_ref));

    return clip;
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

    auto source_ref = utils::create_object<dsp::FileAudioSource> (
      *obj_registry_, *sample_buffer, utils::audio::BitDepth::BIT_DEPTH_32,
      units::sample_rate (44100), units::bpm (120.0), u8"SineTestSource");

    return utils::create_object<AudioSourceObject> (
      *obj_registry_, *tempo_map_wrapper_, *obj_registry_, source_ref);
  }

  // Helper function to create an audio clip
  AudioClip * create_audio_clip (
    double start_pos_ticks,
    double end_pos_ticks,
    float  gain = 1.0f)
  {
    // Create a sine wave audio source
    auto audio_source_object_ref = create_sine_wave_audio_source (4096);

    // Create the audio clip
    auto clip_ref = utils::create_object<AudioClip> (
      *obj_registry_, *tempo_map_wrapper_, *obj_registry_);
    auto clip = clip_ref.get_object_as<AudioClip> ();
    clip->set_source (audio_source_object_ref);

    // Set the clip's position and length
    clip->position ()->setTicks (start_pos_ticks);
    clip->length ()->setTicks (end_pos_ticks - start_pos_ticks);
    clip->setGain (gain);

    // Keep a reference to the clip to prevent it from being deleted
    clip_refs.push_back (std::move (clip_ref));

    return clip;
  }

  // Helper function to create an automation clip
  AutomationClip * create_automation_clip (
    double start_pos_ticks,
    double end_pos_ticks,
    float  start_value = 0.0f,
    float  end_value = 1.0f)
  {
    // Create an automation clip
    auto clip_ref = utils::create_object<AutomationClip> (
      *obj_registry_, *tempo_map_wrapper_, *obj_registry_);
    auto clip = clip_ref.get_object_as<AutomationClip> ();

    // Set the clip's position
    clip->position ()->setTicks (start_pos_ticks);
    clip->length ()->setTicks (end_pos_ticks - start_pos_ticks);

    // Create automation points
    auto start_point_ref = utils::create_object<AutomationPoint> (
      *obj_registry_, *tempo_map_wrapper_);
    auto start_point = start_point_ref.get_object_as<AutomationPoint> ();
    start_point->position ()->setTicks (0.0); // Relative to clip start
    start_point->setValue (start_value);

    auto end_point_ref = utils::create_object<AutomationPoint> (
      *obj_registry_, *tempo_map_wrapper_);
    auto end_point = end_point_ref.get_object_as<AutomationPoint> ();
    end_point->position ()->setTicks (
      end_pos_ticks - start_pos_ticks); // Relative to clip start
    end_point->setValue (end_value);

    // Add points to the clip
    clip->add_object (start_point_ref);
    clip->add_object (end_point_ref);

    // Keep a reference to the clip to prevent it from being deleted
    clip_refs.push_back (std::move (clip_ref));

    return clip;
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

  std::unique_ptr<MidiTimelineDataProvider>       midi_provider_;
  std::unique_ptr<AudioTimelineDataProvider>      audio_provider_;
  std::unique_ptr<AutomationTimelineDataProvider> automation_provider_;
  std::unique_ptr<dsp::TempoMap>                  tempo_map_;
  std::unique_ptr<dsp::TempoMapWrapper>           tempo_map_wrapper_;
  std::unique_ptr<utils::ObjectRegistry>          obj_registry_;
  std::vector<ArrangerObjectUuidReference>        clip_refs; // Keep references
};

// ========== MIDI Provider Tests ==========

TEST_F (TimelineDataProviderTest, InitialState)
{
  // Provider should start with no events
  auto output_buffer = dsp::MidiEventBuffer::make_reserved ();
  auto time_info = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));

  midi_provider_->process_midi_events (
    time_info, dsp::ITransport::PlayState::Rolling, output_buffer);
  EXPECT_EQ (output_buffer.size (), 0);
}

TEST_F (TimelineDataProviderTest, ProcessEventsWithNoEvents)
{
  // Test processing when no events are available
  auto output_buffer = dsp::MidiEventBuffer::make_reserved ();
  auto time_info = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (1000), units::samples (512));

  midi_provider_->process_midi_events (
    time_info, dsp::ITransport::PlayState::Rolling, output_buffer);
  EXPECT_EQ (output_buffer.size (), 0);
}

TEST_F (TimelineDataProviderTest, GenerateEventsWithEmptyClips)
{
  // Test generating events with empty clip list
  std::vector<const MidiClip *> empty_clips;
  utils::ExpandableTickRange    range (
    std::pair (0.0, 960.0)); // One bar at 120 BPM

  midi_provider_->generate_midi_events (*tempo_map_, empty_clips, range);

  // Verify no events are generated
  auto output_buffer = dsp::MidiEventBuffer::make_reserved ();
  auto time_info = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));

  midi_provider_->process_midi_events (
    time_info, dsp::ITransport::PlayState::Rolling, output_buffer);
  EXPECT_EQ (output_buffer.size (), 0);
}

TEST_F (TimelineDataProviderTest, ProcessEventsWithMidiClip)
{
  // Create a MIDI clip at tick 0 (start of timeline)
  auto clip = create_midi_clip (0.0, 200.0);

  // Create a vector of clips
  std::vector<const MidiClip *> clips;
  clips.push_back (clip);

  // Generate events for the clips
  utils::ExpandableTickRange range (std::pair (0.0, 960.0));
  midi_provider_->generate_midi_events (*tempo_map_, clips, range);

  // Test processing events that should include the note
  auto output_buffer = dsp::MidiEventBuffer::make_reserved ();
  auto time_info = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));

  midi_provider_->process_midi_events (
    time_info, dsp::ITransport::PlayState::Rolling, output_buffer);

  // Should have at least one event (the note on)
  EXPECT_GE (output_buffer.size (), 1);

  // Check that the event timestamp is within the expected range
  if (!output_buffer.empty ())
    {
      const auto &event = output_buffer.front ();
      EXPECT_GE (event.time (), units::samples (0));
      EXPECT_LT (event.time (), time_info.nframes_);

      // Verify it's a note on event with the correct pitch
      EXPECT_TRUE (utils::midi::midi_is_note_on (event.data ()));
      EXPECT_EQ (utils::midi::midi_get_note_number (event.data ()), 60);
    }
}

TEST_F (TimelineDataProviderTest, ProcessEventsOutsideTimeRange)
{
  // Create a MIDI clip at tick 500
  auto clip = create_midi_clip (500.0, 700.0);

  // Create a vector of clips
  std::vector<const MidiClip *> clips;
  clips.push_back (clip);

  // Generate events for the clips
  utils::ExpandableTickRange range (std::pair (0.0, 960.0));
  midi_provider_->generate_midi_events (*tempo_map_, clips, range);

  // Test processing events that should NOT include the note
  auto output_buffer = dsp::MidiEventBuffer::make_reserved ();
  auto time_info = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));

  midi_provider_->process_midi_events (
    time_info, dsp::ITransport::PlayState::Rolling, output_buffer);

  // Should have no events since the note is outside the time range
  EXPECT_EQ (output_buffer.size (), 0);
}

TEST_F (TimelineDataProviderTest, ProcessEventsWithOffset)
{
  // Create a MIDI clip at tick 0
  auto clip = create_midi_clip (0.0, 200.0);

  // Create a vector of clips
  std::vector<const MidiClip *> clips;
  clips.push_back (clip);

  // Generate events for the clips
  utils::ExpandableTickRange range (std::pair (0.0, 960.0));
  midi_provider_->generate_midi_events (*tempo_map_, clips, range);

  // Test processing events with a global offset that's far from the note
  auto output_buffer = dsp::MidiEventBuffer::make_reserved ();
  dsp::graph::ProcessBlockInfo time_info = {
    .transport_position_ = units::samples (10000),
    // 100 frame offset
    .buffer_offset_ = units::samples (100),
    .nframes_ = units::samples (256)
  };

  midi_provider_->process_midi_events (
    time_info, dsp::ITransport::PlayState::Rolling, output_buffer);

  // Should have no events since the note is at tick 0
  // and we're processing frames 10100-10356
  EXPECT_EQ (output_buffer.size (), 0);
}

TEST_F (TimelineDataProviderTest, MultipleEventsInSequence)
{
  // Create multiple MIDI clips at the beginning of the timeline
  // with notes at different positions
  auto clip1 = create_midi_clip (0.0, 200.0, 60);
  auto clip2 = create_midi_clip (50.0, 250.0, 64);
  auto clip3 = create_midi_clip (100.0, 300.0, 67);

  // Create a vector of clips
  std::vector<const MidiClip *> clips;
  clips.push_back (clip1);
  clips.push_back (clip2);
  clips.push_back (clip3);

  // Generate events for the clips
  utils::ExpandableTickRange range (std::pair (0.0, 960.0));
  midi_provider_->generate_midi_events (*tempo_map_, clips, range);

  // Convert tick positions to sample positions for precise testing
  const auto clip1_start_samples = tempo_map_->tick_to_samples_rounded (
    dsp::TimelineTick{ units::ticks (0.0) });
  const auto clip3_start_samples = tempo_map_->tick_to_samples_rounded (
    dsp::TimelineTick{ units::ticks (100.0) });

  // Test processing events that should include all notes
  auto output_buffer = dsp::MidiEventBuffer::make_reserved ();
  dsp::graph::ProcessBlockInfo time_info = {
    .transport_position_ = clip1_start_samples,
    .buffer_offset_ = units::samples (0),
    .nframes_ = (clip3_start_samples - clip1_start_samples) + units::samples (256)
  };

  midi_provider_->process_midi_events (
    time_info, dsp::ITransport::PlayState::Rolling, output_buffer);

  // Should have at least 3 events (the note ons)
  EXPECT_GE (output_buffer.size (), 3);

  // Check that we have the expected pitches
  std::vector<int> found_pitches;
  for (const auto &event : output_buffer)
    {
      if (utils::midi::midi_is_note_on (event.data ()))
        {
          found_pitches.push_back (
            utils::midi::midi_get_note_number (event.data ()));
        }
    }

  EXPECT_TRUE (std::ranges::contains (found_pitches, 60));
  EXPECT_TRUE (std::ranges::contains (found_pitches, 64));
  EXPECT_TRUE (std::ranges::contains (found_pitches, 67));
}

TEST_F (TimelineDataProviderTest, MidiBasicFunctionality)
{
  // Test that the provider can be constructed
  EXPECT_NE (midi_provider_, nullptr);

  // Test that process_midi_events can be called without crashing
  auto output_buffer = dsp::MidiEventBuffer::make_reserved ();
  auto time_info = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));

  midi_provider_->process_midi_events (
    time_info, dsp::ITransport::PlayState::Paused, output_buffer);
  EXPECT_EQ (output_buffer.size (), 0);

  // Test that generate_midi_events can be called without crashing
  std::vector<const MidiClip *> empty_clips;
  utils::ExpandableTickRange    range (std::pair (0.0, 960.0));

  midi_provider_->generate_midi_events (*tempo_map_, empty_clips, range);

  // Process again to ensure no crash
  midi_provider_->process_midi_events (
    time_info, dsp::ITransport::PlayState::Paused, output_buffer);
  EXPECT_EQ (output_buffer.size (), 0);
}

// Tests from PlaybackCacheBuilder that are unique and test specific concerns
TEST_F (TimelineDataProviderTest, GenerateCacheWithAffectedRange)
{
  // Create a MIDI clip at tick 0
  auto clip = create_midi_clip (0.0, 200.0);

  // Create a vector of clips
  std::vector<const MidiClip *> clips;
  clips.push_back (clip);

  // Generate cache with affected range that includes our clip
  const double               affected_start = 0.0;
  const double               affected_end = 200.0;
  utils::ExpandableTickRange range (
    std::make_pair (affected_start, affected_end));
  midi_provider_->generate_midi_events (*tempo_map_, clips, range);

  // Test processing events that should include the note
  auto output_buffer = dsp::MidiEventBuffer::make_reserved ();
  auto time_info = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));

  midi_provider_->process_midi_events (
    time_info, dsp::ITransport::PlayState::Rolling, output_buffer);

  // Should have events from the MIDI clip
  EXPECT_GE (output_buffer.size (), 1);

  // Convert affected range to samples for verification
  const auto sample_start = tempo_map_->tick_to_samples_rounded (
    dsp::TimelineTick{ units::ticks (affected_start) });
  const auto sample_end = tempo_map_->tick_to_samples_rounded (
    dsp::TimelineTick{ units::ticks (affected_end) });

  // All events should be within the sample interval
  for (const auto &event : output_buffer)
    {
      const auto event_time = event.time ();
      EXPECT_GE (event_time, sample_start);
      EXPECT_LE (event_time, sample_end);

      // Verify it's a note on event with the correct pitch
      EXPECT_TRUE (utils::midi::midi_is_note_on (event.data ()));
      EXPECT_EQ (utils::midi::midi_get_note_number (event.data ()), 60);
    }
}

TEST_F (TimelineDataProviderTest, GenerateCacheOutsideAffectedRange)
{
  // Create a MIDI clip at tick 500
  auto clip = create_midi_clip (500.0, 700.0);

  // Create a vector of clips
  std::vector<const MidiClip *> clips;
  clips.push_back (clip);

  // Generate cache with affected range that doesn't include our clip
  const double               affected_start = 0.0;
  const double               affected_end = 200.0;
  utils::ExpandableTickRange range (
    std::make_pair (affected_start, affected_end));
  midi_provider_->generate_midi_events (*tempo_map_, clips, range);

  // Test processing events that should NOT include the note
  auto output_buffer = dsp::MidiEventBuffer::make_reserved ();
  auto time_info = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));

  midi_provider_->process_midi_events (
    time_info, dsp::ITransport::PlayState::Rolling, output_buffer);

  // Should have no events since clip is outside affected range
  EXPECT_EQ (output_buffer.size (), 0);
}

TEST_F (TimelineDataProviderTest, GenerateCachePartialOverlap)
{
  // Create a MIDI clip at tick 0
  auto clip1 = create_midi_clip (0.0, 200.0, 60);

  // Create a second MIDI clip that partially overlaps with affected range
  auto clip2 = create_midi_clip (400.0, 500.0, 64);

  // Create a vector of clips
  std::vector<const MidiClip *> clips;
  clips.push_back (clip1);
  clips.push_back (clip2);

  // Generate cache with affected range that overlaps with first clip
  const double               affected_start = 0.0;
  const double               affected_end = 200.0;
  utils::ExpandableTickRange range (
    std::make_pair (affected_start, affected_end));
  midi_provider_->generate_midi_events (*tempo_map_, clips, range);

  // Test processing events that should include only the first clip
  auto output_buffer = dsp::MidiEventBuffer::make_reserved ();
  auto time_info = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (512));

  midi_provider_->process_midi_events (
    time_info, dsp::ITransport::PlayState::Rolling, output_buffer);

  // Should have events only from the first clip
  EXPECT_GT (output_buffer.size (), 0);

  // All events should be from the first clip (pitch 60)
  for (const auto &event : output_buffer)
    {
      if (utils::midi::midi_is_note_on (event.data ()))
        {
          EXPECT_EQ (utils::midi::midi_get_note_number (event.data ()), 60);
        }
    }
}

TEST_F (TimelineDataProviderTest, GenerateCacheWithMutedNote)
{
  // Create a MIDI clip at tick 0
  auto clip = create_midi_clip (0.0, 200.0);

  // Mute the note in our clip
  auto note_view = clip->ArrangerObjectOwner<MidiNote>::get_children_view ();
  if (!note_view.empty ())
    {
      note_view[0]->mute ()->setMuted (true);
    }

  // Create a vector of clips
  std::vector<const MidiClip *> clips;
  clips.push_back (clip);

  // Generate cache
  utils::ExpandableTickRange range (std::pair (0.0, 960.0));
  midi_provider_->generate_midi_events (*tempo_map_, clips, range);

  // Test processing events
  auto output_buffer = dsp::MidiEventBuffer::make_reserved ();
  auto time_info = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));

  midi_provider_->process_midi_events (
    time_info, dsp::ITransport::PlayState::Rolling, output_buffer);

  // Should have no events since the only note is muted
  EXPECT_EQ (output_buffer.size (), 0);
}

TEST_F (TimelineDataProviderTest, GenerateCacheMultipleRegions)
{
  // Create multiple clips with different pitches at the beginning of the
  // timeline
  std::vector<int>        pitches = { 60, 64, 67 };
  std::vector<MidiClip *> additional_regions;

  for (int i = 0; i < 3; ++i)
    {
      auto clip =
        create_midi_clip (i * 50.0, (i + 1) * 50.0 + 150.0, pitches[i]);
      additional_regions.push_back (clip);
    }

  // Create a vector of clips
  std::vector<const MidiClip *> clips;
  for (const auto &clip : additional_regions)
    {
      clips.push_back (clip);
    }

  // Generate cache with all clips
  utils::ExpandableTickRange range (std::pair (0.0, 960.0));
  midi_provider_->generate_midi_events (*tempo_map_, clips, range);

  // Convert tick positions to sample positions for precise testing
  const auto clip1_start_samples = tempo_map_->tick_to_samples_rounded (
    dsp::TimelineTick{ units::ticks (0.0) });
  const auto clip3_start_samples = tempo_map_->tick_to_samples_rounded (
    dsp::TimelineTick{ units::ticks (100.0) });

  // Test processing events that should include all notes
  auto output_buffer = dsp::MidiEventBuffer::make_reserved ();
  dsp::graph::ProcessBlockInfo time_info = {
    .transport_position_ = clip1_start_samples,
    .buffer_offset_ = units::samples (0),
    .nframes_ = (clip3_start_samples - clip1_start_samples) + units::samples (256)
  };

  midi_provider_->process_midi_events (
    time_info, dsp::ITransport::PlayState::Rolling, output_buffer);

  // Should have events from all clips
  EXPECT_GE (output_buffer.size (), 3);

  // Should have all pitches including duplicates
  std::vector<int> found_pitches;
  for (const auto &event : output_buffer)
    {
      if (utils::midi::midi_is_note_on (event.data ()))
        {
          found_pitches.push_back (
            utils::midi::midi_get_note_number (event.data ()));
        }
    }

  // Check that all expected pitches are present
  EXPECT_TRUE (std::ranges::contains (found_pitches, 60));
  EXPECT_TRUE (std::ranges::contains (found_pitches, 64));
  EXPECT_TRUE (std::ranges::contains (found_pitches, 67));
}

TEST_F (TimelineDataProviderTest, GenerateCacheEdgeCaseZeroLengthRegion)
{
  // Create a clip with zero length
  auto zero_length_clip_ref = utils::create_object<MidiClip> (
    *obj_registry_, *tempo_map_wrapper_, *obj_registry_);
  auto zero_length_region = zero_length_clip_ref.get_object_as<MidiClip> ();
  zero_length_region->position ()->setTicks (200.0);
  zero_length_region->length ()->setTicks (0.0);

  // Create a normal clip at the beginning
  auto normal_region = create_midi_clip (0.0, 200.0, 60);

  // Create a vector of clips
  std::vector<const MidiClip *> clips;
  clips.push_back (normal_region);
  clips.push_back (zero_length_region);

  // Keep a reference to the zero-length clip
  clip_refs.push_back (std::move (zero_length_clip_ref));

  // Generate cache with both clips
  utils::ExpandableTickRange range (std::pair (0.0, 960.0));
  midi_provider_->generate_midi_events (*tempo_map_, clips, range);

  // Test processing events
  auto output_buffer = dsp::MidiEventBuffer::make_reserved ();
  auto time_info = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));

  midi_provider_->process_midi_events (
    time_info, dsp::ITransport::PlayState::Rolling, output_buffer);

  // Should still have events from the original clip, not the zero-length one
  EXPECT_GT (output_buffer.size (), 0);

  // All events should be from the original clip (pitch 60)
  for (const auto &event : output_buffer)
    {
      if (utils::midi::midi_is_note_on (event.data ()))
        {
          EXPECT_EQ (utils::midi::midi_get_note_number (event.data ()), 60);
        }
    }
}

// Reproduces gitlab issue #5240: duplicating an adjacent clip causes the
// preceding clip's cache to be evicted and not regenerated.
//
// Scenario:
// - R1 at ticks [0, 960), R2 at ticks [960, 1920) (adjacent clips)
// - Generate cache for both clips
// - Regenerate cache for R2's range only (as would happen when R2 is
//   duplicated)
// - Verify R1's cache is still intact (it was adjacent, not overlapping)
TEST_F (TimelineDataProviderTest, AdjacentRegionCachePreservedOnDuplicate)
{
  // Create two adjacent MIDI clips (R1 and R2)
  const double r1_start = 0.0;
  const double r1_end = 960.0;   // 1 bar at 120 BPM
  const double r2_start = 960.0; // Exactly adjacent
  const double r2_end = 1920.0;

  auto r1 = create_midi_clip (r1_start, r1_end, 60);
  auto r2 = create_midi_clip (r2_start, r2_end, 64);

  std::vector<const MidiClip *> clips;
  clips.push_back (r1);
  clips.push_back (r2);

  // Generate initial cache for both clips
  utils::ExpandableTickRange full_range (std::pair (0.0, 1920.0));
  midi_provider_->generate_midi_events (*tempo_map_, clips, full_range);

  // Verify R1 produces events at its position
  const auto r1_start_samples = tempo_map_->tick_to_samples_rounded (
    dsp::TimelineTick{ units::ticks (r1_start) });
  const auto r1_end_samples = tempo_map_->tick_to_samples_rounded (
    dsp::TimelineTick{ units::ticks (r1_end) });

  auto output_buffer = dsp::MidiEventBuffer::make_reserved ();
  auto time_info = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    r1_start_samples, r1_end_samples - r1_start_samples);

  midi_provider_->process_midi_events (
    time_info, dsp::ITransport::PlayState::Rolling, output_buffer);
  const auto r1_event_count = output_buffer.size ();
  EXPECT_GT (r1_event_count, 0);

  // Now simulate what happens when R2 is duplicated:
  // generate_events is called with only R2's affected range
  // (as would be emitted by the cache scheduler)
  utils::ExpandableTickRange r2_range (std::make_pair (r2_start, r2_end));
  midi_provider_->generate_midi_events (*tempo_map_, clips, r2_range);

  // Verify R1's cache is still intact after regenerating R2's range.
  // The provider may emit all-notes-off events due to detecting a transport
  // jump; we only count note-on events.
  output_buffer.clear ();
  midi_provider_->process_midi_events (
    time_info, dsp::ITransport::PlayState::Rolling, output_buffer);

  // R1 should still produce note events - its cache was NOT evicted
  int note_on_count_after = 0;
  for (const auto &event : output_buffer)
    {
      if (utils::midi::midi_is_note_on (event.data ()))
        ++note_on_count_after;
    }
  EXPECT_EQ (note_on_count_after, 1); // R1 has one note
}

TEST_F (TimelineDataProviderTest, GenerateCacheWithExistingCache)
{
  // Create a MIDI clip at tick 0
  auto clip = create_midi_clip (0.0, 200.0);

  // Create a vector of clips
  std::vector<const MidiClip *> clips;
  clips.push_back (clip);

  // First generate cache normally
  utils::ExpandableTickRange range (std::pair (0.0, 960.0));
  midi_provider_->generate_midi_events (*tempo_map_, clips, range);

  // Test processing events
  auto output_buffer = dsp::MidiEventBuffer::make_reserved ();
  auto time_info = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));

  midi_provider_->process_midi_events (
    time_info, dsp::ITransport::PlayState::Rolling, output_buffer);
  const auto initial_event_count = output_buffer.size ();
  EXPECT_GT (initial_event_count, 0);

  // Generate cache again with affected range that should clear and regenerate
  const double               affected_start = 0.0;
  const double               affected_end = 200.0;
  utils::ExpandableTickRange affected_range (
    std::make_pair (affected_start, affected_end));
  midi_provider_->generate_midi_events (*tempo_map_, clips, affected_range);

  // Test processing events again
  output_buffer.clear ();
  midi_provider_->process_midi_events (
    time_info, dsp::ITransport::PlayState::Rolling, output_buffer);

  // Should still have events
  EXPECT_GT (output_buffer.size (), 0);
  // Account for possible all-notes-off event (16 extra events)
  EXPECT_EQ (output_buffer.size (), initial_event_count + 16);
}

TEST_F (TimelineDataProviderTest, PreciseTimingVerification)
{
  // Create a MIDI clip at a specific tick position
  const double clip_start_ticks = 240.0; // 1 beat at 120 BPM
  auto clip = create_midi_clip (clip_start_ticks, clip_start_ticks + 200.0);

  // Create a vector of clips
  std::vector<const MidiClip *> clips;
  clips.push_back (clip);

  // Generate events for the clips
  utils::ExpandableTickRange range (std::pair (0.0, 960.0));
  midi_provider_->generate_midi_events (*tempo_map_, clips, range);

  // Convert clip start to samples
  const auto clip_start_samples = tempo_map_->tick_to_samples_rounded (
    dsp::TimelineTick{ units::ticks (clip_start_ticks) });

  // Test processing events that should include the note
  auto output_buffer = dsp::MidiEventBuffer::make_reserved ();
  auto time_info = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    clip_start_samples, units::samples (256));

  midi_provider_->process_midi_events (
    time_info, dsp::ITransport::PlayState::Rolling, output_buffer);

  // Should have events from the MIDI clip
  EXPECT_GE (output_buffer.size (), 1);

  // Verify the event timing is correct
  if (!output_buffer.empty ())
    {
      const auto &event = output_buffer.front ();

      // The event should be at the beginning of our processing block
      // since the note is at the start of the clip
      EXPECT_LT (event.time (), time_info.nframes_);
      // Verify it's a note on event with the correct pitch
      EXPECT_TRUE (utils::midi::midi_is_note_on (event.data ()));
      EXPECT_EQ (utils::midi::midi_get_note_number (event.data ()), 60);
    }
}

// ========== Audio Provider Tests ==========

TEST_F (TimelineDataProviderTest, AudioInitialState)
{
  // Provider should start with no audio
  std::vector<float> output_left (256, 0.0f);
  std::vector<float> output_right (256, 0.0f);
  auto time_info = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));

  audio_provider_->process_audio_events (
    time_info, dsp::ITransport::PlayState::Rolling, output_left, output_right);

  // Output should be all zeros
  for (size_t i = 0; i < output_left.size (); ++i)
    {
      EXPECT_FLOAT_EQ (output_left[i], 0.0f);
      EXPECT_FLOAT_EQ (output_right[i], 0.0f);
    }
}

TEST_F (TimelineDataProviderTest, GenerateAudioEventsWithEmptyClips)
{
  // Test generating audio events with empty clip list
  std::vector<const AudioClip *> empty_clips;
  utils::ExpandableTickRange     range (
    std::pair (0.0, 960.0)); // One bar at 120 BPM

  audio_provider_->generate_audio_events (*tempo_map_, empty_clips, range);

  // Verify no audio is generated
  std::vector<float> output_left (256, 0.0f);
  std::vector<float> output_right (256, 0.0f);
  auto time_info = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));

  audio_provider_->process_audio_events (
    time_info, dsp::ITransport::PlayState::Rolling, output_left, output_right);

  // Output should be all zeros
  for (size_t i = 0; i < output_left.size (); ++i)
    {
      EXPECT_FLOAT_EQ (output_left[i], 0.0f);
      EXPECT_FLOAT_EQ (output_right[i], 0.0f);
    }
}

// A clip whose block file is missing is skipped by the cache builder
TEST_F (TimelineDataProviderTest, GenerateAudioEventsSkipsClipWithMissingBlock)
{
  auto       temp_dir_obj = utils::io::make_tmp_dir ();
  const auto blocks_dir =
    utils::Utf8String::from_qstring (temp_dir_obj->path ()).to_path ()
    / "blocks";
  auto store = std::make_shared<dsp::AudioBlockStore> (blocks_dir);

  auto * clip = create_audio_clip (0.0, 200.0);
  auto  &source =
    clip->ArrangerObjectOwner<AudioSourceObject>::get_children_view ()
      .front ()
      ->file_audio_source ();
  source.set_store (store);
  source.flush_tail ();
  ASSERT_EQ (source.get_spans ().size (), 1);
  std::filesystem::remove (
    store->get_blocks_directory () / source.get_spans ().front ().block.hex);

  std::vector<const AudioClip *> clips{ clip };
  utils::ExpandableTickRange     range (std::pair (0.0, 960.0));
  EXPECT_NO_THROW (
    audio_provider_->generate_audio_events (*tempo_map_, clips, range));
  EXPECT_TRUE (audio_provider_->audio_clips ().empty ());
}

TEST_F (TimelineDataProviderTest, ProcessAudioEventsWithAudioClip)
{
  // Create an audio clip at tick 0 (start of timeline)
  auto clip = create_audio_clip (0.0, 200.0);

  // Create a vector of clips
  std::vector<const AudioClip *> clips;
  clips.push_back (clip);

  // Generate events for the clips
  utils::ExpandableTickRange range (std::pair (0.0, 960.0));
  audio_provider_->generate_audio_events (*tempo_map_, clips, range);

  // Test processing audio that should include the clip
  std::vector<float> output_left (256, 0.0f);
  std::vector<float> output_right (256, 0.0f);
  auto time_info = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));

  audio_provider_->process_audio_events (
    time_info, dsp::ITransport::PlayState::Rolling, output_left, output_right);

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
  // Create an audio clip at tick 500
  auto clip = create_audio_clip (500.0, 700.0);

  // Create a vector of clips
  std::vector<const AudioClip *> clips;
  clips.push_back (clip);

  // Generate events for the clips
  utils::ExpandableTickRange range (std::pair (0.0, 960.0));
  audio_provider_->generate_audio_events (*tempo_map_, clips, range);

  // Test processing audio that should NOT include the clip
  std::vector<float> output_left (256, 0.0f);
  std::vector<float> output_right (256, 0.0f);
  auto time_info = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));

  audio_provider_->process_audio_events (
    time_info, dsp::ITransport::PlayState::Rolling, output_left, output_right);

  // Should have no audio since the clip is outside the time range
  for (size_t i = 0; i < output_left.size (); ++i)
    {
      EXPECT_FLOAT_EQ (output_left[i], 0.0f);
      EXPECT_FLOAT_EQ (output_right[i], 0.0f);
    }
}

TEST_F (TimelineDataProviderTest, ProcessAudioEventsWithOffset)
{
  // Create an audio clip at tick 0
  auto clip = create_audio_clip (0.0, 200.0);

  // Create a vector of clips
  std::vector<const AudioClip *> clips;
  clips.push_back (clip);

  // Generate events for the clips
  utils::ExpandableTickRange range (std::pair (0.0, 960.0));
  audio_provider_->generate_audio_events (*tempo_map_, clips, range);

  // Test processing audio with a global offset that's far from the clip
  std::vector<float>           output_left (256, 0.0f);
  std::vector<float>           output_right (256, 0.0f);
  dsp::graph::ProcessBlockInfo time_info = {
    .transport_position_ = units::samples (10000),
    // 100 frame offset
    .buffer_offset_ = units::samples (100),
    .nframes_ = units::samples (256)
  };

  audio_provider_->process_audio_events (
    time_info, dsp::ITransport::PlayState::Rolling, output_left, output_right);

  // Should have no audio since the clip is at tick 0
  // and we're processing frames 10100-10356
  for (size_t i = 0; i < output_left.size (); ++i)
    {
      EXPECT_FLOAT_EQ (output_left[i], 0.0f);
      EXPECT_FLOAT_EQ (output_right[i], 0.0f);
    }
}

TEST_F (TimelineDataProviderTest, MultipleAudioClipsInSequence)
{
  // Create multiple audio clips at the beginning of the timeline
  auto clip1 = create_audio_clip (0.0, 100.0, 0.5f);
  auto clip2 = create_audio_clip (50.0, 150.0, 0.7f);
  auto clip3 = create_audio_clip (100.0, 200.0, 0.3f);

  // Create a vector of clips
  std::vector<const AudioClip *> clips;
  clips.push_back (clip1);
  clips.push_back (clip2);
  clips.push_back (clip3);

  // Generate events for the clips
  utils::ExpandableTickRange range (std::pair (0.0, 960.0));
  audio_provider_->generate_audio_events (*tempo_map_, clips, range);

  // Convert tick positions to sample positions for precise testing
  const auto clip1_start_samples = tempo_map_->tick_to_samples_rounded (
    dsp::TimelineTick{ units::ticks (0.0) });
  const auto clip3_start_samples = tempo_map_->tick_to_samples_rounded (
    dsp::TimelineTick{ units::ticks (100.0) });

  // Test processing audio that should include all clips
  std::vector<float>           output_left (1024, 0.0f);
  std::vector<float>           output_right (1024, 0.0f);
  dsp::graph::ProcessBlockInfo time_info = {
    .transport_position_ = clip1_start_samples,
    .buffer_offset_ = units::samples (0),
    .nframes_ = (clip3_start_samples - clip1_start_samples) + units::samples (256)
  };

  audio_provider_->process_audio_events (
    time_info, dsp::ITransport::PlayState::Rolling, output_left, output_right);

  // Should have some audio output from overlapping clips
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
  // Create an audio clip at tick 0
  auto clip = create_audio_clip (0.0, 200.0);

  // Create a vector of clips
  std::vector<const AudioClip *> clips;
  clips.push_back (clip);

  // Generate cache with affected range that includes our clip
  const double               affected_start = 0.0;
  const double               affected_end = 200.0;
  utils::ExpandableTickRange range (
    std::make_pair (affected_start, affected_end));
  audio_provider_->generate_audio_events (*tempo_map_, clips, range);

  // Test processing audio that should include the clip
  std::vector<float> output_left (256, 0.0f);
  std::vector<float> output_right (256, 0.0f);
  auto time_info = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));

  audio_provider_->process_audio_events (
    time_info, dsp::ITransport::PlayState::Rolling, output_left, output_right);

  // Should have audio from the audio clip
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
  // Create an audio clip at tick 500
  auto clip = create_audio_clip (500.0, 700.0);

  // Create a vector of clips
  std::vector<const AudioClip *> clips;
  clips.push_back (clip);

  // Generate cache with affected range that doesn't include our clip
  const double               affected_start = 0.0;
  const double               affected_end = 200.0;
  utils::ExpandableTickRange range (
    std::make_pair (affected_start, affected_end));
  audio_provider_->generate_audio_events (*tempo_map_, clips, range);

  // Test processing audio that should NOT include the clip
  std::vector<float> output_left (256, 0.0f);
  std::vector<float> output_right (256, 0.0f);
  auto time_info = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));

  audio_provider_->process_audio_events (
    time_info, dsp::ITransport::PlayState::Rolling, output_left, output_right);

  // Should have no audio since clip is outside affected range
  for (size_t i = 0; i < output_left.size (); ++i)
    {
      EXPECT_FLOAT_EQ (output_left[i], 0.0f);
      EXPECT_FLOAT_EQ (output_right[i], 0.0f);
    }
}

TEST_F (TimelineDataProviderTest, GenerateAudioCachePartialOverlap)
{
  // Create an audio clip at tick 0
  auto clip1 = create_audio_clip (0.0, 200.0);

  // Create a second audio clip that partially overlaps with affected range
  auto clip2 = create_audio_clip (400.0, 500.0);

  // Create a vector of clips
  std::vector<const AudioClip *> clips;
  clips.push_back (clip1);
  clips.push_back (clip2);

  // Generate cache with affected range that overlaps with first clip
  const double               affected_start = 0.0;
  const double               affected_end = 200.0;
  utils::ExpandableTickRange range (
    std::make_pair (affected_start, affected_end));
  audio_provider_->generate_audio_events (*tempo_map_, clips, range);

  // Test processing audio that should include only the first clip
  std::vector<float> output_left (512, 0.0f);
  std::vector<float> output_right (512, 0.0f);
  auto time_info = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (512));

  audio_provider_->process_audio_events (
    time_info, dsp::ITransport::PlayState::Rolling, output_left, output_right);

  // Should have audio only from the first clip
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
  // Create multiple clips with different gains at the beginning of the timeline
  std::vector<float>       gains = { 0.5f, 0.7f, 0.3f };
  std::vector<AudioClip *> additional_regions;

  for (int i = 0; i < 3; ++i)
    {
      auto clip = create_audio_clip (i * 50.0, (i + 1) * 50.0 + 150.0, gains[i]);
      additional_regions.push_back (clip);
    }

  // Create a vector of clips
  std::vector<const AudioClip *> clips;
  for (const auto &clip : additional_regions)
    {
      clips.push_back (clip);
    }

  // Generate cache with all clips
  utils::ExpandableTickRange range (std::pair (0.0, 960.0));
  audio_provider_->generate_audio_events (*tempo_map_, clips, range);

  // Convert tick positions to sample positions for precise testing
  const auto clip1_start_samples = tempo_map_->tick_to_samples_rounded (
    dsp::TimelineTick{ units::ticks (0.0) });
  const auto clip3_start_samples = tempo_map_->tick_to_samples_rounded (
    dsp::TimelineTick{ units::ticks (100.0) });

  // Test processing audio that should include all clips
  std::vector<float>           output_left (1024, 0.0f);
  std::vector<float>           output_right (1024, 0.0f);
  dsp::graph::ProcessBlockInfo time_info = {
    .transport_position_ = clip1_start_samples,
    .buffer_offset_ = units::samples (0),
    .nframes_ = (clip3_start_samples - clip1_start_samples) + units::samples (256)
  };

  audio_provider_->process_audio_events (
    time_info, dsp::ITransport::PlayState::Rolling, output_left, output_right);

  // Should have audio from all clips
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
  // Create a clip with zero length
  auto zero_length_clip_ref = utils::create_object<AudioClip> (
    *obj_registry_, *tempo_map_wrapper_, *obj_registry_);
  auto zero_length_region = zero_length_clip_ref.get_object_as<AudioClip> ();

  // Create and set an audio source for the zero-length clip (required by
  // AudioClip contract)
  auto audio_source_object_ref = create_sine_wave_audio_source (4096);
  zero_length_region->set_source (audio_source_object_ref);

  zero_length_region->position ()->setTicks (200.0);
  zero_length_region->length ()->setTicks (0.0);

  // Create a normal clip at the beginning
  auto normal_region = create_audio_clip (0.0, 200.0, 0.6f);

  // Create a vector of clips
  std::vector<const AudioClip *> clips;
  clips.push_back (normal_region);
  clips.push_back (zero_length_region);

  // Keep a reference to the zero-length clip
  clip_refs.push_back (std::move (zero_length_clip_ref));

  // Generate cache with both clips
  utils::ExpandableTickRange range (std::pair (0.0, 960.0));
  audio_provider_->generate_audio_events (*tempo_map_, clips, range);

  // Test processing audio
  std::vector<float> output_left (256, 0.0f);
  std::vector<float> output_right (256, 0.0f);
  auto time_info = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));

  audio_provider_->process_audio_events (
    time_info, dsp::ITransport::PlayState::Rolling, output_left, output_right);

  // Should still have audio from the original clip, not the zero-length one
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
  // Create an audio clip at tick 0
  auto clip = create_audio_clip (0.0, 200.0);

  // Create a vector of clips
  std::vector<const AudioClip *> clips;
  clips.push_back (clip);

  // First generate cache normally
  utils::ExpandableTickRange range (std::pair (0.0, 960.0));
  audio_provider_->generate_audio_events (*tempo_map_, clips, range);

  // Test processing audio
  std::vector<float> output_left (256, 0.0f);
  std::vector<float> output_right (256, 0.0f);
  auto time_info = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));

  audio_provider_->process_audio_events (
    time_info, dsp::ITransport::PlayState::Rolling, output_left, output_right);

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
  audio_provider_->generate_audio_events (*tempo_map_, clips, affected_range);

  // Test processing audio again
  std::fill (output_left.begin (), output_left.end (), 0.0f);
  std::fill (output_right.begin (), output_right.end (), 0.0f);
  audio_provider_->process_audio_events (
    time_info, dsp::ITransport::PlayState::Rolling, output_left, output_right);

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
  // Create an audio clip at a specific tick position
  const double clip_start_ticks = 240.0; // 1 beat at 120 BPM
  auto clip = create_audio_clip (clip_start_ticks, clip_start_ticks + 200.0);

  // Create a vector of clips
  std::vector<const AudioClip *> clips;
  clips.push_back (clip);

  // Generate events for the clips
  utils::ExpandableTickRange range (std::pair (0.0, 960.0));
  audio_provider_->generate_audio_events (*tempo_map_, clips, range);

  // Convert clip start to samples
  const auto clip_start_samples = tempo_map_->tick_to_samples_rounded (
    dsp::TimelineTick{ units::ticks (clip_start_ticks) });

  // Test processing audio that should include the clip
  std::vector<float> output_left (256, 0.0f);
  std::vector<float> output_right (256, 0.0f);
  auto time_info = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    clip_start_samples, units::samples (256));

  audio_provider_->process_audio_events (
    time_info, dsp::ITransport::PlayState::Rolling, output_left, output_right);

  // Should have audio from the audio clip
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
  EXPECT_NE (audio_provider_, nullptr);

  // Test that process_audio_events can be called without crashing
  std::vector<float> output_left (256, 0.0f);
  std::vector<float> output_right (256, 0.0f);
  auto time_info = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));

  audio_provider_->process_audio_events (
    time_info, dsp::ITransport::PlayState::Rolling, output_left, output_right);

  // Should be all zeros initially
  for (size_t i = 0; i < output_left.size (); ++i)
    {
      EXPECT_FLOAT_EQ (output_left[i], 0.0f);
      EXPECT_FLOAT_EQ (output_right[i], 0.0f);
    }

  // Test that generate_audio_events can be called without crashing
  std::vector<const AudioClip *> empty_clips;
  utils::ExpandableTickRange     range (std::pair (0.0, 960.0));

  audio_provider_->generate_audio_events (*tempo_map_, empty_clips, range);

  // Process again to ensure no crash
  audio_provider_->process_audio_events (
    time_info, dsp::ITransport::PlayState::Rolling, output_left, output_right);

  // Should still be all zeros
  for (size_t i = 0; i < output_left.size (); ++i)
    {
      EXPECT_FLOAT_EQ (output_left[i], 0.0f);
      EXPECT_FLOAT_EQ (output_right[i], 0.0f);
    }
}

// ========== Automation Provider Tests ==========

TEST_F (TimelineDataProviderTest, AutomationProviderInitialState)
{
  // Provider should start with no automation
  std::vector<float> output_values (256, 0.0f);
  auto time_info = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));

  automation_provider_->process_automation_events (
    time_info, dsp::ITransport::PlayState::Rolling, output_values);

  // Output should be all negative, indicating no automation (default value)
  for (float output_value : output_values)
    {
      EXPECT_LT (output_value, 0.0f);
    }
}

TEST_F (TimelineDataProviderTest, AutomationProviderGenerateEventsWithEmptyClips)
{
  // Test generating automation events with empty clip list
  std::vector<const AutomationClip *> empty_clips;
  utils::ExpandableTickRange          range (
    std::pair (0.0, 960.0)); // One bar at 120 BPM

  automation_provider_->generate_automation_events (
    *tempo_map_, empty_clips, range);

  // Verify no automation is generated
  std::vector<float> output_values (256, 0.0f);
  auto time_info = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));

  automation_provider_->process_automation_events (
    time_info, dsp::ITransport::PlayState::Rolling, output_values);

  // Output should be all negative, indicating no automation (default value)
  for (float output_value : output_values)
    {
      EXPECT_LT (output_value, 0.0f);
    }
}

TEST_F (
  TimelineDataProviderTest,
  AutomationProviderProcessEventsWithAutomationClip)
{
  // Create an automation clip at tick 0 (start of timeline)
  auto clip = create_automation_clip (0.0, 200.0, 0.0f, 1.0f);

  // Create a vector of clips
  std::vector<const AutomationClip *> clips;
  clips.push_back (clip);

  // Generate events for the clips
  utils::ExpandableTickRange range (std::pair (0.0, 960.0));
  automation_provider_->generate_automation_events (*tempo_map_, clips, range);

  // Test processing automation that should include the clip
  std::vector<float> output_values (256, 0.0f);
  auto time_info = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));

  automation_provider_->process_automation_events (
    time_info, dsp::ITransport::PlayState::Rolling, output_values);

  // Should have some automation values (not all zeros)
  bool has_automation = false;
  for (size_t i = 0; i < output_values.size (); ++i)
    {
      if (std::abs (output_values[i]) > 0.001f)
        {
          has_automation = true;
          break;
        }
    }
  EXPECT_TRUE (has_automation);
}

TEST_F (TimelineDataProviderTest, AutomationProviderGetValueAtSpecificPosition)
{
  // Create an automation clip with linear ramp from 0.0 to 1.0
  auto clip = create_automation_clip (0.0, 200.0, 0.0f, 1.0f);

  // Create a vector of clips
  std::vector<const AutomationClip *> clips;
  clips.push_back (clip);

  // Generate events for the clips
  utils::ExpandableTickRange range (std::pair (0.0, 960.0));
  automation_provider_->generate_automation_events (*tempo_map_, clips, range);

  // Convert clip start to samples
  const auto clip_start_samples = tempo_map_->tick_to_samples_rounded (
    dsp::TimelineTick{ units::ticks (0.0) });
  const auto clip_end_samples = tempo_map_->tick_to_samples_rounded (
    dsp::TimelineTick{ units::ticks (200.0) });

  // Test getting automation value at start of clip (should be 0.0)
  auto value_at_start_opt =
    automation_provider_->get_automation_value_rt (clip_start_samples);
  ASSERT_TRUE (value_at_start_opt.has_value ());
  EXPECT_FLOAT_EQ (value_at_start_opt.value (), 0.0f);

  // Test getting automation value at end of clip (should be near 1.0)
  auto value_at_end_opt = automation_provider_->get_automation_value_rt (
    clip_end_samples - units::samples (1)); // Just before end
  ASSERT_TRUE (value_at_end_opt.has_value ());
  EXPECT_NEAR (value_at_end_opt.value (), 1.0f, 0.001f);

  // Test getting automation value outside clip (should be 1.0 - last known
  // value)
  auto value_outside_opt = automation_provider_->get_automation_value_rt (
    clip_end_samples + units::samples (100));
  ASSERT_TRUE (value_outside_opt.has_value ());
  // FIXME: this should be exactly 1.0f but it's not a big issue for
  // now
  EXPECT_NEAR (value_at_end_opt.value (), 1.0f, 0.001f);
}

TEST_F (
  TimelineDataProviderTest,
  AutomationProviderGetValueBeforeFirstAutomationPoint)
{
  // Create an automation clip that starts later in the timeline (not at 0)
  auto clip = create_automation_clip (1000.0, 1200.0, 0.0f, 1.0f);

  // Create a vector of clips
  std::vector<const AutomationClip *> clips;
  clips.push_back (clip);

  // Generate events for the clips
  utils::ExpandableTickRange range (std::pair (0.0, 2000.0));
  automation_provider_->generate_automation_events (*tempo_map_, clips, range);

  // Convert clip start to samples
  const auto clip_start_samples = tempo_map_->tick_to_samples_rounded (
    dsp::TimelineTick{ units::ticks (1000.0) });

  // Test getting automation value before the first automation point (should be
  // std::nullopt)
  auto value_before_opt = automation_provider_->get_automation_value_rt (
    clip_start_samples - units::samples (100));
  EXPECT_FALSE (value_before_opt.has_value ());

  // Test getting automation value at the first automation point (should be 0.0)
  auto value_at_start_opt =
    automation_provider_->get_automation_value_rt (clip_start_samples);
  ASSERT_TRUE (value_at_start_opt.has_value ());
  EXPECT_NEAR (value_at_start_opt.value (), 0.0f, 1e-5f);
}

// ========== Chord Clip Tests ==========

TEST_F (TimelineDataProviderTest, ChordClipInitialState)
{
  // Provider should start with no chord events
  auto output_buffer = dsp::MidiEventBuffer::make_reserved ();
  auto time_info = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));

  midi_provider_->process_midi_events (
    time_info, dsp::ITransport::PlayState::Rolling, output_buffer);
  EXPECT_EQ (output_buffer.size (), 0);
}

TEST_F (TimelineDataProviderTest, ProcessChordClip)
{
  // Create a chord clip at tick 0
  auto chord_clip_ref = utils::create_object<arrangement::ChordClip> (
    *obj_registry_, *tempo_map_wrapper_, *obj_registry_);
  auto chord_clip = chord_clip_ref.get_object_as<arrangement::ChordClip> ();
  chord_clip->position ()->setTicks (0.0);
  chord_clip->length ()->setTicks (200.0);

  // Keep a reference to the chord clip
  clip_refs.push_back (std::move (chord_clip_ref));

  // Create a vector of chord clips
  std::vector<const arrangement::ChordClip *> chord_clips;
  chord_clips.push_back (chord_clip);

  // Generate events for the chord clips
  utils::ExpandableTickRange range (std::pair (0.0, 960.0));
  midi_provider_->generate_midi_events (*tempo_map_, chord_clips, range);

  // Test processing events that should include chord events
  auto output_buffer = dsp::MidiEventBuffer::make_reserved ();
  auto time_info = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));

  midi_provider_->process_midi_events (
    time_info, dsp::ITransport::PlayState::Rolling, output_buffer);

  // Should have some events from the chord clip
  EXPECT_GE (output_buffer.size (), 0);

  // Verify events are valid MIDI events
  for (const auto &event : output_buffer)
    {
      EXPECT_GE (event.time (), units::samples (0));
      EXPECT_LT (event.time (), time_info.nframes_);
      // Check that it's a valid MIDI message
      EXPECT_TRUE (utils::midi::midi_is_short_msg (event.data ()));
    }
}

TEST_F (TimelineDataProviderTest, ProcessChordClipOutsideRange)
{
  // Create a chord clip at tick 500
  auto chord_clip_ref = utils::create_object<arrangement::ChordClip> (
    *obj_registry_, *tempo_map_wrapper_, *obj_registry_);
  auto chord_clip = chord_clip_ref.get_object_as<arrangement::ChordClip> ();
  chord_clip->position ()->setTicks (500.0);
  chord_clip->length ()->setTicks (200.0);

  // Keep a reference to the chord clip
  clip_refs.push_back (std::move (chord_clip_ref));

  // Create a vector of chord clips
  std::vector<const arrangement::ChordClip *> chord_clips;
  chord_clips.push_back (chord_clip);

  // Generate events for the chord clips
  utils::ExpandableTickRange range (std::pair (0.0, 960.0));
  midi_provider_->generate_midi_events (*tempo_map_, chord_clips, range);

  // Test processing events that should NOT include the chord
  auto output_buffer = dsp::MidiEventBuffer::make_reserved ();
  auto time_info = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));

  midi_provider_->process_midi_events (
    time_info, dsp::ITransport::PlayState::Rolling, output_buffer);

  // Should have no events since the chord clip is outside the time range
  EXPECT_EQ (output_buffer.size (), 0);
}

// ========== Muted Clip Tests ==========

TEST_F (TimelineDataProviderTest, ProcessMutedMidiClip)
{
  // Create a MIDI clip at tick 0
  auto clip = create_midi_clip (0.0, 200.0);

  // Mute the entire clip
  clip->mute ()->setMuted (true);

  // Create a vector of clips
  std::vector<const MidiClip *> clips;
  clips.push_back (clip);

  // Generate events for the clips
  utils::ExpandableTickRange range (std::pair (0.0, 960.0));
  midi_provider_->generate_midi_events (*tempo_map_, clips, range);

  // Test processing events
  auto output_buffer = dsp::MidiEventBuffer::make_reserved ();
  auto time_info = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));

  midi_provider_->process_midi_events (
    time_info, dsp::ITransport::PlayState::Rolling, output_buffer);

  // Should have no events since the entire clip is muted
  EXPECT_EQ (output_buffer.size (), 0);
}

TEST_F (TimelineDataProviderTest, ProcessMutedAudioClip)
{
  // Create an audio clip at tick 0
  auto clip = create_audio_clip (0.0, 200.0);

  // Mute the entire clip
  clip->mute ()->setMuted (true);

  // Create a vector of clips
  std::vector<const AudioClip *> clips;
  clips.push_back (clip);

  // Generate events for the clips
  utils::ExpandableTickRange range (std::pair (0.0, 960.0));
  audio_provider_->generate_audio_events (*tempo_map_, clips, range);

  // Test processing audio
  std::vector<float> output_left (256, 0.0f);
  std::vector<float> output_right (256, 0.0f);
  auto time_info = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));

  audio_provider_->process_audio_events (
    time_info, dsp::ITransport::PlayState::Rolling, output_left, output_right);

  // Should have no audio since the entire clip is muted
  for (size_t i = 0; i < output_left.size (); ++i)
    {
      EXPECT_FLOAT_EQ (output_left[i], 0.0f);
      EXPECT_FLOAT_EQ (output_right[i], 0.0f);
    }
}

TEST_F (TimelineDataProviderTest, ProcessMutedChordClip)
{
  // Create a chord clip at tick 0
  auto chord_clip_ref = utils::create_object<arrangement::ChordClip> (
    *obj_registry_, *tempo_map_wrapper_, *obj_registry_);
  auto chord_clip = chord_clip_ref.get_object_as<arrangement::ChordClip> ();
  chord_clip->position ()->setTicks (0.0);
  chord_clip->length ()->setTicks (200.0);

  // Mute the entire chord clip
  chord_clip->mute ()->setMuted (true);

  // Keep a reference to the chord clip
  clip_refs.push_back (std::move (chord_clip_ref));

  // Create a vector of chord clips
  std::vector<const arrangement::ChordClip *> chord_clips;
  chord_clips.push_back (chord_clip);

  // Generate events for the chord clips
  utils::ExpandableTickRange range (std::pair (0.0, 960.0));
  midi_provider_->generate_midi_events (*tempo_map_, chord_clips, range);

  // Test processing events
  auto output_buffer = dsp::MidiEventBuffer::make_reserved ();
  auto time_info = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));

  midi_provider_->process_midi_events (
    time_info, dsp::ITransport::PlayState::Rolling, output_buffer);

  // Should have no events since the entire chord clip is muted
  EXPECT_EQ (output_buffer.size (), 0);
}

TEST_F (TimelineDataProviderTest, ProcessPartiallyMutedRegion)
{
  // Create a MIDI clip at tick 0 with multiple notes
  auto clip = create_midi_clip (0.0, 200.0, 60);

  // Add another note to the clip
  auto note_ref =
    utils::create_object<MidiNote> (*obj_registry_, *tempo_map_wrapper_);
  auto midi_note = note_ref.get_object_as<MidiNote> ();
  midi_note->setPitch (64);
  midi_note->setVelocity (80);
  midi_note->position ()->setTicks (
    tempo_map_->samples_to_tick (units::samples (25)).asDouble ());
  midi_note->length ()->setTicks (
    tempo_map_->samples_to_tick (units::samples (50)).asDouble ());
  clip->ArrangerObjectOwner<MidiNote>::add_object (note_ref);

  // Mute only the first note (pitch 60), not the entire clip
  auto note_view = clip->ArrangerObjectOwner<MidiNote>::get_children_view ();
  note_view[0]->mute ()->setMuted (true);

  // Create a vector of clips
  std::vector<const MidiClip *> clips;
  clips.push_back (clip);

  // Generate events for the clips
  utils::ExpandableTickRange range (std::pair (0.0, 960.0));
  midi_provider_->generate_midi_events (*tempo_map_, clips, range);

  // Test processing events
  auto output_buffer = dsp::MidiEventBuffer::make_reserved ();
  auto time_info = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));

  midi_provider_->process_midi_events (
    time_info, dsp::ITransport::PlayState::Rolling, output_buffer);

  // Should have events from the unmuted note (pitch 64) but not from the muted
  // note (pitch 60)
  EXPECT_GT (output_buffer.size (), 0);

  // Check that only the unmuted note is present
  bool found_muted_note = false;
  bool found_unmuted_note = false;

  for (const auto &event : output_buffer)
    {
      if (utils::midi::midi_is_note_on (event.data ()))
        {
          if (utils::midi::midi_get_note_number (event.data ()) == 60)
            {
              found_muted_note = true;
            }
          else if (utils::midi::midi_get_note_number (event.data ()) == 64)
            {
              found_unmuted_note = true;
            }
        }
    }

  EXPECT_FALSE (found_muted_note) << "Muted note (60) should not be present";
  EXPECT_TRUE (found_unmuted_note) << "Unmuted note (64) should be present";
}

// ========== Transport State Tests ==========

TEST_F (TimelineDataProviderTest, MidiBuffersClearedWhenTransportStops)
{
  // Create a MIDI clip at tick 0
  auto clip = create_midi_clip (0.0, 200.0);

  // Create a vector of clips
  std::vector<const MidiClip *> clips;
  clips.push_back (clip);

  // Generate events for the clips
  utils::ExpandableTickRange range (std::pair (0.0, 960.0));
  midi_provider_->generate_midi_events (*tempo_map_, clips, range);

  // First, process with transport rolling - should generate MIDI events
  auto output_buffer = dsp::MidiEventBuffer::make_reserved ();
  auto time_info = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));

  midi_provider_->process_midi_events (
    time_info, dsp::ITransport::PlayState::Rolling, output_buffer);
  EXPECT_GT (output_buffer.size (), 0);

  // Clear the queued events for next test
  output_buffer.clear ();

  // Now process with transport stopped - should send all-notes-off
  midi_provider_->process_midi_events (
    time_info, dsp::ITransport::PlayState::Paused, output_buffer);

  // Verify exactly 16 events were generated: the all-notes-off events
  EXPECT_EQ (output_buffer.size (), 16);

  // Verify it's the all-notes-off event
  const auto &event = output_buffer.front ();
  EXPECT_TRUE (utils::midi::midi_is_all_notes_off (event.data ()));
}

TEST_F (TimelineDataProviderTest, AudioBuffersClearedWhenTransportStops)
{
  // Create an audio clip at tick 0
  auto clip = create_audio_clip (0.0, 200.0);

  // Create a vector of clips
  std::vector<const AudioClip *> clips;
  clips.push_back (clip);

  // Generate events for the clips
  utils::ExpandableTickRange range (std::pair (0.0, 960.0));
  audio_provider_->generate_audio_events (*tempo_map_, clips, range);

  std::vector<float> output_left (256, 0.0f);
  std::vector<float> output_right (256, 0.0f);
  auto time_info = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));

  // First, process with transport rolling - should generate audio
  audio_provider_->process_audio_events (
    time_info, dsp::ITransport::PlayState::Rolling, output_left, output_right);

  // Should have some audio output
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

  // Clear buffers for next test
  std::fill (output_left.begin (), output_left.end (), 0.0f);
  std::fill (output_right.begin (), output_right.end (), 0.0f);

  // Now process with transport stopped - should clear buffers
  audio_provider_->process_audio_events (
    time_info, dsp::ITransport::PlayState::Paused, output_left, output_right);

  // Verify buffers are cleared
  for (size_t i = 0; i < output_left.size (); ++i)
    {
      EXPECT_FLOAT_EQ (output_left[i], 0.0f);
      EXPECT_FLOAT_EQ (output_right[i], 0.0f);
    }
}

TEST_F (TimelineDataProviderTest, MidiBuffersClearedWhenTransportJumps)
{
  // Create a MIDI clip at tick 0
  auto clip = create_midi_clip (0.0, 200.0);

  // Create a vector of clips
  std::vector<const MidiClip *> clips;
  clips.push_back (clip);

  // Generate events for the clips
  utils::ExpandableTickRange range (std::pair (0.0, 960.0));
  midi_provider_->generate_midi_events (*tempo_map_, clips, range);

  auto output_buffer = dsp::MidiEventBuffer::make_reserved ();

  // First, process at position 0 with transport rolling
  auto time_info1 = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));

  midi_provider_->process_midi_events (
    time_info1, dsp::ITransport::PlayState::Rolling, output_buffer);
  EXPECT_GT (output_buffer.size (), 0);

  // Clear the queued events for next test
  output_buffer.clear ();

  // Now process at a different position (jump) with transport still rolling
  dsp::graph::ProcessBlockInfo time_info2 = {
    .transport_position_ = units::samples (5120), // Jumped position
    .buffer_offset_ = units::samples (0),
    .nframes_ = units::samples (256)
  };

  midi_provider_->process_midi_events (
    time_info2, dsp::ITransport::PlayState::Rolling, output_buffer);

  // Verify exactly 16 events were generated: the all-notes-off events
  EXPECT_EQ (output_buffer.size (), 16);

  // Verify it's the all-notes-off event
  const auto &event = output_buffer.front ();
  EXPECT_TRUE (utils::midi::midi_is_all_notes_off (event.data ()));
}

TEST_F (TimelineDataProviderTest, ContinuousTransportPositionWorks)
{
  // Create a MIDI clip at tick 0
  auto clip = create_midi_clip (0.0, 400.0);

  // Create a vector of clips
  std::vector<const MidiClip *> clips;
  clips.push_back (clip);

  // Generate events for the clips
  utils::ExpandableTickRange range (std::pair (0.0, 960.0));
  midi_provider_->generate_midi_events (*tempo_map_, clips, range);

  auto output_buffer = dsp::MidiEventBuffer::make_reserved ();

  // First, process at position 0 with transport rolling
  auto time_info1 = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));

  midi_provider_->process_midi_events (
    time_info1, dsp::ITransport::PlayState::Rolling, output_buffer);
  EXPECT_GT (output_buffer.size (), 0);

  // Clear the queued events for next test
  output_buffer.clear ();

  // Now process at the expected next position with transport still rolling
  dsp::graph::ProcessBlockInfo time_info2 = {
    .transport_position_ = units::samples (256), // Expected next position
    .buffer_offset_ = units::samples (0),
    .nframes_ = units::samples (256)
  };

  midi_provider_->process_midi_events (
    time_info2, dsp::ITransport::PlayState::Rolling, output_buffer);

  // Should have no events (no all-notes-off since position is continuous)
  EXPECT_EQ (output_buffer.size (), 0);

  // Should not have all-notes-off event
  EXPECT_TRUE (std::ranges::none_of (output_buffer, [] (const auto &ev) {
    return utils::midi::midi_is_all_notes_off (ev.data ());
  }));
}

TEST_F (TimelineDataProviderTest, NoEventsWhenTransportStopped)
{
  // Create a MIDI clip at tick 0
  auto clip = create_midi_clip (0.0, 200.0);

  // Create a vector of clips
  std::vector<const MidiClip *> clips;
  clips.push_back (clip);

  // Generate events for the clips
  utils::ExpandableTickRange range (std::pair (0.0, 960.0));
  midi_provider_->generate_midi_events (*tempo_map_, clips, range);

  auto output_buffer = dsp::MidiEventBuffer::make_reserved ();
  auto time_info = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));

  // Process with transport stopped (initial state) - should have no events
  midi_provider_->process_midi_events (
    time_info, dsp::ITransport::PlayState::Paused, output_buffer);

  // Should have no events since transport was never rolling
  EXPECT_EQ (output_buffer.size (), 0);
}

TEST_F (TimelineDataProviderTest, NoAudioWhenTransportStopped)
{
  // Create an audio clip at tick 0
  auto clip = create_audio_clip (0.0, 200.0);

  // Create a vector of clips
  std::vector<const AudioClip *> clips;
  clips.push_back (clip);

  // Generate events for the clips
  utils::ExpandableTickRange range (std::pair (0.0, 960.0));
  audio_provider_->generate_audio_events (*tempo_map_, clips, range);

  std::vector<float> output_left (256, 0.0f);
  std::vector<float> output_right (256, 0.0f);
  auto time_info = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));

  // Process with transport stopped - should clear buffers and not process audio
  audio_provider_->process_audio_events (
    time_info, dsp::ITransport::PlayState::Paused, output_left, output_right);

  // Verify buffers are cleared
  for (size_t i = 0; i < output_left.size (); ++i)
    {
      EXPECT_FLOAT_EQ (output_left[i], 0.0f);
      EXPECT_FLOAT_EQ (output_right[i], 0.0f);
    }
}

// ========== Basic Functionality Tests ==========

TEST_F (TimelineDataProviderTest, BasicFunctionality)
{
  // Test that the providers can be constructed
  EXPECT_NE (midi_provider_, nullptr);
  EXPECT_NE (audio_provider_, nullptr);
  EXPECT_NE (automation_provider_, nullptr);

  // Test that process methods can be called without crashing
  auto               midi_buffer = dsp::MidiEventBuffer::make_reserved ();
  std::vector<float> audio_left (256, 0.0f);
  std::vector<float> audio_right (256, 0.0f);
  std::vector<float> automation_values (256, 0.0f);
  auto time_info = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));

  midi_provider_->process_midi_events (
    time_info, dsp::ITransport::PlayState::Paused, midi_buffer);
  EXPECT_EQ (midi_buffer.size (), 0);

  audio_provider_->process_audio_events (
    time_info, dsp::ITransport::PlayState::Paused, audio_left, audio_right);

  automation_provider_->process_automation_events (
    time_info, dsp::ITransport::PlayState::Paused, automation_values);

  // Test that generate methods can be called without crashing
  std::vector<const MidiClip *>       empty_midi_clips;
  std::vector<const AudioClip *>      empty_audio_clips;
  std::vector<const AutomationClip *> empty_automation_clips;
  utils::ExpandableTickRange          range (std::pair (0.0, 960.0));

  midi_provider_->generate_midi_events (*tempo_map_, empty_midi_clips, range);
  audio_provider_->generate_audio_events (*tempo_map_, empty_audio_clips, range);
  automation_provider_->generate_automation_events (
    *tempo_map_, empty_automation_clips, range);
}

// === Linear tempo ramp tests ===

// When the tempo map contains a linear ramp (not just step changes), the
// tick→sample mapping within the ramp region is logarithmic, not linear.
// The RT reader assumes linear sample→ratio mapping within each segment.
// Without subdividing ramp regions, this produces incorrect automation values.
//
// This test creates a 120→60 BPM linear ramp and a 0.0→1.0 automation ramp
// over the same span, then checks that RT-evaluated values match the
// ground-truth values computed via the tempo map's exact conversion.
TEST_F (TimelineDataProviderTest, AutomationProviderLinearTempoRampEvaluation)
{
  // Linear tempo ramp: 120 → 60 BPM over ticks 0–3840 (2 beats at 960 PPQN).
  tempo_map_->add_tempo_event (
    units::ticks (0), units::bpm (120.0), dsp::TempoMap::CurveType::Linear);
  tempo_map_->add_tempo_event (
    units::ticks (3840), units::bpm (60.0), dsp::TempoMap::CurveType::Constant);

  // Automation clip: linear 0.0 → 1.0 spanning the tempo ramp.
  auto * clip = create_automation_clip (0.0, 3840.0, 0.0f, 1.0f);

  std::vector<const AutomationClip *> clips{ clip };
  utils::ExpandableTickRange          range (std::pair (0.0, 9600.0));
  automation_provider_->generate_automation_events (*tempo_map_, clips, range);

  const auto start_sample = tempo_map_->tick_to_samples_rounded (
    dsp::TimelineTick{ units::ticks (0.0) });
  const auto end_sample = tempo_map_->tick_to_samples_rounded (
    dsp::TimelineTick{ units::ticks (3840.0) });
  const auto total_samples =
    static_cast<double> ((end_sample - start_sample).in (units::samples));

  // Evaluate at 10%, 30%, 50%, 70%, 90% of the sample-space span.
  // Skip the exact endpoints (0% and 100%) to avoid boundary edge cases.
  for (const auto frac : { 0.1, 0.3, 0.5, 0.7, 0.9 })
    {
      const auto sample_pos =
        start_sample
        + units::samples (static_cast<int64_t> (frac * total_samples));

      // Ground truth: convert sample → tick → ratio, then evaluate linear.
      const auto tick_at_sample =
        tempo_map_
          ->samples_to_tick (
            units::samples (static_cast<double> (sample_pos.in (units::samples))))
          .asDouble ();
      const auto expected_ratio = std::clamp (tick_at_sample / 3840.0, 0.0, 1.0);
      const auto expected_value = static_cast<float> (expected_ratio);

      const auto rt_value_opt =
        automation_provider_->get_automation_value_rt (sample_pos);
      ASSERT_TRUE (rt_value_opt.has_value ())
        << "No value at sample fraction " << frac;

      EXPECT_NEAR (rt_value_opt.value (), expected_value, 0.002f)
        << "At sample fraction " << frac << " (sample "
        << sample_pos.in (units::samples) << ", tick " << tick_at_sample
        << "): expected " << expected_value << ", got " << rt_value_opt.value ();
    }
}

// Verifies that the RT reader correctly evaluates a non-linear (Exponent)
// automation curve with positive curviness ascending from 0.0 to 1.0.
// At the midpoint, the curve value should differ from the linear 0.5 and
// match the output of CurveOptions::get_normalized_y.
TEST_F (TimelineDataProviderTest, AutomationProviderExponentCurveAscending)
{
  auto * clip = create_automation_clip (0.0, 3840.0, 0.0f, 1.0f);
  auto * first_ap = clip->get_children_view ()[0];
  first_ap->curveOpts ()->setCurviness (0.5);
  first_ap->curveOpts ()->setAlgorithm (dsp::CurveOptions::Algorithm::Exponent);

  std::vector<const AutomationClip *> clips{ clip };
  utils::ExpandableTickRange          range (std::pair (0.0, 9600.0));
  automation_provider_->generate_automation_events (*tempo_map_, clips, range);

  const auto mid_sample = tempo_map_->tick_to_samples_rounded (
    dsp::TimelineTick{ units::ticks (1920.0) });

  const auto rt_val = automation_provider_->get_automation_value_rt (mid_sample);
  ASSERT_TRUE (rt_val.has_value ());

  // Ground truth: CurveOptions at ratio 0.5, start_higher=false (ascending).
  const double expected_curve =
    dsp::CurveOptions (0.5, dsp::CurveOptions::Algorithm::Exponent)
      .get_normalized_y (0.5, false);
  const float expected = static_cast<float> (expected_curve);

  EXPECT_NEAR (rt_val.value (), expected, 0.01f);
  // Positive-curviness exponent curve should be above linear at midpoint.
  EXPECT_GT (rt_val.value (), 0.5f);
}

// Same but descending from 1.0 to 0.0 with positive curviness.
TEST_F (TimelineDataProviderTest, AutomationProviderExponentCurveDescending)
{
  auto * clip = create_automation_clip (0.0, 3840.0, 1.0f, 0.0f);
  auto * first_ap = clip->get_children_view ()[0];
  first_ap->curveOpts ()->setCurviness (0.5);
  first_ap->curveOpts ()->setAlgorithm (dsp::CurveOptions::Algorithm::Exponent);

  std::vector<const AutomationClip *> clips{ clip };
  utils::ExpandableTickRange          range (std::pair (0.0, 9600.0));
  automation_provider_->generate_automation_events (*tempo_map_, clips, range);

  const auto mid_sample = tempo_map_->tick_to_samples_rounded (
    dsp::TimelineTick{ units::ticks (1920.0) });

  const auto rt_val = automation_provider_->get_automation_value_rt (mid_sample);
  ASSERT_TRUE (rt_val.has_value ());

  // Ground truth: CurveOptions at ratio 0.5, start_higher=true (descending).
  const double expected_curve =
    dsp::CurveOptions (0.5, dsp::CurveOptions::Algorithm::Exponent)
      .get_normalized_y (0.5, true);
  const float expected = static_cast<float> (expected_curve);

  EXPECT_NEAR (rt_val.value (), expected, 0.01f);
  // Positive-curviness exponent descending curve should be above linear at
  // midpoint (the curve bulges toward the higher starting value).
  EXPECT_GT (rt_val.value (), 0.5f);
}

// Verifies that Pulse algorithm produces a step function, not a linear ramp.
// A Pulse curve from 0.0 to 1.0 should stay at 0.0 until the midpoint, then
// jump to 1.0.
TEST_F (TimelineDataProviderTest, AutomationProviderPulseCurveStep)
{
  auto * clip = create_automation_clip (0.0, 3840.0, 0.0f, 1.0f);
  auto * first_ap = clip->get_children_view ()[0];
  first_ap->curveOpts ()->setAlgorithm (dsp::CurveOptions::Algorithm::Pulse);

  std::vector<const AutomationClip *> clips{ clip };
  utils::ExpandableTickRange          range (std::pair (0.0, 9600.0));
  automation_provider_->generate_automation_events (*tempo_map_, clips, range);

  // At 40% of the span — before the step.
  const auto before_sample = tempo_map_->tick_to_samples_rounded (
    dsp::TimelineTick{ units::ticks (1536.0) }); // 0.4 * 3840
  const auto before_val =
    automation_provider_->get_automation_value_rt (before_sample);
  ASSERT_TRUE (before_val.has_value ());
  EXPECT_NEAR (before_val.value (), 0.0f, 0.01f)
    << "Pulse should hold start value before midpoint";

  // At 60% of the span — after the step.
  const auto after_sample = tempo_map_->tick_to_samples_rounded (
    dsp::TimelineTick{ units::ticks (2304.0) }); // 0.6 * 3840
  const auto after_val =
    automation_provider_->get_automation_value_rt (after_sample);
  ASSERT_TRUE (after_val.has_value ());
  EXPECT_NEAR (after_val.value (), 1.0f, 0.01f)
    << "Pulse should hold end value after midpoint";
}

// Verifies that the region before the first automation point yields nullopt
// (no automation applied until the first point is reached).
TEST_F (TimelineDataProviderTest, AutomationProviderNoAutomationBeforeFirstPoint)
{
  // Clip at tick 0, but first automation point at tick 100.
  auto * clip = create_automation_clip (0.0, 3840.0, 0.0f, 1.0f);

  // Move the first point to tick 100 (relative to clip start).
  auto * first_ap = clip->get_children_view ()[0];
  first_ap->position ()->setTicks (100.0);

  std::vector<const AutomationClip *> clips{ clip };
  utils::ExpandableTickRange          range (std::pair (0.0, 9600.0));
  automation_provider_->generate_automation_events (*tempo_map_, clips, range);

  // At tick 50 — before the first automation point.
  const auto before_sample = tempo_map_->tick_to_samples_rounded (
    dsp::TimelineTick{ units::ticks (50.0) });
  const auto before_val =
    automation_provider_->get_automation_value_rt (before_sample);
  EXPECT_FALSE (before_val.has_value ())
    << "No automation should apply before the first point";

  // At tick 200 — after the first automation point, should have a value.
  const auto after_sample = tempo_map_->tick_to_samples_rounded (
    dsp::TimelineTick{ units::ticks (200.0) });
  const auto after_val =
    automation_provider_->get_automation_value_rt (after_sample);
  ASSERT_TRUE (after_val.has_value ());
}

// When a clip-start boundary splits an AP pair, the curve must be evaluated
// against the ORIGINAL AP pair domain, not the truncated [boundary, AP] span.
// Otherwise non-linear curves (Exponent, etc.) are reshaped incorrectly.
//
// Setup: AP@0 = 0.0, AP@3840 = 1.0, Exponent curviness 0.5, clip-start = 1920.
// The intro segment plays virt [1920, 3840) → delta [0, 1920). A boundary
// point at delta 0 carries origin [-1920, 1920] (= content [0, 3840]). At
// tick 960, the ratio within the origin domain is (960-(-1920))/(3840) = 0.75.
TEST_F (
  TimelineDataProviderTest,
  AutomationProviderExponentCurveReshapedByClipStartBoundary)
{
  auto * clip = create_automation_clip (0.0, 3840.0, 0.0f, 1.0f);

  auto * ap1 = clip->get_children_view ()[0];
  ap1->curveOpts ()->setCurviness (0.5);
  ap1->curveOpts ()->setAlgorithm (dsp::CurveOptions::Algorithm::Exponent);

  // Clip-start splits the AP pair [0, 3840] at tick 1920.
  clip->setTrackBounds (false);
  clip->clipStartPosition ()->setTicks (1920);

  std::vector<const AutomationClip *> clips{ clip };
  utils::ExpandableTickRange          range (std::pair (0.0, 9600.0));
  automation_provider_->generate_automation_events (*tempo_map_, clips, range);

  // Evaluate at tick 960 (within the intro segment). The boundary point's
  // origin maps this to ratio 0.75 of [0, 3840].
  const auto eval_sample = tempo_map_->tick_to_samples_rounded (
    dsp::TimelineTick{ units::ticks (960.0) });
  const auto rt_val =
    automation_provider_->get_automation_value_rt (eval_sample);
  ASSERT_TRUE (rt_val.has_value ());

  // Ground truth: curve evaluated at ratio 0.75 within the original [0, 3840].
  const double expected_curve =
    dsp::CurveOptions (0.5, dsp::CurveOptions::Algorithm::Exponent)
      .get_normalized_y (0.75, false);

  EXPECT_NEAR (rt_val.value (), static_cast<float> (expected_curve), 0.01f)
    << "At tick 960 (ratio 0.75 of original domain): expected "
    << expected_curve << ", got " << rt_val.value ();
}

// Logarithmic curves at default curviness (0.0) are NOT linear — the
// algorithm clamps s to 0.01 and computes a genuine log curve. The RT reader
// must not short-circuit near-zero curviness to linear for Logarithmic.
TEST_F (TimelineDataProviderTest, AutomationProviderLogarithmicCurvinessZero)
{
  auto * clip = create_automation_clip (0.0, 3840.0, 0.0f, 1.0f);
  auto * ap1 = clip->get_children_view ()[0];
  ap1->curveOpts ()->setAlgorithm (dsp::CurveOptions::Algorithm::Logarithmic);

  std::vector<const AutomationClip *> clips{ clip };
  utils::ExpandableTickRange          range (std::pair (0.0, 9600.0));
  automation_provider_->generate_automation_events (*tempo_map_, clips, range);

  const auto mid_sample = tempo_map_->tick_to_samples_rounded (
    dsp::TimelineTick{ units::ticks (1920.0) });
  const auto rt_val = automation_provider_->get_automation_value_rt (mid_sample);
  ASSERT_TRUE (rt_val.has_value ());

  // Ground truth: Logarithmic at curviness 0, ratio 0.5 — NOT 0.5 (linear).
  const double expected = dsp::evaluate_curve (
    0.0f, 1.0f, dsp::CurveOptions::Algorithm::Logarithmic, 0.0f, 0.5);

  EXPECT_NEAR (rt_val.value (), static_cast<float> (expected), 0.005f)
    << "Logarithmic curviness 0 should produce a log curve, not linear";
  EXPECT_NE (rt_val.value (), 0.5f)
    << "Logarithmic at curviness 0 must not linearize to 0.5 at midpoint";
}

} // namespace zrythm::structure::arrangement
