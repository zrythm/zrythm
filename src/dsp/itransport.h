#pragma once

#include "dsp/position.h"

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
   * Adds frames to the given position similar to @ref Position.add_frames,
   * except that it adjusts the new Position if the loop end point was crossed.
   */
  virtual void
  position_add_frames (Position &pos, signed_frame_t frames) const = 0;

  /**
   * Returns the loop range positions.
   */
  virtual std::pair<Position, Position> get_loop_range_positions () const = 0;

  virtual PlayState get_play_state () const = 0;

  virtual Position get_playhead_position () const = 0;

  virtual bool get_loop_enabled () const = 0;

  /**
   * Returns the number of processable frames until and excluding the loop end
   * point as a positive number (>= 1) if the loop point was met between
   * g_start_frames and (g_start_frames + nframes), otherwise returns 0;
   */
  [[nodiscard]] ATTR_HOT inline nframes_t
  is_loop_point_met (const signed_frame_t g_start_frames, const nframes_t nframes)
  {
    auto [loop_start_pos, loop_end_pos] = get_loop_range_positions ();
    bool loop_end_between_start_and_end =
      (loop_end_pos.frames_ > g_start_frames
       && loop_end_pos.frames_ <= g_start_frames + (long) nframes);

    if (loop_end_between_start_and_end && get_loop_enabled ()) [[unlikely]]
      {
        return (nframes_t) (loop_end_pos.frames_ - g_start_frames);
      }
    return 0;
  }
};

} // namespace zrythm::dsp
