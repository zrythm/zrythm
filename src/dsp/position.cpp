// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/engine.h"
#include "dsp/position.h"
#include "dsp/snap_grid.h"
#include "dsp/tempo_track.h"
#include "dsp/transport.h"
#include "project.h"
#include "utils/exceptions.h"
#include "utils/math.h"
#include "zrythm.h"

#include "gtk_wrapper.h"
#include <fmt/format.h>

void
Position::update_ticks_from_frames (double ticks_per_frame)
{
  if (math_doubles_equal (ticks_per_frame, 0.0))
    {
      ticks_per_frame = AUDIO_ENGINE->ticks_per_frame_;
    }
  g_return_if_fail (ticks_per_frame > 0);
  ticks_ = (double) frames_ * ticks_per_frame;
}

/**
 * Converts ticks to frames.
 */
signed_frame_t
Position::get_frames_from_ticks (double ticks, double frames_per_tick)
{
  if (math_doubles_equal (frames_per_tick, 0.0))
    {
      frames_per_tick = AUDIO_ENGINE->frames_per_tick_;
    }
  g_return_val_if_fail (frames_per_tick > 0, -1);
  return math_round_double_to_signed_frame_t ((ticks * frames_per_tick));
}

void
Position::update_frames_from_ticks (double frames_per_tick)
{
  frames_ = get_frames_from_ticks (ticks_, frames_per_tick);

  /* assert that no overflow occurred */
  if (ticks_ >= 0)
    g_return_if_fail (frames_ >= 0);
  else
    g_return_if_fail (frames_ <= 0);
}

void
Position::set_to_bar (int bar)
{
  g_return_if_fail (
    TRANSPORT->ticks_per_bar_ > 0
    /* don't use INT_MAX, it results in a negative position */
    && bar <= POSITION_MAX_BAR);

  *this = Position ();

  if (bar > 0)
    {
      bar--;
      from_ticks (TRANSPORT->ticks_per_bar_ * bar);
    }
  else if (bar < 0)
    {
      bar++;
      from_ticks (TRANSPORT->ticks_per_bar_ * bar);
    }
  else
    {
      g_return_if_reached ();
    }
}

signed_ms_t
Position::to_ms () const
{
  if (frames_ == 0)
    {
      return 0;
    }
  return math_round_double_to_signed_frame_t (
    (1000.0 * (double) frames_) / ((double) AUDIO_ENGINE->sample_rate_));
}

signed_frame_t
Position::ms_to_frames (double ms)
{
  return math_round_double_to_signed_frame_t (
    (ms / 1000.0) * (double) AUDIO_ENGINE->sample_rate_);
}

void
Position::set_min_size (
  const Position &start_pos,
  Position       &end_pos,
  const SnapGrid &snap)
{
  end_pos = start_pos;
  end_pos.add_ticks (snap.get_default_ticks ());
}

bool
Position::get_prev_snap_point (
  Track *         track,
  Region *        region,
  const SnapGrid &sg,
  Position       &prev_sp) const
{
  prev_sp = *this;
  if (frames_ < 0 || ticks_ < 0)
    {
      /* negative not supported, set to same position */
      return false;
    }

  bool     snapped = false;
  Position snap_point = *this;
  if (sg.snap_to_grid_)
    {
      double snap_ticks = sg.get_snap_ticks ();
      double ticks_from_prev = fmod (ticks_, snap_ticks);
      snap_point.add_ticks (-ticks_from_prev);
      prev_sp = snap_point;
      snapped = true;
    }

  if (track && track->has_lanes())
    {
      std::visit (
        [&] (auto &&track) {
          for (auto &lane : track->lanes_)
            {
              for (auto &r : lane->regions_)
                {
                  snap_point = r->pos_;
                  if (snap_point <= *this && snap_point > prev_sp)
                    {
                      prev_sp = snap_point;
                      snapped = true;
                    }
                  snap_point = r->end_pos_;
                  if (snap_point <= *this && snap_point > prev_sp)
                    {
                      prev_sp = snap_point;
                      snapped = true;
                    }
                }
            }
        },
        convert_to_variant<LanedTrackPtrVariant> (track));
    }
  else if (region)
    {
      /* TODO */
    }

  /* if no point to snap to, set to same position */
  if (!snapped)
    {
      prev_sp = *this;
    }

  return snapped;
}

