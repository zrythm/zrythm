// SPDX-FileCopyrightText: © 2018-2021, 2023-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef ZRYTHM_COMMON_DSP_POSITION_H
#define ZRYTHM_COMMON_DSP_POSITION_H

#include "utils/serialization.h"
#include "utils/types.h"

namespace zrythm::dsp
{

struct FramesPerTick
    : type_safe::strong_typedef<FramesPerTick, double>,
      type_safe::strong_typedef_op::equality_comparison<FramesPerTick>,
      type_safe::strong_typedef_op::relational_comparison<FramesPerTick>
{
  using FramesPerTickTag = void; // used by the fmt formatter below
  using type_safe::strong_typedef<FramesPerTick, double>::strong_typedef;

  explicit FramesPerTick () = default;

  static_assert (StrongTypedef<FramesPerTick>);
};
static_assert (std::regular<FramesPerTick>);

struct TicksPerFrame
    : type_safe::strong_typedef<TicksPerFrame, double>,
      type_safe::strong_typedef_op::equality_comparison<TicksPerFrame>,
      type_safe::strong_typedef_op::relational_comparison<TicksPerFrame>
{
  using TicksPerFrameTag = void; // used by the fmt formatter below
  using type_safe::strong_typedef<TicksPerFrame, double>::strong_typedef;

  explicit TicksPerFrame () = default;

  static_assert (StrongTypedef<TicksPerFrame>);
};
static_assert (std::regular<TicksPerFrame>);

static constexpr TicksPerFrame
to_ticks_per_frame (const FramesPerTick &frames_per_tick)
{
  return TicksPerFrame (1.0 / type_safe::get (frames_per_tick));
}
static constexpr FramesPerTick
to_frames_per_tick (const TicksPerFrame &ticks_per_frame)
{
  return FramesPerTick (1.0 / type_safe::get (ticks_per_frame));
}

/**
 * @brief Represents the position of an object.
 *
 * This class provides various methods for working with positions, including
 * converting between ticks, frames, and other time units, as well as snapping
 * positions to a grid.
 *
 * The position is stored as both a precise number of ticks and a number of
 * frames (samples). Most calculations should use the frames, while the ticks
 * are used for more precise positioning.
 *
 * The class also provides various comparison operators and utility functions
 * for working with positions.
 */
class Position
{
public:
  static constexpr int    TICKS_PER_QUARTER_NOTE = 960;
  static constexpr int    TICKS_PER_SIXTEENTH_NOTE = 240;
  static constexpr double TICKS_PER_QUARTER_NOTE_DBL = 960.0;
  static constexpr double TICKS_PER_SIXTEENTH_NOTE_DBL = 240.0;
  static constexpr double TICKS_PER_NINETYSIXTH_NOTE_DBL = 40.0;
  static constexpr int    POSITION_MAX_BAR = 160000;

public:
  // Rule of 0
  Position () = default;

  /**
   * Parses a position from the given string.
   *
   * @throw ZrythmException if the string is invalid.
   */
  Position (
    const char *  str,
    int           beats_per_bar,
    int           sixteenths_per_beat,
    FramesPerTick frames_per_tick);

  /** Construct from total number of ticks.*/
  Position (double ticks, dsp::FramesPerTick frames_per_tick)
  {
    from_ticks (ticks, frames_per_tick);
  }

  /** Construct from total number of frames. */
  Position (signed_frame_t frames, TicksPerFrame ticks_per_frame)
  {
    from_frames (frames, ticks_per_frame);
  }

  void zero ()
  {
    ticks_ = 0.0;
    frames_ = 0;
  }

  /** Getter */
  signed_frame_t get_total_frames () const { return frames_; }

  /** Getter */
  double get_total_ticks () const { return ticks_; }

  /**
   * Compares 2 positions based on their frames.
   *
   * negative = p1 is earlier
   * 0 = equal
   * positive = p2 is earlier
   */
  static signed_frame_t compare_frames (const Position &p1, const Position &p2)
  {
    return p1.frames_ - p2.frames_;
  }

  bool ticks_equal (const Position &other) const
  {
    return fabs (ticks_ - other.ticks_) <= DBL_EPSILON;
  }

  /** Returns minimum of p1 and p2 */
  static const Position &get_min (const Position &p1, const Position &p2)
  {
    return compare_frames (p1, p2) < 0 ? p1 : p2;
  }

  /** Returns maximum of p1 and p2 */
  static const Position &get_max (const Position &p1, const Position &p2)
  {
    return compare_frames (p1, p2) > 0 ? p1 : p2;
  }

  bool is_positive () const { return frames_ >= 0 && ticks_ >= 0; }

  /**
   * Whether the position starts on or after @p f1 and before @p f2 (ie, the
   * position is between @p f1 and @p f2, exclusive of f2).
   */
  bool
  is_between_frames_excluding_2nd (signed_frame_t f1, signed_frame_t f2) const
  {
    return frames_ >= f1 && frames_ < f2;
  }

