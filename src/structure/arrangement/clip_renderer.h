// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/tick_types.h"
#include "structure/arrangement/arranger_object_all.h"
#include "utils/audio.h"
#include "utils/units.h"

#include <juce_audio_basics/juce_audio_basics.h>

namespace zrythm::structure::arrangement
{

/**
 * @brief A class that converts clip data to serialized formats.
 *
 * This class provides methods to serialize both MIDI and Audio clips,
 * handling looped playback and various constraints.
 */
class ClipRenderer final
{
public:
  using TimelineRange = std::pair<dsp::TimelineTick, dsp::TimelineTick>;

  /**
   * @brief Serializes a MIDI clip to a MIDI message sequence.
   *
   * Event timestamps are in ticks relative to the clip start (tick 0 = clip
   * start). Looping and clip-start offsets are applied. Callers that need
   * absolute timeline positions should add @c clip.position()->ticks() to
   * each event timestamp (e.g. via @c events.addTimeToMessages()).
   *
   * @param clip The MIDI clip to serialize.
   * @param events Output MIDI message sequence.
   * @param timeline_range_ticks Currently unused.
   */
  static void serialize_to_sequence (
    const MidiClip              &clip,
    juce::MidiMessageSequence   &events,
    std::optional<TimelineRange> timeline_range_ticks = std::nullopt);

  /**
   * @brief Serializes a Chord clip to a MIDI message sequence.
   *
   * Event timestamps are in ticks relative to the clip start (tick 0 = clip
   * start). See the MidiClip overload for details.
   *
   * @param clip The Chord clip to serialize.
   * @param events Output MIDI message sequence.
   * @param timeline_range_ticks Currently unused.
   */
  static void serialize_to_sequence (
    const ChordClip             &clip,
    juce::MidiMessageSequence   &events,
    std::optional<TimelineRange> timeline_range_ticks = std::nullopt);

  /**
   * @brief Serializes an Audio clip to an audio sample buffer.
   *
   * The buffer represents the clip's audio as played back with looping and
   * clip-start applied. Buffer index 0 corresponds to the clip start.
   *
   * @param clip The Audio clip to serialize.
   * @param buffer Output audio sample buffer.
   * @param timeline_range_ticks Currently unused.
   */
  static void serialize_to_buffer (
    const AudioClip             &clip,
    juce::AudioSampleBuffer     &buffer,
    std::optional<TimelineRange> timeline_range_ticks = std::nullopt);

  /**
   * Serializes an Automation clip to sample-accurate automation values.
   *
   * The output buffer always represents the full clip length, with index 0
   * corresponding to the clip's start position. Automation point positions
   * are relative to the clip start.
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
   * is always the full clip length, regardless of constraints.
   *
   * @param clip The Automation clip to serialize
   * @param[out] values Output vector of normalized automation values
   * (sample-accurate). Values are resized to the full clip length.
   * Negative values (-1.0) indicate no automation present.
   * @param start Optional global timeline start position in ticks for
   * constraints
   * @param end Optional global timeline end position in ticks for constraints
   */
  static void serialize_to_automation_values (
    const AutomationClip        &clip,
    std::vector<float>          &values,
    std::optional<TimelineRange> timeline_range_ticks = std::nullopt)
    [[clang::blocking]];

private:
  /**
   * @brief Common loop parameters extracted from a clip.
   */
  struct LoopParameters
  {
    dsp::ContentTick loop_start;
    dsp::ContentTick loop_end;
    dsp::ContentTick clip_start;
    dsp::ContentTick loop_length;
    dsp::ContentTick clip_length;
    int              num_loops;

    LoopParameters (const Clip &clip);
  };

