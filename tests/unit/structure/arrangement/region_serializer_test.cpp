// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cmath>

#include "structure/arrangement/region_serializer.h"

#include <gtest/gtest.h>
#include <juce_audio_formats/juce_audio_formats.h>

namespace zrythm::structure::arrangement
{
class RegionSerializerTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    tempo_map = std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));

    // Set up MIDI region
    midi_region = std::make_unique<MidiRegion> (
      *tempo_map, registry, file_audio_source_registry, nullptr);
    midi_region->position ()->setTicks (100);
    midi_region->bounds ()->length ()->setTicks (200);
    midi_region->loopRange ()->loopStartPosition ()->setTicks (50);
    midi_region->loopRange ()->loopEndPosition ()->setTicks (150);
    midi_region->loopRange ()->clipStartPosition ()->setTicks (0);

    // Set up Audio region with proper audio source
    setup_audio_region ();
  }

  void add_midi_note (
    int    pitch,
    int    velocity,
    double position_ticks,
    double length_ticks)
  {
    // Create MidiNote using registry
    auto   note_ref = registry.create_object<MidiNote> (*tempo_map);
    auto * note = note_ref.get_object_as<MidiNote> ();
    note->setPitch (pitch);
    note->setVelocity (velocity);
    note->position ()->setTicks (position_ticks);
    note->bounds ()->length ()->setTicks (length_ticks);

    // Add to region
    midi_region->add_object (note_ref);
  }

  void setup_audio_region ()
  {
    // Create a sine wave audio source
    auto audio_source_object_ref = create_sine_wave_audio_source (4096);

    // Create audio region
    audio_region = std::make_unique<AudioRegion> (
      *tempo_map, registry, file_audio_source_registry, [] () { return true; },
      nullptr);

    audio_region->set_source (audio_source_object_ref);
    audio_region->position ()->setSamples (1000);
    audio_region->bounds ()->length ()->setSamples (500);
    audio_region->loopRange ()->loopStartPosition ()->setSamples (250);
    audio_region->loopRange ()->loopEndPosition ()->setSamples (750);
    audio_region->loopRange ()->clipStartPosition ()->setSamples (0);

    // Prepare for playback
    audio_region->prepare_to_play (1024, 44100.0);
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

    auto source_ref = file_audio_source_registry.create_object<
      dsp::FileAudioSource> (
      *sample_buffer, utils::audio::BitDepth::BIT_DEPTH_32, 44100, 120.f,
      u8"SineTestSource");

    return registry.create_object<AudioSourceObject> (
      *tempo_map, file_audio_source_registry, source_ref);
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

  std::unique_ptr<dsp::TempoMap> tempo_map;
  ArrangerObjectRegistry         registry;
  dsp::FileAudioSourceRegistry   file_audio_source_registry;
  std::unique_ptr<MidiRegion>    midi_region;
  std::unique_ptr<AudioRegion>   audio_region;
  juce::AudioSampleBuffer        test_audio_buffer;
};

// ========== MIDI Region Tests ==========

TEST_F (RegionSerializerTest, SerializeMidiEventsSimple)
{
  // Add a note within the region
  add_midi_note (60, 90, 100, 50);

  juce::MidiMessageSequence events;
  RegionSerializer::serialize_to_sequence (
    *midi_region, events, std::nullopt, std::nullopt, true, false);

  // Verify events
  EXPECT_EQ (events.getNumEvents (), 2); // Note on + note off

  // First event should be note on
  auto note_on_event = events.getEventPointer (0);
  EXPECT_TRUE (note_on_event->message.isNoteOn ());
  EXPECT_EQ (note_on_event->message.getNoteNumber (), 60);
  EXPECT_EQ (note_on_event->message.getVelocity (), 90);
  EXPECT_EQ (note_on_event->message.getTimeStamp (), 200); // 100 (position) +
                                                           // 100 (region start)

  // Second event should be note off
  auto note_off_event = events.getEventPointer (1);
  EXPECT_TRUE (note_off_event->message.isNoteOff ());
  EXPECT_EQ (note_off_event->message.getNoteNumber (), 60);
  EXPECT_EQ (note_off_event->message.getVelocity (), 90);
  EXPECT_EQ (
    note_off_event->message.getTimeStamp (),
    250); // 100 (position) + 50 (length) + 100 (region start)
}

