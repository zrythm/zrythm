/*
 * Copyright (C) 2018-2021 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <math.h>

#include "audio/engine.h"
#include "audio/position.h"
#include "audio/snap_grid.h"
#include "audio/tempo_track.h"
#include "audio/transport.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/editor_ruler.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_ruler.h"
#include "gui/widgets/top_bar.h"
#include "project.h"
#include "utils/algorithms.h"
#include "utils/math.h"
#include "utils/objects.h"

#include <gtk/gtk.h>

PURE
static int
position_cmpfunc (
  const void * a,
  const void * b)
{
  const Position * posa =
    (Position const *) a;
  const Position * posb =
    (Position const *) b;
  return
    (int)
    CLAMP (
      position_compare (posa, posb), -1, 1);
}

void
position_sort_array (
  Position *   array,
  const size_t size)
{
  qsort (
    array, size, sizeof (Position),
    position_cmpfunc);
}

/**
 * Updates ticks.
 */
void
position_update_ticks_from_frames (
  Position * self)
{
  g_return_if_fail (
    AUDIO_ENGINE->ticks_per_frame > 0);
  self->ticks =
    (double)
    self->frames * AUDIO_ENGINE->ticks_per_frame;
}

/**
 * Converts ticks to frames.
 */
long
position_get_frames_from_ticks (
  double ticks)
{
  g_return_val_if_fail (
    AUDIO_ENGINE->frames_per_tick > 0, -1);
  return
    math_round_double_to_long (
      (ticks * AUDIO_ENGINE->frames_per_tick));
}

/**
 * Updates frames.
 */
void
position_update_frames_from_ticks (
  Position * self)
{
  self->frames =
    position_get_frames_from_ticks (self->ticks);
}

/**
 * Sets position to given bar.
 */
void
position_set_to_bar (
  Position * self,
  int        bar)
{
  g_return_if_fail (
    TRANSPORT->ticks_per_bar > 0
    /* don't use INT_MAX, it results in a negative
     * position */
    && bar <= POSITION_MAX_BAR);

  position_init (self);

  if (bar > 0)
    {
      bar--;
      self->ticks = TRANSPORT->ticks_per_bar * bar;
      position_from_ticks (self, self->ticks);
    }
  else if (bar < 0)
    {
      bar++;
      self->ticks = TRANSPORT->ticks_per_bar * bar;
      position_from_ticks (self, self->ticks);
    }
  else
    {
      g_return_if_reached ();
    }
}

/**
 * Adds the frames to the position and updates
 * the rest of the fields, and makes sure the
 * frames are still accurate.
 */
void
position_add_frames (
  Position * pos,
  const long frames)
{
  pos->frames += frames;
  position_update_ticks_from_frames (pos);
}

/**
 * Returns the Position in milliseconds.
 */
long
position_to_ms (
  const Position * pos)
{
  if (position_to_frames (pos) == 0)
    {
      return 0;
    }
  return
    math_round_double_to_long (
      (1000.0 * (double) position_to_frames (pos)) /
       ((double) AUDIO_ENGINE->sample_rate));
}

long
position_ms_to_frames (
  const long ms)
{
  return
    math_round_double_to_long (
      ((double) ms / 1000) *
        (double) AUDIO_ENGINE->sample_rate);
}

void
position_add_ms (
  Position * pos,
  long       ms)
{
  long frames = position_ms_to_frames (ms);
  position_add_frames (pos, frames);
}

void
position_add_minutes (
  Position * pos,
  int        mins)
{
  long frames =
    position_ms_to_frames (mins * 60 * 1000);
  position_add_frames (pos, frames);
}

void
position_add_seconds (
  Position * pos,
  long       seconds)
{
  long frames =
    position_ms_to_frames (seconds * 1000);
  position_add_frames (pos, frames);
}

/**
 * Sets the end position to be 1 snap point away
 * from  the start pos.
 *
 * FIXME rename to something more meaningful.
 * @param start_pos Start Position.
 * @param end_pos Position to set.
 * @param snap The SnapGrid.
 */
void
position_set_min_size (
  const Position * start_pos,
  Position * end_pos,
  SnapGrid * snap)
{
  position_set_to_pos (end_pos, start_pos);
  position_add_ticks (
    end_pos,
    snap_grid_get_default_ticks (snap));
}

/**
 * Returns closest snap point.
 *
 * @param pos Position.
 * @param p1 Snap point 1.
 * @param p2 Snap point 2.
 */
