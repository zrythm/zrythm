// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <regex>

#include "dsp/position.h"
#include "utils/exceptions.h"
#include "utils/gtest_wrapper.h"
#include "utils/math.h"

#include <fmt/format.h>

namespace zrythm::dsp
{

Position::Position (
  const char *  str,
  int           beats_per_bar,
  int           sixteenths_per_beat,
  FramesPerTick frames_per_tick)
{
  static const std::regex position_regex (
    R"((-?\d+)\.(\d+)\.(\d+)\.(\d+(?:\.\d+)?))");
  std::cmatch matches;

  if (!std::regex_match (str, matches, position_regex))
    {
      throw ZrythmException ("Invalid position string");
    }

  // Get sign from the bars component
  bool is_negative = matches[1].str ().starts_with ('-');

  // Convert string components to numbers (1-based to 0-based)
  int    bars = std::abs (std::stoi (matches[1].str ())) - 1;
  int    beats = std::stoi (matches[2].str ()) - 1;
  int    sixteenths = std::stoi (matches[3].str ()) - 1;
  double ticks = std::stod (matches[4].str ());
  if (bars == -1 || beats == -1 || sixteenths == -1)
    {
      throw ZrythmException (
        "Invalid position string - no zeros allowed for bars, beats, and sixteenths");
    }

  // Calculate total ticks
  double total_ticks =
    (double) (bars * beats_per_bar * sixteenths_per_beat)
      * TICKS_PER_SIXTEENTH_NOTE_DBL
    + (double) (beats * sixteenths_per_beat) * TICKS_PER_SIXTEENTH_NOTE_DBL
    + (double) sixteenths * TICKS_PER_SIXTEENTH_NOTE_DBL + ticks;

  // Apply sign
  if (is_negative)
    {
      total_ticks = -total_ticks;
    }

  from_ticks (total_ticks, frames_per_tick);
}

void
Position::update_ticks_from_frames (TicksPerFrame ticks_per_frame)
{
  z_return_if_fail (type_safe::get (ticks_per_frame) > 0);
  ticks_ = (double) frames_ * type_safe::get (ticks_per_frame);
}

signed_frame_t
Position::get_frames_from_ticks (double ticks, dsp::FramesPerTick frames_per_tick)
{
  assert (type_safe::get (frames_per_tick) > 0);
  return utils::math::round_to_signed_frame_t (
    (ticks * type_safe::get (frames_per_tick)));
}

void
Position::update_frames_from_ticks (FramesPerTick frames_per_tick)
{
  frames_ = get_frames_from_ticks (ticks_, frames_per_tick);

  /* assert that no overflow occurred */
  if (ticks_ >= 0)
    {
      assert (frames_ >= 0);
    }
  else
    {
      assert (frames_ <= 0);
    }
}

void
Position::
  set_to_bar (int bar, int ticks_per_bar, dsp::FramesPerTick frames_per_tick)
{
  z_return_if_fail (
    ticks_per_bar > 0
    /* don't use INT_MAX, it results in a negative position */
    && bar <= POSITION_MAX_BAR);

  *this = Position ();

  if (bar > 0)
    {
      bar--;
      from_ticks (ticks_per_bar * bar, frames_per_tick);
    }
  else if (bar < 0)
    {
      bar++;
      from_ticks (ticks_per_bar * bar, frames_per_tick);
    }
  else
    {
      z_return_if_reached ();
    }
}

signed_ms_t
Position::to_ms (sample_rate_t sample_rate) const
{
  if (frames_ == 0)
    {
      return 0;
    }
  return utils::math::round_to_signed_frame_t (
    (1000.0 * (double) frames_) / ((double) sample_rate));
}

signed_frame_t
Position::ms_to_frames (double ms, sample_rate_t sample_rate)
{
  return utils::math::round_to_signed_frame_t (
    (ms / 1000.0) * (double) sample_rate);
}

void
Position::from_seconds (
  double        secs,
  sample_rate_t sample_rate,
  TicksPerFrame ticks_per_frame)
{
  from_frames (
    juce::roundToInt (secs * static_cast<double> (sample_rate)),
    ticks_per_frame);
}

void
Position::
  add_bars (int bars, int ticks_per_bar, dsp::FramesPerTick frames_per_tick)
{
  z_warn_if_fail (ticks_per_bar > 0);
  add_ticks (bars * ticks_per_bar, frames_per_tick);
}

void
Position::
  add_beats (int beats, int ticks_per_beat, dsp::FramesPerTick frames_per_tick)
{
  if (ticks_per_beat <= 0)
    {
      throw std::invalid_argument ("ticks_per_beat must be > 0");
    }
  add_ticks (beats * ticks_per_beat, frames_per_tick);
}

std::string
Position::to_string (
  int           beats_per_bar,
  int           sixteenths_per_beat,
  FramesPerTick frames_per_tick,
  int           decimal_places) const
{
  char buf[80];
  to_string (
    beats_per_bar, sixteenths_per_beat, frames_per_tick, buf, decimal_places);
  return buf;
}

void
Position::to_string (
  int           beats_per_bar,
  int           sixteenths_per_beat,
  FramesPerTick frames_per_tick,
  char *        buf,
  int           decimal_places) const
{
  int bars = get_bars (
    true, beats_per_bar * sixteenths_per_beat * TICKS_PER_SIXTEENTH_NOTE);
  int beats = get_beats (
    true, beats_per_bar, TICKS_PER_SIXTEENTH_NOTE * sixteenths_per_beat);
  int sixteenths =
    get_sixteenths (true, beats_per_bar, sixteenths_per_beat, frames_per_tick);
  double ticks = get_ticks_part (frames_per_tick);
  z_return_if_fail (bars > -80000);
  if (ZRYTHM_TESTING)
    {
      sprintf (
        buf, "%d.%d.%d.%03.*f (%" SIGNED_FRAME_FORMAT ")", bars, abs (beats),
        abs (sixteenths), decimal_places, fabs (ticks), frames_);
    }
  else
    {
      sprintf (
        buf, "%d.%d.%d.%03.*f", bars, abs (beats), abs (sixteenths),
        decimal_places, fabs (ticks));
    }
}

void
Position::print (
  int           beats_per_bar,
  int           sixteenths_per_beat,
  FramesPerTick frames_per_tick) const
{
  z_debug (fmt::format (
    "{} ({} frames | {} ticks)",
    to_string (beats_per_bar, sixteenths_per_beat, frames_per_tick), frames_,
    ticks_));
}

void
Position::print_range (
  int             beats_per_bar,
  int             sixteenths_per_beat,
  FramesPerTick   frames_per_tick,
  const Position &p1,
  const Position &p2)
{
  z_debug (fmt::format (
    "{} ({}) - {} ({}) <delta {} frames {} ticks>",
    p1.to_string (beats_per_bar, sixteenths_per_beat, frames_per_tick),
    p1.frames_,
    p2.to_string (beats_per_bar, sixteenths_per_beat, frames_per_tick),
    p2.frames_, p2.frames_ - p1.frames_, p2.ticks_ - p1.ticks_));
}

int
Position::get_total_bars (
  bool          include_current,
  int           ticks_per_bar,
  FramesPerTick frames_per_tick) const
{
  int bars = get_bars (false, ticks_per_bar);
  int cur_bars = get_bars (true, ticks_per_bar);

  if (include_current || bars == 0)
    {
      return bars;
    }

  /* if we are at the start of the bar, don't count this bar */
  Position pos_at_bar;
  pos_at_bar.set_to_bar (cur_bars, ticks_per_bar, frames_per_tick);
  if (pos_at_bar.frames_ == frames_)
    {
      bars--;
    }

  return bars;
}

int
Position::get_total_beats (
  bool          include_current,
  int           beats_per_bar,
  int           ticks_per_beat,
  FramesPerTick frames_per_tick) const
{
  int beats = get_beats (false, beats_per_bar, ticks_per_beat);
  int bars = get_bars (false, ticks_per_beat * beats_per_bar);

  int ret = beats + bars * beats_per_bar;

  if (include_current || ret == 0)
    {
      return ret;
    }

  Position tmp;
  tmp.from_ticks ((double) ret * ticks_per_beat, frames_per_tick);
  if (tmp.frames_ == frames_)
    {
      ret--;
    }

  return ret;
}

int
Position::get_total_sixteenths (
  bool          include_current,
  FramesPerTick frames_per_tick) const
{
  int ret;
  if (ticks_ >= 0)
    {
      ret = (int) utils::math::round_to_signed_32 (
        floor (ticks_ / TICKS_PER_SIXTEENTH_NOTE_DBL));
    }
  else
    {
      ret = (int) utils::math::round_to_signed_32 (
        ceil (ticks_ / TICKS_PER_SIXTEENTH_NOTE_DBL));
    }

  if (include_current || ret == 0)
    {
      return ret;
    }

  Position tmp ((double) ret * TICKS_PER_SIXTEENTH_NOTE_DBL, frames_per_tick);
  if (tmp.frames_ == frames_)
    {
      ret--;
    }

  return ret;
}

int
Position::get_bars (bool start_at_one, int ticks_per_bar) const
{
  z_return_val_if_fail (ticks_per_bar > 0, -1);

  double total_bars = ticks_ / ticks_per_bar;
  if (total_bars >= 0.0)
    {
      int ret = (int) floor (total_bars);
      if (start_at_one)
        {
          ret++;
        }
      return ret;
    }
  else
    {
      int ret = (int) ceil (total_bars);
      if (start_at_one)
        {
          ret--;
        }
      return ret;
    }
}

int
Position::get_beats (bool start_at_one, int beats_per_bar, int ticks_per_beat)
  const
{
  z_return_val_if_fail (ticks_per_beat > 0, -1);
  z_return_val_if_fail (beats_per_bar > 0, -1);

  auto   total_bars = (double) get_bars (false, ticks_per_beat * beats_per_bar);
  double total_beats = ticks_ / ticks_per_beat;
  total_beats -= (double) (total_bars * beats_per_bar);
  if (total_beats >= 0.0)
    {
      int ret = (int) floor (total_beats);
      if (start_at_one)
        {
          ret++;
        }
      return ret;
    }
  else
    {
      int ret = (int) ceil (total_beats);
      if (start_at_one)
        {
          ret--;
        }
      return ret;
    }
}

int
Position::get_sixteenths (
  bool          start_at_one,
  int           beats_per_bar,
  int           sixteenths_per_beat,
  FramesPerTick frames_per_tick) const
{
  z_return_val_if_fail (sixteenths_per_beat > 0, -1);

  auto total_beats = (double) get_total_beats (
    true, beats_per_bar,
    utils::math::round_to_signed_64 (
      TICKS_PER_SIXTEENTH_NOTE_DBL * (double) sixteenths_per_beat),
    frames_per_tick);
  /*z_info ("total beats {:f}", total_beats);*/
  double total_sixteenths = ticks_ / TICKS_PER_SIXTEENTH_NOTE_DBL;
  /*z_info ("total sixteenths {:f}",*/
  /*total_sixteenths);*/
  total_sixteenths -= (double) (total_beats * sixteenths_per_beat);
  if (total_sixteenths >= 0.0)
    {
      int ret = (int) floor (total_sixteenths);
      if (start_at_one)
        {
          ret++;
        }
      return ret;
    }
  else
    {
      int ret = (int) ceil (total_sixteenths);
      if (start_at_one)
        {
          ret--;
        }
      return ret;
    }
}

double
Position::get_ticks_part (FramesPerTick frames_per_tick) const
{
  double total_sixteenths = get_total_sixteenths (true, frames_per_tick);
  /*z_debug ("total sixteenths {:f}", total_sixteenths);*/
  return ticks_ - (total_sixteenths * TICKS_PER_SIXTEENTH_NOTE_DBL);
}

bool
Position::validate () const
{
  return true;
}

void
dsp::Position::define_fields (const Context &ctx)
{
  serialize_fields (
    ctx, make_field ("ticks", ticks_), make_field ("frames", frames_));
}

}; // namespace zrythm::dsp
