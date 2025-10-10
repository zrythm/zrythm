// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "utils/units.h"

#include <juce_wrapper.h>

namespace zrythm::structure::arrangement
{
class MidiRegion;
class AudioRegion;
class ChordRegion;

/**
 * @brief A class that converts region data to serialized formats.
 *
 * This class provides methods to serialize both MIDI and Audio regions,
 * handling looped playback and various constraints.
 */
class RegionSerializer final
{
public:
  /**
   * Serializes a MIDI region to a MIDI message sequence.
   *
   * @note Event timings will be set in ticks.
   *
   * @param region The MIDI region to serialize
   * @param events Output MIDI message sequence
   * @param start Optional timeline start position in ticks
   * @param end Optional timeline end position in ticks
   * @param add_region_start Add region start offset to positions
   * @param as_played Serialize as it would be played in the timeline (with
   * loops and clip start)
   */
  static void serialize_to_sequence (
    const MidiRegion          &region,
    juce::MidiMessageSequence &events,
    std::optional<double>      start = std::nullopt,
    std::optional<double>      end = std::nullopt,
    bool                       add_region_start = true,
    bool                       as_played = true)
  {
    serialize_midi_region (
      region, events, start, end, add_region_start, as_played);
  }

  /**
   * Serializes a Chord region to a MIDI message sequence.
   *
   * @note Event timings will be set in ticks.
   *
   * @param region The Chord region to serialize
   * @param events Output MIDI message sequence
   * @param start Optional timeline start position in ticks
   * @param end Optional timeline end position in ticks
   * @param add_region_start Add region start offset to positions
   * @param as_played Serialize as it would be played in the timeline (with
   * loops and clip start)
   */
  static void serialize_to_sequence (
    const ChordRegion         &region,
    juce::MidiMessageSequence &events,
    std::optional<double>      start = std::nullopt,
    std::optional<double>      end = std::nullopt,
    bool                       add_region_start = true,
    bool                       as_played = true)
  {
    serialize_chord_region (
      region, events, start, end, add_region_start, as_played);
  }

  /**
   * Serializes an Audio region to an audio sample buffer.
   *
   * Audio regions are always serialized as they would be played in the timeline
   * (with loops and clip start). The buffer always starts at the region's audio
   * data without any leading silence.
   *
   * @param region The Audio region to serialize
   * @param buffer Output audio sample buffer
   * @param start Optional timeline start position in ticks
   * @param end Optional timeline end position in ticks
   * @param builtin_fade_frames Number of frames to use for built-in fades
   */
  static void serialize_to_buffer (
    const AudioRegion       &region,
    juce::AudioSampleBuffer &buffer,
    std::optional<double>    start = std::nullopt,
    std::optional<double>    end = std::nullopt,
    int builtin_fade_frames = AudioRegion::BUILTIN_FADE_FRAMES)
  {
    serialize_audio_region (region, buffer, start, end, builtin_fade_frames);
  }

private:
  /**
   * @brief Common loop parameters extracted from a region.
   */
  struct LoopParameters
  {
    units::precise_tick_t loop_start;
    units::precise_tick_t loop_end;
    units::precise_tick_t clip_start;
    units::precise_tick_t loop_length;
    units::precise_tick_t region_length;
    int                   num_loops;

    LoopParameters (const ArrangerObject &region);
  };

  /**
   * Serializes a MIDI region to a MIDI message sequence.
   */
  static void serialize_midi_region (
    const MidiRegion          &region,
    juce::MidiMessageSequence &events,
    std::optional<double>      start,
    std::optional<double>      end,
    bool                       add_region_start,
    bool                       as_played);

  /**
   * Serializes an Audio region to an audio sample buffer.
   *
   * Audio regions are always serialized as they would be played in the timeline
   * (with loops and clip start).
   */
  static void serialize_audio_region (
    const AudioRegion       &region,
    juce::AudioSampleBuffer &buffer,
    std::optional<double>    start,
    std::optional<double>    end,
    int                      builtin_fade_frames);