bool
Position::get_next_snap_point (
  Track *         track,
  Region *        region,
  const SnapGrid &sg,
  Position       &next_sp) const
{
  next_sp = *this;
  if (frames_ < 0 || ticks_ < 0)
    {
      /* negative not supported, set to same position */
      return false;
    }

  bool     snapped = false;
  Position snap_point = *this;
  if (sg.snap_to_grid_)
    {
      double snap_ticks = sg.get_snap_ticks ();
      double ticks_from_prev = fmod (ticks_, snap_ticks);
      double ticks_to_next = snap_ticks - ticks_from_prev;
      snap_point.add_ticks (ticks_to_next);
      next_sp = snap_point;
      snapped = true;
    }

  if (track && track->has_lanes ())
    {
      std::visit (
        [&] (auto &&track) {
          for (auto &lane : track->lanes_)
            {
              for (auto &r : lane->regions_)
                {
                  snap_point = r->pos_;
                  if (snap_point > *this && snap_point < next_sp)
                    {
                      next_sp = snap_point;
                      snapped = true;
                    }
                  snap_point = r->end_pos_;
                  if (snap_point > *this && snap_point < next_sp)
                    {
                      next_sp = snap_point;
                      snapped = true;
                    }
                }
            }
        },
        convert_to_variant<LanedTrackPtrVariant> (track));
    }
  else if (region)
    {
      /* TODO */
    }

  /* if no point to snap to, set to same position */
  if (!snapped)
    {
      next_sp = *this;
    }

  return snapped;
}

bool
Position::get_closest_snap_point (
  Track *         track,
  Region *        region,
  const SnapGrid &sg,
  Position       &closest_sp) const
{
  /* get closest snap point */
  Position prev_sp, next_sp;
  bool     prev_snapped = get_prev_snap_point (track, region, sg, prev_sp);
  bool     next_snapped = get_next_snap_point (track, region, sg, next_sp);
  if (prev_snapped && next_snapped)
    {
      closest_sp = get_closest_position (prev_sp, next_sp);
      return true;
    }
  else if (prev_snapped)
    {
      closest_sp = prev_sp;
      return true;
    }
  else if (next_snapped)
    {
      closest_sp = next_sp;
      return true;
    }
  else
    {
      closest_sp = *this;
      return false;
    }
}

void
Position::snap (
  const Position * start_pos,
  Track *          track,
  Region *         region,
  const SnapGrid  &sg)
{
  /* this should only be called if snap is on. the check should be done before
   * calling */
  g_warn_if_fail (sg.any_snap());

  /* position must be positive - only global positions allowed */
  g_return_if_fail (frames_ >= 0 && ticks_ >= 0);

  if (!sg.snap_to_events_)
    {
      region = nullptr;
      track = nullptr;
    }

  /* snap to grid with offset */
  if (sg.snap_to_grid_keep_offset_)
    {
      /* get previous snap point from start pos */
      g_return_if_fail (start_pos);
      Position prev_sp_from_start_pos;
      start_pos->get_prev_snap_point (track, region, sg, prev_sp_from_start_pos);

      /* get diff from previous snap point */
      double ticks_delta = start_pos->ticks_ - prev_sp_from_start_pos.ticks_;

      /* add ticks and check the closest snap point */
      add_ticks (-ticks_delta);

      /* get closest snap point */
      Position closest_sp;
      bool     have_closest_sp =
        get_closest_snap_point (track, region, sg, closest_sp);
      if (have_closest_sp)
        {
          /* move to closest snap point */
          *this = closest_sp;
        }

      /* readd ticks */
      add_ticks (ticks_delta);
    }
  /* else if snap to grid without offset */
  else
    {
      /* get closest snap point */
      Position closest_sp;
      get_closest_snap_point (track, region, sg, closest_sp);

      /* move to closest snap point */
      *this = closest_sp;
    }
}