TEST_F (RegionSerializerTest, SerializeMidiEventsWithLooping)
{
  // Add a note within the loop range
  add_midi_note (60, 90, 50, 50);

  juce::MidiMessageSequence events;
  RegionSerializer::serialize_to_sequence (
    *midi_region, events, std::nullopt, std::nullopt, true, true);

  // Should create multiple events due to looping
  // Loop length = 100 ticks (150-50)
  // Region length = 200 ticks
  // Number of repeats = ceil((200 - 50 + 0) / 100) = ceil(150/100) = 2
  EXPECT_EQ (events.getNumEvents (), 4); // Two sets of note on/off

  // First occurrence
  auto first_note_on = events.getEventPointer (0);
  EXPECT_TRUE (first_note_on->message.isNoteOn ());
  EXPECT_EQ (
    first_note_on->message.getTimeStamp (), 150); // 50 + 100 region start

  // Second occurrence (loop repeat)
  auto second_note_on = events.getEventPointer (2);
  EXPECT_TRUE (second_note_on->message.isNoteOn ());
  EXPECT_EQ (
    second_note_on->message.getTimeStamp (),
    250); // 150 (first loop end) + 100 region start
}

TEST_F (RegionSerializerTest, SerializeMidiEventsWithStartEndConstraints)
{
  // Add a note
  add_midi_note (60, 90, 100, 50);

  juce::MidiMessageSequence events;
  RegionSerializer::serialize_to_sequence (
    *midi_region, events, 150.0, 250.0, true, false);

  // Verify events
  EXPECT_EQ (events.getNumEvents (), 2); // Note on and note off

  auto note_on_event = events.getEventPointer (0);
  EXPECT_TRUE (note_on_event->message.isNoteOn ());
  EXPECT_EQ (
    note_on_event->message.getTimeStamp (), 50); // 200 - 150 (start constraint)

  auto note_off_event = events.getEventPointer (1);
  EXPECT_TRUE (note_off_event->message.isNoteOff ());
  EXPECT_EQ (note_off_event->message.getTimeStamp (), 100); // 250 - 150 (start
                                                            // constraint)
}

TEST_F (RegionSerializerTest, MutedMidiNoteNotSerialized)
{
  // Add a note and mute it
  add_midi_note (60, 90, 100, 50);
  midi_region->get_children_view ()[0]->mute ()->setMuted (true);

  juce::MidiMessageSequence events;
  RegionSerializer::serialize_to_sequence (
    *midi_region, events, std::nullopt, std::nullopt, true, true);

  // No events should be added for muted note
  EXPECT_EQ (events.getNumEvents (), 0);
}

TEST_F (RegionSerializerTest, MidiNoteOutsideLoopRangeNotSerialized)
{
  // Adjust clip start
  midi_region->loopRange ()->clipStartPosition ()->setTicks (50);

  // Add note before clip start and loop range
  add_midi_note (60, 90, 40, 10); // At 40-50 ticks, loop starts at 50

  juce::MidiMessageSequence events;
  RegionSerializer::serialize_to_sequence (
    *midi_region, events, std::nullopt, std::nullopt, true, true);

  // Should not be added when full=true
  EXPECT_EQ (events.getNumEvents (), 0);
}

TEST_F (RegionSerializerTest, SerializeMultipleMidiNotes)
{
  // Add multiple notes
  add_midi_note (60, 90, 50, 30);
  add_midi_note (64, 80, 80, 40);
  add_midi_note (67, 100, 120, 20);

  juce::MidiMessageSequence events;
  RegionSerializer::serialize_to_sequence (
    *midi_region, events, std::nullopt, std::nullopt, true, true);

  // Should have events for all notes, including looped ones
  EXPECT_GT (events.getNumEvents (), 6); // At least 3 notes * 2 events each

  // Verify note ordering (should be sorted by timestamp)
  for (int i = 1; i < events.getNumEvents (); ++i)
    {
      EXPECT_LE (
        events.getEventPointer (i - 1)->message.getTimeStamp (),
        events.getEventPointer (i)->message.getTimeStamp ());
    }
}

TEST_F (RegionSerializerTest, SerializeMidiWithoutRegionStart)
{
  // Add a note
  add_midi_note (60, 90, 100, 50);

  juce::MidiMessageSequence events;
  RegionSerializer::serialize_to_sequence (
    *midi_region, events, std::nullopt, std::nullopt, false, false);

  // Verify positions are relative to region start (not global)
  auto note_on_event = events.getEventPointer (0);
  EXPECT_EQ (note_on_event->message.getTimeStamp (), 100); // Just note position

  auto note_off_event = events.getEventPointer (1);
  EXPECT_EQ (
    note_off_event->message.getTimeStamp (), 150); // Note position + length
}

// ========== Audio Region Tests ==========

