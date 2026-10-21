// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cmath>
#include <map>

#include "dsp/timebase.h"
#include "structure/arrangement/clip_renderer.h"
#include "utils/object_registry.h"
#include "utils/registry_utils.h"

#include <gtest/gtest.h>
#include <juce_audio_formats/juce_audio_formats.h>

namespace zrythm::structure::arrangement
{
class ClipRendererTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    tempo_map = std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));
    tempo_map_wrapper = std::make_unique<dsp::TempoMapWrapper> (*tempo_map);

    // Set up MIDI clip
    midi_clip =
      std::make_unique<MidiClip> (*tempo_map_wrapper, registry, nullptr);
    midi_clip->position ()->setTicks (100);
    midi_clip->length ()->setTicks (200);
    midi_clip->loopStartPosition ()->setTicks (50);
    midi_clip->loopEndPosition ()->setTicks (150);
    midi_clip->clipStartPosition ()->setTicks (0);

    // Set up Audio clip with proper audio source
    setup_audio_clip ();

    // Set up Automation clip
    automation_clip =
      std::make_unique<AutomationClip> (*tempo_map_wrapper, registry, nullptr);
    automation_clip->position ()->setTicks (100);
    automation_clip->length ()->setTicks (200);

    // Set up Chord clip (no looping)
    chord_clip =
      std::make_unique<ChordClip> (*tempo_map_wrapper, registry, nullptr);
    chord_clip->position ()->setTicks (0);
    chord_clip->length ()->setTicks (200);
    chord_clip->loopStartPosition ()->setTicks (0);
    chord_clip->loopEndPosition ()->setTicks (200);
    chord_clip->clipStartPosition ()->setTicks (0);
  }

  void add_automation_point (float value, double position_ticks)
  {
    // Create AutomationPoint using registry
    auto ap_ref =
      utils::create_object<AutomationPoint> (registry, *tempo_map_wrapper);
    auto * ap = ap_ref.get_object_as<AutomationPoint> ();
    ap->setValue (value);
    ap->position ()->setTicks (position_ticks);

    // Add to clip
    automation_clip->add_object (ap_ref);
  }

  void add_midi_note (
    int    pitch,
    int    velocity,
    double position_ticks,
    double length_ticks)
  {
    // Create MidiNote using registry
    auto note_ref =
      utils::create_object<MidiNote> (registry, *tempo_map_wrapper);
    auto * note = note_ref.get_object_as<MidiNote> ();
    note->setPitch (pitch);
    note->setVelocity (velocity);
    note->position ()->setTicks (position_ticks);
    note->length ()->setTicks (length_ticks);

    // Add to clip
    midi_clip->ArrangerObjectOwner<MidiNote>::add_object (note_ref);
  }

  void add_chord_object (
    dsp::MusicalNote root,
    dsp::ChordType   type,
    double           position_ticks)
  {
    auto chord_ref = utils::create_object<ChordObject> (
      registry, *tempo_map_wrapper, chord_clip.get ());
    auto * descr = chord_ref.get_object_as<ChordObject> ()->chordDescriptor ();
    descr->setRootNote (root);
    descr->setChordType (type);
    chord_ref.get_object_as<ChordObject> ()->position ()->setTicks (
      position_ticks);
    chord_clip->add_object (chord_ref);
  }

  void setup_audio_clip ()
  {
    // Create a sine wave audio source
    auto audio_source_object_ref = create_sine_wave_audio_source (4096);

    // Create audio clip
    audio_clip =
      std::make_unique<AudioClip> (*tempo_map_wrapper, registry, nullptr);

    audio_clip->set_source (audio_source_object_ref);
    audio_clip->position ()->setTicks (
      tempo_map->samples_to_tick (units::samples (1000)).asDouble ());
    audio_clip->length ()->setTicks (
      tempo_map->samples_to_tick (units::samples (500)).asDouble ());
    audio_clip->loopStartPosition ()->setTicks (
      tempo_map->samples_to_tick (units::samples (250)).asDouble ());
    audio_clip->loopEndPosition ()->setTicks (
      tempo_map->samples_to_tick (units::samples (750)).asDouble ());
    audio_clip->clipStartPosition ()->setTicks (
      tempo_map->samples_to_tick (units::samples (0)).asDouble ());
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
      registry, *sample_buffer, utils::audio::BitDepth::BIT_DEPTH_32,
      units::sample_rate (44100), units::bpm (120.0), u8"SineTestSource");

    return utils::create_object<AudioSourceObject> (
      registry, *tempo_map_wrapper, registry, source_ref);
  }

  /// Creates a musical-mode AudioClip with a sine source at the given BPM,
  /// positioned at @p position_ticks on the timeline.  When @p amplitude_ramp
  /// is true, the source carries a linear amplitude envelope (0→1) so that
  /// stretched vs. truncated output can be distinguished by checking the
  /// amplitude at known positions.
  std::unique_ptr<AudioClip> create_musical_test_clip (
    units::bpm_t source_bpm = units::bpm (100.0),
    int64_t      frames = 44100,
    int          channels = 2,
    double       position_ticks = 0.0,
    bool         amplitude_ramp = false)
  {
    utils::audio::AudioBuffer src (channels, static_cast<int> (frames));
    for (int c = 0; c < channels; ++c)
      for (int64_t i = 0; i < frames; ++i)
        {
          const float amp =
            amplitude_ramp
              ? static_cast<float> (i) / static_cast<float> (frames)
              : 1.0f;
          src.setSample (
            c, static_cast<int> (i),
            amp
              * static_cast<float> (std::sin (2.0 * M_PI * 441.0 * i / 44100.0)));
        }

    auto source_ref = utils::create_object<dsp::FileAudioSource> (
      registry, src, utils::audio::BitDepth::BIT_DEPTH_32,
      units::sample_rate (44100), source_bpm, u8"musical_src");
    auto aso_ref = utils::create_object<AudioSourceObject> (
      registry, *tempo_map_wrapper, registry, source_ref);

    auto clip =
      std::make_unique<AudioClip> (*tempo_map_wrapper, registry, nullptr);
    clip->set_source (aso_ref);
    clip->position ()->setTicks (position_ticks);
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

  std::unique_ptr<dsp::TempoMap>        tempo_map;
  std::unique_ptr<dsp::TempoMapWrapper> tempo_map_wrapper;
  utils::ObjectRegistry                 registry;
  std::unique_ptr<MidiClip>             midi_clip;
  std::unique_ptr<AudioClip>            audio_clip;
  std::unique_ptr<AutomationClip>       automation_clip;
  std::unique_ptr<ChordClip>            chord_clip;
  juce::AudioSampleBuffer               test_audio_buffer;
};

// ========== MIDI Clip Tests ==========

TEST_F (ClipRendererTest, SerializeMidiEventsSimple)
{
  // Add a note within the clip
  add_midi_note (60, 90, 100, 50);

  juce::MidiMessageSequence events;
  ClipRenderer::serialize_to_sequence (*midi_clip, events, std::nullopt);
  events.addTimeToMessages (midi_clip->position ()->ticks ());

  // Verify events
  EXPECT_EQ (events.getNumEvents (), 2); // Note on + note off

  // First event should be note on
  auto note_on_event = events.getEventPointer (0);
  EXPECT_TRUE (note_on_event->message.isNoteOn ());
  EXPECT_EQ (note_on_event->message.getNoteNumber (), 60);
  EXPECT_EQ (note_on_event->message.getVelocity (), 90);
  EXPECT_EQ (note_on_event->message.getTimeStamp (), 200); // 100 (position) +
                                                           // 100 (clip start)

  // Second event should be note off
  auto note_off_event = events.getEventPointer (1);
  EXPECT_TRUE (note_off_event->message.isNoteOff ());
  EXPECT_EQ (note_off_event->message.getNoteNumber (), 60);
  EXPECT_EQ (note_off_event->message.getVelocity (), 90);
  EXPECT_EQ (
    note_off_event->message.getTimeStamp (),
    250); // 100 (position) + 50 (length) + 100 (clip start)
}

TEST_F (ClipRendererTest, SerializeMidiEventsWithLooping)
{
  // Add a note within the loop range
  add_midi_note (60, 90, 50, 50);

  juce::MidiMessageSequence events;
  ClipRenderer::serialize_to_sequence (*midi_clip, events);
  events.addTimeToMessages (midi_clip->position ()->ticks ());

  // Should create multiple events due to looping
  // Loop length = 100 ticks (150-50)
  // Clip length = 200 ticks
  // Number of repeats = ceil((200 - 50 + 0) / 100) = ceil(150/100) = 2
  EXPECT_EQ (events.getNumEvents (), 4); // Two sets of note on/off

  // First occurrence
  auto first_note_on = events.getEventPointer (0);
  EXPECT_TRUE (first_note_on->message.isNoteOn ());
  EXPECT_EQ (first_note_on->message.getTimeStamp (), 150); // 50 + 100 clip start

  // Second occurrence (loop repeat)
  auto second_note_on = events.getEventPointer (2);
  EXPECT_TRUE (second_note_on->message.isNoteOn ());
  EXPECT_EQ (
    second_note_on->message.getTimeStamp (),
    250); // 150 (first loop end) + 100 clip start
}

