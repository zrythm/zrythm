// SPDX-FileCopyrightText: © 2018-2021, 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Position struct and API.
 */

#ifndef __AUDIO_POSITION_H__
#define __AUDIO_POSITION_H__

#include "io/serialization/iserializable.h"
#include "utils/types.h"

/**
 * @addtogroup dsp
 *
 * @{
 */

constexpr int    TICKS_PER_QUARTER_NOTE = 960;
constexpr int    TICKS_PER_SIXTEENTH_NOTE = 240;
constexpr double TICKS_PER_QUARTER_NOTE_DBL = 960.0;
constexpr double TICKS_PER_SIXTEENTH_NOTE_DBL = 240.0;
constexpr double TICKS_PER_NINETYSIXTH_NOTE_DBL = 40.0;
constexpr int    POSITION_MAX_BAR = 160000;

class SnapGrid;
class Track;
class Region;

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
class Position final : public ISerializable<Position>
{
public:
  // Rule of 0
  Position () = default;

  /**
   * Parses a position from the given string.
   *
   * @throw ZrythmException if the string is invalid.
   */
  Position (const char * str);

  /** Construct from total number of ticks.*/
  Position (double ticks, double frames_per_tick = 0.0)
  {
    from_ticks (ticks, frames_per_tick);
  }

  /** Construct from total number of frames. */
  Position (signed_frame_t frames, double ticks_per_frame = 0.0)
  {
    from_frames (frames, ticks_per_frame);
  }

  void zero () { *this = Position (); }

  DECLARE_DEFINE_FIELDS_METHOD ();

  /** Getter */
  inline signed_frame_t get_total_frames () const { return frames_; }

  /** Getter */
  inline double get_total_ticks () const { return ticks_; }

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

  static int compare_frames_cmpfunc (const void * p1, const void * p2)
  {
    return compare_frames (*(const Position *) p1, *(const Position *) p2);
  }

  bool ticks_equal (const Position &other)
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

  inline bool is_positive () const { return frames_ >= 0 && ticks_ >= 0; }

  /**
   * Whether the position starts on or after @p f1 and before @p f2 (ie, the
   * position is between @p f1 and @p f2, exclusive of f2).
   */
  inline bool
  is_between_frames_excluding_2nd (signed_frame_t f1, signed_frame_t f2) const
  {
    return frames_ >= f1 && frames_ < f2;
  }

  /** Returns if the position is after or equal to @p start and before @p end. */
  inline bool
  is_between_excl_2nd (const Position &start, const Position &end) const
  {
    return is_between_frames_excluding_2nd (start.frames_, end.frames_);
  }

  /** Returns if the position is after or equal to @p start and before or equal
   * to @p end (ie, inclusive of @p end). */
  inline bool
  is_between_incl_2nd (const Position &start, const Position &end) const
  {
    return frames_ >= start.frames_ && frames_ <= end.frames_;
  }

  inline bool
  is_between_excl_both (const Position &start, const Position &end) const
  {
    return frames_ > start.frames_ && frames_ < end.frames_;
  }

  inline bool
  is_between_excl_1st_incl_2nd (const Position &start, const Position &end) const
  {
    return frames_ > start.frames_ && frames_ <= end.frames_;
  }

  /**
   * Sets position to given bar.
   */
  void set_to_bar (int bar);

  /**
   * Adds the frames to the position and updates the rest of the fields, and
   * makes sure the frames are still accurate.
   */
  inline void add_frames (signed_frame_t frames, double ticks_per_frame = 0.0)
  {
    frames_ += frames;
    update_ticks_from_frames (ticks_per_frame);
  }

  /**
   * Converts seconds to position and puts the result in the given Position.
   */
  void from_seconds (double secs);

  inline void
  from_frames (const signed_frame_t frames, double ticks_per_frame = 0.0)
  {
    frames_ = frames;
    update_ticks_from_frames (ticks_per_frame);
  }

  /**
   * Sets position to the given total tick count.
   */
  inline void from_ticks (double ticks, double frames_per_tick = 0.0)
  {
    ticks_ = ticks;
    update_frames_from_ticks (frames_per_tick);
  }

  void from_ms (const signed_ms_t ms)
  {
    *this = Position ();
    add_ms (ms);
  }

  void from_bars (int bars)
  {
    *this = Position ();
    add_bars (bars);
  }

  void add_bars (int bars);

  void add_beats (int beats);