PURE
static inline Position *
closest_snap_point (
  const Position * const pos,
  Position * const       p1,
  Position * const       p2)
{
  if (pos->ticks - p1->ticks <=
      p2->ticks - pos->ticks)
    {
      return p1;
    }
  else
    {
      return p2;
    }
}

/**
 * Gets the previous snap point.
 *
 * @param pos The position to reference.
 * @param track Track, used when moving things in
 *   the timeline. If keep offset is on and this is
 *   passed, the objects in the track will be taken
 *   into account. If keep offset is on and this is
 *   NULL, all applicable objects will be taken into
 *   account. Not used if keep offset is off.
 * @param region Region, used when moving
 *   things in the editor. Same behavior as @ref
 *   track.
 * @param sg SnapGrid options.
 * @param prev_snap_point The position to set.
 *
 * @return Whether a snap point was found or not.
 */
HOT
static bool
get_prev_snap_point (
  const Position * pos,
  Track *          track,
  ZRegion *        region,
  const SnapGrid * sg,
  Position *       prev_sp)
{
  g_return_val_if_fail (
    pos->frames >= 0 && pos->ticks >= 0, NULL);

  bool snapped = false;
  Position * snap_point = NULL;
  if (sg->snap_to_grid)
    {
      snap_point =
        (Position *)
        algorithms_binary_search_nearby (
          pos, sg->snap_points,
          (size_t) sg->num_snap_points,
          sizeof (Position), position_cmp_func,
          true, true);
    }
  if (snap_point)
    {
      position_set_to_pos (prev_sp, snap_point);
      snapped = true;
    }

  if (track)
    {
      for (int i = 0; i < track->num_lanes; i++)
        {
          TrackLane * lane = track->lanes[i];
          for (int j = 0; j < lane->num_regions;
               j++)
            {
              ZRegion * r = lane->regions[j];
              ArrangerObject * r_obj =
                (ArrangerObject *) r;
              snap_point = &r_obj->pos;
              if (position_is_before_or_equal (
                    snap_point, pos) &&
                  position_is_after (
                    snap_point, prev_sp))
                {
                  position_set_to_pos (
                    prev_sp, snap_point);
                  snapped = true;
                }
              snap_point = &r_obj->end_pos;
              if (position_is_before_or_equal (
                    snap_point, pos) &&
                  position_is_after (
                    snap_point, prev_sp))
                {
                  position_set_to_pos (
                    prev_sp, snap_point);
                  snapped = true;
                }
            }
        }
    }
  else if (region)
    {
      /* TODO */
    }

  /* if no point to snap to, set to same position */
  if (!snapped)
    {
      position_set_to_pos (prev_sp, pos);
    }

  return snapped;
}

/**
 * Get next snap point.
 *
 * @param pos Position to reference.
 * @param track Track, used when moving things in
 *   the timeline. If keep offset is on and this is
 *   passed, the objects in the track will be taken
 *   into account. If keep offset is on and this is
 *   NULL, all applicable objects will be taken into
 *   account. Not used if keep offset is off.
 * @param region Region, used when moving
 *   things in the editor. Same behavior as @ref
 *   track.
 * @param sg SnapGrid options.
 * @param next_snap_point Position to set.
 *
 * @return Whether a snap point was found or not.
 */
HOT
static bool
get_next_snap_point (
  const Position * pos,
  Track *          track,
  ZRegion *        region,
  const SnapGrid * sg,
  Position *       next_sp)
{
  g_return_val_if_fail (
    pos->frames >= 0 && pos->ticks >= 0, NULL);

  bool snapped = false;
  Position * snap_point = NULL;
  if (sg->snap_to_grid)
    {
      snap_point =
        algorithms_binary_search_nearby (
          pos, sg->snap_points,
          (size_t) sg->num_snap_points,
          sizeof (Position), position_cmp_func,
          false, false);
    }
  if (snap_point)
    {
      position_set_to_pos (next_sp, snap_point);
      snapped = true;
    }

  if (track)
    {
      for (int i = 0; i < track->num_lanes; i++)
        {
          TrackLane * lane = track->lanes[i];
          for (int j = 0; j < lane->num_regions;
               j++)
            {
              ZRegion * r = lane->regions[j];
              ArrangerObject * r_obj =
                (ArrangerObject *) r;
              snap_point = &r_obj->pos;
              if (position_is_after (
                    snap_point, pos) &&
                  position_is_before (
                    snap_point, next_sp))
                {
                  position_set_to_pos (
                    next_sp, snap_point);
                  snapped = true;
                }
              snap_point = &r_obj->end_pos;
              if (position_is_after (
                    snap_point, pos) &&
                  position_is_before (
                    snap_point, next_sp))
                {
                  position_set_to_pos (
                    next_sp, snap_point);
                  snapped = true;
                }
            }
        }
    }
  else if (region)
    {
      /* TODO */
    }

  /* if no point to snap to, set to same position */
  if (!snapped)
    {
      position_set_to_pos (next_sp, pos);
    }

  return snapped;
}