#if 0
TEST_F (ClipRendererTest, SerializeMidiEventsWithStartEndConstraints)
{
  // Add a note
  add_midi_note (60, 90, 100, 50);

  juce::MidiMessageSequence events;
  ClipRenderer::serialize_to_sequence (
    *midi_clip, events,
    std::make_pair (
      dsp::TimelineTick{ units::ticks (150.0) },
      dsp::TimelineTick{ units::ticks (250.0) }));
  events.addTimeToMessages (midi_clip->position ()->ticks ());

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

TEST_F (ClipRendererTest, MutedMidiNoteNotSerialized)
{
  // Add a note and mute it
  add_midi_note (60, 90, 100, 50);
  midi_clip->ArrangerObjectOwner<MidiNote>::get_children_view ()[0]
    ->mute ()
    ->setMuted (true);

  juce::MidiMessageSequence events;
  ClipRenderer::serialize_to_sequence (*midi_clip, events);
  events.addTimeToMessages (midi_clip->position ()->ticks ());

  // No events should be added for muted note
  EXPECT_EQ (events.getNumEvents (), 0);
}

TEST_F (ClipRendererTest, MidiNoteOutsideLoopRangeNotSerialized)
{
  // Adjust clip start
  midi_clip->clipStartPosition ()->setTicks (50);

  // Add note before clip start and loop range
  add_midi_note (60, 90, 40, 10); // At 40-50 ticks, loop starts at 50

  juce::MidiMessageSequence events;
  ClipRenderer::serialize_to_sequence (*midi_clip, events);
  events.addTimeToMessages (midi_clip->position ()->ticks ());

  // Should not be added when full=true
  EXPECT_EQ (events.getNumEvents (), 0);
}

TEST_F (ClipRendererTest, SerializeMultipleMidiNotes)
{
  // Add multiple notes
  add_midi_note (60, 90, 50, 30);
  add_midi_note (64, 80, 80, 40);
  add_midi_note (67, 100, 120, 20);

  juce::MidiMessageSequence events;
  ClipRenderer::serialize_to_sequence (*midi_clip, events);
  events.addTimeToMessages (midi_clip->position ()->ticks ());

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

TEST_F (ClipRendererTest, SerializeMidiWithoutClipStart)
{
  // Add a note
  add_midi_note (60, 90, 100, 50);

  juce::MidiMessageSequence events;
  ClipRenderer::serialize_to_sequence (*midi_clip, events);

  // Verify positions are relative to clip start (not global)
  auto note_on_event = events.getEventPointer (0);
  EXPECT_EQ (note_on_event->message.getTimeStamp (), 100); // Just note position

  auto note_off_event = events.getEventPointer (1);
  EXPECT_EQ (
    note_off_event->message.getTimeStamp (), 150); // Note position + length
}

TEST_F (ClipRendererTest, MidiNoteChannelRendered)
{
  add_midi_note (60, 90, 50, 30);
  midi_clip->ArrangerObjectOwner<MidiNote>::get_children_view ()[0]
    ->setMidiChannel (4);

  juce::MidiMessageSequence events;
  ClipRenderer::serialize_to_sequence (*midi_clip, events);

  ASSERT_GE (events.getNumEvents (), 2);
  EXPECT_EQ (events.getEventPointer (0)->message.getChannel (), 5)
    << "MIDI channel should be stored value + 1";
  EXPECT_EQ (events.getEventPointer (1)->message.getChannel (), 5);
}

// ========== Chord Clip Tests ==========

namespace
{
// Collects note-off timestamps keyed by pitch from a MIDI sequence.
std::map<int, double>
collect_note_off_times (const juce::MidiMessageSequence &events)
{
  std::map<int, double> note_off_times;
  for (const auto &ev : events)
    {
      if (ev->message.isNoteOff ())
        note_off_times[ev->message.getNoteNumber ()] =
          ev->message.getTimeStamp ();
    }
  return note_off_times;
}
} // namespace

// Regression test: when multiple chords follow each other in a chord clip,
// each chord's notes must be released (note-off) when the next chord begins,
// rather than sustaining until the end of the clip.
TEST_F (ClipRendererTest, ChordNotesStopAtNextChordStart)
{
  // C Major at tick 0 -> pitches {48, 52, 55}
  add_chord_object (dsp::MusicalNote::C, dsp::ChordType::Major, 0);
  // A Minor at tick 100 -> pitches {57, 60, 64}
  add_chord_object (dsp::MusicalNote::A, dsp::ChordType::Minor, 100);

  juce::MidiMessageSequence events;
  ClipRenderer::serialize_to_sequence (*chord_clip, events);

  const auto note_off_times = collect_note_off_times (events);

  // C Major notes must stop at tick 100 (start of the next chord), not at the
  // clip end (200).
  for (int pitch : { 48, 52, 55 })
    {
      SCOPED_TRACE ("C Major pitch " + std::to_string (pitch));
      ASSERT_TRUE (note_off_times.count (pitch))
        << "missing note-off for C Major pitch";
      EXPECT_DOUBLE_EQ (note_off_times.at (pitch), 100.0)
        << "C Major should stop when A Minor begins";
    }

  // A Minor notes stop at the clip end (tick 200) since it is the last chord.
  for (int pitch : { 57, 60, 64 })
    {
      SCOPED_TRACE ("A Minor pitch " + std::to_string (pitch));
      ASSERT_TRUE (note_off_times.count (pitch))
        << "missing note-off for A Minor pitch";
      EXPECT_DOUBLE_EQ (note_off_times.at (pitch), 200.0)
        << "last chord should stop at clip end";
    }
}

// ========== Audio Clip Tests ==========

TEST_F (ClipRendererTest, SerializeAudioClipSimple)
{
  juce::AudioSampleBuffer buffer;

  // Serialize the audio clip
  ClipRenderer::serialize_to_buffer (*audio_clip, buffer);

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
  if (buffer.getNumSamples () > AudioClip::BUILTIN_FADE_FRAMES * 2)
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
  const int start_check = AudioClip::BUILTIN_FADE_FRAMES;
  const int end_check = last_sample - AudioClip::BUILTIN_FADE_FRAMES;
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
TEST_F (ClipRendererTest, SerializeAudioClipWithConstraints)
{
  juce::AudioSampleBuffer buffer;

  // Get the clip's position in ticks to create appropriate constraints
  const auto clip_pos_ticks = audio_clip->position ()->ticks ();

  // Create constraints that overlap with the clip
  // Start at clip position, end at clip position + 50 ticks
  const double constraint_start = clip_pos_ticks;
  const double constraint_end = clip_pos_ticks + 50.0;

  ClipRenderer::serialize_to_buffer (
    *audio_clip, buffer,
    std::make_pair (
      dsp::TimelineTick{ units::ticks (constraint_start) },
      dsp::TimelineTick{ units::ticks (constraint_end) }));

  // Buffer should be created
  EXPECT_GT (buffer.getNumSamples (), 0);
  EXPECT_EQ (buffer.getNumChannels (), 2);

  // Calculate expected sample count: 100 ticks at 120 BPM = 0.5 seconds
  // At 44100 Hz, that's 22050 samples
  const int expected_samples = static_cast<int> (
    tempo_map
      ->tick_to_samples_rounded (dsp::TimelineTick{ units::ticks (constraint_end - constraint_start) })
      .in (units::samples));
  EXPECT_EQ (buffer.getNumSamples (), expected_samples);

  // Verify the pattern starts at the correct position in our test buffer
  auto * left_channel = buffer.getReadPointer (0);
  auto * right_channel = buffer.getReadPointer (1);

  // First sample is 0 due to built-in fade in
  EXPECT_FLOAT_EQ (left_channel[0], 0.0f);
  EXPECT_FLOAT_EQ (right_channel[0], 0.0f);

  // After the built-in fade, we should see the sine wave pattern
  if (buffer.getNumSamples () > AudioClip::BUILTIN_FADE_FRAMES)
    {
      // Get expected values at this sample position
      auto [expected_left, expected_right] =
        get_expected_sine_values (AudioClip::BUILTIN_FADE_FRAMES);

      // The value should be close to the expected sine wave value
      EXPECT_NEAR (
        left_channel[AudioClip::BUILTIN_FADE_FRAMES], expected_left, 0.05f);
      EXPECT_NEAR (
        right_channel[AudioClip::BUILTIN_FADE_FRAMES], expected_right, 0.05f);
    }
}
#endif

TEST_F (ClipRendererTest, SerializeAudioClipWithLooping)
{
  juce::AudioSampleBuffer buffer;

  // Serialize with full looping enabled
  ClipRenderer::serialize_to_buffer (*audio_clip, buffer);

  // With looping, buffer should contain the looped pattern
  EXPECT_GT (buffer.getNumSamples (), 0);
  EXPECT_EQ (buffer.getNumChannels (), 2);

  // The loop range is 50-150 ticks (100 ticks)
  // With clip length of 200 ticks, we should get 2 full loops
  // Verify the pattern repeats
  auto *    left_channel = buffer.getReadPointer (0);
  auto *    right_channel = buffer.getReadPointer (1);
  const int loop_length_samples = static_cast<int> (
    tempo_map
      ->tick_to_samples_rounded (dsp::TimelineTick{ units::ticks (100.0) })
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
        > loop_length_samples + AudioClip::BUILTIN_FADE_FRAMES)
        {
          const int sample_in_first_loop = AudioClip::BUILTIN_FADE_FRAMES;
          const int sample_in_second_loop =
            loop_length_samples + AudioClip::BUILTIN_FADE_FRAMES;

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

TEST_F (ClipRendererTest, SerializeAudioClipAppliesWholeClipInvertEntry)
{
  // Baseline without entries
  juce::AudioSampleBuffer base;
  ClipRenderer::serialize_to_buffer (*audio_clip, base);

  // Invert the clip's audible span; the entry applies before gain and
  // fades, which are all multiplicative, so every sample negates exactly
  AudioFunction::Entry entry{
    .start = dsp::ContentTick{ units::ticks (0.) },
    .end =
      audio_clip->clipStartPosition ()->asTick ()
      + audio_clip->length ()->asTick (),
    .kind = AudioFunction::Invert{}
  };
  audio_clip->set_transform_entries ({ entry });

  juce::AudioSampleBuffer transformed;
  ClipRenderer::serialize_to_buffer (*audio_clip, transformed);

  ASSERT_EQ (transformed.getNumSamples (), base.getNumSamples ());
  for (const auto ch : std::views::iota (0, 2))
    {
      for (const auto i : std::views::iota (0, base.getNumSamples ()))
        {
          EXPECT_NEAR (
            transformed.getSample (ch, i), -base.getSample (ch, i), 1e-6)
            << "ch " << ch << " frame " << i;
        }
    }
}

TEST_F (ClipRendererTest, SerializeAudioClipSkipsDisabledEntries)
{
  // Baseline without entries
  juce::AudioSampleBuffer base;
  ClipRenderer::serialize_to_buffer (*audio_clip, base);

  // The same whole-clip invert as the test above, disabled
  AudioFunction::Entry entry{
    .start = dsp::ContentTick{ units::ticks (0.) },
    .end =
      audio_clip->clipStartPosition ()->asTick ()
      + audio_clip->length ()->asTick (),
    .enabled = false,
    .kind = AudioFunction::Invert{}
  };
  audio_clip->set_transform_entries ({ entry });

  juce::AudioSampleBuffer transformed;
  ClipRenderer::serialize_to_buffer (*audio_clip, transformed);

  ASSERT_EQ (transformed.getNumSamples (), base.getNumSamples ());
  for (const auto ch : std::views::iota (0, 2))
    {
      for (const auto i : std::views::iota (0, base.getNumSamples ()))
        {
          EXPECT_FLOAT_EQ (transformed.getSample (ch, i), base.getSample (ch, i))
            << "ch " << ch << " frame " << i;
        }
    }
}

TEST_F (ClipRendererTest, SerializeAudioClipRangedReverseRepeatsPerLoopIteration)
{
  // A dedicated clip long enough for two loop legs: first leg plays
  // [clip start, loop end), then the [loop start, loop end) region repeats
  auto looped_clip =
    std::make_unique<AudioClip> (*tempo_map_wrapper, registry, nullptr);
  looped_clip->set_source (create_sine_wave_audio_source (4096));
  looped_clip->position ()->setTicks (0.);
  looped_clip->length ()->setTicks (
    tempo_map->samples_to_tick (units::samples (2000)).asDouble ());
  looped_clip->clipStartPosition ()->setTicks (
    tempo_map->samples_to_tick (units::samples (0)).asDouble ());
  looped_clip->loopStartPosition ()->setTicks (
    tempo_map->samples_to_tick (units::samples (250)).asDouble ());
  looped_clip->loopEndPosition ()->setTicks (
    tempo_map->samples_to_tick (units::samples (750)).asDouble ());

  // Baseline without entries
  juce::AudioSampleBuffer base;
  ClipRenderer::serialize_to_buffer (*looped_clip, base);

  // Reverse the window up to the loop end. The first leg plays
  // [clip start, loop end) of the reversed window; every loop iteration
  // then repeats the reversed [loop start, loop end) span.
  constexpr int loop_end_frame = 750;
  constexpr int loop_start_frame = 250;

  AudioFunction::Entry entry{
    .start = looped_clip->clipStartPosition ()->asTick (),
    .end = looped_clip->loopEndPosition ()->asTick (),
    .kind = AudioFunction::Reverse{}
  };
  looped_clip->set_transform_entries ({ entry });

  juce::AudioSampleBuffer transformed;
  ClipRenderer::serialize_to_buffer (*looped_clip, transformed);

  // First leg (away from the builtin fades): sample i of the reversed
  // window is window[loop end - 1 - i]
  for (const auto i : std::views::iota (15, 700))
    {
      EXPECT_NEAR (
        transformed.getSample (0, i),
        base.getSample (0, loop_end_frame - 1 - i), 1e-6)
        << "first leg frame " << i;
    }

  // Second leg: each output sample j reads window[loop start + j] of the
  // entry window, which holds orig[loop end - 1 - (loop start + j)]
  constexpr int second_leg_start = loop_end_frame;
  constexpr int leg_span = loop_end_frame - loop_start_frame;
  for (const auto j : std::views::iota (15, leg_span - 15))
    {
      EXPECT_NEAR (
        transformed.getSample (0, second_leg_start + j),
        base.getSample (0, (loop_end_frame - 1 - loop_start_frame) - j), 1e-6)
        << "second leg frame " << j;
    }
}

TEST_F (ClipRendererTest, SerializeAudioClipWithGain)
{
  // Set gain to 0.5
  audio_clip->setGain (0.5f);

  juce::AudioSampleBuffer buffer;
  ClipRenderer::serialize_to_buffer (*audio_clip, buffer);

  // Verify gain was applied
  auto * left_channel = buffer.getReadPointer (0);
  auto * right_channel = buffer.getReadPointer (1);

  // First audio sample should be affected by both built-in fade in and gain
  EXPECT_LT (std::abs (left_channel[0]),
             0.1f); // 0.5 * fade * 0.5

  // Last sample should be affected by both built-in fade out and gain
  // The built-in fade out reduces the last samples to near 0
  const int last_sample = buffer.getNumSamples () - 1;
  if (last_sample >= AudioClip::BUILTIN_FADE_FRAMES)
    {
      // Check a sample before the built-in fade out starts
      const int sample_before_fade_out =
        last_sample - AudioClip::BUILTIN_FADE_FRAMES;

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

TEST_F (ClipRendererTest, SerializeAudioClipWithClipStart)
{
  // Set clip start to 50 samples
  audio_clip->clipStartPosition ()->setTicks (
    tempo_map->samples_to_tick (units::samples (50)).asDouble ());

  juce::AudioSampleBuffer buffer;
  ClipRenderer::serialize_to_buffer (*audio_clip, buffer);

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
  if (buffer.getNumSamples () > AudioClip::BUILTIN_FADE_FRAMES)
    {
      // Get expected value at this position (accounting for clip start offset)
      const int clip_start_samples = 50;
      auto [expected_left, expected_right] = get_expected_sine_values (
        AudioClip::BUILTIN_FADE_FRAMES + clip_start_samples);

      // The value should be close to the expected sine wave value
      EXPECT_NEAR (
        left_channel[AudioClip::BUILTIN_FADE_FRAMES], expected_left, 0.05f);
      EXPECT_NEAR (
        right_channel[AudioClip::BUILTIN_FADE_FRAMES], expected_right, 0.05f);
    }
}

#if 0
TEST_F (ClipRendererTest, SerializeAudioClipWithNegativeConstraints)
{
  juce::AudioSampleBuffer buffer;

  // Serialize with constraints that don't overlap the clip
  // Clip is at ~43.5 ticks, so -100 to -50 should definitely not overlap
  ClipRenderer::serialize_to_buffer (
    *audio_clip, buffer,
    std::make_pair (
      dsp::TimelineTick{ units::ticks (-100.0) },
      dsp::TimelineTick{ units::ticks (-50.0) }));

  // Buffer should be empty since constraints don't include the clip
  EXPECT_EQ (buffer.getNumSamples (), 0);
}

TEST_F (ClipRendererTest, SerializeAudioClipLargeConstraints)
{
  juce::AudioSampleBuffer buffer;

  // Get the clip's position to create appropriate constraints
  const auto clip_pos_ticks = audio_clip->position ()->ticks ();
  const auto clip_length_ticks = audio_clip->length ()->ticks ();

  // Serialize with large constraints that fully include the clip
  // Start before the clip and end after the clip
  ClipRenderer::serialize_to_buffer (
    *audio_clip, buffer,
    std::make_pair (
      dsp::TimelineTick{ units::ticks (clip_pos_ticks - 10.0) },
      dsp::TimelineTick{ units::ticks (clip_pos_ticks + clip_length_ticks + 10.0) }));

  // Buffer should contain the full clip
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
  if (buffer.getNumSamples () > AudioClip::BUILTIN_FADE_FRAMES * 2)
    {
      const auto middle_sample = buffer.getNumSamples () / 2;

      // For constraints, we need to account for the offset where the clip
      // audio starts The constraint starts before the clip, so there's
      // silence at the beginning
      const auto clip_start_offset =
        tempo_map
          ->tick_to_samples_rounded (
            units::ticks (clip_pos_ticks - (clip_pos_ticks - 10.0)))
          .in (units::samples);

      // Calculate the actual sample position in the audio source
      const auto actual_sample_pos = middle_sample - clip_start_offset;

      if (actual_sample_pos >= 0)
        {
          auto [expected_left, expected_right] =
            get_expected_sine_values (static_cast<int> (actual_sample_pos));
          EXPECT_NEAR (left_channel[middle_sample], expected_left, 0.01f);
          EXPECT_NEAR (right_channel[middle_sample], expected_right, 0.01f);
        }
      else
        {
          // This should be silence (before the clip starts)
          EXPECT_NEAR (left_channel[middle_sample], 0.0f, 0.01f);
          EXPECT_NEAR (right_channel[middle_sample], 0.0f, 0.01f);
        }
    }
}
#endif

// ========== Ported FillStereoPorts Tests ==========

TEST_F (ClipRendererTest, SerializeAudioClipGainAndFades)
{
  // Set gain to 0.5
  audio_clip->setGain (0.5f);

  juce::AudioSampleBuffer buffer;
  ClipRenderer::serialize_to_buffer (*audio_clip, buffer);

  // Verify gain was applied to our test pattern
  auto * left_channel = buffer.getReadPointer (0);
  auto * right_channel = buffer.getReadPointer (1);

  // First sample: 0.5 * fade * 0.5, should be very quiet
  EXPECT_LT (std::abs (left_channel[0]), 0.1f);
  // Right channel first sample: -0.5 * fade * 0.5, should be very quiet
  EXPECT_LT (std::abs (right_channel[0]), 0.1f);

  // Check samples after the built-in fade in
  if (buffer.getNumSamples () > AudioClip::BUILTIN_FADE_FRAMES)
    {
      const int sample_after_fade = AudioClip::BUILTIN_FADE_FRAMES;
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

TEST_F (ClipRendererTest, SerializeAudioClipWithBuiltinFadeIn)
{
  // Create a small clip to test built-in fade in
  // Create a new audio source with sine wave pattern
  auto audio_source_object_ref = create_sine_wave_audio_source (5000);

  auto small_clip =
    std::make_unique<AudioClip> (*tempo_map_wrapper, registry, nullptr);
  small_clip->set_source (audio_source_object_ref);

  // Set small clip length (less than built-in fade frames)
  small_clip->position ()->setTicks (0);
  small_clip->length ()->setTicks (100);

  juce::AudioSampleBuffer buffer;
  ClipRenderer::serialize_to_buffer (*audio_clip, buffer);

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

TEST_F (ClipRendererTest, SerializeAudioClipWithBuiltinFadeOut)
{
  // Create a clip positioned near the end to test built-in fade out
  // Create a new audio source with sine wave pattern
  auto audio_source_object_ref = create_sine_wave_audio_source (5000);

  auto fade_out_clip =
    std::make_unique<AudioClip> (*tempo_map_wrapper, registry, nullptr);
  fade_out_clip->set_source (audio_source_object_ref);

  // Set clip length to be just a bit more than built-in fade frames
  fade_out_clip->position ()->setTicks (0);
  fade_out_clip->length ()->setTicks (200);

  juce::AudioSampleBuffer buffer;
  ClipRenderer::serialize_to_buffer (*fade_out_clip, buffer);

  // Verify built-in fade out was applied
  auto * left_channel = buffer.getReadPointer (0);
  auto * right_channel = buffer.getReadPointer (1);

  // First sample after fade in should be loud
  // (The very first sample is 0 due to built-in fade in)
  if (buffer.getNumSamples () > AudioClip::BUILTIN_FADE_FRAMES)
    {
      EXPECT_GT (std::abs (left_channel[AudioClip::BUILTIN_FADE_FRAMES]), 0.1f);
      EXPECT_GT (std::abs (right_channel[AudioClip::BUILTIN_FADE_FRAMES]), 0.1f);
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
    buffer.getNumSamples () - AudioClip::BUILTIN_FADE_FRAMES;
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

TEST_F (ClipRendererTest, SerializeAudioClipWithCustomFades)
{
  // Set custom fade positions
  audio_clip->fadeRange ()->startOffset ()->setTicks (
    tempo_map->samples_to_tick (units::samples (100)).asDouble ());
  audio_clip->fadeRange ()->endOffset ()->setTicks (
    tempo_map->samples_to_tick (units::samples (100)).asDouble ());

  juce::AudioSampleBuffer buffer;
  ClipRenderer::serialize_to_buffer (*audio_clip, buffer);

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

TEST_F (ClipRendererTest, SerializeAudioClipFadesCombinedWithGain)
{
  // Set gain and custom fades
  audio_clip->setGain (0.7f);
  audio_clip->fadeRange ()->startOffset ()->setTicks (
    tempo_map->samples_to_tick (units::samples (50)).asDouble ());
  audio_clip->fadeRange ()->endOffset ()->setTicks (
    tempo_map->samples_to_tick (units::samples (50)).asDouble ());

  juce::AudioSampleBuffer buffer;
  ClipRenderer::serialize_to_buffer (*audio_clip, buffer);

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

TEST_F (ClipRendererTest, SerializeEmptyMidiClip)
{
  juce::MidiMessageSequence events;
  ClipRenderer::serialize_to_sequence (*midi_clip, events);

  // No events should be added for empty clip
  EXPECT_EQ (events.getNumEvents (), 0);
}

#if 0
TEST_F (ClipRendererTest, SerializeMidiClipWithNegativeConstraints)
{
  add_midi_note (60, 90, 100, 50);

  juce::MidiMessageSequence events;
  ClipRenderer::serialize_to_sequence (
    *midi_clip, events,
    std::make_pair (
      dsp::TimelineTick{ units::ticks (-50.0) },
      dsp::TimelineTick{ units::ticks (50.0) }));

  // Should not include any events as they're outside the constraint range
  EXPECT_EQ (events.getNumEvents (), 0);
}

TEST_F (ClipRendererTest, SerializeMidiClipWithLargeConstraints)
{
  add_midi_note (60, 90, 100, 50);

  juce::MidiMessageSequence events;
  ClipRenderer::serialize_to_sequence (
    *midi_clip, events,
    std::make_pair (
      dsp::TimelineTick{ units::ticks (0.0) },
      dsp::TimelineTick{ units::ticks (1000.0) }));

  // Should include all events as they're within the large constraint range
  EXPECT_EQ (events.getNumEvents (), 2);
}
#endif

TEST_F (ClipRendererTest, SerializeAudioClipBeyondSourceLength)
{
  // Create a small audio source with sine wave (100 samples)
  auto audio_source_object_ref = create_sine_wave_audio_source (100);

  // Create a clip that extends beyond the source length
  auto large_clip =
    std::make_unique<AudioClip> (*tempo_map_wrapper, registry, nullptr);
  large_clip->set_source (audio_source_object_ref);
  large_clip->position ()->setTicks (
    tempo_map->samples_to_tick (units::samples (0)).asDouble ());
  large_clip->length ()->setTicks (
    tempo_map->samples_to_tick (units::samples (200))
      .asDouble ()); // 200 samples > 100 source

  juce::AudioSampleBuffer buffer;
  ClipRenderer::serialize_to_buffer (*large_clip, buffer);

  // Buffer should be created
  EXPECT_GT (buffer.getNumSamples (), 0);
  EXPECT_EQ (buffer.getNumChannels (), 2);

  auto * left_channel = buffer.getReadPointer (0);
  auto * right_channel = buffer.getReadPointer (1);

  // First part should contain the audio source data
  for (int i = 0; i < 100; ++i)
    {
      // First samples are affected by built-in fade in
      if (i < AudioClip::BUILTIN_FADE_FRAMES)
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

// Musical mode stretches the clip to follow the project tempo. With source_bpm
// 100 and project tempo 120, a 1 s clip (44100 samples) is sped up to match 120
// BPM: its musical length is 1600 ticks, which at 120 BPM is 0.833 s = 36750
// samples. Non-musical mode plays at the native 1 s duration instead.
TEST_F (ClipRendererTest, SerializeAudioClipMusicalModeStretchesToTempo)
{
  // 1 s sine source at source_bpm 100 (≠ project tempo 120).
  constexpr int             kFrames = 44100;
  utils::audio::AudioBuffer src (2, kFrames);
  for (int i = 0; i < kFrames; ++i)
    {
      const float v =
        static_cast<float> (std::sin (2.0 * M_PI * 441.0 * i / 44100.0));
      src.setSample (0, i, v);
      src.setSample (1, i, v);
    }
  auto source_ref = utils::create_object<dsp::FileAudioSource> (
    registry, src, utils::audio::BitDepth::BIT_DEPTH_32,
    units::sample_rate (44100), units::bpm (100.0), u8"src100");
  auto aso_ref = utils::create_object<AudioSourceObject> (
    registry, *tempo_map_wrapper, registry, source_ref);

  auto clip =
    std::make_unique<AudioClip> (*tempo_map_wrapper, registry, nullptr);
  clip->set_source (
    aso_ref); // musical on + source_bpm 100 => length = 1600 ticks
  clip->position ()->setTicks (
    tempo_map->samples_to_tick (units::samples (0)).asDouble ());

  // Musical mode: 1600 ticks @ 120 BPM = 36750 samples (sped up 100 -> 120).
  juce::AudioSampleBuffer buffer;
  ClipRenderer::serialize_to_buffer (*clip, buffer);
  EXPECT_EQ (buffer.getNumChannels (), 2);
  EXPECT_EQ (buffer.getNumSamples (), 36750);
  // Sped-up content should still carry energy (not silent).
  EXPECT_GT (
    std::abs (buffer.getSample (0, buffer.getNumSamples () / 2)), 0.01f);

  // Non-musical mode: native duration (no stretch).
  clip->timebaseProvider ()->setOverride (dsp::Timebase::Absolute);
  ClipRenderer::serialize_to_buffer (*clip, buffer);
  EXPECT_EQ (buffer.getNumSamples (), kFrames);
}

// A sub-range render (the path used for per-frame recording waveform updates)
// must produce the same audio as the corresponding slice of a full render.
// The fixture's clip has source_bpm 120 == project tempo 120, so this
// exercises the cheap O(range) no-stretch path.
TEST_F (ClipRendererTest, SerializeAudioClipRangeMatchesFullSlice)
{
  juce::AudioSampleBuffer full_buf;
  ClipRenderer::serialize_to_buffer (*audio_clip, full_buf);

  const auto rs = audio_clip->position ()->asTick ();
  const auto re = timeline_end_ticks (*audio_clip);
  const auto range_start = dsp::TimelineTick{
    units::ticks (rs.asDouble () + (re.asDouble () - rs.asDouble ()) * 0.25)
  };
  const auto range_end = dsp::TimelineTick{
    units::ticks (rs.asDouble () + (re.asDouble () - rs.asDouble ()) * 0.75)
  };
  juce::AudioSampleBuffer range_buf;
  ClipRenderer::serialize_to_buffer (
    *audio_clip, range_buf, std::make_pair (range_start, range_end));

  ASSERT_EQ (range_buf.getNumChannels (), 2);
  ASSERT_GT (range_buf.getNumSamples (), 0);

  // Sample offset of the range within the clip.
  const auto clip_start_sample = tempo_map->tick_to_samples_rounded (rs);
  const auto range_start_sample =
    tempo_map->tick_to_samples_rounded (range_start);
  const int off = static_cast<int> (
    (range_start_sample - clip_start_sample).in (units::samples));

  // Compare the interior (skip builtin-fade edges on both buffers).
  const int fade = AudioClip::BUILTIN_FADE_FRAMES;
  const int j_end = std::min (
    range_buf.getNumSamples () - fade, full_buf.getNumSamples () - off - fade);
  for (int j = fade; j < j_end; ++j)
    {
      EXPECT_NEAR (
        range_buf.getSample (0, j), full_buf.getSample (0, off + j), 1e-6f)
        << "At range sample " << j;
      EXPECT_NEAR (
        range_buf.getSample (1, j), full_buf.getSample (1, off + j), 1e-6f)
        << "At range sample " << j;
    }
}

// A sub-range render of a MUSICAL (stretched) clip must produce the same
// audio as the corresponding slice of a full render.  The stretch path renders
// the full clip offline then slices, so the two must agree sample-for-sample.
TEST_F (ClipRendererTest, SerializeAudioClipMusicalModeRangeMatchesFullSlice)
{
  auto clip = create_musical_test_clip (units::bpm (100.0));

  // Full render: 1600 musical ticks @ 120 BPM = 36750 samples.
  juce::AudioSampleBuffer full_buf;
  ClipRenderer::serialize_to_buffer (*clip, full_buf);
  ASSERT_EQ (full_buf.getNumSamples (), 36750);

  // Range render: middle 50 % of the clip.
  const auto rs = clip->position ()->asTick ();
  const auto re = timeline_end_ticks (*clip);
  const auto range_start = dsp::TimelineTick{
    units::ticks (rs.asDouble () + (re.asDouble () - rs.asDouble ()) * 0.25)
  };
  const auto range_end = dsp::TimelineTick{
    units::ticks (rs.asDouble () + (re.asDouble () - rs.asDouble ()) * 0.75)
  };
  juce::AudioSampleBuffer range_buf;
  ClipRenderer::serialize_to_buffer (
    *clip, range_buf, std::make_pair (range_start, range_end));
  ASSERT_GT (range_buf.getNumSamples (), 0);

  // Expected offset of the range within the clip (in output samples).
  const auto clip_start_sample = tempo_map->tick_to_samples_rounded (rs);
  const auto range_start_sample =
    tempo_map->tick_to_samples_rounded (range_start);
  const int off = static_cast<int> (
    (range_start_sample - clip_start_sample).in (units::samples));

  // Compare interior (skip builtin-fade edges on both buffers).
  const int fade = AudioClip::BUILTIN_FADE_FRAMES;
  const int j_end = std::min (
    range_buf.getNumSamples () - fade, full_buf.getNumSamples () - off - fade);
  for (int j = fade; j < j_end; ++j)
    {
      EXPECT_NEAR (
        range_buf.getSample (0, j), full_buf.getSample (0, off + j), 1e-6f)
        << "At range sample " << j;
      EXPECT_NEAR (
        range_buf.getSample (1, j), full_buf.getSample (1, off + j), 1e-6f)
        << "At range sample " << j;
    }
}

// Looping + musical mode: a clip extended beyond the clip via looping is
// stretched to the correct output length and carries energy in the looped
// portion (not silence).
TEST_F (ClipRendererTest, SerializeAudioClipMusicalModeWithLooping)
{
  auto clip = create_musical_test_clip (units::bpm (100.0));
  // Default length after set_source: 1600 musical ticks (full clip).
  // Extend to 2400 ticks with a loop at [800, 1200] ticks.
  clip->length ()->setTicks (2400); // tracking resets loop pts
  clip->loopStartPosition ()->setTicks (800);
  clip->loopEndPosition ()->setTicks (1200);

  juce::AudioSampleBuffer buf;
  ClipRenderer::serialize_to_buffer (*clip, buf);

  // 2400 musical ticks @ 120 BPM => tick_to_samples(2400) output samples.
  const int expected = static_cast<int> (
    tempo_map
      ->tick_to_samples_rounded (dsp::TimelineTick{ units::ticks (2400.0) })
      .in (units::samples));
  EXPECT_EQ (buf.getNumSamples (), expected);
  EXPECT_EQ (buf.getNumChannels (), 2);

  // Energy in the second half (loops should fill it, not silence).
  bool has_energy = false;
  for (int i = buf.getNumSamples () / 2; i < buf.getNumSamples (); ++i)
    {
      if (std::abs (buf.getSample (0, i)) > 0.001f)
        {
          has_energy = true;
          break;
        }
    }
  EXPECT_TRUE (has_energy);
}

// A musical-mode clip at a non-zero timeline position, rendered with a range
// that starts before the clip, must have leading silence followed by the
// stretched audio at the correct offset.
TEST_F (ClipRendererTest, SerializeAudioClipMusicalModeNonZeroPosition)
{
  // Clip at bar 2 (1920 ticks @ 120 BPM).
  constexpr double kPosTicks = 1920.0;
  auto clip = create_musical_test_clip (units::bpm (100.0), 44100, 2, kPosTicks);

  // Range covers [0, 3840] ticks (2 bars).
  juce::AudioSampleBuffer buf;
  ClipRenderer::serialize_to_buffer (
    *clip, buf,
    std::make_pair (
      dsp::TimelineTick{ units::ticks (0.0) },
      dsp::TimelineTick{ units::ticks (3840.0) }));

  const auto clip_start_sample = tempo_map->tick_to_samples_rounded (
    dsp::TimelineTick{ units::ticks (kPosTicks) });
  const int off = static_cast<int> (clip_start_sample.in (units::samples));

  // Leading silence before the clip start.
  for (int i = 0; i < off; ++i)
    {
      EXPECT_FLOAT_EQ (buf.getSample (0, i), 0.0f)
        << "Expected silence before clip at sample " << i;
    }

  // Stretched audio starts at 'off'.
  // 1600 musical ticks @ 120 BPM = 36750 samples.
  ASSERT_GE (buf.getNumSamples (), off + 36750 - 1);
  // Check a sample well inside the clip (past any fade).
  EXPECT_GT (std::abs (buf.getSample (0, off + 100)), 0.001f);
}

// Custom object fades applied to a stretched buffer must align with the
// stretched (output) dimensions, not the native (pre-stretch) dimensions.
TEST_F (ClipRendererTest, SerializeAudioClipMusicalModeFades)
{
  auto clip = create_musical_test_clip (units::bpm (100.0));
  clip->fadeRange ()->startOffset ()->setTicks (
    tempo_map->samples_to_tick (units::samples (500)).asDouble ());
  clip->fadeRange ()->endOffset ()->setTicks (
    tempo_map->samples_to_tick (units::samples (500)).asDouble ());

  juce::AudioSampleBuffer buf;
  ClipRenderer::serialize_to_buffer (*clip, buf);

  // Stretched length: 1600 ticks @ 120 BPM = 36750 samples.
  EXPECT_EQ (buf.getNumSamples (), 36750);

  // Fade-in clip: first 500 samples should ramp up (not full amplitude).
  EXPECT_LT (std::abs (buf.getSample (0, 50)), 0.3f);

  // A sample just past the fade-in should be near full sine amplitude.
  EXPECT_GT (std::abs (buf.getSample (0, 600)), 0.3f);

  // Fade-out clip: last sample should be very quiet.
  const int last = buf.getNumSamples () - 1;
  EXPECT_LT (std::abs (buf.getSample (0, last)), 0.1f);
  // A sample just before the fade-out should have normal amplitude.
  EXPECT_GT (std::abs (buf.getSample (0, last - 600)), 0.3f);
}

// Unknown source BPM (0): musical mode is inert — no stretch, no crash,
// native duration.  With no DSP in the path the output is an exact copy of
// the source sine, so we verify sample-level correctness.
TEST_F (ClipRendererTest, SerializeAudioClipUnknownSourceBpm)
{
  auto clip = create_musical_test_clip (units::bpm (0.0));

  juce::AudioSampleBuffer buf;
  ClipRenderer::serialize_to_buffer (*clip, buf);

  // Native duration: 44100 samples (1 s @ 44.1 kHz), no stretch.
  EXPECT_EQ (buf.getNumSamples (), 44100);
  EXPECT_EQ (buf.getNumChannels (), 2);

  // The source is a 441 Hz sine at 44100 Hz (100 samples/period, phase 0).
  // With default fades (offset 0) the only modification is the 10-sample
  // builtin fade-in/out at the buffer edges.  Verify exact values at peaks
  // well clear of the fades.
  // Sample 25:  sin(2*pi*0.25) = 1.0  (quarter period)
  EXPECT_NEAR (buf.getSample (0, 25), 1.0f, 1e-5f);
  // Sample 75:  sin(2*pi*0.75) = -1.0 (three-quarter period)
  EXPECT_NEAR (buf.getSample (0, 75), -1.0f, 1e-5f);
  // Sample 125: sin(2*pi*1.25) = 1.0  (next peak)
  EXPECT_NEAR (buf.getSample (0, 125), 1.0f, 1e-5f);
  // Sample 22075: sin(2*pi*220.75) = -1.0 (well past fades, ~halfway)
  EXPECT_NEAR (buf.getSample (0, 22075), -1.0f, 1e-5f);
}

// Mono source + musical mode: stretch must handle mono input correctly.
TEST_F (ClipRendererTest, SerializeAudioClipMusicalModeMono)
{
  auto clip =
    create_musical_test_clip (units::bpm (100.0), 44100, /*channels=*/1);

  juce::AudioSampleBuffer buf;
  ClipRenderer::serialize_to_buffer (*clip, buf);

  // Stretched length: 1600 ticks @ 120 BPM = 36750 samples.
  EXPECT_EQ (buf.getNumSamples (), 36750);
  // Renderer always creates stereo output buffer.
  EXPECT_EQ (buf.getNumChannels (), 2);
  // Channel 0 should have stretched energy.
  EXPECT_GT (std::abs (buf.getSample (0, buf.getNumSamples () / 2)), 0.01f);
}

// Verifies that a musical-mode stretch actually compresses the source content
// — not just truncates it.  Uses a source with a linear amplitude ramp (0→1):
// in a correct stretch (44100→36750, ratio 0.833), the amplitude at 75% of the
// output corresponds to ~75% of the source ramp, whereas a truncation would
// show only ~62.5%.
TEST_F (ClipRendererTest, SerializeAudioClipMusicalModeStretchContent)
{
  // 441 Hz sine with linear amplitude ramp, source_bpm 100 (≠ project 120).
  auto clip = create_musical_test_clip (
    units::bpm (100.0), 44100, 2, 0.0, /*amplitude_ramp=*/true);

  juce::AudioSampleBuffer buf;
  ClipRenderer::serialize_to_buffer (*clip, buf);
  ASSERT_EQ (buf.getNumSamples (), 36750);

  // Helper: find the peak amplitude in a ±half-period window around a
  // position.  For 441 Hz at 44100 Hz the period is 100 samples.
  const auto peak_around = [&] (int center) {
    const int half = 50;
    float     peak = 0.0f;
    for (
      int i = std::max (0, center - half);
      i < std::min (buf.getNumSamples (), center + half); ++i)
      peak = std::max (peak, std::abs (buf.getSample (0, i)));
    return peak;
  };

  // At 75% of output (sample ~27562):
  //   Stretched:  source maps to ~33075 → amplitude ≈ 0.75 → peak ≈ 0.75
  //   Truncated:  sample 27562 → amplitude ≈ 0.625 → peak ≈ 0.625
  // Threshold 0.69 separates the two cases.
  const float peak_75 = peak_around (buf.getNumSamples () * 3 / 4);
  EXPECT_GT (peak_75, 0.69f)
    << "Amplitude at 75% too low — stretch likely didn't happen (truncated?)";

  // At 25% of output (sample ~9187):
  //   Stretched:  source maps to ~11025 → amplitude ≈ 0.25 → peak ≈ 0.25
  //   Truncated:  sample 9187 → amplitude ≈ 0.208 → peak ≈ 0.208
  // The gap is narrower, threshold 0.23 separates them.
  const float peak_25 = peak_around (buf.getNumSamples () / 4);
  EXPECT_GT (peak_25, 0.23f)
    << "Amplitude at 25% too low — stretch likely didn't happen (truncated?)";
}

// ========== Sub-range rendering at non-zero position ==========

// Sub-range render at a non-zero position must match the corresponding portion
// of the full render (excluding builtin fade edges). This is the contract the
// waveform fast-path relies on.
TEST_F (ClipRendererTest, SubRangeMatchesFullRenderAtNonZeroPosition)
{
  auto clip = create_musical_test_clip (units::bpm (120.0), 44100, 2, 1920.0);
  // Disable object fades so only the 10-sample builtin edges differ.
  clip->fadeRange ()->startOffset ()->setTicks (
    tempo_map->samples_to_tick (units::samples (0)).asDouble ());
  clip->fadeRange ()->endOffset ()->setTicks (
    tempo_map->samples_to_tick (units::samples (0)).asDouble ());

  juce::AudioSampleBuffer full_buf;
  ClipRenderer::serialize_to_buffer (*clip, full_buf);
  ASSERT_GT (full_buf.getNumSamples (), 0);

  // Render only the second half via an absolute tick range.
  const auto clip_start_tick = clip->position ()->asTick ();
  const auto clip_end_tick = timeline_end_ticks (*clip);
  const auto mid_tick = dsp::TimelineTick{ units::ticks (
    (clip_start_tick.asDouble () + clip_end_tick.asDouble ()) / 2.0) };

  juce::AudioSampleBuffer half_buf;
  ClipRenderer::serialize_to_buffer (
    *clip, half_buf, std::make_pair (mid_tick, clip_end_tick));
  ASSERT_GT (half_buf.getNumSamples (), 0);

  // The sub-range should match the second half of the full render, skipping
  // the builtin fade edges (first/last AudioClip::BUILTIN_FADE_FRAMES).
  constexpr int kSkip = AudioClip::BUILTIN_FADE_FRAMES;
  const int     offset = full_buf.getNumSamples () - half_buf.getNumSamples ();
  ASSERT_GE (offset, 0);
  for (int c = 0; c < 2; ++c)
    {
      for (int i = kSkip; i < half_buf.getNumSamples () - kSkip; ++i)
        {
          EXPECT_NEAR (
            half_buf.getSample (c, i), full_buf.getSample (c, offset + i), 1e-4f)
            << "Mismatch at channel " << c << " sample " << i;
        }
    }
}

// Sub-range render with BPM automation at a non-zero position. The tempo
// change makes the tick→sample mapping non-linear, so absolute tick
// computation is critical.
TEST_F (ClipRendererTest, SubRangeWithTempoChangesAtNonZeroPosition)
{
  // Add a tempo change in the middle of the timeline.
  tempo_map->add_tempo_event (
    units::ticks (3840), units::bpm (240.0), dsp::TempoMap::CurveType::Constant);

  // Clip at tick 1920, source BPM 120, 1 second of audio.
  auto clip = create_musical_test_clip (units::bpm (120.0), 44100, 2, 1920.0);
  clip->fadeRange ()->startOffset ()->setTicks (
    tempo_map->samples_to_tick (units::samples (0)).asDouble ());
  clip->fadeRange ()->endOffset ()->setTicks (
    tempo_map->samples_to_tick (units::samples (0)).asDouble ());

  juce::AudioSampleBuffer full_buf;
  ClipRenderer::serialize_to_buffer (*clip, full_buf);
  ASSERT_GT (full_buf.getNumSamples (), 0);

  // Render the second half via absolute ticks.
  const auto clip_start_tick = clip->position ()->asTick ();
  const auto clip_end_tick = timeline_end_ticks (*clip);
  const auto mid_tick = dsp::TimelineTick{ units::ticks (
    (clip_start_tick.asDouble () + clip_end_tick.asDouble ()) / 2.0) };

  juce::AudioSampleBuffer half_buf;
  ClipRenderer::serialize_to_buffer (
    *clip, half_buf, std::make_pair (mid_tick, clip_end_tick));
  ASSERT_GT (half_buf.getNumSamples (), 0);

  // Second half of full render should match sub-range render (skip edges).
  constexpr int kSkip = AudioClip::BUILTIN_FADE_FRAMES;
  const int     offset = full_buf.getNumSamples () - half_buf.getNumSamples ();
  ASSERT_GE (offset, 0);
  for (int i = kSkip; i < half_buf.getNumSamples () - kSkip; ++i)
    {
      EXPECT_NEAR (
        half_buf.getSample (0, i), full_buf.getSample (0, offset + i), 1e-4f)
        << "Mismatch at sample " << i;
    }
}

// Simulates the waveform fast-path: render the full clip, then ask for only
// the "grown tail" via an absolute tick range. The tail must match the
// corresponding portion of the full render. This is the test that would have
// caught the original bug (treating relative samples as absolute).
TEST_F (ClipRendererTest, SubRangeGrownTailMatchesFullRender)
{
  // Clip at tick 9600 (well past tick 0), source BPM 120, 1s audio.
  auto clip = create_musical_test_clip (units::bpm (120.0), 44100, 2, 9600.0);
  clip->fadeRange ()->startOffset ()->setTicks (
    tempo_map->samples_to_tick (units::samples (0)).asDouble ());
  clip->fadeRange ()->endOffset ()->setTicks (
    tempo_map->samples_to_tick (units::samples (0)).asDouble ());

  juce::AudioSampleBuffer full_buf;
  ClipRenderer::serialize_to_buffer (*clip, full_buf);
  ASSERT_GT (full_buf.getNumSamples (), 0);

  // Compute absolute ticks for the second half, as the waveform fast path
  // should: clip_start_samples + prev_samples → absolute tick.
  const auto clip_start_samples =
    tempo_map->tick_to_samples_rounded (clip->position ()->asTick ())
      .in (units::samples);
  const auto half_samples = full_buf.getNumSamples () / 2;
  const auto full_samples = full_buf.getNumSamples ();

  const auto prev_end_tick = tempo_map->samples_to_tick (
    units::precise_sample_t (units::samples (clip_start_samples + half_samples)));
  const auto new_end_tick = tempo_map->samples_to_tick (
    units::precise_sample_t (units::samples (clip_start_samples + full_samples)));

  juce::AudioSampleBuffer tail_buf;
  ClipRenderer::serialize_to_buffer (
    *clip, tail_buf, std::make_pair (prev_end_tick, new_end_tick));
  ASSERT_GT (tail_buf.getNumSamples (), 0);

  // The tail should match the second half of the full render (skip edges).
  constexpr int kSkip = AudioClip::BUILTIN_FADE_FRAMES;
  for (int i = kSkip; i < tail_buf.getNumSamples () - kSkip; ++i)
    {
      EXPECT_NEAR (
        tail_buf.getSample (0, i), full_buf.getSample (0, half_samples + i),
        1e-4f)
        << "Mismatch at sample " << i;
    }
}

// ========== Automation Clip Tests ==========

namespace
{
// Finds the first rendered automation point at (approximately) the given tick
// position relative to clip start, or nullptr if none.
const ClipRenderer::RenderedAutomationPoint *
find_rendered_point_at (
  const std::vector<ClipRenderer::RenderedAutomationPoint> &points,
  double                                                    tick)
{
  for (const auto &p : points)
    if (std::abs (p.position.asDouble () - tick) < 1e-6)
      return &p;
  return nullptr;
}
} // namespace

TEST_F (ClipRendererTest, SerializeAutomationClipSimple)
{
  // Add a simple automation point
  add_automation_point (0.5f, 100);

  std::vector<ClipRenderer::RenderedAutomationPoint> points;

  ClipRenderer::serialize_to_points (*automation_clip, points);

  // No boundary point before the first AP — automation doesn't apply until the
  // first point is reached.
  ASSERT_EQ (points.size (), 1u);
  EXPECT_NEAR (points[0].position.asDouble (), 100.0, 1e-6);
  EXPECT_FLOAT_EQ (points[0].value, 0.5f);
}

// Two points: the rendered output is the set of control points (per-sample
// interpolation is now the consumer's responsibility), so we verify the points
// are emitted at the correct tick positions and values.
TEST_F (ClipRendererTest, SerializeAutomationClipWithInterpolation)
{
  // Add two automation points
  add_automation_point (0.0f, 50);
  add_automation_point (1.0f, 150);

  std::vector<ClipRenderer::RenderedAutomationPoint> points;

  ClipRenderer::serialize_to_points (*automation_clip, points);

  // The two user points — no boundary at clip start since the first AP is at
  // 50 (>0).
  ASSERT_EQ (points.size (), 2u);
  EXPECT_NEAR (points[0].position.asDouble (), 50.0, 1e-6);
  EXPECT_FLOAT_EQ (points[0].value, 0.0f);
  EXPECT_NEAR (points[1].position.asDouble (), 150.0, 1e-6);
  EXPECT_FLOAT_EQ (points[1].value, 1.0f);
}

/**
 * @brief Verifies that rendered automation points land at the correct tick
 * positions even when tempo changes exist within the segment.
 *
 * Point positions are emitted in tick space (relative to clip start), so a
 * tempo change — which only warps the tick→sample mapping, not the ticks
 * themselves — must not shift them.
 */
TEST_F (ClipRendererTest, SerializeAutomationClipLinearWithTempoChange)
{
  // Place clip at timeline position 0 with a 4-beat length (3840 ticks).
  automation_clip->position ()->setTicks (0);
  automation_clip->length ()->setTicks (3840);

  // Tempo change at the tick midpoint (1920 ticks = 2 beats):
  //   120 BPM for ticks 0..1920  -> 44100 samples (1 second)
  //    60 BPM for ticks 1920..3840 -> 88200 samples (2 seconds)
  tempo_map->add_tempo_event (
    units::ticks (1920), units::bpm (60.0), dsp::TempoMap::CurveType::Constant);

  // Linear ramp: 0.0 at start, 1.0 at end.
  add_automation_point (0.0f, 0);
  add_automation_point (1.0f, 3840);

  std::vector<ClipRenderer::RenderedAutomationPoint> points;
  ClipRenderer::serialize_to_points (*automation_clip, points);

  ASSERT_GE (points.size (), 2u);

  // No two co-located points should disagree on curve params (would indicate
  // a boundary/AP mismatch from eval_at_virt_tick returning the wrong AP).
  for (size_t i = 0; i + 1 < points.size (); ++i)
    {
      if (
        std::abs (
          points[i].position.asDouble () - points[i + 1].position.asDouble ())
        < 1e-6)
        {
          EXPECT_EQ (points[i].curve_algo, points[i + 1].curve_algo);
          EXPECT_FLOAT_EQ (
            points[i].curve_curviness, points[i + 1].curve_curviness);
        }
    }

  // Start point at tick 0 with value 0.0.
  const auto * start = find_rendered_point_at (points, 0.0);
  ASSERT_NE (start, nullptr);
  EXPECT_FLOAT_EQ (start->value, 0.0f);

  // End point exactly at tick 3840 with value 1.0 — the tempo change must not
  // move it in tick space.
  const auto * end = find_rendered_point_at (points, 3840.0);
  ASSERT_NE (end, nullptr);
  EXPECT_FLOAT_EQ (end->value, 1.0f);

  // Every emitted point lies within the clip's tick span.
  for (const auto &p : points)
    {
      EXPECT_GE (p.position.asDouble (), 0.0);
      EXPECT_LE (p.position.asDouble (), 3840.0);
    }
}

TEST_F (ClipRendererTest, SerializeAutomationClipWithLooping)
{
  // One automation point inside the loop range.
  add_automation_point (0.7f, 75);

  // Loop the clip 50..150 ticks (a 100-tick loop), total length 200 ticks.
  automation_clip->setTrackBounds (false);
  automation_clip->loopStartPosition ()->setTicks (50);
  automation_clip->loopEndPosition ()->setTicks (150);

  std::vector<ClipRenderer::RenderedAutomationPoint> points;

  ClipRenderer::serialize_to_points (*automation_clip, points);

  // The point at content tick 75 must be unwrapped to both 75 and 175
  // (75 + one loop length of 100 ticks).
  const auto * first_occurrence = find_rendered_point_at (points, 75.0);
  ASSERT_NE (first_occurrence, nullptr);
  EXPECT_FLOAT_EQ (first_occurrence->value, 0.7f);

  const auto * second_occurrence = find_rendered_point_at (points, 175.0);
  ASSERT_NE (second_occurrence, nullptr);
  EXPECT_FLOAT_EQ (second_occurrence->value, 0.7f);

  // No boundary at clip start 0 or at loop-wrap 150 — first AP is at 75,
  // automation doesn't apply before the first point in the current segment.

  // No point is emitted past the clip end (200 ticks).
  for (const auto &p : points)
    EXPECT_LE (p.position.asDouble (), 200.0);
}

TEST_F (ClipRendererTest, SerializeAutomationClipEmpty)
{
  // Don't add any automation points

  std::vector<ClipRenderer::RenderedAutomationPoint> points;

  ClipRenderer::serialize_to_points (*automation_clip, points);

  // With no automation points there is nothing to render.
  EXPECT_TRUE (points.empty ());
}

// Verifies that curve parameters (algorithm + curviness) propagate from each
// AutomationPoint into the rendered control point for the region it drives.
TEST_F (ClipRendererTest, SerializeAutomationClipWithCurve)
{
  // Add two automation points
  add_automation_point (0.0f, 50);
  add_automation_point (1.0f, 150);

  // Set curve options on the first point
  auto * first_ap = automation_clip->get_children_view ()[0];
  first_ap->curveOpts ()->setCurviness (0.5);
  first_ap->curveOpts ()->setAlgorithm (dsp::CurveOptions::Algorithm::Exponent);

  std::vector<ClipRenderer::RenderedAutomationPoint> points;

  ClipRenderer::serialize_to_points (*automation_clip, points);

  // The first point carries its curve parameters (no boundary at clip start
  // since the first AP is at 50 > 0).
  const auto * first = find_rendered_point_at (points, 50.0);
  ASSERT_NE (first, nullptr);
  EXPECT_FLOAT_EQ (first->value, 0.0f);
  EXPECT_EQ (first->curve_algo, dsp::CurveOptions::Algorithm::Exponent);
  EXPECT_FLOAT_EQ (first->curve_curviness, 0.5f);

  // The second point carries its own (default) curve parameters.
  const auto * second = find_rendered_point_at (points, 150.0);
  ASSERT_NE (second, nullptr);
  EXPECT_FLOAT_EQ (second->value, 1.0f);
  EXPECT_EQ (second->curve_algo, dsp::CurveOptions::Algorithm::Exponent);
  EXPECT_FLOAT_EQ (second->curve_curviness, 0.0f);
}

TEST_F (ClipRendererTest, SerializeAutomationClipWithClipStart)
{
  // Play back from clip-start tick 25 (stop tracking bounds so it sticks).
  automation_clip->setTrackBounds (false);
  automation_clip->clipStartPosition ()->setTicks (25);

  // A point before the clip start and one after.
  add_automation_point (0.0f, 10);
  add_automation_point (1.0f, 50);

  std::vector<ClipRenderer::RenderedAutomationPoint> points;

  ClipRenderer::serialize_to_points (*automation_clip, points);

  // With clip_start=25, the intro segment plays virt [25, 200) → timeline
  // delta [0, 175). Content position ct maps to delta (ct - 25).
  // The clip also wraps (clip_start > 0 ⇒ looped), producing a second
  // segment virt [0, 25) → delta [175, 200).

  // A boundary control point at delta 0 (= clip-start content tick 25)
  // carrying the interpolated value between the two surrounding points
  // ((25-10)/(50-10) = 0.375 of the way from 0.0 to 1.0).
  const auto * boundary = find_rendered_point_at (points, 0.0);
  ASSERT_NE (boundary, nullptr);
  EXPECT_NEAR (boundary->value, 0.375f, 0.01f);

  // The in-range AP@50 appears at delta 25 (= 50 - clip_start 25).
  const auto * after = find_rendered_point_at (points, 25.0);
  ASSERT_NE (after, nullptr);
  EXPECT_FLOAT_EQ (after->value, 1.0f);

  // AP@10 reappears in the loop-wrap segment at delta 185 (= 175 + 10).
  const auto * wrapped = find_rendered_point_at (points, 185.0);
  ASSERT_NE (wrapped, nullptr);
  EXPECT_FLOAT_EQ (wrapped->value, 0.0f);
}

// The clip ends before the second point: only the first point (and the
// clip-start boundary) are rendered; the out-of-range point is dropped.
TEST_F (ClipRendererTest, SerializeAutomationClipEndingMidCurve)
{
  // First point at 50 ticks, second point at 250 ticks (beyond the clip end).
  add_automation_point (0.0f, 50);
  add_automation_point (1.0f, 250);

  // Clip length 200 ticks: ends before the second point.
  automation_clip->length ()->setTicks (200);

  std::vector<ClipRenderer::RenderedAutomationPoint> points;

  ClipRenderer::serialize_to_points (*automation_clip, points);

  // First AP at 50, plus a closing boundary at 200 (clip end) that carries
  // the interpolated curve toward AP@250.
  ASSERT_EQ (points.size (), 2u);
  EXPECT_FLOAT_EQ (find_rendered_point_at (points, 50.0)->value, 0.0f);

  // The point beyond the clip end is not rendered.
  EXPECT_EQ (find_rendered_point_at (points, 250.0), nullptr);
}

// A looped clip whose tail ends mid-curve: the rendered points must unwrap the
// loop, repeating the in-loop points at the correct offsets and inserting an
// interpolated boundary control point at each loop wrap-around.
TEST_F (ClipRendererTest, SerializeAutomationClipLoopedEndingMidCurve)
{
  add_automation_point (0.0f, 25);  // Before loop
  add_automation_point (0.5f, 75);  // Inside loop
  add_automation_point (1.0f, 125); // Inside loop
  add_automation_point (
    0.2f, 175); // After loop (outside the played region — never reached)

  // Loop 50..150 ticks (100-tick loop), total length 200 ticks.
  automation_clip->setTrackBounds (false);
  automation_clip->loopStartPosition ()->setTicks (50);
  automation_clip->loopEndPosition ()->setTicks (150);
  automation_clip->length ()->setTicks (200);

  std::vector<ClipRenderer::RenderedAutomationPoint> points;

  ClipRenderer::serialize_to_points (*automation_clip, points);

  // Segment 1 (intro, virt [0,150)): 3 APs @25/@75/@125 + closing @150
  // (curve from AP@125 toward AP@175, which lies beyond the loop).
  // Segment 2 (loop, virt [50,100)): start boundary @150, AP@75 @175,
  // closing @200.
  // Total: 3 + 1 + 1 + 1 + 1 = 7 points.
  ASSERT_EQ (points.size (), 7u);
  EXPECT_FLOAT_EQ (find_rendered_point_at (points, 25.0)->value, 0.0f);
  EXPECT_FLOAT_EQ (find_rendered_point_at (points, 75.0)->value, 0.5f);
  EXPECT_FLOAT_EQ (find_rendered_point_at (points, 125.0)->value, 1.0f);

  // At the loop-wrap position (150) two points co-exist:
  //  - closing from segment 1 (curve toward AP@175=0.2)
  //  - start boundary from segment 2 (interpolated at loop-start tick 50,
  //    halfway between AP@25=0.0 and AP@75=0.5 -> 0.25)
  bool found_wrap_start = false;
  for (const auto &p : points)
    {
      if (
        std::abs (p.position.asDouble () - 150.0) < 1e-6
        && std::abs (p.value - 0.25f) < 0.01f)
        found_wrap_start = true;
    }
  EXPECT_TRUE (found_wrap_start);

  // The point at 75 (0.5) repeats one loop length later at 175.
  EXPECT_FLOAT_EQ (find_rendered_point_at (points, 175.0)->value, 0.5f);

  // The point at 175 (0.2) lies outside the played region and must not appear.
  for (const auto &p : points)
    EXPECT_NE (p.value, 0.2f);
}

TEST_F (ClipRendererTest, SerializeAutomationClipWithPointBeforeClip)
{
  // Add an automation point before the clip start (negative position)
  add_automation_point (0.2f, -25); // 25 ticks before clip start
  add_automation_point (0.8f, 150); // 150 ticks after clip start

  std::vector<ClipRenderer::RenderedAutomationPoint> points;

  ClipRenderer::serialize_to_points (*automation_clip, points);

  // The pre-clip point (tick -25) is outside the played region and is not
  // emitted on its own...
  EXPECT_EQ (find_rendered_point_at (points, -25.0), nullptr);

  // ...but its influence is captured by the clip-start boundary control point,
  // which holds the value interpolated between the two points at tick 0:
  // (0 - (-25)) / (150 - (-25)) = 25/175 ~= 0.1429 of the way from 0.2 to 0.8.
  ASSERT_EQ (points.size (), 2u);
  EXPECT_NEAR (points[0].position.asDouble (), 0.0, 1e-6);
  EXPECT_NEAR (points[0].value, 0.2857f, 0.01f);

  // The in-range point is rendered at its own position with its exact value.
  EXPECT_NEAR (points[1].position.asDouble (), 150.0, 1e-6);
  EXPECT_FLOAT_EQ (points[1].value, 0.8f);
}

// Loop start falls in the middle of a curve. The rendered points must unwrap
// the loop: in-loop points repeat at each iteration's offset, and an
// interpolated boundary control point is emitted at every loop wrap-around.
TEST_F (ClipRendererTest, SerializeAutomationClipLoopStartMidCurve)
{
  add_automation_point (0.0f, 25);  // Before loop start
  add_automation_point (1.0f, 75);  // After loop start
  add_automation_point (0.5f, 125); // Inside loop

  // Loop 50..150 ticks (100-tick loop), total length 300 ticks (3 iterations).
  automation_clip->setTrackBounds (false);
  automation_clip->loopStartPosition ()->setTicks (50);
  automation_clip->loopEndPosition ()->setTicks (150);
  automation_clip->length ()->setTicks (300);

  std::vector<ClipRenderer::RenderedAutomationPoint> points;

  ClipRenderer::serialize_to_points (*automation_clip, points);

  // Segment 1 (intro, virt [0,150)): 3 APs. No closing boundary (last AP
  // held flat to end).
  // Segment 2 (loop,  virt [50,150)): start boundary @150, 2 APs. No closing
  // (last AP held flat).
  // Segment 3 (loop,  virt [50,100)): start boundary @250, 1 AP + closing
  // @300 (next AP@125 provides curve info).
  // Total: 3 + 1 + 2 + 1 + 1 + 1 = 9 points.
  ASSERT_EQ (points.size (), 9u);
  EXPECT_FLOAT_EQ (find_rendered_point_at (points, 25.0)->value, 0.0f);
  EXPECT_FLOAT_EQ (find_rendered_point_at (points, 75.0)->value, 1.0f);
  EXPECT_FLOAT_EQ (find_rendered_point_at (points, 125.0)->value, 0.5f);

  // Each loop wrap start carries the interpolated value at the loop start
  // (tick 50), halfway between the point at 25 (0.0) and the point at 75
  // (1.0) -> 0.5.
  EXPECT_NEAR (find_rendered_point_at (points, 150.0)->value, 0.5f, 0.01f);
  EXPECT_NEAR (find_rendered_point_at (points, 250.0)->value, 0.5f, 0.01f);

  // The point at 75 (1.0) repeats in every loop iteration.
  EXPECT_FLOAT_EQ (find_rendered_point_at (points, 175.0)->value, 1.0f);
  EXPECT_FLOAT_EQ (find_rendered_point_at (points, 275.0)->value, 1.0f);
  // The point at 125 (0.5) repeats in the second iteration.
  EXPECT_FLOAT_EQ (find_rendered_point_at (points, 225.0)->value, 0.5f);

  // No point is emitted past the clip end (300 ticks).
  for (const auto &p : points)
    EXPECT_LE (p.position.asDouble (), 300.0);
}

// ========== MIDI Control Event Rendering Tests ==========

TEST_F (ClipRendererTest, SerializeMidiControlChange)
{
  auto ev_ref =
    utils::create_object<MidiControlEvent> (registry, *tempo_map_wrapper);
  auto * ev = ev_ref.get_object_as<MidiControlEvent> ();
  ev->setControlType (MidiControlEvent::EventType::ControlChange);
  ev->setChannel (2);
  ev->setController (74);
  ev->setValue (100);
  ev->position ()->setTicks (120);
  midi_clip->ArrangerObjectOwner<MidiControlEvent>::add_object (ev_ref);

  add_midi_note (60, 90, 100, 50);

  juce::MidiMessageSequence events;
  ClipRenderer::serialize_to_sequence (*midi_clip, events, std::nullopt);
  events.addTimeToMessages (midi_clip->position ()->ticks ());

  // Note on, CC, note off = 3 events
  EXPECT_EQ (events.getNumEvents (), 3);

  auto cc_event = events.getEventPointer (1);
  EXPECT_TRUE (cc_event->message.isController ());
  EXPECT_EQ (cc_event->message.getControllerNumber (), 74);
  EXPECT_EQ (cc_event->message.getControllerValue (), 100);
}

TEST_F (ClipRendererTest, SerializeMidiPitchBend)
{
  auto ev_ref =
    utils::create_object<MidiControlEvent> (registry, *tempo_map_wrapper);
  auto * ev = ev_ref.get_object_as<MidiControlEvent> ();
  ev->setControlType (MidiControlEvent::EventType::PitchBend);
  ev->setChannel (0);
  ev->setValue (8192);
  ev->position ()->setTicks (110);
  midi_clip->ArrangerObjectOwner<MidiControlEvent>::add_object (ev_ref);

  juce::MidiMessageSequence events;
  ClipRenderer::serialize_to_sequence (*midi_clip, events, std::nullopt);
  events.addTimeToMessages (midi_clip->position ()->ticks ());

  EXPECT_EQ (events.getNumEvents (), 1);

  auto pb_event = events.getEventPointer (0);
  EXPECT_TRUE (pb_event->message.isPitchWheel ());
  EXPECT_EQ (pb_event->message.getPitchWheelValue (), 8192);
}

TEST_F (ClipRendererTest, SerializeMidiChannelPressure)
{
  auto ev_ref =
    utils::create_object<MidiControlEvent> (registry, *tempo_map_wrapper);
  auto * ev = ev_ref.get_object_as<MidiControlEvent> ();
  ev->setControlType (MidiControlEvent::EventType::ChannelPressure);
  ev->setChannel (3);
  ev->setValue (64);
  ev->position ()->setTicks (130);
  midi_clip->ArrangerObjectOwner<MidiControlEvent>::add_object (ev_ref);

  juce::MidiMessageSequence events;
  ClipRenderer::serialize_to_sequence (*midi_clip, events, std::nullopt);
  events.addTimeToMessages (midi_clip->position ()->ticks ());

  EXPECT_EQ (events.getNumEvents (), 1);

  auto cp_event = events.getEventPointer (0);
  EXPECT_TRUE (cp_event->message.isChannelPressure ());
  EXPECT_EQ (cp_event->message.getChannelPressureValue (), 64);
}

TEST_F (ClipRendererTest, SerializeMidiProgramChange)
{
  auto ev_ref =
    utils::create_object<MidiControlEvent> (registry, *tempo_map_wrapper);
  auto * ev = ev_ref.get_object_as<MidiControlEvent> ();
  ev->setControlType (MidiControlEvent::EventType::ProgramChange);
  ev->setChannel (0);
  ev->setValue (42);
  ev->position ()->setTicks (100);
  midi_clip->ArrangerObjectOwner<MidiControlEvent>::add_object (ev_ref);

  juce::MidiMessageSequence events;
  ClipRenderer::serialize_to_sequence (*midi_clip, events, std::nullopt);
  events.addTimeToMessages (midi_clip->position ()->ticks ());

  EXPECT_EQ (events.getNumEvents (), 1);

  auto pc_event = events.getEventPointer (0);
  EXPECT_TRUE (pc_event->message.isProgramChange ());
  EXPECT_EQ (pc_event->message.getProgramChangeNumber (), 42);
}

TEST_F (ClipRendererTest, SerializeMidiNotesAndControlEventsTogether)
{
  add_midi_note (60, 90, 100, 50);

  auto cc_ref =
    utils::create_object<MidiControlEvent> (registry, *tempo_map_wrapper);
  auto * cc = cc_ref.get_object_as<MidiControlEvent> ();
  cc->setControlType (MidiControlEvent::EventType::ControlChange);
  cc->setChannel (0);
  cc->setController (1);
  cc->setValue (127);
  cc->position ()->setTicks (105);
  midi_clip->ArrangerObjectOwner<MidiControlEvent>::add_object (cc_ref);

  juce::MidiMessageSequence events;
  ClipRenderer::serialize_to_sequence (*midi_clip, events, std::nullopt);
  events.addTimeToMessages (midi_clip->position ()->ticks ());

  // Note on, CC, note off = 3 events
  EXPECT_EQ (events.getNumEvents (), 3);
  EXPECT_TRUE (events.getEventPointer (0)->message.isNoteOn ());
  EXPECT_TRUE (events.getEventPointer (1)->message.isController ());
  EXPECT_TRUE (events.getEventPointer (2)->message.isNoteOff ());
}

// When an automation point sits exactly at the loop-start position, the
// second loop iteration starts playback at that AP's virtual tick. Because
// eval_at_virt_tick returns the AP directly on an exact match (no synthetic
// boundary point is emitted), the only rendered point at the loop-wrap
// position is the AP itself, which naturally carries its own curve params.
TEST_F (ClipRendererTest, SerializeAutomationClipPointAtLoopBoundary)
{
  add_automation_point (0.0f, 25);
  add_automation_point (0.5f, 50); // exactly at loop_start
  add_automation_point (1.0f, 100);

  // Give AP1 and AP2 different curviness so we can tell them apart.
  auto * ap1 = automation_clip->get_children_view ()[0];
  ap1->curveOpts ()->setCurviness (0.8);
  ap1->curveOpts ()->setAlgorithm (dsp::CurveOptions::Algorithm::Exponent);

  auto * ap2 = automation_clip->get_children_view ()[1];
  ap2->curveOpts ()->setCurviness (-0.3);
  ap2->curveOpts ()->setAlgorithm (dsp::CurveOptions::Algorithm::Exponent);

  automation_clip->setTrackBounds (false);
  automation_clip->loopStartPosition ()->setTicks (50);
  automation_clip->loopEndPosition ()->setTicks (150);

  std::vector<ClipRenderer::RenderedAutomationPoint> points;
  ClipRenderer::serialize_to_points (*automation_clip, points);

  // The second loop iteration starts at virtual tick 50 (= AP2's position).
  // The playback position of the loop wrap is 150 (first iteration length).
  // The segment-2 start point at 150 must carry AP2's curve params.
  // (A closing boundary from the previous segment may also sit at 150 —
  // that one carries the last AP's curve, not AP2's.)
  for (const auto &p : points)
    {
      if (
        std::abs (p.position.asDouble () - 150.0) < 1e-6
        && std::abs (p.value - 0.5f) < 1e-6)
        {
          EXPECT_FLOAT_EQ (p.curve_curviness, -0.3f)
            << "Loop-start point at 150 should carry AP2's curve (curviness "
               "-0.3), got "
            << p.curve_curviness;
        }
    }
}

// Source-mode clip with a tempo change: loop iterations must have non-uniform
// timeline spacing because the same source-BPM duration maps to different tick
// counts at different tempo positions.
//
// Setup: 120 BPM at tick 0, drops to 60 BPM at tick 1920. Source BPM 120.
// Loop [0, 1920) = 1 second of content at 120 BPM. Clip length 3840 (2
// iterations).
//
// Expected: note at content tick 960 appears at delta 960 (first iteration,
// before tempo change) and delta 2400 (second iteration, after tempo change).
// Spacing = 1440, NOT 1920 (which the old uniform-spacing bug would produce).
TEST_F (ClipRendererTest, SerializeMidiClipSourceModeWithTempoChangeAndLoop)
{
  tempo_map->add_tempo_event (
    units::ticks (1920), units::bpm (60.0), dsp::TempoMap::CurveType::Constant);

  midi_clip->position ()->setTicks (0);
  midi_clip->length ()->setTicks (3840);
  midi_clip->setTrackBounds (false);
  midi_clip->loopStartPosition ()->setTicks (0);
  midi_clip->loopEndPosition ()->setTicks (1920);
  midi_clip->clipStartPosition ()->setTicks (0);
  midi_clip->contentWarp ()->configure_as_source (units::bpm (120.0));

  add_midi_note (60, 100, 960, 100);

  juce::MidiMessageSequence events;
  ClipRenderer::serialize_to_sequence (*midi_clip, events);

  std::vector<double> note_ons;
  for (int i = 0; i < events.getNumEvents (); ++i)
    if (events.getEventPointer (i)->message.isNoteOn ())
      note_ons.push_back (events.getEventPointer (i)->message.getTimeStamp ());

  ASSERT_EQ (note_ons.size (), 2u)
    << "Note should appear once per loop iteration";

  EXPECT_NEAR (note_ons[0], 960.0, 1.0)
    << "First occurrence at 120 BPM: 0.5s -> 960 ticks";
  EXPECT_NEAR (note_ons[1], 2400.0, 1.0)
    << "Second occurrence: 1.5s -> 1920 + 480 = 2400 ticks";

  const auto spacing = note_ons[1] - note_ons[0];
  EXPECT_NEAR (spacing, 1440.0, 1.0)
    << "Non-uniform spacing (1440, not 1920) proves tempo change is respected";
}

} // namespace zrythm::structure::arrangement