  inline void add_sixteenths (int sixteenths)
  {
    add_ticks (sixteenths * TICKS_PER_SIXTEENTH_NOTE_DBL);
  }

  inline void add_ticks (double ticks)
  {
    ticks_ += ticks;
    update_frames_from_ticks (0.0);
  }

  /**
   * Returns the Position in milliseconds.
   */
  signed_ms_t to_ms () const;

  static signed_frame_t ms_to_frames (double ms);

  static double ms_to_ticks (double ms)
  {
    /* FIXME simplify - this is a roundabout way not suitable for realtime
     * calculations */
    const signed_frame_t frames = ms_to_frames (ms);
    Position             tmp;
    tmp.from_frames (frames);
    return tmp.ticks_;
  }

  inline void add_ms (double ms) { add_frames (ms_to_frames (ms)); }

  void add_minutes (int mins) { add_frames (ms_to_frames (mins * 60 * 1'000)); }

  void add_seconds (signed_sec_t seconds)
  {
    add_frames (ms_to_frames (seconds * 1'000));
  }

  /**
   * Snaps position using given options.
   *
   * @param start_pos The previous position (ie, the position the drag started
   * at. This is only used when the "keep offset" setting is on.
   * @param track Track, used when moving things in the timeline. If keep offset
   * is on and this is passed, the objects in the track will be taken into
   * account. If keep offset is on and this is nullptr, all applicable objects
   * will be taken into account. Not used if keep offset is off.
   * @param region Region, used when moving things in the editor. Same behavior
   * as @p track.
   * @param sg SnapGrid options.
   */
  void snap (
    const Position * start_pos,
    Track *          track,
    Region *         region,
    const SnapGrid  &sg);

  void snap_simple (const SnapGrid &sg)
  {
    snap (nullptr, nullptr, nullptr, sg);
  }

  /**
   * Sets the end position to be 1 snap point away from the start pos.
   *
   * FIXME rename to something more meaningful.
   *
   * @param start_pos Start Position.
   * @param end_pos End Position.
   * @param snap SnapGrid.
   */
  static void set_min_size (
    const Position &start_pos,
    Position       &end_pos,
    const SnapGrid &snap);

  /**
   * Updates ticks.
   *
   * @param ticks_per_frame If zero, AudioEngine.ticks_per_frame
   *   will be used instead.
   */
  ATTR_HOT void update_ticks_from_frames (double ticks_per_frame);

  /**
   * Converts ticks to frames.
   *
   * @param frames_per_tick If zero, AudioEngine.frames_per_tick
   *   will be used instead.
   */
  static signed_frame_t
  get_frames_from_ticks (double ticks, double frames_per_tick);

  /**
   * Updates frames.
   *
   * @param frames_per_tick If zero, AudioEngine.frames_per_tick
   *   will be used instead.
   */
  ATTR_HOT ATTR_NONNULL void update_frames_from_ticks (double frames_per_tick);

  /**
   * Updates the position from ticks or frames.
   *
   * @param from_ticks Whether to update the
   *   position based on ticks (true) or frames
   *   (false).
   * @param ratio Frames per tick when @ref from_ticks is true
   *   and ticks per frame when false.
   */
  inline void update (bool from_ticks, double ratio)
  {
    if (from_ticks)
      update_frames_from_ticks (ratio);
    else
      update_ticks_from_frames (ratio);
  }

  /**
   * @brief Sets the position to the midway point between the two given
   * positions.
   *
   * @param pos Position to set to.
   */
  inline void
  set_to_midway_pos (const Position &start_pos, const Position &end_pos)
  {
    ticks_ = start_pos.ticks_ + (end_pos.ticks_ - start_pos.ticks_) / 2.0;
    update_frames_from_ticks (0.0);
  }

  /**
   * Returns the difference in ticks between the two Position's, snapped based
   * on the given SnapGrid (if any).
   *
   * @param end_pos End position.
   * @param start_pos Start Position.
   * @param sg SnapGrid to snap with, or NULL to not snap.
   */
  static double get_ticks_diff (
    const Position  &end_pos,
    const Position  &start_pos,
    const SnapGrid * sg);

  /**
   * Creates a string in the form of "0.0.0.0" from the given position.
   */
  std::string to_string (int decimal_places = 4) const;

  ATTR_NONNULL void to_string (char * buf, int decimal_places = 4) const;

  /**
   * Prints the Position in the "0.0.0.0" form.
   */
  void print () const;

  static void print_range (const Position &p1, const Position &p2);

  /**
   * Returns the total number of beats.
   *
   * @param include_current Whether to count the current beat if it is at the
   * beat start.
   */
  int get_total_bars (bool include_current) const;

  /**
   * Returns the total number of beats.
   *
   * @param include_current Whether to count the current beat if it is at the
   * beat start.
   */
  int get_total_beats (bool include_current) const;

  /**
   * Returns the total number of sixteenths not including the current one.
   */
  int get_total_sixteenths (bool include_current) const;

  /**
   * @brief Changes the sign of the position.
   *
   * For example, 4.2.1.21 would become -4.2.1.21.
   */
  inline void change_sign ()
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
  int get_bars (bool start_at_one) const;

  /**
   * Gets the beats of the position.
   *
   * Eg, if the position is equivalent to 4.1.2.42, this will return 1.
   *
   * @param start_at_one Start at 1 or -1 instead of 0.
   */
  int get_beats (bool start_at_one) const;

  /**
   * Gets the sixteenths of the position.
   *
   * Eg, if the position is equivalent to 4.1.2.42, this will return 2.
   *
   * @param start_at_one Start at 1 or -1 instead of 0.
   */
  int get_sixteenths (bool start_at_one) const;

  /**
   * Gets the ticks of the position.
   *
   * Ie, if the position is equivalent to 4.1.2.42, this will return 42.
   */
  double get_ticks () const;

  bool validate () const;

private:
  /**
   * @brief Returns the closest position.
   *
   * @param p1 Snap point 1.
   * @param p2 Snap point 2.
   */
  inline Position &get_closest_position (Position &p1, Position &p2) const
  {
    if (ticks_ - p1.ticks_ <= p2.ticks_ - ticks_)
      {
        return p1;
      }
    else
      {
        return p2;
      }
  }

  /**
   * @brief Get closest snap point.
   *
   * @param track Track, used when moving things in the timeline. If keep offset
   * is on and this is passed, the objects in the track will be taken into
   * account. If keep offset is on and this is nullptr, all applicable objects
   * will be taken into account. Not used if keep offset is off.
   * @param region Region, used when moving things in the editor. Same behavior
   * as @ref track.
   * @param sg SnapGrid options.
   * @param closest_sp Position to set.
   *
   * @return Whether a snap point was found or not.
   */
  bool get_closest_snap_point (
    Track *         track,
    Region *        region,
    const SnapGrid &sg,
    Position       &closest_sp) const;

  /**
   * @brief Get next snap point.
   *
   * Must only be called on positive positions.
   *
   * @param track Track, used when moving things in the timeline. If keep offset
   * is on and this is passed, the objects in the track will be taken into
   * account. If keep offset is on and this is nullptr, all applicable objects
   * will be taken into account. Not used if keep offset is off.
   * @param region Region, used when moving things in the editor. Same behavior
   * as @ref track.
   * @param sg SnapGrid options.
   * @param next_snap_point Position to set.
   *
   * @return Whether a snap point was found or not.
   */
  ATTR_HOT bool get_next_snap_point (
    Track *         track,
    Region *        region,
    const SnapGrid &sg,
    Position       &next_sp) const;

  /**
   * Gets the previous snap point.
   *
   * Must only be called on positive positions.
   *
   * TODO check what happens if there is no snap point & improve this API
   *
   * @param track Track, used when moving things in the timeline. If keep offset
   * is on and this is passed, the objects in the track will be taken into
   * account. If keep offset is on and this is nullptr, all applicable objects
   * will be taken into account. Not used if keep offset is off.
   * @param region Region, used when moving things in the editor. Same behavior
   * as @p track.
   * @param sg SnapGrid options.
   * @param prev_snap_point The position to set.
   *
   * @return Whether a snap point was found or not.
   */
  ATTR_HOT bool get_prev_snap_point (
    Track *         track,
    Region *        region,
    const SnapGrid &sg,
    Position       &prev_sp) const;

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

/** Note: only checks frames.*/
inline auto
operator<=> (const Position &lhs, const Position &rhs)
{
  return lhs.frames_ <=> rhs.frames_;
}

inline bool
operator== (const Position &lhs, const Position &rhs)
{
  return (lhs <=> rhs) == 0;
}

// Other comparison operators are automatically generated by the spaceship
// operator

DEFINE_OBJECT_FORMATTER (Position, [] (const Position &obj) {
  return obj.to_string ();
});

/**
 * @}
 */

#endif