  template <ClipObject ClipT, typename EventsT>
  static void serialize_clip (
    const ClipT &clip,
    EventsT     &events,
    // currently unused
    std::optional<TimelineRange> timeline_range_ticks = std::nullopt)
  {
    const auto * warp = clip.contentWarp ();
    const auto   clip_length = clip.length ()->asTick ();

    // contentToTimeline() returns absolute timeline positions (it includes
    // the clip's start position).  We subtract clip_start so that all
    // positions passed to handle_*_clip_range are relative to the clip
    // start — matching the committed behaviour and what callers expect.
    const auto clip_start = warp->contentToTimeline (dsp::ContentTick{});

    // Timeline end of the clip and span of one loop iteration (relative).
    const auto tl_clip_end = warp->contentToTimeline (clip_length) - clip_start;
    const auto tl_loop_length =
      warp->contentToTimeline (clip.loopEndPosition ()->asTick ())
      - warp->contentToTimeline (clip.loopStartPosition ()->asTick ());

    auto loop_segment_virtual_start = clip.clipStartPosition ()->asTick ();
    auto loop_segment_virtual_end = clip.loopEndPosition ()->asTick ();

    // Timeline positions of the current segment (relative to clip start).
    auto loop_segment_start =
      warp->contentToTimeline (loop_segment_virtual_start) - clip_start;
    auto loop_segment_end =
      warp->contentToTimeline (loop_segment_virtual_end) - clip_start;

    // Clip first segment to clip bounds.
    if (loop_segment_end > tl_clip_end)
      {
        loop_segment_virtual_end =
          warp->timelineToContent (tl_clip_end + clip_start);
        loop_segment_end = tl_clip_end;
      }

    while (loop_segment_start < tl_clip_end)
      {
        if constexpr (std::is_same_v<ClipT, MidiClip>)
          {
            handle_midi_clip_range (
              clip, events,
              std::make_pair (
                loop_segment_virtual_start, loop_segment_virtual_end),
              loop_segment_start);
          }
        else if constexpr (std::is_same_v<ClipT, ChordClip>)
          {
            handle_chord_clip_range (
              clip, events,
              std::make_pair (
                loop_segment_virtual_start, loop_segment_virtual_end),
              loop_segment_start);
          }
        else if constexpr (std::is_same_v<ClipT, AudioClip>)
          {
            handle_audio_clip_range (
              clip, events,
              std::make_pair (
                loop_segment_virtual_start, loop_segment_virtual_end),
              loop_segment_start);
          }
        else if constexpr (std::is_same_v<ClipT, AutomationClip>)
          {
            handle_automation_clip_range (
              clip, events,
              std::make_pair (
                loop_segment_virtual_start, loop_segment_virtual_end),
              loop_segment_start);
          }

        // Advance to next segment
        const auto currentLen = loop_segment_end - loop_segment_start;
        loop_segment_virtual_start = clip.loopStartPosition ()->asTick ();
        loop_segment_virtual_end = clip.loopEndPosition ()->asTick ();
        loop_segment_start = loop_segment_start + currentLen;
        loop_segment_end = loop_segment_end + tl_loop_length;

        // Clip final segment to clip bounds
        if (loop_segment_end > tl_clip_end)
          {
            loop_segment_virtual_end = warp->timelineToContent (
              tl_clip_end - loop_segment_start
              + warp->contentToTimeline (loop_segment_virtual_start));
            loop_segment_end = tl_clip_end;
          }
      }
  }

  static void handle_midi_clip_range (
    const MidiClip                               &clip,
    juce::MidiMessageSequence                    &events,
    std::pair<dsp::ContentTick, dsp::ContentTick> virtual_range,
    dsp::TimelineTick                             segment_start);

  static void handle_chord_clip_range (
    const ChordClip                              &clip,
    juce::MidiMessageSequence                    &events,
    std::pair<dsp::ContentTick, dsp::ContentTick> virtual_range,
    dsp::TimelineTick                             segment_start);

  static void handle_audio_clip_range (
    const AudioClip                              &clip,
    juce::AudioSampleBuffer                      &buffer,
    std::pair<dsp::ContentTick, dsp::ContentTick> virtual_range,
    dsp::TimelineTick                             segment_start);

  static void handle_automation_clip_range (
    const AutomationClip                         &clip,
    std::vector<float>                           &values,
    std::pair<dsp::ContentTick, dsp::ContentTick> virtual_range,
    dsp::TimelineTick                             segment_start);

  /**
   * @brief Reads the clip's content for an output sample range, in playback
   * (loop) order at the clip's native sample rate.
   *
   * @p out_start and @p out_end are output positions in native samples relative
   * to the clip start. The clip read position for @p out_start is computed
   * directly in O(1) (modular arithmetic on the loop length), so requesting a
   * sub-range never touches samples before @p out_start. This keeps
   * incremental/range renders (e.g. per-frame recording waveform updates)
   * O(range) instead of O(clip).
   */
  static utils::audio::AudioBuffer build_native_looped_buffer (
    const AudioClip &clip,
    units::sample_t  out_start,
    units::sample_t  out_end);

  /**
   * Applies gain to the entire audio buffer as a separate pass.
   */
  static void
  apply_gain_pass (const AudioClip &clip, juce::AudioSampleBuffer &buffer);

  /**
   * Applies clip (object) fades to the audio buffer as a separate pass.
   */
  static void
  apply_clip_fades_pass (const AudioClip &clip, juce::AudioSampleBuffer &buffer);

  /**
   * Applies built-in fades to the audio buffer as a separate pass.
   */
  static void apply_builtin_fades_pass (
    const AudioClip         &clip,
    juce::AudioSampleBuffer &buffer,
    int                      builtin_fade_frames);
};

} // namespace zrythm::structure::arrangement