/**
 * Snaps position using given options.
 *
 * @param start_pos The previous position (ie, the
 *   position the drag started at. This is only used
 *   when the "keep offset" setting is on.
 * @param pos Position to edit.
 * @param track Track, used when moving things in
 *   the timeline. If snap to events is on and this is
 *   passed, the objects in the track will be taken
 *   into account. If snap to events is on and this is
 *   NULL, all applicable objects will be taken into
 *   account. Not used if snap to events is off.
 * @param region Region, used when moving
 *   things in the editor. Same behavior as @ref
 *   track.
 * @param sg SnapGrid options.
 */
void
position_snap (
  Position *       start_pos,
  Position *       pos,
  Track *          track,
  ZRegion *        region,
  const SnapGrid * sg)
{
  g_return_if_fail (sg);

  /* this should only be called if snap is on.
   * the check should be done before calling */
  g_warn_if_fail (SNAP_GRID_ANY_SNAP (sg));

  /* position must be positive - only global
   * positions allowed */
  g_return_if_fail (
    pos->frames >= 0 && pos->ticks >= 0);

  if (!sg->snap_to_events)
    {
      region = NULL;
      track = NULL;
    }

  /* snap to grid without offset */
  if (!sg->snap_to_grid_keep_offset)
    {
      /* get closest snap point */
      Position prev_sp, next_sp;
      bool prev_snapped =
        get_prev_snap_point (
          pos, track, region, sg, &prev_sp);
      bool next_snapped =
        get_next_snap_point (
          pos, track, region, sg, &next_sp);
      Position * closest_sp = NULL;
      if (prev_snapped && next_snapped)
        {
          closest_sp =
            closest_snap_point (
              pos, &prev_sp, &next_sp);
        }
      else if (prev_snapped)
        {
          closest_sp = &prev_sp;
        }
      else if (next_snapped)
        {
          closest_sp = &next_sp;
        }
      else
        {
          closest_sp = pos;
        }

      /* move to it */
      position_set_to_pos (
        pos, closest_sp);
    }
  /* snap to grid with offset */
  else
    {
      /* get closest snap point */
      Position prev_sp, next_sp;
      bool prev_snapped =
        get_prev_snap_point (
          pos, track, region, sg, &prev_sp);
      bool next_snapped =
        get_next_snap_point (
          pos, track, region, sg, &next_sp);
      Position * closest_sp = NULL;
      if (prev_snapped && next_snapped)
        {
          closest_sp =
            closest_snap_point (
              pos, &prev_sp, &next_sp);
        }
      else if (prev_snapped)
        {
          closest_sp = &prev_sp;
        }
      else if (next_snapped)
        {
          closest_sp = &next_sp;
        }
      else
        {
          closest_sp = pos;
        }

      /* get previous snap point from start pos */
      Position prev_sp_from_start_pos;
      position_init (&prev_sp_from_start_pos);
      get_prev_snap_point (
        start_pos, track, region, sg,
        &prev_sp_from_start_pos);

      /* get diff from previous snap point */
      double ticks_delta =
        start_pos->ticks -
        prev_sp_from_start_pos.ticks;

      /*g_debug ("ticks delta %f", ticks_delta);*/

      /* move to closest snap point */
      position_set_to_pos (
        pos, closest_sp);

      /* add diff */
      position_add_ticks (
        pos, ticks_delta);
    }
}

/**
 * Converts seconds to position and puts the result
 * in the given Position.
 */
void
position_from_seconds (
  Position * position,
  double secs)
{
  position_from_ticks (
    position,
    (secs * (double) AUDIO_ENGINE->sample_rate) /
      (double) AUDIO_ENGINE->frames_per_tick);
}

/**
 * Sets position to the given total tick count.
 */
void
position_from_ticks (
  Position * pos,
  double     ticks)
{
  pos->schema_version = POSITION_SCHEMA_VERSION;
  pos->ticks = ticks;
  position_update_frames_from_ticks (pos);
}

void
position_from_frames (
  Position * pos,
  long       frames)
{
  pos->schema_version = POSITION_SCHEMA_VERSION;
  pos->frames = frames;
  position_update_ticks_from_frames (pos);
}

