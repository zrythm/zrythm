// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/tracks/track_fwd.h"
#include "utils/types.h"

#include <farbot/RealtimeObject.hpp>

namespace juce
{
class MidiMessageSequence;
}

namespace zrythm::structure::tracks
{

/**
 * @brief Event provider for clip launcher-based MIDI events.
 */
class ClipLauncherMidiEventProvider final
{
  struct Cache
  {
    Cache (
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

public:
  ClipLauncherMidiEventProvider (const dsp::TempoMap &tempo_map);

  /**
   * @brief Generate the MIDI event sequence to be used during realtime
   * processing.
   *
   * To be called as needed from the UI thread when a new cache is requested.
   */
  void generate_events (
    const arrangement::MidiRegion        &midi_region,
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
  void process_events (
    const EngineProcessTimeInfo &time_nfo,
    dsp::MidiEventVector        &output_buffer) noexcept [[clang::nonblocking]];

  /**
   * @brief Whether currently playing any part of the clip.
   */
  auto playing () const { return playing_.load (); }

  auto current_playback_position_in_clip () const
  {
    return internal_clip_buffer_position_.load ();
  }

private:
  /** Active MIDI playback sequence for realtime access. */
  farbot::RealtimeObject<
    std::optional<Cache>,
    farbot::RealtimeObjectOptions::nonRealtimeMutatable>
    active_midi_playback_sequence_;

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
   * This is only accessed by the ClipLauncherMidiEventProvider (audio thread)
   * and doesn't need to be atomic.
   */
  units::sample_t last_seen_timeline_position_;

  /**
   * @brief Timeline position where the clip was launched
   *
   * This is only accessed by the ClipLauncherMidiEventProvider (audio thread)
   * and doesn't need to be atomic.
   */
  units::sample_t clip_launch_timeline_position_;

  static_assert (decltype (internal_clip_buffer_position_)::is_always_lock_free);
};

}