  /** Returns if the position is after or equal to @p start and before @p end. */
  bool is_between_excl_2nd (const Position &start, const Position &end) const
  {
    return is_between_frames_excluding_2nd (start.frames_, end.frames_);
  }

  /** Returns if the position is after or equal to @p start and before or equal
   * to @p end (ie, inclusive of @p end). */
  bool is_between_incl_2nd (const Position &start, const Position &end) const
  {
    return frames_ >= start.frames_ && frames_ <= end.frames_;
  }

  bool is_between_excl_both (const Position &start, const Position &end) const
  {
    return frames_ > start.frames_ && frames_ < end.frames_;
  }

  bool
  is_between_excl_1st_incl_2nd (const Position &start, const Position &end) const
  {
    return frames_ > start.frames_ && frames_ <= end.frames_;
  }

  /**
   * Sets position to given bar.
   */
  void
  set_to_bar (int bar, int ticks_per_bar, dsp::FramesPerTick frames_per_tick);

  /**
   * Adds the frames to the position and updates the rest of the fields, and
   * makes sure the frames are still accurate.
   */
  void add_frames (signed_frame_t frames, TicksPerFrame ticks_per_frame)
  {
    frames_ += frames;
    update_ticks_from_frames (ticks_per_frame);
  }

  /**
   * Converts seconds to position and puts the result in the given Position.
   */
  void from_seconds (
    double        secs,
    sample_rate_t sample_rate,
    TicksPerFrame ticks_per_frame);

  void from_frames (const signed_frame_t frames, TicksPerFrame ticks_per_frame)
  {
    frames_ = frames;
    update_ticks_from_frames (ticks_per_frame);
  }

  /**
   * Sets position to the given total tick count.
   */
  void from_ticks (double ticks, dsp::FramesPerTick frames_per_tick)
  {
    ticks_ = ticks;
    update_frames_from_ticks (frames_per_tick);
  }

  void from_ms (
    const double  ms,
    sample_rate_t sample_rate,
    TicksPerFrame ticks_per_frame)
  {
    zero ();
    add_ms (ms, sample_rate, ticks_per_frame);
  }

  void
  from_bars (int bars, int ticks_per_bar, dsp::FramesPerTick frames_per_tick)
  {
    zero ();
    add_bars (bars, ticks_per_bar, frames_per_tick);
  }

  void
  add_bars (int bars, int ticks_per_bar, dsp::FramesPerTick frames_per_tick);

  void
  add_beats (int beats, int ticks_per_beat, dsp::FramesPerTick frames_per_tick);

  void add_sixteenths (int sixteenths, dsp::FramesPerTick frames_per_tick)
  {
    add_ticks (sixteenths * TICKS_PER_SIXTEENTH_NOTE_DBL, frames_per_tick);
  }

  void add_ticks (double ticks, dsp::FramesPerTick frames_per_tick)
  {
    ticks_ += ticks;
    update_frames_from_ticks (frames_per_tick);
  }

  /**
   * Returns the Position in milliseconds.
   */
  signed_ms_t to_ms (sample_rate_t sample_rate) const;

  static signed_frame_t ms_to_frames (double ms, sample_rate_t sample_rate);

  static double
  ms_to_ticks (double ms, sample_rate_t sample_rate, TicksPerFrame ticks_per_frame)
  {
    /* FIXME simplify - this is a roundabout way not suitable for realtime
     * calculations */
    const signed_frame_t frames = ms_to_frames (ms, sample_rate);
    Position             tmp;
    tmp.from_frames (frames, ticks_per_frame);
    return tmp.ticks_;
  }

  void
  add_ms (double ms, sample_rate_t sample_rate, TicksPerFrame ticks_per_frame)
  {
    add_frames (ms_to_frames (ms, sample_rate), ticks_per_frame);
  }