void
Position::from_seconds (double secs)
{
  from_ticks (
    (secs * (double) AUDIO_ENGINE->sample_rate_)
    / (double) AUDIO_ENGINE->frames_per_tick_);
}

void
Position::add_bars (int bars)
{
  g_warn_if_fail (TRANSPORT->ticks_per_bar_ > 0);
  add_ticks (bars * TRANSPORT->ticks_per_bar_);
}

void
Position::add_beats (int beats)
{
  g_return_if_fail (TRANSPORT->ticks_per_beat_ > 0);
  add_ticks (beats * TRANSPORT->ticks_per_beat_);
}

double
Position::get_ticks_diff (
  const Position  &end_pos,
  const Position  &start_pos,
  const SnapGrid * sg)
{
  double   ticks_diff = end_pos.ticks_ - start_pos.ticks_;
  int    is_negative = ticks_diff < 0.0;
  Position diff_pos;
  diff_pos.add_ticks (fabs (ticks_diff));
  if (sg && sg->any_snap())
    {
      diff_pos.snap (nullptr, nullptr, nullptr, *sg);
    }
  ticks_diff = diff_pos.ticks_;
  if (is_negative)
    ticks_diff = -ticks_diff;

  return ticks_diff;
}

std::string
Position::to_string (int decimal_places) const
{
  char buf[80];
  to_string (buf, decimal_places);
  return buf;
}

void
Position::to_string (char * buf, int decimal_places) const
{
  int    bars = get_bars (true);
  int    beats = get_beats (true);
  int    sixteenths = get_sixteenths (true);
  double ticks = get_ticks ();
  g_return_if_fail (bars > -80000);
  if (ZRYTHM_TESTING)
    {
      sprintf (
        buf, "%d.%d.%d.%.*f (%" SIGNED_FRAME_FORMAT ")", bars, abs (beats),
        abs (sixteenths), decimal_places, fabs (ticks), frames_);
    }
  else
    {
      sprintf (
        buf, "%d.%d.%d.%.*f", bars, abs (beats), abs (sixteenths),
        decimal_places, fabs (ticks));
    }
}

Position::Position (const char * str)
{
  int   bars, beats, sixteenths;
  float ticksf;
  int   res = sscanf (str, "%d.%d.%d.%f", &bars, &beats, &sixteenths, &ticksf);
  if (res != 4 || res == EOF)
    throw ZrythmException ("Invalid position string");

  /* adjust for starting from 1 */
  if (bars > 0)
    bars--;
  else if (bars < 0)
    bars++;
  else
    throw ZrythmException ("Invalid position string");

  if (beats > 0)
    beats--;
  else if (beats < 0)
    beats++;
  else
    throw ZrythmException ("Invalid position string");

  if (sixteenths > 0)
    sixteenths--;
  else if (sixteenths < 0)
    sixteenths++;
  else
    throw ZrythmException ("Invalid position string");

  double ticks = (double) ticksf;

  int beats_per_bar = P_TEMPO_TRACK->get_beats_per_bar();
  int sixteenths_per_beat = TRANSPORT->sixteenths_per_beat_;
  int total_sixteenths =
    bars * beats_per_bar * sixteenths_per_beat + beats * sixteenths_per_beat
    + sixteenths;
  double total_ticks =
    (double) (total_sixteenths) *TICKS_PER_SIXTEENTH_NOTE_DBL + ticks;
  from_ticks (total_ticks);
}

void
Position::print () const
{
  z_debug (
    fmt::format ("{} ({} frames | {} ticks)", to_string (), frames_, ticks_));
}