TEST_F (RegionSerializerTest, SerializeAudioRegionSimple)
{
  juce::AudioSampleBuffer buffer;

  // Serialize the audio region
  RegionSerializer::serialize_to_buffer (
    *audio_region, buffer, std::nullopt, std::nullopt);

  // Verify buffer was created and has content
  EXPECT_GT (buffer.getNumSamples (), 0);
  EXPECT_EQ (buffer.getNumChannels (), 2);

  // Verify the buffer contains our sine wave pattern with built-in fades
  auto * left_channel = buffer.getReadPointer (0);
  auto * right_channel = buffer.getReadPointer (1);

  // First audio sample should be very quiet due to built-in fade in
  EXPECT_LT (std::abs (left_channel[0]), 0.1f);
  EXPECT_LT (std::abs (right_channel[0]), 0.1f);

  // Last samples will be affected by built-in fade out
  const int last_sample = buffer.getNumSamples () - 1;
  EXPECT_NEAR (left_channel[last_sample], 0.0f, 0.1f);  // Affected by fade out
  EXPECT_NEAR (right_channel[last_sample], 0.0f, 0.1f); // Affected by fade out

  // Check a sample in the middle that should match the original sine wave
  if (buffer.getNumSamples () > AudioRegion::BUILTIN_FADE_FRAMES * 2)
    {
      const int middle_sample = buffer.getNumSamples () / 2;
      // Get expected values at this sample position
      auto [expected_left, expected_right] =
        get_expected_sine_values (middle_sample);

      // The sample should match the original sine wave (after gain and fades)
      EXPECT_NEAR (left_channel[middle_sample], expected_left, 0.01f);
      EXPECT_NEAR (right_channel[middle_sample], expected_right, 0.01f);
    }

  // Verify the sine wave pattern is consistent (only check the audio part,
  // excluding fades)
  const int start_check = AudioRegion::BUILTIN_FADE_FRAMES;
  const int end_check = last_sample - AudioRegion::BUILTIN_FADE_FRAMES;
  if (end_check > start_check)
    {
      // Check a few samples to verify the sine wave pattern
      for (int i = start_check; i < std::min (start_check + 10, end_check); ++i)
        {
          auto [expected_left, expected_right] = get_expected_sine_values (i);
          EXPECT_NEAR (left_channel[i], expected_left, 0.01f)
            << "At sample " << i;
          EXPECT_NEAR (right_channel[i], expected_right, 0.01f)
            << "At sample " << i;
        }

      // Also check that the pattern repeats by comparing samples a period apart
      const int samples_per_period =
        100; // 44100 Hz / 441 Hz = 100 samples per period
      if (end_check > start_check + samples_per_period)
        {
          for (
            int i = start_check;
            i < std::min (start_check + 5, end_check - samples_per_period); ++i)
            {

              // Values should be approximately the same (sine wave repeats)
              EXPECT_NEAR (
                left_channel[i], left_channel[i + samples_per_period], 0.01f)
                << "At sample " << i << " and " << (i + samples_per_period);
              EXPECT_NEAR (
                right_channel[i], right_channel[i + samples_per_period], 0.01f)
                << "At sample " << i << " and " << (i + samples_per_period);
            }
        }
    }
}

TEST_F (RegionSerializerTest, SerializeAudioRegionWithConstraints)
{
  juce::AudioSampleBuffer buffer;

  // Get the region's position in ticks to create appropriate constraints
  const auto region_pos_ticks = audio_region->position ()->ticks ();

  // Create constraints that overlap with the region
  // Start at region position, end at region position + 50 ticks
  const double constraint_start = region_pos_ticks;
  const double constraint_end = region_pos_ticks + 50.0;

  RegionSerializer::serialize_to_buffer (
    *audio_region, buffer, constraint_start, constraint_end);

  // Buffer should be created
  EXPECT_GT (buffer.getNumSamples (), 0);
  EXPECT_EQ (buffer.getNumChannels (), 2);

  // Calculate expected sample count: 100 ticks at 120 BPM = 0.5 seconds
  // At 44100 Hz, that's 22050 samples
  const int expected_samples = static_cast<int> (
    tempo_map->tick_to_samples (units::ticks (constraint_end - constraint_start))
      .in (units::samples));
  EXPECT_EQ (buffer.getNumSamples (), expected_samples);

  // Verify the pattern starts at the correct position in our test buffer
  auto * left_channel = buffer.getReadPointer (0);
  auto * right_channel = buffer.getReadPointer (1);

  // First sample is 0 due to built-in fade in
  EXPECT_FLOAT_EQ (left_channel[0], 0.0f);
  EXPECT_FLOAT_EQ (right_channel[0], 0.0f);

  // After the built-in fade, we should see the sine wave pattern
  if (buffer.getNumSamples () > AudioRegion::BUILTIN_FADE_FRAMES)
    {
      // Get expected values at this sample position
      auto [expected_left, expected_right] =
        get_expected_sine_values (AudioRegion::BUILTIN_FADE_FRAMES);

      // The value should be close to the expected sine wave value
      EXPECT_NEAR (
        left_channel[AudioRegion::BUILTIN_FADE_FRAMES], expected_left, 0.05f);
      EXPECT_NEAR (
        right_channel[AudioRegion::BUILTIN_FADE_FRAMES], expected_right, 0.05f);
    }
}