  /**
   * Serializes a Chord region to a MIDI message sequence.
   *
   * @param region The Chord region to serialize
   * @param events Output MIDI message sequence
   * @param start Optional timeline start position in ticks
   * @param end Optional timeline end position in ticks
   * @param add_region_start Add region start offset to positions
   * @param as_played Serialize as it would be played in the timeline
   */
  static void serialize_chord_region (
    const ChordRegion         &region,
    juce::MidiMessageSequence &events,
    std::optional<double>      start,
    std::optional<double>      end,
    bool                       add_region_start,
    bool                       as_played);

  /**
   * Processes a single MIDI note across all loop iterations.
   */
  static void process_midi_note (
    const MidiNote                      &note,
    const LoopParameters                &loop_params,
    units::precise_tick_t                region_start_offset,
    std::optional<units::precise_tick_t> start,
    std::optional<units::precise_tick_t> end,
    bool                                 as_played,
    juce::MidiMessageSequence           &events);

  /**
   * Processes a single chord object across all loop iterations.
   */
  static void process_chord_object (
    const arrangement::ChordObject      &chord_obj,
    const LoopParameters                &loop_params,
    units::precise_tick_t                region_start_offset,
    std::optional<units::precise_tick_t> start,
    std::optional<units::precise_tick_t> end,
    bool                                 as_played,
    juce::MidiMessageSequence           &events);

  /**
   * Processes a single arranger object across all loop iterations.
   *
   * @tparam ObjectType The type of arranger object (MidiNote or ChordObject)
   * @tparam EventGenerator Function that generates MIDI events for the object
   * @param obj The arranger object to process
   * @param loop_params Loop parameters for the region
   * @param region_start_offset Offset to add to positions for global timeline
   * @param start Optional start position constraint
   * @param end Optional end position constraint
   * @param as_played Whether to process as played (with loops)
   * @param events Output MIDI message sequence
   * @param event_generator Function that generates events for the object
   */
  template <typename ObjectType, typename EventGenerator>
  static void process_arranger_object (
    const ObjectType                    &obj,
    const LoopParameters                &loop_params,
    units::precise_tick_t                region_start_offset,
    std::optional<units::precise_tick_t> start,
    std::optional<units::precise_tick_t> end,
    bool                                 as_played,
    juce::MidiMessageSequence           &events,
    EventGenerator                       event_generator);

  /**
   * Processes a single audio loop iteration.
   *
   * Audio regions are always processed as they would be played in the timeline
   * (with loops and clip start).
   */
  static void process_audio_loop (
    const AudioRegion                   &region,
    juce::PositionableAudioSource       &audio_source,
    const LoopParameters                &loop_params,
    int                                  loop_idx,
    units::precise_sample_t              buffer_offset_samples,
    std::optional<units::precise_tick_t> start,
    std::optional<units::precise_tick_t> end,
    juce::AudioSampleBuffer             &buffer);

  /**
   * Applies gain to the entire audio buffer as a separate pass.
   */
  static void
  apply_gain_pass (const AudioRegion &region, juce::AudioSampleBuffer &buffer);

  /**
   * Applies region (object) fades to the audio buffer as a separate pass.
   */
  static void apply_region_fades_pass (
    const AudioRegion       &region,
    juce::AudioSampleBuffer &buffer,
    const LoopParameters    &loop_params);

  /**
   * Applies built-in fades to the audio buffer as a separate pass.
   */
  static void apply_builtin_fades_pass (
    const AudioRegion       &region,
    juce::AudioSampleBuffer &buffer,
    int                      builtin_fade_frames);

  /**
   * Calculates the position of an event in looped playback.
   */
  static units::precise_tick_t get_looped_position (
    units::precise_tick_t original_pos,
    const LoopParameters &loop_params,
    int                   loop_index);

  /**
   * Checks if a position range falls within the specified constraints.
   */
  static bool is_position_in_range (
    units::precise_tick_t                start_pos,
    units::precise_tick_t                end_pos,
    std::optional<units::precise_tick_t> range_start,
    std::optional<units::precise_tick_t> range_end);

  /**
   * Checks if a MIDI note is in a playable part of the region.
   */
  static bool
  is_midi_note_playable (const MidiNote &note, const LoopParameters &loop_params);
};

} // namespace zrythm::structure::arrangement
