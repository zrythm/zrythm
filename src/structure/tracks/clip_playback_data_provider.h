// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/tracks/track_fwd.h"
#include "utils/types.h"

#include <farbot/RealtimeObject.hpp>
#include <juce_wrapper.h>

namespace zrythm::structure::tracks
{

/**
 * @brief Event provider for clip launcher-based MIDI and audio events.
 */
class ClipPlaybackDataProvider final
{
  struct MidiCache
  {
    MidiCache (
      juce::MidiMessageSequence           &&midi_seq,
      structure::tracks::ClipQuantizeOption quantize_opt,
      units::precise_tick_t                 end_position)
        : midi_seq_ (std::move (midi_seq)), quantize_opt_ (quantize_opt),
          end_position_ (end_position)
    {
    }
    juce::MidiMessageSequence             midi_seq_;
    structure::tracks::ClipQuantizeOption quantize_opt_;

    /**
     * @brief End position to loop at.
     */
    units::precise_tick_t end_position_;
  };

  struct AudioCache
  {
    AudioCache (
      juce::AudioSampleBuffer               audio_buffer,
      structure::tracks::ClipQuantizeOption quantize_opt,
      units::precise_tick_t                 end_position)
        : audio_buffer_ (std::move (audio_buffer)),
          quantize_opt_ (quantize_opt), end_position_ (end_position)
    {
    }

    juce::AudioSampleBuffer               audio_buffer_;
    structure::tracks::ClipQuantizeOption quantize_opt_;

    /**
     * @brief End position to loop at.
     */
    units::precise_tick_t end_position_;
  };

public:
  ClipPlaybackDataProvider (const dsp::TempoMap &tempo_map);

  /**
   * @brief Generate the MIDI event sequence to be used during realtime
   * processing.
   *
   * To be called as needed from the UI thread when a new cache is requested.
   */
  void generate_midi_events (
    const arrangement::MidiRegion        &midi_region,
    structure::tracks::ClipQuantizeOption quantize_option);

  /**
   * @brief Generate the audio buffer to be used during realtime processing.
   *
   * To be called as needed from the UI thread when a new cache is requested.
   */
  void generate_audio_events (
    const arrangement::AudioRegion       &audio_region,
    structure::tracks::ClipQuantizeOption quantize_option);

  void clear_events ();

  /**
   * @brief Requests that playback is stopped at the next quantization point.
   *
   * @param quantize_option
   */
  void
  queue_stop_playback (structure::tracks::ClipQuantizeOption quantize_option);

  // TrackEventProvider interface
  void process_midi_events (
    const EngineProcessTimeInfo &time_nfo,
    dsp::MidiEventVector        &output_buffer) noexcept [[clang::nonblocking]];

  /**
   * @brief Process audio events for clip launcher playback.
   */
  void process_audio_events (
    const EngineProcessTimeInfo &time_nfo,
    std::span<float>             left_buffer,
    std::span<float>             right_buffer) noexcept [[clang::nonblocking]];

  /**
   * @brief Whether currently playing any part of the clip.
   */
  auto playing () const { return playing_.load (); }

  auto current_playback_position_in_clip () const
  {
    return internal_clip_buffer_position_.load ();
  }

private:
  /**
   * @brief Common logic for updating playback position based on timeline
   * changes.
   */
  void update_playback_position (
    const EngineProcessTimeInfo &time_nfo,
    units::sample_t              clip_loop_end_samples);

  /**
   * @brief Common logic for handling quantization and playback start.
   */
  std::pair<units::sample_t, units::sample_t> handle_quantization_and_start (
    const EngineProcessTimeInfo          &time_nfo,
    units::sample_t                       clip_loop_end_samples,
    structure::tracks::ClipQuantizeOption quantize_opt);

  /**
   * @brief Template method for processing chunks with looping.
   */
  template <typename ProcessFunc>
  void process_chunks_with_looping (
    units::sample_t internal_buffer_start_offset,
    units::sample_t samples_to_process,
    units::sample_t clip_loop_end_samples,
    units::sample_t output_buffer_timestamp_offset,
    ProcessFunc     process_chunk);

  /** Active MIDI playback sequence for realtime access. */
  farbot::RealtimeObject<
    std::optional<MidiCache>,
    farbot::RealtimeObjectOptions::nonRealtimeMutatable>
    active_midi_playback_sequence_;

  /** Active audio playback buffer for realtime access. */
  farbot::RealtimeObject<
    std::optional<AudioCache>,
    farbot::RealtimeObjectOptions::nonRealtimeMutatable>
    active_audio_playback_buffer_;

  const dsp::TempoMap &tempo_map_;

  /**
   * @brief Whether currently playing.
   *
   * If false, we are currently queued or stopped.
   *
   * This is atomic because it may be accessed from the UI thread to update
   * the clip launcher visuals (e.g., to show play/pause state).
   */
  std::atomic<bool> playing_;

  /**
   * @brief Current internal buffer position (offset within the clip)
   *
   * This is atomic because it may be accessed from the UI thread to update
   * the clip launcher visuals (e.g., to show playback progress within the clip).
   */
  std::atomic<units::sample_t> internal_clip_buffer_position_;

  /**
   * @brief Last timeline position we've seen
   *
   * This is only accessed by the ClipPlaybackDataProvider (audio thread)
   * and doesn't need to be atomic.
   */
  units::sample_t last_seen_timeline_position_;

  /**
   * @brief Timeline position where the clip was launched
   *
   * This is only accessed by the ClipPlaybackDataProvider (audio thread)
   * and doesn't need to be atomic.
   */
  units::sample_t clip_launch_timeline_position_;

  static_assert (decltype (internal_clip_buffer_position_)::is_always_lock_free);
};

}