TEST_F (RegionSerializerTest, SerializeAudioRegionWithLooping)
{
  juce::AudioSampleBuffer buffer;

  // Serialize with full looping enabled
  RegionSerializer::serialize_to_buffer (
    *audio_region, buffer, std::nullopt, std::nullopt);

  // With looping, buffer should contain the looped pattern
  EXPECT_GT (buffer.getNumSamples (), 0);
  EXPECT_EQ (buffer.getNumChannels (), 2);

  // The loop range is 50-150 ticks (100 ticks)
  // With region length of 200 ticks, we should get 2 full loops
  // Verify the pattern repeats
  auto *    left_channel = buffer.getReadPointer (0);
  auto *    right_channel = buffer.getReadPointer (1);
  const int loop_length_samples = static_cast<int> (
    tempo_map->tick_to_samples (units::ticks (100.0)).in (units::samples));

  // First sample of second loop should match first sample of first loop
  // But both will be affected by built-in fade in
  if (buffer.getNumSamples () > loop_length_samples)
    {
      // Both samples should be 0 due to built-in fade in
      EXPECT_FLOAT_EQ (left_channel[loop_length_samples], 0.0f);
      EXPECT_FLOAT_EQ (right_channel[loop_length_samples], 0.0f);
      EXPECT_FLOAT_EQ (left_channel[0], 0.0f);
      EXPECT_FLOAT_EQ (right_channel[0], 0.0f);

      // Check samples after the built-in fade - they should match due to looping
      if (
        buffer.getNumSamples ()
        > loop_length_samples + AudioRegion::BUILTIN_FADE_FRAMES)
        {
          const int sample_in_first_loop = AudioRegion::BUILTIN_FADE_FRAMES;
          const int sample_in_second_loop =
            loop_length_samples + AudioRegion::BUILTIN_FADE_FRAMES;

          // Get expected values for both positions
          auto [expected_first, expected_first_right] =
            get_expected_sine_values (sample_in_first_loop);
          auto [expected_second, expected_second_right] =
            get_expected_sine_values (sample_in_second_loop);

          // The actual values should match their respective expected values
          EXPECT_NEAR (
            left_channel[sample_in_first_loop], expected_first, 0.01f);
          EXPECT_NEAR (
            right_channel[sample_in_first_loop], expected_first_right, 0.01f);
          EXPECT_NEAR (
            left_channel[sample_in_second_loop], expected_second, 0.01f);
          EXPECT_NEAR (
            right_channel[sample_in_second_loop], expected_second_right, 0.01f);

          // And the second loop should match the first loop pattern
          EXPECT_NEAR (
            left_channel[sample_in_second_loop],
            left_channel[sample_in_first_loop], 0.01f);
          EXPECT_NEAR (
            right_channel[sample_in_second_loop],
            right_channel[sample_in_first_loop], 0.01f);
        }
    }
}

TEST_F (RegionSerializerTest, SerializeAudioRegionWithGain)
{
  // Set gain to 0.5
  audio_region->setGain (0.5f);

  juce::AudioSampleBuffer buffer;
  RegionSerializer::serialize_to_buffer (
    *audio_region, buffer, std::nullopt, std::nullopt);

  // Verify gain was applied
  auto * left_channel = buffer.getReadPointer (0);
  auto * right_channel = buffer.getReadPointer (1);

  // First audio sample should be affected by both built-in fade in and gain
  EXPECT_LT (std::abs (left_channel[0]),
             0.1f); // 0.5 * fade * 0.5

  // Last sample should be affected by both built-in fade out and gain
  // The built-in fade out reduces the last samples to near 0
  const int last_sample = buffer.getNumSamples () - 1;
  if (last_sample >= AudioRegion::BUILTIN_FADE_FRAMES)
    {
      // Check a sample before the built-in fade out starts
      const int sample_before_fade_out =
        last_sample - AudioRegion::BUILTIN_FADE_FRAMES;

      // Get expected value at this position
      auto [expected_left, expected_right] =
        get_expected_sine_values (sample_before_fade_out);

      // The value should be the original sine wave value multiplied by gain
      EXPECT_NEAR (
        left_channel[sample_before_fade_out], expected_left * 0.5f, 0.01f);
      EXPECT_NEAR (
        right_channel[sample_before_fade_out], expected_right * 0.5f, 0.01f);
    }
}

