// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cmath>

#include "structure/arrangement/region_renderer.h"

#include <gtest/gtest.h>
#include <juce_audio_formats/juce_audio_formats.h>

namespace zrythm::structure::arrangement
{
class RegionRendererTest : public ::testing::Test
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

    // Set up Automation region
    automation_region = std::make_unique<AutomationRegion> (
      *tempo_map, registry, file_audio_source_registry, nullptr);
    automation_region->position ()->setTicks (100);
    automation_region->bounds ()->length ()->setTicks (200);
  }

  void add_automation_point (float value, double position_ticks)
  {
    // Create AutomationPoint using registry
    auto   ap_ref = registry.create_object<AutomationPoint> (*tempo_map);
    auto * ap = ap_ref.get_object_as<AutomationPoint> ();
    ap->setValue (value);
    ap->position ()->setTicks (position_ticks);

    // Add to region
    automation_region->add_object (ap_ref);
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
      *sample_buffer, utils::audio::BitDepth::BIT_DEPTH_32,
      units::sample_rate (44100), 120.f, u8"SineTestSource");

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

  std::unique_ptr<dsp::TempoMap>    tempo_map;
  ArrangerObjectRegistry            registry;
  dsp::FileAudioSourceRegistry      file_audio_source_registry;
  std::unique_ptr<MidiRegion>       midi_region;
  std::unique_ptr<AudioRegion>      audio_region;
  std::unique_ptr<AutomationRegion> automation_region;
  juce::AudioSampleBuffer           test_audio_buffer;
};

// ========== MIDI Region Tests ==========