void
position_from_bars (
  Position * pos,
  int        bars)
{
  position_init (pos);
  position_add_bars (pos, bars);
}

void
position_add_ticks (
  Position * self,
  double     ticks)
{
  position_from_ticks (self, self->ticks + ticks);
}

/**
 * Calculates the midway point between the two
 * positions and sets it on pos.
 *
 * @param pos Position to set to.
 */
inline void
position_get_midway_pos (
  Position * start_pos,
  Position * end_pos,
  Position * pos)
{
  double end_ticks, start_ticks, ticks_diff;
  start_ticks = position_to_ticks (start_pos);
  end_ticks = position_to_ticks (end_pos);
  ticks_diff = end_ticks - start_ticks;
  position_set_to_pos (pos, start_pos);
  position_add_ticks (pos, ticks_diff / 2.0);
}

/**
 * Returns the difference in ticks between the two
 * Position's, snapped based on the given SnapGrid
 * (if any).
 *
 * TODO, refactor, too confusing on what it's
 * supposed to do.
 *
 * @param end_pos End position.
 * @param start_pos Start Position.
 * @param sg SnapGrid to snap with, or NULL to not
 *   snap.
 */
double
position_get_ticks_diff (
  const Position * end_pos,
  const Position * start_pos,
  const SnapGrid * sg)
{
  double ticks_diff =
    end_pos->ticks -
    start_pos->ticks;
  int is_negative = ticks_diff < 0.0;
  POSITION_INIT_ON_STACK (diff_pos);
  position_add_ticks (
    &diff_pos, fabs (ticks_diff));
  if (sg && SNAP_GRID_ANY_SNAP(sg))
    {
      position_snap (
        NULL, &diff_pos, NULL, NULL, sg);
    }
  ticks_diff = diff_pos.ticks;
  if (is_negative)
    ticks_diff = - ticks_diff;

  return ticks_diff;
}

/**
 * Creates a string in the form of "0.0.0.0" from
 * the given position.
 *
 * Must be free'd by caller.
 */
char *
position_to_string_alloc (
  const Position * pos)
{
  char buf[80];
  position_to_string (pos, buf);
  return g_strdup (buf);
}

/**
 * Creates a string in the form of "0.0.0.0" from
 * the given position.
 */
NONNULL
void
position_to_string (
  const Position * pos,
  char *           buf)
{
  int bars = position_get_bars (pos, true);
  int beats = position_get_beats (pos, true);
  int sixteenths =
    position_get_sixteenths (pos, true);
  double ticks = position_get_ticks (pos);
  g_return_if_fail (bars > -80000);
  sprintf (
    buf, "%d.%d.%d.%f",
    bars, beats, sixteenths, ticks);
}

/**
 * Prints the Position in the "0.0.0.0" form.
 */
void
position_print (
  const Position * pos)
{
  char buf[140];
  position_to_string (pos, buf);
  g_message ("%s (%ld)", buf, pos->frames);
}

void
position_print_range (
  const Position * pos,
  const Position * pos2)
{
  char buf[140];
  position_to_string (pos, buf);
  char buf2[140];
  position_to_string (pos2, buf2);
  g_message (
    "%s (%ld) - %s (%ld) "
    "<delta %ld frames %f ticks>",
    buf, pos->frames, buf2, pos2->frames,
    pos2->frames - pos->frames,
    pos2->ticks - pos->ticks);
}

/**
 * Returns the total number of beats.
 *
 * @param include_current Whether to count the
 *   current beat if it is at the beat start.
 */
int
position_get_total_bars (
  const Position * pos,
  bool  include_current)
{
  int bars = position_get_bars (pos, false);
  int cur_bars = position_get_bars (pos, true);

  if (include_current || bars == 0)
    {
      return bars;
    }

  /* if we are at the start of the bar, don't
   * count this bar */
  Position pos_at_bar;
  position_set_to_bar (&pos_at_bar, cur_bars);
  if (pos_at_bar.frames == pos->frames)
    {
      bars--;
    }

  return bars;
}

/**
 * Returns the total number of beats.
 *
 * @param include_current Whether to count the
 *   current beat if it is at the beat start.
 */