TEST_F (RegionSerializerTest, SerializeAudioRegionWithClipStart)
{
  // Set clip start to 50 ticks
  audio_region->loopRange ()->clipStartPosition ()->setTicks (50);

  juce::AudioSampleBuffer buffer;
  RegionSerializer::serialize_to_buffer (
    *audio_region, buffer, std::nullopt, std::nullopt);

  // Buffer should be created with clip start offset applied
  EXPECT_GT (buffer.getNumSamples (), 0);
  EXPECT_EQ (buffer.getNumChannels (), 2);

  // With clip start at 50 ticks, playback should start 50 ticks into our buffer
  auto * left_channel = buffer.getReadPointer (0);
  auto * right_channel = buffer.getReadPointer (1);

  // The first sample will be 0 due to built-in fade in
  EXPECT_FLOAT_EQ (left_channel[0], 0.0f);
  EXPECT_FLOAT_EQ (right_channel[0], 0.0f);

  // After the built-in fade, we should see the clip start position
  if (buffer.getNumSamples () > AudioRegion::BUILTIN_FADE_FRAMES)
    {
      // Get expected value at this position (accounting for clip start offset)
      const int clip_start_samples = static_cast<int> (
        tempo_map->tick_to_samples (units::ticks (50)).in (units::samples));
      auto [expected_left, expected_right] = get_expected_sine_values (
        AudioRegion::BUILTIN_FADE_FRAMES + clip_start_samples);

      // The value should be close to the expected sine wave value
      EXPECT_NEAR (
        left_channel[AudioRegion::BUILTIN_FADE_FRAMES], expected_left, 0.05f);
      EXPECT_NEAR (
        right_channel[AudioRegion::BUILTIN_FADE_FRAMES], expected_right, 0.05f);
    }
}

TEST_F (RegionSerializerTest, SerializeAudioRegionZeroLengthLoop)
{
  // Set zero-length loop
  audio_region->loopRange ()->loopStartPosition ()->setTicks (100);
  audio_region->loopRange ()->loopEndPosition ()->setTicks (100);

  juce::AudioSampleBuffer buffer;
  RegionSerializer::serialize_to_buffer (
    *audio_region, buffer, std::nullopt, std::nullopt);

  // Should still work but without looping
  EXPECT_GT (buffer.getNumSamples (), 0);
  EXPECT_EQ (buffer.getNumChannels (), 2);

  // Should contain the original pattern without repetition
  auto * left_channel = buffer.getReadPointer (0);
  auto * right_channel = buffer.getReadPointer (1);

  // First sample should be very quiet due to built-in fade in
  EXPECT_LT (std::abs (left_channel[0]), 0.1f);
  EXPECT_LT (std::abs (right_channel[0]), 0.1f);

  // Last sample should be near 0 due to built-in fade out
  const int last_sample = buffer.getNumSamples () - 1;
  EXPECT_NEAR (left_channel[last_sample], 0.0f, 0.1f);
  EXPECT_NEAR (right_channel[last_sample], 0.0f, 0.1f);

  // Check a sample in the middle that should be at full value
  if (buffer.getNumSamples () > AudioRegion::BUILTIN_FADE_FRAMES * 2)
    {
      const int middle_sample = buffer.getNumSamples () / 2;
      auto [expected_left, expected_right] =
        get_expected_sine_values (middle_sample);
      EXPECT_NEAR (left_channel[middle_sample], expected_left, 0.01f);
      EXPECT_NEAR (right_channel[middle_sample], expected_right, 0.01f);
    }
}

TEST_F (RegionSerializerTest, SerializeAudioRegionWithNegativeConstraints)
{
  juce::AudioSampleBuffer buffer;

  // Serialize with constraints that don't overlap the region
  // Region is at ~43.5 ticks, so -100 to -50 should definitely not overlap
  RegionSerializer::serialize_to_buffer (*audio_region, buffer, -100.0, -50.0);

  // Buffer should be empty since constraints don't include the region
  EXPECT_EQ (buffer.getNumSamples (), 0);
}

