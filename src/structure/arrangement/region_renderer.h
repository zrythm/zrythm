// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/arrangement/arranger_object_all.h"
#include "utils/units.h"

#include <juce_wrapper.h>

namespace zrythm::structure::arrangement
{

/**
 * @brief A class that converts region data to serialized formats.
 *
 * This class provides methods to serialize both MIDI and Audio regions,
 * handling looped playback and various constraints.
 */
class RegionRenderer final
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
   * @param add_region_start Add region start offset to positions (false)
   * @param as_played Serialize as it would be played in the timeline (with
   * loops and clip start) (true)
   */
  static void serialize_to_sequence (
    const MidiRegion          &region,
    juce::MidiMessageSequence &events,
    std::optional<std::pair<double, double>> timeline_range_ticks = std::nullopt);

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
    std::optional<std::pair<double, double>> timeline_range_ticks = std::nullopt);

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
    std::optional<std::pair<double, double>> timeline_range_ticks = std::nullopt);

  /**
   * Serializes an Automation region to sample-accurate automation values.
   *
   * The output buffer always represents the full region length, with index 0
   * corresponding to the region's start position. Automation point positions
   * are relative to the region start.
   *
   * Values between automation points are interpolated using the curve algorithm
   * defined on each automation point. The last automation point fills all
   * remaining samples with its value.
   *
   * Playback behavior:
   * - Starts from clip start (position 0 in output buffer)
   * - Plays until loop end, then loops back to loop start
   * - Automation points before loop start appear only once
   * - Automation points within loop range (loop_start to loop_end) appear in
   * each loop iteration
   *
   * Constraints (start/end) are global timeline positions and are applied by
   * setting values outside the constraint range to -1.0. The output buffer size
   * is always the full region length, regardless of constraints.
   *
   * @param region The Automation region to serialize
   * @param[out] values Output vector of normalized automation values
   * (sample-accurate). Values are resized to the full region length.
   * Negative values (-1.0) indicate no automation present.
   * @param start Optional global timeline start position in ticks for
   * constraints
   * @param end Optional global timeline end position in ticks for constraints
   */
  static void serialize_to_automation_values (
    const AutomationRegion &region,
    std::vector<float>     &values,
    std::optional<std::pair<double, double>> timeline_range_ticks = std::nullopt)
    [[clang::blocking]];

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

  template <RegionObject RegionT, typename EventsT>
  static void serialize_region (
    const RegionT &region,
    EventsT       &events,
    // currently unused
    std::optional<std::pair<double, double>> timeline_range_ticks = std::nullopt)
  {
    const auto * loop_range = region.loopRange ();
    const auto   loop_length = loop_range->get_loop_length_in_ticks ();
    const auto   region_length =
      units::ticks (region.bounds ()->length ()->ticks ());

    auto loop_segment_virtual_start =
      units::ticks (loop_range->clipStartPosition ()->ticks ());
    auto loop_segment_virtual_end =
      units::ticks (loop_range->loopEndPosition ()->ticks ());
    auto loop_segment_absolute_start = units::ticks (0.0);
    auto loop_segment_absolute_end =
      loop_segment_virtual_end - loop_segment_virtual_start;
    if (loop_segment_absolute_end > region_length)
      {
        const auto delta = loop_segment_absolute_end - region_length;
        loop_segment_virtual_end -= delta;
        loop_segment_absolute_end -= delta;
      }

    while (loop_segment_absolute_start < region_length)
      {
        if constexpr (std::is_same_v<RegionT, MidiRegion>)
          {
            handle_midi_region_range (
              region, events,
              std::make_pair (
                loop_segment_virtual_start, loop_segment_virtual_end),
              loop_segment_absolute_start);
          }
        else if constexpr (std::is_same_v<RegionT, ChordRegion>)
          {
            handle_chord_region_range (
              region, events,
              std::make_pair (
                loop_segment_virtual_start, loop_segment_virtual_end),
              loop_segment_absolute_start);
          }
        else if constexpr (std::is_same_v<RegionT, AudioRegion>)
          {
            handle_audio_region_range (
              region, events,
              std::make_pair (
                loop_segment_virtual_start, loop_segment_virtual_end),
              loop_segment_absolute_start);
          }
        else if constexpr (std::is_same_v<RegionT, AutomationRegion>)
          {
            handle_automation_region_range (
              region, events,
              std::make_pair (
                loop_segment_virtual_start, loop_segment_virtual_end),
              loop_segment_absolute_start);
          }

        // Advance to next segment
        const auto currentLen =
          loop_segment_absolute_end - loop_segment_absolute_start;
        loop_segment_virtual_start =
          units::ticks (loop_range->loopStartPosition ()->ticks ());
        loop_segment_virtual_end =
          units::ticks (loop_range->loopEndPosition ()->ticks ());
        loop_segment_absolute_start += currentLen;
        loop_segment_absolute_end += loop_length;

        // Clip final segment to region bounds
        if (loop_segment_absolute_end > region_length)
          {
            const auto delta = loop_segment_absolute_end - region_length;
            loop_segment_virtual_end -= delta;
            loop_segment_absolute_end -= delta;
          }
      }
  }

  static void handle_midi_region_range (
    const MidiRegion                                       &region,
    juce::MidiMessageSequence                              &events,
    std::pair<units::precise_tick_t, units::precise_tick_t> virtual_range,
    units::precise_tick_t                                   absolute_start);

  static void handle_chord_region_range (
    const ChordRegion                                      &region,
    juce::MidiMessageSequence                              &events,
    std::pair<units::precise_tick_t, units::precise_tick_t> virtual_range,
    units::precise_tick_t                                   absolute_start);

  static void handle_audio_region_range (
    const AudioRegion                                      &region,
    juce::AudioSampleBuffer                                &buffer,
    std::pair<units::precise_tick_t, units::precise_tick_t> virtual_range,
    units::precise_tick_t                                   absolute_start);

  static void handle_automation_region_range (
    const AutomationRegion                                 &region,
    std::vector<float>                                     &values,
    std::pair<units::precise_tick_t, units::precise_tick_t> virtual_range,
    units::precise_tick_t                                   absolute_start);

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
    juce::AudioSampleBuffer &buffer);

  /**
   * Applies built-in fades to the audio buffer as a separate pass.
   */
  static void apply_builtin_fades_pass (
    const AudioRegion       &region,
    juce::AudioSampleBuffer &buffer,
    int                      builtin_fade_frames);
};

} // namespace zrythm::structure::arrangement
