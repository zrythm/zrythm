// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <cstdint>

#include "utils/units.h"

namespace zrythm::dsp
{
/**
 * @brief Computes the playhead position after adding frames, wrapping around
 * the loop end if looping is enabled.
 *
 * Shared by ITransport's default implementation and Transport's audio-thread
 * path.
 */
inline units::sample_t
playhead_position_after_adding_frames (
  units::sample_t current_position,
  units::sample_t frames_to_add,
  bool            loop_enabled,
  units::sample_t loop_start,
  units::sample_t loop_end) noexcept
{
  auto new_pos = current_position + frames_to_add;

  /* if start frames were before the loop-end point and the new frames are
   * after (loop crossed) */
  if (loop_enabled)
    {
      while (current_position < loop_end && new_pos >= loop_end)
        {
          /* adjust the new frames */
          new_pos += loop_start - loop_end;
        }
    }

  return new_pos;
}

/**
 * @brief Interface for transport.
 */
class ITransport
{
public:
  enum class PlayState : std::uint8_t
  {
    RollRequested,
    Rolling,
    PauseRequested,
    Paused
  };

public:
  virtual ~ITransport () = default;

  /**
   * Returns the loop range positions in samples.
   */
  virtual std::pair<units::sample_t, units::sample_t>
  get_loop_range_positions () const noexcept [[clang::nonblocking]] = 0;

  /**
   * @brief Returns the punch recording range positions in samples.
   */
  virtual std::pair<units::sample_t, units::sample_t>
  get_punch_range_positions () const noexcept [[clang::nonblocking]] = 0;

  virtual PlayState get_play_state () const noexcept [[clang::nonblocking]] = 0;

  /**
   * @brief Get the playhead position.
   *
   * @return The position in samples.
   */
  virtual units::sample_t
  get_playhead_position_in_audio_thread () const noexcept
    [[clang::nonblocking]] = 0;

  /**
   * Gets the playhead position, similarly to @ref get_playhead_position(),
   * except that it adjusts the new position if the loop end point was crossed.
   *
   * @return The position in samples.
   */
  units::sample_t get_playhead_position_after_adding_frames_in_audio_thread (
    const units::sample_t current_playhead_position,
    const units::sample_t frames_to_add) const noexcept [[clang::nonblocking]]
  {
    const auto [loop_start, loop_end] = get_loop_range_positions ();
    return playhead_position_after_adding_frames (
      current_playhead_position, frames_to_add, loop_enabled (), loop_start,
      loop_end);
  }

  virtual bool loop_enabled () const noexcept [[clang::nonblocking]] = 0;

  virtual bool punch_enabled () const noexcept [[clang::nonblocking]] = 0;

  /**
   * @brief Returns whether recording is enabled.
   */
  virtual bool recording_enabled () const noexcept [[clang::nonblocking]] = 0;

  /**
   * @brief Frames remaining to preroll (playing back some time
   * earlier before actually recording/rolling).
   *
   * Preroll is a number of frames earlier to start at before the punch in
   * position during recording.
   */
  virtual units::sample_t recording_preroll_frames_remaining () const noexcept
    [[clang::nonblocking]] = 0;

  bool
  has_recording_preroll_frames_remaining () const noexcept [[clang::nonblocking]]
  {
    return recording_preroll_frames_remaining () > units::samples (0);
  }

  /**
   * @brief Frames remaining for metronome countin.
   *
   * @note Count-in happens while the playhead is not moving.
   */
  virtual units::sample_t metronome_countin_frames_remaining () const noexcept
    [[clang::nonblocking]] = 0;

  /**
   * Returns the number of processable frames until and excluding the loop end
   * point as a positive number (>= 1) if the loop point was met between
   * g_start_frames and (g_start_frames + nframes), otherwise returns 0;
   */
  units::sample_t is_loop_point_met_in_audio_thread (
    units::sample_t g_start_frames,
    units::sample_t nframes) const noexcept [[clang::nonblocking]]
  {
    auto [loop_start_pos, loop_end_pos] = get_loop_range_positions ();
    bool loop_end_between_start_and_end =
      (loop_end_pos > g_start_frames && loop_end_pos <= g_start_frames + nframes);

    if (loop_end_between_start_and_end && loop_enabled ()) [[unlikely]]
      {
        return loop_end_pos - g_start_frames;
      }
    return units::samples (0);
  }
};

} // namespace zrythm::dsp
