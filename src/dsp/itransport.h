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
  get_loop_range_positions () const = 0;

  /**
   * @brief Returns the punch recording range positions in samples.
   */
  virtual std::pair<unsigned_frame_t, unsigned_frame_t>
  get_punch_range_positions () const = 0;

  virtual PlayState get_play_state () const = 0;

  /**
   * @brief Get the playhead position.
   *
   * @return The position in samples.
   */
  virtual signed_frame_t get_playhead_position_in_audio_thread () const = 0;

  /**
   * Gets the playhead position, similarly to @ref get_playhead_position(),
   * except that it adjusts the new position if the loop end point was crossed.
   *
   * @return The position in samples.
   */
  virtual signed_frame_t
  get_playhead_position_after_adding_frames_in_audio_thread (
    signed_frame_t frames) const = 0;

  virtual bool loop_enabled () const = 0;

  virtual bool punch_enabled () const = 0;

  /**
   * @brief Returns whether recording is enabled.
   */
  virtual bool recording_enabled () const = 0;

  /**
   * @brief Whether we still have frames to preroll (playing back some time
   * earlier before actually recording).
   */
  virtual bool has_preroll_frames_remaining () const = 0;

  /**
   * Returns the number of processable frames until and excluding the loop end
   * point as a positive number (>= 1) if the loop point was met between
   * g_start_frames and (g_start_frames + nframes), otherwise returns 0;
   */
  virtual nframes_t is_loop_point_met_in_audio_thread (
    unsigned_frame_t g_start_frames,
    nframes_t        nframes) const = 0;
};

} // namespace zrythm::dsp