void
Position::print_range (const Position &p1, const Position &p2)
{
  z_debug (fmt::format (
    "{} ({}) - {} ({}) <delta {} frames {} ticks>", p1.to_string (), p1.frames_,
    p2.to_string (), p2.frames_, p2.frames_ - p1.frames_,
    p2.ticks_ - p1.ticks_));
}

int
Position::get_total_bars (bool include_current) const
{
  int bars = get_bars (false);
  int cur_bars = get_bars (true);

  if (include_current || bars == 0)
    {
      return bars;
    }

  /* if we are at the start of the bar, don't count this bar */
  Position pos_at_bar;
  pos_at_bar.set_to_bar (cur_bars);
  if (pos_at_bar.frames_ == frames_)
    {
      bars--;
    }

  return bars;
}

int
Position::get_total_beats (bool include_current) const
{
  int beats = get_beats (false);
  int bars = get_bars (false);

  int beats_per_bar = P_TEMPO_TRACK->get_beats_per_bar();
  int ret = beats + bars * beats_per_bar;

  if (include_current || ret == 0)
    {
      return ret;
    }

  Position tmp;
  tmp.from_ticks ((double) ret * TRANSPORT->ticks_per_beat_);
  if (tmp.frames_ == frames_)
    {
      ret--;
    }

  return ret;
}

int
Position::get_total_sixteenths (bool include_current) const
{
  int ret;
  if (ticks_ >= 0)
    {
      ret = (int) math_round_double_to_signed_32 (
        floor (ticks_ / TICKS_PER_SIXTEENTH_NOTE_DBL));
    }
  else
    {
      ret = (int) math_round_double_to_signed_32 (
        ceil (ticks_ / TICKS_PER_SIXTEENTH_NOTE_DBL));
    }

  if (include_current || ret == 0)
    {
      return ret;
    }

  Position tmp ((double) ret * TICKS_PER_SIXTEENTH_NOTE_DBL);
  if (tmp.frames_ == frames_)
    {
      ret--;
    }

  return ret;
}

int
Position::get_bars (bool start_at_one) const
{
  g_return_val_if_fail (
    gZrythm && PROJECT && TRANSPORT && TRANSPORT->ticks_per_bar_ > 0, -1);

  double total_bars = ticks_ / TRANSPORT->ticks_per_bar_;
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
Position::get_beats (bool start_at_one) const
{
  g_return_val_if_fail (
    gZrythm && PROJECT && TRANSPORT && TRANSPORT->ticks_per_bar_ > 0
      && TRANSPORT->ticks_per_beat_ > 0 && P_TEMPO_TRACK,
    -1);

  int beats_per_bar = P_TEMPO_TRACK->get_beats_per_bar();
  g_return_val_if_fail (beats_per_bar > 0, -1);

  auto total_bars = (double) get_bars (false);
  double total_beats = ticks_ / TRANSPORT->ticks_per_beat_;
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
Position::get_sixteenths (bool start_at_one) const
{
  g_return_val_if_fail (
    gZrythm && PROJECT && TRANSPORT && TRANSPORT->sixteenths_per_beat_ > 0, -1);

  double total_beats = (double) get_total_beats (true);
  /*g_message ("total beats %f", total_beats);*/
  double total_sixteenths = ticks_ / TICKS_PER_SIXTEENTH_NOTE_DBL;
  /*g_message ("total sixteenths %f",*/
  /*total_sixteenths);*/
  total_sixteenths -= (double) (total_beats * TRANSPORT->sixteenths_per_beat_);
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
Position::get_ticks () const
{
  double total_sixteenths = get_total_sixteenths (true);
  /*g_debug ("total sixteenths %f", total_sixteenths);*/
  return ticks_ - (total_sixteenths * TICKS_PER_SIXTEENTH_NOTE_DBL);
}

bool
Position::validate () const
{
  return true;
}