TEST_F (RegionSerializerTest, SerializeAudioRegionLargeConstraints)
{
  juce::AudioSampleBuffer buffer;

  // Get the region's position to create appropriate constraints
  const auto region_pos_ticks = audio_region->position ()->ticks ();
  const auto region_length_ticks = audio_region->bounds ()->length ()->ticks ();

  // Serialize with large constraints that fully include the region
  // Start before the region and end after the region
  RegionSerializer::serialize_to_buffer (
    *audio_region, buffer, region_pos_ticks - 10.0,
    region_pos_ticks + region_length_ticks + 10.0);

  // Buffer should contain the full region
  EXPECT_GT (buffer.getNumSamples (), 0);
  EXPECT_EQ (buffer.getNumChannels (), 2);

  // Should contain our complete test pattern
  auto * left_channel = buffer.getReadPointer (0);
  auto * right_channel = buffer.getReadPointer (1);

  // First sample should be very quiet due to built-in fade in
  EXPECT_LT (std::abs (left_channel[0]), 0.1f);
  EXPECT_LT (std::abs (right_channel[0]), 0.1f);

  // Last sample should be near 0 due to built-in fade out
  const int last_sample = buffer.getNumSamples () - 1;
  EXPECT_NEAR (left_channel[last_sample], 0.0f, 0.1f);
  EXPECT_NEAR (right_channel[last_sample], 0.0f, 0.1f);

  // Check a sample in the middle that should be at full value
  if (buffer.getNumSamples () > AudioRegion::BUILTIN_FADE_FRAMES * 2)
    {
      const int middle_sample = buffer.getNumSamples () / 2;

      // For constraints, we need to account for the offset where the region
      // audio starts The constraint starts before the region, so there's
      // silence at the beginning
      const int region_start_offset = static_cast<int> (
        tempo_map
          ->tick_to_samples (
            units::ticks (region_pos_ticks - (region_pos_ticks - 10.0)))
          .in (units::samples));

      // Calculate the actual sample position in the audio source
      const int actual_sample_pos = middle_sample - region_start_offset;

      if (actual_sample_pos >= 0)
        {
          auto [expected_left, expected_right] =
            get_expected_sine_values (actual_sample_pos);
          EXPECT_NEAR (left_channel[middle_sample], expected_left, 0.01f);
          EXPECT_NEAR (right_channel[middle_sample], expected_right, 0.01f);
        }
      else
        {
          // This should be silence (before the region starts)
          EXPECT_NEAR (left_channel[middle_sample], 0.0f, 0.01f);
          EXPECT_NEAR (right_channel[middle_sample], 0.0f, 0.01f);
        }
    }
}

// ========== Ported FillStereoPorts Tests ==========

TEST_F (RegionSerializerTest, SerializeAudioRegionGainAndFades)
{
  // Set gain to 0.5
  audio_region->setGain (0.5f);

  juce::AudioSampleBuffer buffer;
  RegionSerializer::serialize_to_buffer (
    *audio_region, buffer, std::nullopt, std::nullopt);

  // Verify gain was applied to our test pattern
  auto * left_channel = buffer.getReadPointer (0);
  auto * right_channel = buffer.getReadPointer (1);

  // First sample: 0.5 * fade * 0.5, should be very quiet
  EXPECT_LT (std::abs (left_channel[0]), 0.1f);
  // Right channel first sample: -0.5 * fade * 0.5, should be very quiet
  EXPECT_LT (std::abs (right_channel[0]), 0.1f);

  // Check samples after the built-in fade in
  if (buffer.getNumSamples () > AudioRegion::BUILTIN_FADE_FRAMES)
    {
      const int sample_after_fade = AudioRegion::BUILTIN_FADE_FRAMES;
      auto [expected_left, expected_right] =
        get_expected_sine_values (sample_after_fade);

      // Values should be the original sine wave values multiplied by gain
      EXPECT_NEAR (left_channel[sample_after_fade], expected_left * 0.5f, 0.01f);
      EXPECT_NEAR (
        right_channel[sample_after_fade], expected_right * 0.5f, 0.01f);
    }

  // Last samples will be affected by built-in fade out
  const int last_sample = buffer.getNumSamples () - 1;
  EXPECT_NEAR (left_channel[last_sample], 0.0f, 0.1f);  // Affected by fade out
  EXPECT_NEAR (right_channel[last_sample], 0.0f, 0.1f); // Affected by fade out
}

TEST_F (RegionSerializerTest, SerializeAudioRegionWithBuiltinFadeIn)
{
  // Create a small region to test built-in fade in
  // Create a new audio source with sine wave pattern
  auto audio_source_object_ref = create_sine_wave_audio_source (5000);

  auto small_region = std::make_unique<AudioRegion> (
    *tempo_map, registry, file_audio_source_registry, [] () { return true; },
    nullptr);
  small_region->set_source (audio_source_object_ref);

  // Set small region length (less than built-in fade frames)
  small_region->position ()->setTicks (0);
  small_region->bounds ()->length ()->setTicks (100);
  small_region->prepare_to_play (1024, 44100.0);

  juce::AudioSampleBuffer buffer;
  RegionSerializer::serialize_to_buffer (
    *small_region, buffer, std::nullopt, std::nullopt);

  // Verify built-in fade in was applied
  auto * left_channel = buffer.getReadPointer (0);
  auto * right_channel = buffer.getReadPointer (1);

  // First sample should be very quiet (near 0 due to fade in)
  EXPECT_LT (std::abs (left_channel[0]), 0.1f);
  EXPECT_LT (std::abs (right_channel[0]), 0.1f);

  // Last sample should be louder
  const int last_sample = buffer.getNumSamples () - 1;
  EXPECT_GT (std::abs (left_channel[last_sample]), std::abs (left_channel[0]));
  EXPECT_GT (std::abs (right_channel[last_sample]), std::abs (right_channel[0]));
}