TEST_F (RegionRendererTest, SerializeMidiEventsSimple)
{
  // Add a note within the region
  add_midi_note (60, 90, 100, 50);

  juce::MidiMessageSequence events;
  RegionRenderer::serialize_to_sequence (*midi_region, events, std::nullopt);
  events.addTimeToMessages (midi_region->position ()->ticks ());

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

TEST_F (RegionRendererTest, SerializeMidiEventsWithLooping)
{
  // Add a note within the loop range
  add_midi_note (60, 90, 50, 50);

  juce::MidiMessageSequence events;
  RegionRenderer::serialize_to_sequence (*midi_region, events);
  events.addTimeToMessages (midi_region->position ()->ticks ());

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

#if 0
TEST_F (RegionRendererTest, SerializeMidiEventsWithStartEndConstraints)
{
  // Add a note
  add_midi_note (60, 90, 100, 50);

  juce::MidiMessageSequence events;
  RegionRenderer::serialize_to_sequence (
    *midi_region, events, std::make_pair (150.0, 250.0));
  events.addTimeToMessages (midi_region->position ()->ticks ());

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
#endif

TEST_F (RegionRendererTest, MutedMidiNoteNotSerialized)
{
  // Add a note and mute it
  add_midi_note (60, 90, 100, 50);
  midi_region->get_children_view ()[0]->mute ()->setMuted (true);

  juce::MidiMessageSequence events;
  RegionRenderer::serialize_to_sequence (*midi_region, events);
  events.addTimeToMessages (midi_region->position ()->ticks ());

  // No events should be added for muted note
  EXPECT_EQ (events.getNumEvents (), 0);
}

TEST_F (RegionRendererTest, MidiNoteOutsideLoopRangeNotSerialized)
{
  // Adjust clip start
  midi_region->loopRange ()->clipStartPosition ()->setTicks (50);

  // Add note before clip start and loop range
  add_midi_note (60, 90, 40, 10); // At 40-50 ticks, loop starts at 50

  juce::MidiMessageSequence events;
  RegionRenderer::serialize_to_sequence (*midi_region, events);
  events.addTimeToMessages (midi_region->position ()->ticks ());

  // Should not be added when full=true
  EXPECT_EQ (events.getNumEvents (), 0);
}

TEST_F (RegionRendererTest, SerializeMultipleMidiNotes)
{
  // Add multiple notes
  add_midi_note (60, 90, 50, 30);
  add_midi_note (64, 80, 80, 40);
  add_midi_note (67, 100, 120, 20);

  juce::MidiMessageSequence events;
  RegionRenderer::serialize_to_sequence (*midi_region, events);
  events.addTimeToMessages (midi_region->position ()->ticks ());

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

TEST_F (RegionRendererTest, SerializeMidiWithoutRegionStart)
{
  // Add a note
  add_midi_note (60, 90, 100, 50);

  juce::MidiMessageSequence events;
  RegionRenderer::serialize_to_sequence (*midi_region, events);

  // Verify positions are relative to region start (not global)
  auto note_on_event = events.getEventPointer (0);
  EXPECT_EQ (note_on_event->message.getTimeStamp (), 100); // Just note position

  auto note_off_event = events.getEventPointer (1);
  EXPECT_EQ (
    note_off_event->message.getTimeStamp (), 150); // Note position + length
}

// ========== Audio Region Tests ==========

TEST_F (RegionRendererTest, SerializeAudioRegionSimple)
{
  juce::AudioSampleBuffer buffer;

  // Serialize the audio region
  RegionRenderer::serialize_to_buffer (*audio_region, buffer);

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

#if 0
TEST_F (RegionRendererTest, SerializeAudioRegionWithConstraints)
{
  juce::AudioSampleBuffer buffer;

  // Get the region's position in ticks to create appropriate constraints
  const auto region_pos_ticks = audio_region->position ()->ticks ();

  // Create constraints that overlap with the region
  // Start at region position, end at region position + 50 ticks
  const double constraint_start = region_pos_ticks;
  const double constraint_end = region_pos_ticks + 50.0;

  RegionRenderer::serialize_to_buffer (
    *audio_region, buffer, std::make_pair (constraint_start, constraint_end));

  // Buffer should be created
  EXPECT_GT (buffer.getNumSamples (), 0);
  EXPECT_EQ (buffer.getNumChannels (), 2);

  // Calculate expected sample count: 100 ticks at 120 BPM = 0.5 seconds
  // At 44100 Hz, that's 22050 samples
  const int expected_samples = static_cast<int> (
    tempo_map
      ->tick_to_samples_rounded (units::ticks (constraint_end - constraint_start))
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
#endif

TEST_F (RegionRendererTest, SerializeAudioRegionWithLooping)
{
  juce::AudioSampleBuffer buffer;

  // Serialize with full looping enabled
  RegionRenderer::serialize_to_buffer (*audio_region, buffer);

  // With looping, buffer should contain the looped pattern
  EXPECT_GT (buffer.getNumSamples (), 0);
  EXPECT_EQ (buffer.getNumChannels (), 2);

  // The loop range is 50-150 ticks (100 ticks)
  // With region length of 200 ticks, we should get 2 full loops
  // Verify the pattern repeats
  auto *    left_channel = buffer.getReadPointer (0);
  auto *    right_channel = buffer.getReadPointer (1);
  const int loop_length_samples = static_cast<int> (
    tempo_map->tick_to_samples_rounded (units::ticks (100.0))
      .in (units::samples));

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

TEST_F (RegionRendererTest, SerializeAudioRegionWithGain)
{
  // Set gain to 0.5
  audio_region->setGain (0.5f);

  juce::AudioSampleBuffer buffer;
  RegionRenderer::serialize_to_buffer (*audio_region, buffer);

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

TEST_F (RegionRendererTest, SerializeAudioRegionWithClipStart)
{
  // Set clip start to 50 samples
  audio_region->loopRange ()->clipStartPosition ()->setSamples (50);

  juce::AudioSampleBuffer buffer;
  RegionRenderer::serialize_to_buffer (*audio_region, buffer);

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
      const int clip_start_samples = 50;
      auto [expected_left, expected_right] = get_expected_sine_values (
        AudioRegion::BUILTIN_FADE_FRAMES + clip_start_samples);

      // The value should be close to the expected sine wave value
      EXPECT_NEAR (
        left_channel[AudioRegion::BUILTIN_FADE_FRAMES], expected_left, 0.05f);
      EXPECT_NEAR (
        right_channel[AudioRegion::BUILTIN_FADE_FRAMES], expected_right, 0.05f);
    }
}

#if 0
TEST_F (RegionRendererTest, SerializeAudioRegionWithNegativeConstraints)
{
  juce::AudioSampleBuffer buffer;

  // Serialize with constraints that don't overlap the region
  // Region is at ~43.5 ticks, so -100 to -50 should definitely not overlap
  RegionRenderer::serialize_to_buffer (
    *audio_region, buffer, std::make_pair (-100.0, -50.0));

  // Buffer should be empty since constraints don't include the region
  EXPECT_EQ (buffer.getNumSamples (), 0);
}

TEST_F (RegionRendererTest, SerializeAudioRegionLargeConstraints)
{
  juce::AudioSampleBuffer buffer;

  // Get the region's position to create appropriate constraints
  const auto region_pos_ticks = audio_region->position ()->ticks ();
  const auto region_length_ticks = audio_region->bounds ()->length ()->ticks ();

  // Serialize with large constraints that fully include the region
  // Start before the region and end after the region
  RegionRenderer::serialize_to_buffer (
    *audio_region, buffer,
    std::make_pair (
      region_pos_ticks - 10.0, region_pos_ticks + region_length_ticks + 10.0));

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
      const auto middle_sample = buffer.getNumSamples () / 2;

      // For constraints, we need to account for the offset where the region
      // audio starts The constraint starts before the region, so there's
      // silence at the beginning
      const auto region_start_offset =
        tempo_map
          ->tick_to_samples_rounded (
            units::ticks (region_pos_ticks - (region_pos_ticks - 10.0)))
          .in (units::samples);

      // Calculate the actual sample position in the audio source
      const auto actual_sample_pos = middle_sample - region_start_offset;

      if (actual_sample_pos >= 0)
        {
          auto [expected_left, expected_right] =
            get_expected_sine_values (static_cast<int> (actual_sample_pos));
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
#endif

// ========== Ported FillStereoPorts Tests ==========

TEST_F (RegionRendererTest, SerializeAudioRegionGainAndFades)
{
  // Set gain to 0.5
  audio_region->setGain (0.5f);

  juce::AudioSampleBuffer buffer;
  RegionRenderer::serialize_to_buffer (*audio_region, buffer);

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

TEST_F (RegionRendererTest, SerializeAudioRegionWithBuiltinFadeIn)
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

  juce::AudioSampleBuffer buffer;
  RegionRenderer::serialize_to_buffer (*audio_region, buffer);

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

TEST_F (RegionRendererTest, SerializeAudioRegionWithBuiltinFadeOut)
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

  juce::AudioSampleBuffer buffer;
  RegionRenderer::serialize_to_buffer (*fade_out_region, buffer);

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

TEST_F (RegionRendererTest, SerializeAudioRegionWithCustomFades)
{
  // Set custom fade positions
  audio_region->fadeRange ()->startOffset ()->setSamples (100);
  audio_region->fadeRange ()->endOffset ()->setSamples (100);

  juce::AudioSampleBuffer buffer;
  RegionRenderer::serialize_to_buffer (*audio_region, buffer);

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

TEST_F (RegionRendererTest, SerializeAudioRegionFadesCombinedWithGain)
{
  // Set gain and custom fades
  audio_region->setGain (0.7f);
  audio_region->fadeRange ()->startOffset ()->setSamples (50);
  audio_region->fadeRange ()->endOffset ()->setSamples (50);

  juce::AudioSampleBuffer buffer;
  RegionRenderer::serialize_to_buffer (*audio_region, buffer);

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

TEST_F (RegionRendererTest, SerializeEmptyMidiRegion)
{
  juce::MidiMessageSequence events;
  RegionRenderer::serialize_to_sequence (*midi_region, events);

  // No events should be added for empty region
  EXPECT_EQ (events.getNumEvents (), 0);
}

#if 0
TEST_F (RegionRendererTest, SerializeMidiRegionWithNegativeConstraints)
{
  add_midi_note (60, 90, 100, 50);

  juce::MidiMessageSequence events;
  RegionRenderer::serialize_to_sequence (
    *midi_region, events, std::make_pair (-50.0, 50.0));

  // Should not include any events as they're outside the constraint range
  EXPECT_EQ (events.getNumEvents (), 0);
}

TEST_F (RegionRendererTest, SerializeMidiRegionWithLargeConstraints)
{
  add_midi_note (60, 90, 100, 50);

  juce::MidiMessageSequence events;
  RegionRenderer::serialize_to_sequence (
    *midi_region, events, std::make_pair (0.0, 1000.0));

  // Should include all events as they're within the large constraint range
  EXPECT_EQ (events.getNumEvents (), 2);
}
#endif

TEST_F (RegionRendererTest, SerializeAudioRegionBeyondSourceLength)
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

  juce::AudioSampleBuffer buffer;
  RegionRenderer::serialize_to_buffer (*large_region, buffer);

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

// ========== Automation Region Tests ==========

TEST_F (RegionRendererTest, SerializeAutomationRegionSimple)
{
  // Add a simple automation point
  add_automation_point (0.5f, 100);

  // Pass empty vector - the API will resize it appropriately
  std::vector<float> values;

  RegionRenderer::serialize_to_automation_values (*automation_region, values);

  // Calculate the sample position of the automation point
  const auto point_samples = static_cast<size_t> (
    tempo_map->tick_to_samples_rounded (units::ticks (100)).in (units::samples));

  // Verify values before the first point are -1.0
  for (size_t i = 0; i < point_samples; ++i)
    {
      EXPECT_FLOAT_EQ (values[i], -1.0f);
    }

  // Verify values from the first point onward are 0.5
  for (size_t i = point_samples; i < values.size (); ++i)
    {
      EXPECT_FLOAT_EQ (values[i], 0.5f);
    }
}

TEST_F (RegionRendererTest, SerializeAutomationRegionWithInterpolation)
{
  // Add two automation points
  add_automation_point (0.0f, 50);
  add_automation_point (1.0f, 150);

  // Pass empty vector - the API will resize it appropriately
  std::vector<float> values;

  RegionRenderer::serialize_to_automation_values (*automation_region, values);

  // Verify interpolation
  // First point at 50 ticks, second at 150 ticks
  const auto first_point_samples = static_cast<size_t> (
    tempo_map->tick_to_samples_rounded (units::ticks (50)).in (units::samples));
  const auto second_point_samples = static_cast<size_t> (
    tempo_map->tick_to_samples_rounded (units::ticks (150)).in (units::samples));

  // Before first point should be -1.0
  for (size_t i = 0; i < first_point_samples; ++i)
    {
      EXPECT_FLOAT_EQ (values[i], -1.0f);
    }

  // After second point should be 1.0
  for (size_t i = second_point_samples; i < values.size (); ++i)
    {
      EXPECT_FLOAT_EQ (values[i], 1.0f);
    }

  // Between points should interpolate
  if (second_point_samples > first_point_samples + 1)
    {
      const size_t mid_point = (first_point_samples + second_point_samples) / 2;
      EXPECT_GT (values[mid_point], 0.0f);
      EXPECT_LT (values[mid_point], 1.0f);
    }
}

TEST_F (RegionRendererTest, SerializeAutomationRegionWithLooping)
{
  // Add an automation point within the loop range
  add_automation_point (0.7f, 75);

  // Pass empty vector - the API will resize it appropriately
  std::vector<float> values;

  RegionRenderer::serialize_to_automation_values (*automation_region, values);

  // With looping, the value should appear multiple times
  // Loop range is 50-150 ticks (100 ticks)
  // Region length is 200 ticks, so we get 2 loops
  // The point is at 75 ticks, which is within the loop range (50-150)
  // So the first occurrence is at 75 ticks (relative to region start)
  const auto first_point_samples = static_cast<size_t> (
    tempo_map->tick_to_samples_rounded (units::ticks (75)).in (units::samples));

  // Check that values before the first point are -1.0
  for (size_t i = 0; i < first_point_samples && i < values.size (); ++i)
    {
      EXPECT_FLOAT_EQ (values[i], -1.0f);
    }

  // Check that the value appears at the right positions in both loops
  EXPECT_FLOAT_EQ (values[first_point_samples], 0.7f);

  // Second occurrence should be one loop length later
  // The loop starts at 50 ticks, so the second occurrence is at 75 + 100 = 175
  // ticks
  const auto second_point_samples = static_cast<size_t> (
    tempo_map->tick_to_samples_rounded (units::ticks (175)).in (units::samples));
  if (second_point_samples < values.size ())
    {
      EXPECT_FLOAT_EQ (values[second_point_samples], 0.7f);
    }
}

#if 0
TEST_F (RegionRendererTest, SerializeAutomationRegionWithConstraints)
{
  // Add automation points
  add_automation_point (0.0f, 50);
  add_automation_point (1.0f, 150);

  // Serialize with constraints
  const double constraint_start = 100.0;
  const double constraint_end = 200.0;

  // Pass empty vector - the API will resize it appropriately
  std::vector<float> values;

  RegionRenderer::serialize_to_automation_values (
    *automation_region, values,
    std::make_pair (constraint_start, constraint_end));

  // Verify the constraint range is respected
  EXPECT_GT (values.size (), 0);

  // The constraint starts at 100 ticks, which is between our two points
  // The first point is at 150 ticks global (within constraint)
  // The second point is at 250 ticks global (outside constraint)
  // So we should see interpolated values but not the value 1.0
  bool found_interpolated = false;
  bool found_one = false;
  for (float value : values)
    {
      if (value > 0.0f && value < 1.0f)
        {
          found_interpolated = true;
        }
      else if (value >= 0.999f) // Allow for floating point precision
        {
          found_one = true;
        }
    }
  EXPECT_TRUE (found_interpolated);
  EXPECT_FALSE (found_one); // The value 1.0 should be trimmed as it's outside
                            // the constraint
}
#endif

TEST_F (RegionRendererTest, SerializeAutomationRegionEmpty)
{
  // Don't add any automation points

  // Pass empty vector - the API will resize it appropriately
  std::vector<float> values;

  RegionRenderer::serialize_to_automation_values (*automation_region, values);

  // All values should be -1.0 (no automation points)
  for (float value : values)
    {
      EXPECT_FLOAT_EQ (value, -1.0f);
    }
}

TEST_F (RegionRendererTest, SerializeAutomationRegionWithCurve)
{
  // Add two automation points
  add_automation_point (0.0f, 50);
  add_automation_point (1.0f, 150);

  // Set curve options on the first point
  auto * first_ap = automation_region->get_children_view ()[0];
  first_ap->curveOpts ()->setCurviness (0.5);
  first_ap->curveOpts ()->setAlgorithm (dsp::CurveOptions::Algorithm::Exponent);

  // Pass empty vector - the API will resize it appropriately
  std::vector<float> values;

  RegionRenderer::serialize_to_automation_values (*automation_region, values);

  // Verify that the curve affects the interpolation
  const auto first_point_samples = static_cast<size_t> (
    tempo_map->tick_to_samples_rounded (units::ticks (50)).in (units::samples));
  const auto second_point_samples = static_cast<size_t> (
    tempo_map->tick_to_samples_rounded (units::ticks (150)).in (units::samples));

  // Check a point in the middle - with exponential curve, it should be
  // different from linear interpolation
  if (second_point_samples > first_point_samples + 1)
    {
      const size_t mid_point = (first_point_samples + second_point_samples) / 2;
      // With exponential curve and positive curviness, the value should be
      // higher than linear interpolation (0.5)
      EXPECT_GT (values[mid_point], 0.5f);
      EXPECT_LT (values[mid_point], 1.0f);
    }
}

#if 0
TEST_F (RegionRendererTest, SerializeAutomationRegionNonOverlappingConstraints)
{
  // Add an automation point
  add_automation_point (0.5f, 100);

  // Use constraints that don't overlap the region
  const double constraint_start = -100.0;
  const double constraint_end = -50.0;

  // Pass empty vector - the API will resize it appropriately
  std::vector<float> values;

  RegionRenderer::serialize_to_automation_values (
    *automation_region, values,
    std::make_pair (constraint_start, constraint_end));

  // All values should be -1.0 (no automation points in constraint)
  for (float value : values)
    {
      EXPECT_FLOAT_EQ (value, -1.0f);
    }
}

TEST_F (RegionRendererTest, SerializeAutomationRegionLargeConstraints)
{
  // Add automation points
  add_automation_point (0.0f, 50);
  add_automation_point (1.0f, 150);

  // Use large constraints that fully include the region
  const auto region_pos = automation_region->position ()->ticks ();
  const auto region_length = automation_region->bounds ()->length ()->ticks ();

  const double constraint_start = region_pos - 50.0;
  const double constraint_end = region_pos + region_length + 50.0;

  // Pass empty vector - the API will resize it appropriately
  std::vector<float> values;

  RegionRenderer::serialize_to_automation_values (
    *automation_region, values,
    std::make_pair (constraint_start, constraint_end));

  // Verify the constraint includes padding before and after the region
  EXPECT_GT (values.size (), 0);

  // Calculate where the region starts in the constraint
  const auto region_start_offset = static_cast<size_t> (
    tempo_map->tick_to_samples_rounded (units::ticks (50.0)).in (units::samples));

  // Before the region should be -1.0
  for (size_t i = 0; i < region_start_offset && i < values.size (); ++i)
    {
      EXPECT_FLOAT_EQ (values[i], -1.0f);
    }

  // Within the region should have the automation values
  // (More detailed checks would require knowing exact sample positions)
}
#endif

TEST_F (RegionRendererTest, SerializeAutomationRegionWithClipStart)
{
  // Set clip start
  automation_region->loopRange ()->clipStartPosition ()->setTicks (25);

  // Add an automation point before clip start
  add_automation_point (0.3f, 30);

  // Pass empty vector - the API will resize it appropriately
  std::vector<float> values;

  RegionRenderer::serialize_to_automation_values (*automation_region, values);

  // With clip start, the point before clip start should not appear
  // The output should start from the clip start position
  EXPECT_GT (values.size (), 0);

  // The values should reflect the clip start offset
  // (Specific verification would depend on exact sample calculations)
}

TEST_F (RegionRendererTest, SerializeAutomationRegionEndingMidCurve)
{
  // Create a region that ends in the middle of a curve
  // First point at 50 ticks, second point at 250 ticks (outside region)
  add_automation_point (0.0f, 50);
  add_automation_point (1.0f, 250);

  // Set region length to 200 ticks (ends at 300 ticks, which is in the middle)
  automation_region->bounds ()->length ()->setTicks (200);

  // Pass empty vector - the API will resize it appropriately
  std::vector<float> values;

  RegionRenderer::serialize_to_automation_values (*automation_region, values);

  // Calculate sample positions
  const auto first_point_samples = static_cast<size_t> (
    tempo_map->tick_to_samples_rounded (units::ticks (50)).in (units::samples));
  const auto region_end_samples = static_cast<size_t> (
    tempo_map->tick_to_samples_rounded (units::ticks (300)).in (units::samples));

  // Before first point should be -1.0
  for (size_t i = 0; i < first_point_samples && i < values.size (); ++i)
    {
      EXPECT_FLOAT_EQ (values[i], -1.0f);
    }

  // Values should interpolate from first point toward the second point
  // The region ends before reaching the second point
  bool found_interpolation = false;
  for (
    size_t i = first_point_samples;
    i < values.size () && i < region_end_samples; ++i)
    {
      if (values[i] > 0.0f && values[i] < 1.0f)
        {
          found_interpolation = true;
        }
    }
  EXPECT_TRUE (found_interpolation);

  // The last value should be the interpolated value at the region end
  if (values.size () > 0)
    {
      const auto last_value = values[values.size () - 1];
      EXPECT_GT (last_value, 0.0f); // Should be interpolated, not 0.0
      EXPECT_LT (last_value, 1.0f); // Should be interpolated, not 1.0
    }
}

TEST_F (RegionRendererTest, SerializeAutomationRegionLoopedEndingMidCurve)
{
  // Create a looped region that ends in the middle of a curve
  add_automation_point (0.0f, 25);  // Before loop
  add_automation_point (0.5f, 75);  // Inside loop
  add_automation_point (1.0f, 125); // Inside loop
  add_automation_point (
    0.2f, 175); // After loop (won't be reached in first loop)

  // Set up looping: loop from 50-150 ticks
  automation_region->loopRange ()->setTrackBounds (false);
  automation_region->loopRange ()->loopStartPosition ()->setTicks (50);
  automation_region->loopRange ()->loopEndPosition ()->setTicks (150);
  // Set region length to 200 ticks (ends at 200 ticks, which is in the middle
  // of interpolating from 0.5 to 1.0 in the second loop)
  // Playback order: 0-150 (first iteration), 50-200 (second loop, ends mid-curve)
  automation_region->bounds ()->length ()->setTicks (200);

  // Pass empty vector - the API will resize it appropriately
  std::vector<float> values;

  RegionRenderer::serialize_to_automation_values (*automation_region, values);

  // Calculate sample positions
  const auto first_point_samples = static_cast<size_t> (
    tempo_map->tick_to_samples_rounded (units::ticks (25)).in (units::samples));
  const auto second_point_samples = static_cast<size_t> (
    tempo_map->tick_to_samples_rounded (units::ticks (75)).in (units::samples));
  const auto third_point_samples = static_cast<size_t> (
    tempo_map->tick_to_samples_rounded (units::ticks (125)).in (units::samples));
  const auto loop_point_samples = static_cast<size_t> (
    tempo_map->tick_to_samples_rounded (units::ticks (150)).in (units::samples));

  // Before first point should be -1.0
  for (size_t i = 0; i < first_point_samples && i < values.size (); ++i)
    {
      EXPECT_FLOAT_EQ (values[i], -1.0f);
    }

  // Check that we have the expected values at each point
  EXPECT_FLOAT_EQ (values[first_point_samples], 0.0f);
  EXPECT_FLOAT_EQ (values[second_point_samples], 0.5f);
  EXPECT_FLOAT_EQ (values[third_point_samples], 1.0f);

  // Check that values are interpolated from the third point (1.0) to the fourth
  // point (0.2) in the first loop
  const auto check_point_after_third_point = third_point_samples + 10;
  // The value should be between 1.0 and 0.2 but closer to 1.0
  const auto check_value_after_third_point =
    values[check_point_after_third_point];
  EXPECT_GT (
    check_value_after_third_point, 0.2f); // Should be > 0.5 (closer to 1.0)
  EXPECT_LT (check_value_after_third_point, 1.0f); // Should be < 1.0

  // Check just before we loop we're still interpolating
  const auto check_point_before_loop_back = loop_point_samples - 1;
  const auto check_value_before_loop_back = values[check_point_before_loop_back];
  EXPECT_GT (
    check_value_before_loop_back, 0.2f); // Should be > 0.5 (closer to 1.0)
  EXPECT_LT (
    check_value_before_loop_back,
    check_value_after_third_point); // Should be < the previous value since we
                                    // are curving downwards

  // Check the parts after looping back
  const auto samples_after_looping_back = static_cast<size_t> (
    tempo_map->tick_to_samples_rounded (units::ticks (150)).in (units::samples));
  const auto samples_after_looping_back_plus_25 = static_cast<size_t> (
    tempo_map->tick_to_samples_rounded (units::ticks (175)).in (units::samples));
  const auto samples_at_end = static_cast<size_t> (
    tempo_map->tick_to_samples_rounded (units::ticks (200)).in (units::samples));
  EXPECT_NEAR (values[samples_after_looping_back], 0.25f, 0.0001f);
  EXPECT_NEAR (values[samples_after_looping_back_plus_25], 0.5f, 0.0001f);
  EXPECT_NEAR (values[samples_at_end - 1], 0.75f, 0.001f);
}

// Parameterized test fixture for automation curves with different curviness
// values
class RegionRendererTestAutomationCurvesTest
    : public RegionRendererTest,
      public ::testing::WithParamInterface<float>
{
};

// Test parameters for curviness values
INSTANTIATE_TEST_SUITE_P (
  CurvinessValues,
  RegionRendererTestAutomationCurvesTest,
  ::testing::Values (-0.5f, 0.0f, 0.5f));

TEST_P (
  RegionRendererTestAutomationCurvesTest,
  SerializeAutomationRegionCurveFromLowerToHigher)
{
  const float curviness = GetParam ();

  // Create a curve that goes from a lower value to a higher value
  // This is the specific case that was reported as broken
  add_automation_point (0.2f, 50);  // Lower value at start
  add_automation_point (0.8f, 150); // Higher value at end

  // Set curve options on the first point to ensure we're testing the curve logic
  auto * first_ap = automation_region->get_children_view ()[0];
  first_ap->curveOpts ()->setCurviness (curviness);
  first_ap->curveOpts ()->setAlgorithm (dsp::CurveOptions::Algorithm::Exponent);

  // Pass empty vector - the API will resize it appropriately
  std::vector<float> values;

  RegionRenderer::serialize_to_automation_values (*automation_region, values);

  // Calculate sample positions
  const auto first_point_samples = static_cast<size_t> (
    tempo_map->tick_to_samples_rounded (units::ticks (50)).in (units::samples));
  const auto second_point_samples = static_cast<size_t> (
    tempo_map->tick_to_samples_rounded (units::ticks (150)).in (units::samples));

  // Verify the first point has the correct value
  if (first_point_samples < values.size ())
    {
      EXPECT_FLOAT_EQ (values[first_point_samples], 0.2f)
        << "First automation point should have value 0.2f";
    }

  // Verify the second point has the correct value
  if (second_point_samples < values.size ())
    {
      EXPECT_FLOAT_EQ (values[second_point_samples], 0.8f)
        << "Second automation point should have value 0.8f";
    }

  // Check that interpolation is working correctly between the points
  // The curve should go from lower (0.2) to higher (0.8)
  if (second_point_samples > first_point_samples + 1 && values.size () > 0)
    {
      // Check a point in the middle of the curve
      const size_t mid_point = (first_point_samples + second_point_samples) / 2;

      // The value should be between 0.2 and 0.8
      if (mid_point < values.size ())
        {
          const auto mid_value = values[mid_point];
          EXPECT_GT (mid_value, 0.2f)
            << "Middle value should be greater than start value (0.2f)";
          EXPECT_LT (mid_value, 0.8f)
            << "Middle value should be less than end value (0.8f)";

          // Check curviness behavior
          if (curviness > 0.0f)
            {
              // With positive curviness going from lower to higher, the curve
              // should be above the linear interpolation line
              EXPECT_GT (mid_value, 0.5f)
                << "With positive curviness, the curve should be above linear interpolation (0.5f)";
            }
          else if (curviness < 0.0f)
            {
              // With negative curviness going from lower to higher, the curve
              // should be below the linear interpolation line
              EXPECT_LT (mid_value, 0.5f)
                << "With negative curviness, the curve should be below linear interpolation (0.5f)";
            }
          else
            {
              // With zero curviness (linear), the value should be close to 0.5
              EXPECT_NEAR (mid_value, 0.5f, 0.01f)
                << "With zero curviness, the curve should be linear (0.5f)";
            }
        }

      // Verify monotonic increase - values should always be increasing
      // from the first point to the second point
      bool  found_decrease = false;
      float prev_value = values[first_point_samples];
      for (
        size_t i = first_point_samples + 1;
        i < second_point_samples && i < values.size (); ++i)
        {
          if (values[i] < prev_value - 0.0001f) // Allow small floating point
                                                // errors
            {
              found_decrease = true;
              break;
            }
          prev_value = values[i];
        }
      EXPECT_FALSE (found_decrease)
        << "Curve from lower to higher should be monotonically increasing";
    }
}

TEST_P (
  RegionRendererTestAutomationCurvesTest,
  SerializeAutomationRegionCurveFromHigherToLower)
{
  const float curviness = GetParam ();

  // Create a curve that goes from a higher value to a lower value
  // This tests the opposite direction to ensure both cases work
  add_automation_point (0.8f, 50);  // Higher value at start
  add_automation_point (0.2f, 150); // Lower value at end

  // Set curve options on the first point
  auto * first_ap = automation_region->get_children_view ()[0];
  first_ap->curveOpts ()->setCurviness (curviness);
  first_ap->curveOpts ()->setAlgorithm (dsp::CurveOptions::Algorithm::Exponent);

  // Pass empty vector - the API will resize it appropriately
  std::vector<float> values;

  RegionRenderer::serialize_to_automation_values (*automation_region, values);

  // Calculate sample positions
  const auto first_point_samples = static_cast<size_t> (
    tempo_map->tick_to_samples_rounded (units::ticks (50)).in (units::samples));
  const auto second_point_samples = static_cast<size_t> (
    tempo_map->tick_to_samples_rounded (units::ticks (150)).in (units::samples));

  // Verify the first point has the correct value
  if (first_point_samples < values.size ())
    {
      EXPECT_FLOAT_EQ (values[first_point_samples], 0.8f)
        << "First automation point should have value 0.8f";
    }

  // Verify the second point has the correct value
  if (second_point_samples < values.size ())
    {
      EXPECT_FLOAT_EQ (values[second_point_samples], 0.2f)
        << "Second automation point should have value 0.2f";
    }

  // Check that interpolation is working correctly between the points
  // The curve should go from higher (0.8) to lower (0.2)
  if (second_point_samples > first_point_samples + 1 && values.size () > 0)
    {
      // Check a point in the middle of the curve
      const size_t mid_point = (first_point_samples + second_point_samples) / 2;

      // The value should be between 0.2 and 0.8
      if (mid_point < values.size ())
        {
          const auto mid_value = values[mid_point];
          EXPECT_LT (mid_value, 0.8f)
            << "Middle value should be less than start value (0.8f)";
          EXPECT_GT (mid_value, 0.2f)
            << "Middle value should be greater than end value (0.2f)";

          // Check curviness behavior
          if (curviness > 0.0f)
            {
              // With positive curviness going from higher to lower, the curve
              // should be above the linear interpolation line
              EXPECT_GT (mid_value, 0.5f)
                << "With positive curviness, the curve should be above linear interpolation (0.5f)";
            }
          else if (curviness < 0.0f)
            {
              // With negative curviness going from higher to lower, the curve
              // should be below the linear interpolation line
              EXPECT_LT (mid_value, 0.5f)
                << "With negative curviness, the curve should be below linear interpolation (0.5f)";
            }
          else
            {
              // With zero curviness (linear), the value should be close to 0.5
              EXPECT_NEAR (mid_value, 0.5f, 0.01f)
                << "With zero curviness, the curve should be linear (0.5f)";
            }
        }

      // Verify monotonic decrease - values should always be decreasing
      // from the first point to the second point
      bool  found_increase = false;
      float prev_value = values[first_point_samples];
      for (
        size_t i = first_point_samples + 1;
        i < second_point_samples && i < values.size (); ++i)
        {
          if (values[i] > prev_value + 0.0001f) // Allow small floating point
                                                // errors
            {
              found_increase = true;
              break;
            }
          prev_value = values[i];
        }
      EXPECT_FALSE (found_increase)
        << "Curve from higher to lower should be monotonically decreasing";
    }
}

TEST_F (RegionRendererTest, SerializeAutomationRegionWithPointBeforeRegion)
{
  // Add an automation point before the region start (negative position)
  add_automation_point (0.2f, -25); // 25 ticks before region start
  add_automation_point (0.8f, 150); // 150 ticks after region start

  // Pass empty vector - the API will resize it appropriately
  std::vector<float> values;

  RegionRenderer::serialize_to_automation_values (*automation_region, values);

  // Calculate sample positions
  const auto second_point_samples = static_cast<size_t> (
    tempo_map->tick_to_samples_rounded (units::ticks (150)).in (units::samples));

  // The region should start with an interpolated value from the first point
  // Since the first point is at -25 ticks (before region), and the second at 150
  // ticks, the region start (at 0 ticks relative to region start) should
  // have an interpolated value between 0.2 and 0.8
  if (values.size () > 0)
    {
      const auto first_value = values[0];
      // Should be interpolated, not the default -1.0
      EXPECT_GT (first_value, 0.2f);
      EXPECT_LT (first_value, 0.8f);
    }

  // Values should continue interpolating toward the second point
  bool found_interpolation = false;
  for (size_t i = 0; i < values.size () && i < second_point_samples; ++i)
    {
      if (values[i] > 0.2f && values[i] < 0.8f)
        {
          found_interpolation = true;
          break;
        }
    }
  EXPECT_TRUE (found_interpolation);

  // At the second point position, we should have the exact value
  if (second_point_samples < values.size ())
    {
      EXPECT_FLOAT_EQ (values[second_point_samples], 0.8f);
    }
}

TEST_F (RegionRendererTest, SerializeAutomationRegionLoopStartMidCurve)
{
  // Set up looping where loop start is in the middle of a curve
  add_automation_point (0.0f, 25);  // Before loop start
  add_automation_point (1.0f, 75);  // After loop start
  add_automation_point (0.5f, 125); // Inside loop

  // Set up looping: loop from 50-150 ticks
  automation_region->loopRange ()->setTrackBounds (false);
  automation_region->loopRange ()->loopStartPosition ()->setTicks (50);
  automation_region->loopRange ()->loopEndPosition ()->setTicks (150);
  // Set region length to 300 ticks
  automation_region->bounds ()->length ()->setTicks (300);

  // First part (0-150 ticks):
  // * 0-25 ticks: No automation (values should be -1.0)
  // * 25-75 ticks: Interpolation from point at 25 ticks (0.0) to point at 75
  // ticks (1.0)
  // * 75-125 ticks: Interpolation from point at 75 ticks (1.0) to
  // point at 125 ticks (0.5)
  // * 125-150 ticks: Flat at point 125's value (0.5)
  //
  // Second part (150-250 ticks - looped segment):
  // * 150-175 ticks: Interpolation from point at 25 ticks (0.0) to point at 75
  // ticks (1.0) - but only the portion equivalent to 50-75 ticks
  // * 175-225 ticks: Interpolation from point at 75 ticks (1.0) to point at 125
  // ticks (0.5)
  // * 225-250 ticks: Flat at point 125's value (0.5)
  //
  // Third part (250-300 ticks - looped segment):
  // * 250-275 ticks: Interpolation from point at 25 ticks (0.0) to point at 75
  // ticks (1.0) - but only the portion equivalent to 50-75 ticks
  // * 275-300 ticks: Interpolation from point at 75 ticks (1.0) to point at 125
  // ticks (0.5) - but only for the portion equivalent to 75-100 ticks

  // Pass empty vector - the API will resize it appropriately
  std::vector<float> values;

  RegionRenderer::serialize_to_automation_values (*automation_region, values);

  // Calculate sample positions
  const auto loop_start_samples = static_cast<size_t> (
    tempo_map->tick_to_samples_rounded (units::ticks (50)).in (units::samples));
  const auto second_point_samples = static_cast<size_t> (
    tempo_map->tick_to_samples_rounded (units::ticks (75)).in (units::samples));
  const auto loop_length_samples = static_cast<size_t> (
    tempo_map->tick_to_samples_rounded (units::ticks (100)).in (units::samples));

  // At loop start (50 ticks), we should have an interpolated value
  // between the first point (0.0 at 25 ticks) and second point (1.0 at 75 ticks)
  const auto loop_start_value = values[loop_start_samples];
  // Should be interpolated between 0.0 and 1.0
  EXPECT_NEAR (loop_start_value, 0.5f, 0.001f);

  // In the second loop iteration, the loop start should have the same
  // interpolated value (continuing the curve from the previous loop)
  {
    const auto loop_start_in_second_loop =
      loop_start_samples + loop_length_samples;
    const auto second_loop_value = values[loop_start_in_second_loop];
    // Should be the same interpolated value as the first loop start
    EXPECT_NEAR (second_loop_value, loop_start_value, 0.001f);
  }

  // The second point should appear at the correct position in both loops
  {
    // at 75 ticks
    EXPECT_FLOAT_EQ (values[second_point_samples], 1.0f);
    const auto second_point_in_second_loop =
      second_point_samples + loop_length_samples;

    // at 175 ticks
    EXPECT_FLOAT_EQ (values[second_point_in_second_loop], 1.0f);
  }
}

} // namespace zrythm::structure::arrangement
