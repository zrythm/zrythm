// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "utils/types.h"

namespace zrythm::dsp
{
/**
 * @brief Interface for transport.
 */
class ITransport
{
public:
  enum class PlayState
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
  virtual std::pair<unsigned_frame_t, unsigned_frame_t>
  get_loop_range_positions () const noexcept [[clang::nonblocking]] = 0;

  /**
   * @brief Returns the punch recording range positions in samples.
   */
  virtual std::pair<unsigned_frame_t, unsigned_frame_t>
  get_punch_range_positions () const noexcept [[clang::nonblocking]] = 0;

  virtual PlayState get_play_state () const noexcept [[clang::nonblocking]] = 0;

  /**
   * @brief Get the playhead position.
   *
   * @return The position in samples.
   */
  virtual signed_frame_t get_playhead_position_in_audio_thread () const noexcept
    [[clang::nonblocking]] = 0;

  /**
   * Gets the playhead position, similarly to @ref get_playhead_position(),
   * except that it adjusts the new position if the loop end point was crossed.
   *
   * @return The position in samples.
   */
  signed_frame_t get_playhead_position_after_adding_frames_in_audio_thread (
    const signed_frame_t current_playhead_position,
    const signed_frame_t frames_to_add) const noexcept [[clang::nonblocking]]
  {
    auto new_pos = current_playhead_position + frames_to_add;

    /* if start frames were before the loop-end point and the new frames are
     * after (loop crossed) */
    if (loop_enabled ())
      {
        const auto loop_points = get_loop_range_positions ();
        const auto loop_start = static_cast<signed_frame_t> (loop_points.first);
        const auto loop_end = static_cast<signed_frame_t> (loop_points.second);
        if (current_playhead_position < loop_end && new_pos >= loop_end)
          {
            /* adjust the new frames */
            new_pos += loop_start - loop_end;
            assert (new_pos < loop_end);
          }
      }

    return new_pos;
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
  virtual unsigned_frame_t recording_preroll_frames_remaining () const noexcept
    [[clang::nonblocking]] = 0;

  bool
  has_recording_preroll_frames_remaining () const noexcept [[clang::nonblocking]]
  {
    return recording_preroll_frames_remaining () > 0;
  }

  /**
   * @brief Frames remaining for metronome countin.
   *
   * @note Count-in happens while the playhead is not moving.
   */
  virtual unsigned_frame_t metronome_countin_frames_remaining () const noexcept
    [[clang::nonblocking]] = 0;

  /**
   * Returns the number of processable frames until and excluding the loop end
   * point as a positive number (>= 1) if the loop point was met between
   * g_start_frames and (g_start_frames + nframes), otherwise returns 0;
   */
  virtual nframes_t is_loop_point_met_in_audio_thread (
    unsigned_frame_t g_start_frames,
    nframes_t        nframes) const noexcept [[clang::nonblocking]] = 0;
};

} // namespace zrythm::dsp