TEST_F (RegionSerializerTest, SerializeAudioRegionWithBuiltinFadeOut)
{
  // Create a region positioned near the end to test built-in fade out
  // Create a new audio source with sine wave pattern
  auto audio_source_object_ref = create_sine_wave_audio_source (5000);

  auto fade_out_region = std::make_unique<AudioRegion> (
    *tempo_map, registry, file_audio_source_registry, [] () { return true; },
    nullptr);
  fade_out_region->set_source (audio_source_object_ref);

  // Set region length to be just a bit more than built-in fade frames
  fade_out_region->position ()->setTicks (0);
  fade_out_region->bounds ()->length ()->setTicks (200);
  fade_out_region->prepare_to_play (1024, 44100.0);

  juce::AudioSampleBuffer buffer;
  RegionSerializer::serialize_to_buffer (
    *fade_out_region, buffer, std::nullopt, std::nullopt);

  // Verify built-in fade out was applied
  auto * left_channel = buffer.getReadPointer (0);
  auto * right_channel = buffer.getReadPointer (1);

  // First sample after fade in should be loud
  // (The very first sample is 0 due to built-in fade in)
  if (buffer.getNumSamples () > AudioRegion::BUILTIN_FADE_FRAMES)
    {
      EXPECT_GT (
        std::abs (left_channel[AudioRegion::BUILTIN_FADE_FRAMES]), 0.1f);
      EXPECT_GT (
        std::abs (right_channel[AudioRegion::BUILTIN_FADE_FRAMES]), 0.1f);
    }

  // Last sample should be very quiet (near 0 due to fade out)
  const int last_sample = buffer.getNumSamples () - 1;

  // The built-in fade out should bring the last sample very close to zero
  // We allow a small tolerance since the fade out is linear and the original
  // sine wave value might be high. With a 10-sample linear fade, the last
  // sample should be multiplied by approximately 0.1 (or less).
  EXPECT_NEAR (left_channel[last_sample], 0.0f, 0.1f);
  EXPECT_NEAR (right_channel[last_sample], 0.0f, 0.1f);

  // Also check that the fade out is working by comparing a sample before
  // the fade out with the last sample
  const int fade_out_start =
    buffer.getNumSamples () - AudioRegion::BUILTIN_FADE_FRAMES;
  if (fade_out_start > 0)
    {
      // The sample at the start of fade out should be louder than the last sample
      EXPECT_GT (
        std::abs (left_channel[fade_out_start]),
        std::abs (left_channel[last_sample]));
      EXPECT_GT (
        std::abs (right_channel[fade_out_start]),
        std::abs (right_channel[last_sample]));
    }
}

TEST_F (RegionSerializerTest, SerializeAudioRegionWithCustomFades)
{
  // Set custom fade positions
  audio_region->fadeRange ()->startOffset ()->setSamples (100);
  audio_region->fadeRange ()->endOffset ()->setSamples (100);

  juce::AudioSampleBuffer buffer;
  RegionSerializer::serialize_to_buffer (
    *audio_region, buffer, std::nullopt, std::nullopt);

  // Verify custom fades were applied
  auto * left_channel = buffer.getReadPointer (0);
  auto * right_channel = buffer.getReadPointer (1);

  // First audio sample should be affected by fade in
  EXPECT_LT (std::abs (left_channel[0]), 0.5f);
  EXPECT_LT (std::abs (right_channel[0]), 0.5f);

  // Middle samples should be at full volume
  const int middle_sample = buffer.getNumSamples () / 2;
  EXPECT_GT (std::abs (left_channel[middle_sample]), 0.4f);
  EXPECT_GT (std::abs (right_channel[middle_sample]), 0.4f);

  // Last sample should be affected by fade out
  const int last_sample = buffer.getNumSamples () - 1;
  EXPECT_LT (std::abs (left_channel[last_sample]), 0.5f);
  EXPECT_LT (std::abs (right_channel[last_sample]), 0.5f);
}

TEST_F (RegionSerializerTest, SerializeAudioRegionFadesCombinedWithGain)
{
  // Set gain and custom fades
  audio_region->setGain (0.7f);
  audio_region->fadeRange ()->startOffset ()->setSamples (50);
  audio_region->fadeRange ()->endOffset ()->setSamples (50);

  juce::AudioSampleBuffer buffer;
  RegionSerializer::serialize_to_buffer (
    *audio_region, buffer, std::nullopt, std::nullopt);

  // Verify both gain and fades were applied
  auto * left_channel = buffer.getReadPointer (0);
  auto * right_channel = buffer.getReadPointer (1);

  // First audio sample: original value * fade in * gain
  // Should be very quiet due to both fade in and gain reduction
  EXPECT_LT (std::abs (left_channel[0]), 0.35f); // 0.5 * 1.0 * 0.7 = 0.35 max
  EXPECT_LT (std::abs (right_channel[0]), 0.35f);

  // Middle sample: original value * gain (no fade)
  const int middle_sample = buffer.getNumSamples () / 2;
  auto [expected_left, expected_right] =
    get_expected_sine_values (middle_sample);

  // Values should be the original sine wave values multiplied by gain
  EXPECT_NEAR (left_channel[middle_sample], expected_left * 0.7f, 0.01f);
  EXPECT_NEAR (right_channel[middle_sample], expected_right * 0.7f, 0.01f);
}