int
position_get_total_beats (
  const Position * pos,
  bool  include_current)
{
  int beats = position_get_beats (pos, false);
  int bars = position_get_bars (pos, false);

  int beats_per_bar =
    tempo_track_get_beats_per_bar (P_TEMPO_TRACK);
  int ret = beats + bars * beats_per_bar;

  if (include_current || ret == 0)
    {
      return ret;
    }

  Position tmp;
  position_from_ticks (
    &tmp, (double) ret * TRANSPORT->ticks_per_beat);
  if (tmp.frames == pos->frames)
    {
      ret--;
    }

  return ret;
}

/**
 * Returns the total number of sixteenths not
 * including the current one.
 */
int
position_get_total_sixteenths (
  const Position * pos,
  bool             include_current)
{
  int ret;
  if (pos->ticks >= 0)
    {
      ret =
        math_round_double_to_int (
          floor (
            pos->ticks /
              TICKS_PER_SIXTEENTH_NOTE_DBL));
    }
  else
    {
      ret =
        math_round_double_to_int (
          ceil (
            pos->ticks /
              TICKS_PER_SIXTEENTH_NOTE_DBL));
    }

  if (include_current || ret == 0)
    {
      return ret;
    }

  Position tmp;
  position_from_ticks (
    &tmp,
    (double) ret * TICKS_PER_SIXTEENTH_NOTE_DBL);
  if (tmp.frames == pos->frames)
    {
      ret--;
    }

  return ret;
}

/**
 * Changes the sign of the position.
 *
 * For example, 4.2.1.21 would become -4.2.1.21.
 */
void
position_change_sign (
  Position * pos)
{
  pos->ticks = - pos->ticks;
  pos->frames = - pos->frames;
}

/**
 * Gets the bars of the position.
 *
 * Ie, if the position is equivalent to 4.1.2.42,
 * this will return 4.
 *
 * @param start_at_one Start at 1 or -1 instead of
 *   0.
 */
int
position_get_bars (
  const Position * pos,
  bool  start_at_one)
{
  g_return_val_if_fail (
    ZRYTHM && PROJECT && TRANSPORT &&
    TRANSPORT->ticks_per_bar > 0, -1);

  double total_bars =
    pos->ticks / TRANSPORT->ticks_per_bar;
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

/**
 * Gets the beats of the position.
 *
 * Ie, if the position is equivalent to 4.1.2.42,
 * this will return 1.
 *
 * @param start_at_one Start at 1 or -1 instead of
 *   0.
 */
int
position_get_beats (
  const Position * pos,
  bool  start_at_one)
{
  g_return_val_if_fail (
    ZRYTHM && PROJECT && TRANSPORT &&
    TRANSPORT->ticks_per_bar > 0 &&
    TRANSPORT->ticks_per_beat > 0 &&
    P_TEMPO_TRACK, -1);

  int beats_per_bar =
    tempo_track_get_beats_per_bar (P_TEMPO_TRACK);
  g_return_val_if_fail (beats_per_bar > 0, -1);

  double total_bars =
    (double) position_get_bars (pos, false);
  double total_beats =
    pos->ticks / TRANSPORT->ticks_per_beat;
  total_beats -=
    (double) (total_bars * beats_per_bar);
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

/**
 * Gets the sixteenths of the position.
 *
 * Ie, if the position is equivalent to 4.1.2.42,
 * this will return 2.
 *
 * @param start_at_one Start at 1 or -1 instead of
 *   0.
 */
int
position_get_sixteenths (
  const Position * pos,
  bool  start_at_one)
{
  g_return_val_if_fail (
    ZRYTHM && PROJECT && TRANSPORT &&
    TRANSPORT->sixteenths_per_beat > 0, -1);

  double total_beats =
    (double) position_get_total_beats (pos, true);
  /*g_message ("total beats %f", total_beats);*/
  double total_sixteenths =
    pos->ticks / TICKS_PER_SIXTEENTH_NOTE_DBL;
  /*g_message ("total sixteenths %f",*/
    /*total_sixteenths);*/
  total_sixteenths -=
    (double)
    (total_beats * TRANSPORT->sixteenths_per_beat);
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

/**
 * Gets the ticks of the position.
 *
 * Ie, if the position is equivalent to
 * 4.1.2.42.40124, this will return 42.40124.
 */
double
position_get_ticks (
  const Position * pos)
{
  double total_sixteenths =
    position_get_total_sixteenths (pos, true);
  /*g_debug ("total sixteenths %f", total_sixteenths);*/
  return
    pos->ticks -
    (total_sixteenths *
     TICKS_PER_SIXTEENTH_NOTE_DBL);
}

bool
position_validate (
  const Position * pos)
{
  if (!pos->schema_version)
    {
      return false;
    }

  return true;
}