  void
  add_minutes (int mins, sample_rate_t sample_rate, TicksPerFrame ticks_per_frame)
  {
    add_frames (ms_to_frames (mins * 60 * 1'000, sample_rate), ticks_per_frame);
  }

  void add_seconds (
    signed_sec_t  seconds,
    sample_rate_t sample_rate,
    TicksPerFrame ticks_per_frame)
  {
    add_frames (
      ms_to_frames (static_cast<double> (seconds * 1'000), sample_rate),
      ticks_per_frame);
  }

  /**
   * Updates ticks.
   *
   * @param ticks_per_frame If zero, AudioEngine.ticks_per_frame
   *   will be used instead.
   */
  [[gnu::hot]] void update_ticks_from_frames (TicksPerFrame ticks_per_frame);

  /**
   * Converts ticks to frames.
   *
   * @param frames_per_tick If zero, AudioEngine.frames_per_tick
   *   will be used instead.
   */
  static signed_frame_t
  get_frames_from_ticks (double ticks, dsp::FramesPerTick frames_per_tick);

  /**
   * Updates frames.
   */
  [[gnu::hot]] void update_frames_from_ticks (FramesPerTick frames_per_tick);

  /**
   * @brief Sets the position to the midway point between the two given
   * positions.
   *
   * @param pos Position to set to.
   */
  void set_to_midway_pos (
    const Position &start_pos,
    const Position &end_pos,
    FramesPerTick   frames_per_tick)
  {
    ticks_ = start_pos.ticks_ + (end_pos.ticks_ - start_pos.ticks_) / 2.0;
    update_frames_from_ticks (frames_per_tick);
  }

  /**
   * Creates a string in the form of "0.0.0.0" from the given position.
   */
  std::string to_string (
    int           beats_per_bar,
    int           sixteenths_per_beat,
    FramesPerTick frames_per_tick,
    int           decimal_places = 4) const;

  [[gnu::nonnull]] void to_string (
    int           beats_per_bar,
    int           sixteenths_per_beat,
    FramesPerTick frames_per_tick,
    char *        buf,
    int           decimal_places = 4) const;

  /**
   * Prints the Position in the "0.0.0.0" form.
   */
  void print (
    int           beats_per_bar,
    int           sixteenths_per_beat,
    FramesPerTick frames_per_tick) const;

  static void print_range (
    int             beats_per_bar,
    int             sixteenths_per_beat,
    FramesPerTick   frames_per_tick,
    const Position &p1,
    const Position &p2);

  /**
   * Returns the total number of beats.
   *
   * @param include_current Whether to count the current beat if it is at the
   * beat start.
   */
  int get_total_bars (
    bool          include_current,
    int           ticks_per_bar,
    FramesPerTick frames_per_tick) const;

  /**
   * Returns the total number of beats.
   *
   * @param include_current Whether to count the current beat if it is at the
   * beat start.
   */
  int get_total_beats (
    bool          include_current,
    int           beats_per_bar,
    int           ticks_per_beat,
    FramesPerTick frames_per_tick) const;

  /**
   * Returns the total number of sixteenths not including the current one.
   */
  int
  get_total_sixteenths (bool include_current, dsp::FramesPerTick frames_per_tick)
    const;

  /**
   * @brief Changes the sign of the position.
   *
   * For example, 4.2.1.21 would become -4.2.1.21.
   */
  void change_sign ()
  {
    ticks_ = -ticks_;
    frames_ = -frames_;
  }

  /**
   * @brief Gets the bars of the position.
   *
   * Eg, if the position is equivalent to 4.1.2.42, this will return 4.
   *
   * @param start_at_one Start at 1 or -1 instead of 0.
   */
  int get_bars (bool start_at_one, int ticks_per_bar) const;

  /**
   * Gets the beats of the position.
   *
   * Eg, if the position is equivalent to 4.1.2.42, this will return 1.
   *
   * @param start_at_one Start at 1 or -1 instead of 0.
   */
  int
  get_beats (bool start_at_one, int beats_per_bar, int ticks_per_beat) const;

  /**
   * Gets the sixteenths of the position.
   *
   * Eg, if the position is equivalent to 4.1.2.42, this will return 2.
   *
   * @param start_at_one Start at 1 or -1 instead of 0.
   */
  int get_sixteenths (
    bool          start_at_one,
    int           beats_per_bar,
    int           sixteenths_per_beat,
    FramesPerTick frames_per_tick) const;

  /**
   * Gets the ticks of the position.
   *
   * Ie, if the position is equivalent to 4.1.2.42, this will return 42.
   */
  double get_ticks_part (FramesPerTick frames_per_tick) const;

  void set_to_position (const Position &pos)
  {
    ticks_ = pos.ticks_;
    frames_ = pos.frames_;
  }

  bool validate () const;

  /**
   * @brief Returns the closest position.
   *
   * @param p1 Snap point 1.
   * @param p2 Snap point 2.
   */
  Position &get_closest_position (Position &p1, Position &p2) const
  {
    if (ticks_ - p1.ticks_ <= p2.ticks_ - ticks_)
      {
        return p1;
      }

    return p2;
  }

  /** Note: only checks frames.*/
  friend auto operator<=> (const Position &lhs, const Position &rhs)
  {
    return lhs.frames_ <=> rhs.frames_;
  }

  friend bool operator== (const Position &lhs, const Position &rhs)
  {
    return (lhs <=> rhs) == 0;
  }

private:
  NLOHMANN_DEFINE_TYPE_INTRUSIVE (Position, ticks_, frames_)

public:
  /** Precise total number of ticks. */
  double ticks_ = 0.0;

  /**
   * Position in frames (samples).
   *
   * This should be used in most calculations.
   */
  signed_frame_t frames_ = 0;

  /**
   * Precise position in frames (samples).
   *
   * To be used where more precision than Position.frames is needed.
   *
   * @note Does not seem needed, keep comment around for reference in the
   * future.
   */
  // double precise_frames;
};

// Other comparison operators are automatically generated by the spaceship
// operator

}; // namespace zynth::dsp

DEFINE_OBJECT_FORMATTER (zrythm::dsp::Position, Position, [] (const auto &obj) {
  return fmt::format ("{:.3f} ticks ({} frames)", obj.ticks_, obj.frames_);
});

#endif