// ========== Edge Case Tests ==========

TEST_F (RegionSerializerTest, SerializeEmptyMidiRegion)
{
  juce::MidiMessageSequence events;
  RegionSerializer::serialize_to_sequence (
    *midi_region, events, std::nullopt, std::nullopt, true, true);

  // No events should be added for empty region
  EXPECT_EQ (events.getNumEvents (), 0);
}

TEST_F (RegionSerializerTest, SerializeMidiRegionWithZeroLengthLoop)
{
  // Set loop start and end to the same position (zero-length loop)
  midi_region->loopRange ()->loopStartPosition ()->setTicks (100);
  midi_region->loopRange ()->loopEndPosition ()->setTicks (100);

  add_midi_note (60, 90, 50, 30);

  juce::MidiMessageSequence events;
  RegionSerializer::serialize_to_sequence (
    *midi_region, events, std::nullopt, std::nullopt, true, true);

  // Should have events for the original note but no loops
  EXPECT_EQ (events.getNumEvents (), 2);
}

TEST_F (RegionSerializerTest, SerializeMidiRegionWithNegativeConstraints)
{
  add_midi_note (60, 90, 100, 50);

  juce::MidiMessageSequence events;
  RegionSerializer::serialize_to_sequence (
    *midi_region, events, -50.0, 50.0, true, false);

  // Should not include any events as they're outside the constraint range
  EXPECT_EQ (events.getNumEvents (), 0);
}

TEST_F (RegionSerializerTest, SerializeMidiRegionWithLargeConstraints)
{
  add_midi_note (60, 90, 100, 50);

  juce::MidiMessageSequence events;
  RegionSerializer::serialize_to_sequence (
    *midi_region, events, 0.0, 1000.0, true, false);

  // Should include all events as they're within the large constraint range
  EXPECT_EQ (events.getNumEvents (), 2);
}

TEST_F (RegionSerializerTest, SerializeAudioRegionBeyondSourceLength)
{
  // Create a small audio source with sine wave (100 samples)
  auto audio_source_object_ref = create_sine_wave_audio_source (100);

  // Create a region that extends beyond the source length
  auto large_region = std::make_unique<AudioRegion> (
    *tempo_map, registry, file_audio_source_registry, [] () { return true; },
    nullptr);
  large_region->set_source (audio_source_object_ref);
  large_region->position ()->setSamples (0);
  large_region->bounds ()->length ()->setSamples (
    200); // 200 samples > 100 source
  large_region->prepare_to_play (1024, 44100.0);

  juce::AudioSampleBuffer buffer;
  RegionSerializer::serialize_to_buffer (
    *large_region, buffer, std::nullopt, std::nullopt);

  // Buffer should be created
  EXPECT_GT (buffer.getNumSamples (), 0);
  EXPECT_EQ (buffer.getNumChannels (), 2);

  auto * left_channel = buffer.getReadPointer (0);
  auto * right_channel = buffer.getReadPointer (1);

  // First part should contain the audio source data
  for (int i = 0; i < 100; ++i)
    {
      // First samples are affected by built-in fade in
      if (i < AudioRegion::BUILTIN_FADE_FRAMES)
        {
          // With sine wave starting at 45 degrees, the first sample is ~0.707
          // The built-in fade in reduces this, but it may still be > 0.5 for
          // early samples We'll check that the fade is being applied (values
          // should be less than the original)
          auto [expected_left, expected_right] = get_expected_sine_values (i);
          EXPECT_LT (
            std::abs (left_channel[i]), std::abs (expected_left) + 0.01f);
          EXPECT_LT (
            std::abs (right_channel[i]), std::abs (expected_right) + 0.01f);
        }
      else
        {
          auto [expected_left, expected_right] = get_expected_sine_values (i);
          EXPECT_NEAR (left_channel[i], expected_left, 0.01f);
          EXPECT_NEAR (right_channel[i], expected_right, 0.01f);
        }
    }

  // Second part should be silent (padded with zeros)
  for (int i = 100; i < buffer.getNumSamples (); ++i)
    {
      EXPECT_FLOAT_EQ (left_channel[i], 0.0f);
      EXPECT_FLOAT_EQ (right_channel[i], 0.0f);
    }
}

} // namespace zrythm::structure::arrangement
