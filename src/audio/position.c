/*
 * Copyright (C) 2018-2020 Alexandros Theodotou <alex at zrythm dot org>
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
#include "utils/math.h"

#include <gtk/gtk.h>

/**
 * Converts position bars/beats/quarter beats/ticks
 * to frames
 *
 * Note: transport must be setup by this point.
 */
long
position_to_frames (
  const Position * position)
{
  double frames =
    AUDIO_ENGINE->frames_per_tick *
      (position->bars > 0 ?
       (double) (position->bars - 1) :
       (double) (position->bars + 1)) *
      (double) TRANSPORT->beats_per_bar *
      (double) TRANSPORT->ticks_per_beat;
  if (position->beats)
    frames +=
      (AUDIO_ENGINE->frames_per_tick *
        (position->beats > 0 ?
         (double) (position->beats - 1) :
         (double) (position->beats + 1)) *
        (double) TRANSPORT->ticks_per_beat);
  if (position->sixteenths)
    frames +=
      (AUDIO_ENGINE->frames_per_tick *
        (position->sixteenths > 0 ?
         (double) (position->sixteenths - 1) :
         (double) (position->sixteenths + 1)) *
        (double) TICKS_PER_SIXTEENTH_NOTE);
  if (position->ticks)
    frames +=
      (AUDIO_ENGINE->frames_per_tick *
        (double) position->ticks);
  frames +=
    AUDIO_ENGINE->frames_per_tick *
      position->sub_tick;
  /*g_message ("pos frames %f", frames);*/
  return (long) math_round_double_to_long (frames);
}

static int
cmpfunc (
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
    array, size, sizeof (Position), cmpfunc);
}

/**
 * Updates frames
 */
void
position_update_ticks_and_frames (
  Position * position)
{
  position->total_ticks =
    position_to_ticks (position);
  position->frames = position_to_frames (position);
}

/**
 * Sets position to given bar
 */
void
position_set_to_bar (
  Position * position,
  int        bar)
{
  if (bar < 1)
    bar = 1;
  position->bars = bar;
  position->beats = 1;
  position->sixteenths = 1;
  position->ticks = 0;
  position->sub_tick = 0.0;
  position_update_ticks_and_frames (position);
}

void
position_set_bar (
  Position * position,
  int        bar)
{
  if (bar < 1)
    bar = 1;
  position->bars = bar;
  position_update_ticks_and_frames (position);
}

void
position_set_beat (
  Position * position,
  int        beat)
{
  while (beat < 1 || beat > TRANSPORT->beats_per_bar)
    {
      if (beat < 1)
        {
          if (position->bars == 1)
            {
              beat = 1;
              break;
            }
          beat += TRANSPORT->beats_per_bar;
          position_set_bar (position,
                            position->bars - 1);
        }
      else if (beat > TRANSPORT->beats_per_bar)
        {
          beat -= TRANSPORT->beats_per_bar;
          position_set_bar (position,
                            position->bars + 1);
        }
    }
  position->beats = beat;
  position_update_ticks_and_frames (position);
}

void
position_set_sixteenth (
  Position * position,
  int        sixteenth)
{
  while (sixteenth < 1 ||
         sixteenth > TRANSPORT->sixteenths_per_beat)
    {
      if (sixteenth < 1)
        {
          if (position->bars == 1 &&
              position->beats == 1)
            {
              sixteenth = 1;
              break;
            }
          sixteenth +=
            TRANSPORT->sixteenths_per_beat;
          position_set_beat (
            position, position->beats - 1);
        }
      else if (sixteenth >
                 TRANSPORT->sixteenths_per_beat)
        {
          sixteenth -=
            TRANSPORT->sixteenths_per_beat;
          position_set_beat (
            position, position->beats + 1);
        }
    }
  position->sixteenths = sixteenth;
  position_update_ticks_and_frames (position);
}


/**
 * Sets the tick of the Position.
 */
void
position_set_tick (
  Position *  pos,
  double      tick)
{
  while (tick < 0.0 ||
         tick >=
           (TICKS_PER_SIXTEENTH_NOTE_DBL))
    {
      if (tick < 0.0)
        {
          if (pos->bars == 1 &&
              pos->beats == 1 &&
              pos->sixteenths == 1)
            {
              tick = 0.0;
              break;
            }
          tick += (double) TICKS_PER_SIXTEENTH_NOTE;
          position_set_sixteenth (
            pos, pos->sixteenths - 1);
        }
      else if (tick >
                 (double)
                 TICKS_PER_SIXTEENTH_NOTE - 1.0)
        {
          tick -= (double) TICKS_PER_SIXTEENTH_NOTE;
          position_set_sixteenth (
            pos, pos->sixteenths + 1);
        }
    }
  pos->ticks = (int) floor (tick);
  g_warn_if_fail (
    pos->ticks <
      TICKS_PER_SIXTEENTH_NOTE_DBL - 0.001);
  pos->sub_tick =
    tick - (double) pos->ticks;
  position_update_ticks_and_frames (pos);
  g_warn_if_fail (
    pos->sub_tick >= 0.0 &&
    pos->sub_tick < 1.0);
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
  long new_frames = pos->frames + frames;
  position_set_tick (
    pos,
    (double) pos->ticks + pos->sub_tick +
      ((double) frames /
        AUDIO_ENGINE->frames_per_tick));
  pos->frames = new_frames;
  g_warn_if_fail (
    pos->sub_tick >= 0.0 &&
    pos->sub_tick < 1.0);
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
  long ms)
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
 * Returns closest snap point.
 */
static inline Position *
closest_snap_point (Position * pos, ///< position
                    Position * p1, ///< snap point 1
                    Position * p2) ///< snap point 2
{
  if (pos->total_ticks - p1->total_ticks <=
      p2->total_ticks - pos->total_ticks)
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
 * @param sg SnapGrid options.
 * @param prev_snap_point The position to set.
 */
static inline void
get_prev_snap_point (
  const Position * pos,
  const SnapGrid * sg,
  Position * prev_snap_point)
{
  const Position * snap_point;
  for (int i = sg->num_snap_points - 1; i >= 0; i--)
    {
      snap_point = &sg->snap_points[i];
      if (position_is_before_or_equal (
            snap_point, pos))
        {
          position_set_to_pos (
            prev_snap_point,
            snap_point);
          return;
        }
    }
  g_return_if_reached ();
}

/**
 * Get next snap point.
 *
 * @param pos Position to reference.
 * @param sg SnapGrid options.
 * @param next_snap_point Position to set.
 */
static inline void
get_next_snap_point (
  const Position * pos,
  const SnapGrid *sg,
  Position * next_snap_point)
{
  const Position * snap_point;
  for (int i = 0; i < sg->num_snap_points; i++)
    {
      snap_point = &sg->snap_points[i];
      if (position_compare (snap_point,
                            pos) > 0)
        {
          position_set_to_pos (next_snap_point,
                               snap_point);
          return;
        }
    }
  g_return_if_reached ();
}

/**
 * Snap the given Position using the options in the
 * given SnapGrid.
 */
static void
snap_pos (
  Position * pos,
  const SnapGrid * sg)
{
  Position prev_snap_point;
  get_prev_snap_point (
    pos, sg, &prev_snap_point);
  Position next_snap_point;
  get_next_snap_point (
    pos, sg, &next_snap_point);
  Position * csp =
    closest_snap_point (
      pos,
      &prev_snap_point,
      &next_snap_point);
  position_set_to_pos (
    pos, csp);
}

/**
 * Sets the end position to be 1 snap point away from the start pos.
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
    snap_grid_get_note_ticks (
      snap->note_length, snap->note_type));
}

/**
 * Snaps position using given options.
 *
 * @param prev_pos Previous Position.
 * @param pos Position moved to.
 * @param track Track at new Position (for Region
 *   moving) FIXME needed?.
 * @param region ZRegion at new Position (for
 *   MidiNote moving) FIXME needed?.
 * @param sg SnapGrid options.
 */
void
position_snap (
  const Position * prev_pos,
  Position * pos,
  Track    * track,
  ZRegion   * region,
  const SnapGrid * sg)
{
  /* this should only be called if snap is on.
   * the check should be done before calling */
  g_warn_if_fail (SNAP_GRID_ANY_SNAP (sg));

  if ((sg->snap_to_grid &&
       !sg->snap_to_grid_keep_offset &&
      !sg->snap_to_events) ||
      (!track && !region))
    {
      snap_pos (pos, sg);
    }
  else if (!sg->snap_to_grid && sg->snap_to_grid_keep_offset &&
           !sg->snap_to_events)
    {
      g_warn_if_fail (prev_pos);
      /* TODO */
      /* get closest snap point to prev_pos */

      /* get diff from closest snap point */

      /* snap pos*/

      /* add diff */
    }
}

/**
 * Converts seconds to position and puts the result
 * in the given Position.
 * TODO
 */
void
position_from_seconds (
  Position * position,
  double secs)
{
  /*AUDIO_ENGINE->frames_per_tick;*/
  /*AUDIO_ENGINE->sample_rate;*/

}

double
position_to_ticks (
  const Position * pos)
{
  g_warn_if_fail (TRANSPORT->ticks_per_bar > 0);
  double ticks;
  if (pos->bars > 0)
    {
      ticks =
        (double)
        ((pos->bars - 1) * TRANSPORT->ticks_per_bar);
      if (pos->beats)
        ticks +=
          (double)
          ((pos->beats - 1) *
             TRANSPORT->ticks_per_beat);
      if (pos->sixteenths)
        ticks +=
          (double)
          ((pos->sixteenths - 1) *
            TICKS_PER_SIXTEENTH_NOTE);
      if (pos->ticks)
        ticks += (double) pos->ticks;
      ticks += pos->sub_tick;
      g_warn_if_fail (
        pos->sub_tick >= 0.0 &&
        pos->sub_tick < 1.0);
      return ticks;
    }
  else if (pos->bars < 0)
    {
      ticks =
        (double)
        ((pos->bars + 1) * TRANSPORT->ticks_per_bar);
      if (pos->beats)
        ticks +=
          (double)
          ((pos->beats + 1) *
             TRANSPORT->ticks_per_beat);
      if (pos->sixteenths)
        ticks +=
          (double)
          ((pos->sixteenths + 1) *
            TICKS_PER_SIXTEENTH_NOTE);
      if (pos->ticks)
        ticks += pos->ticks;
      ticks += pos->sub_tick;
      g_warn_if_fail (
        pos->sub_tick >= 0.0 &&
        pos->sub_tick < 1.0);
      return ticks;
    }
  else
    g_return_val_if_reached (-1);
}

/**
 * Sets position to the given total tick count.
 */
inline void
position_from_ticks (
  Position * pos,
  double     ticks)
{
  g_return_if_fail (
    TRANSPORT->ticks_per_bar > 0.0 &&
    TRANSPORT->ticks_per_beat > 0.0 &&
    TICKS_PER_SIXTEENTH_NOTE > 0 &&
    AUDIO_ENGINE->frames_per_tick > 0);
  pos->total_ticks = ticks;
  if (ticks >= 0)
    {
      pos->bars =
        (int)
        (ticks / TRANSPORT->ticks_per_bar + 1);
      ticks =
        fmod (ticks, TRANSPORT->ticks_per_bar);
      pos->beats =
        (int)
        (ticks / TRANSPORT->ticks_per_beat + 1);
      ticks =
        fmod (ticks, TRANSPORT->ticks_per_beat);
      pos->sixteenths =
        (int)
        (ticks / TICKS_PER_SIXTEENTH_NOTE + 1);
      ticks =
        fmod (
          ticks, (double) TICKS_PER_SIXTEENTH_NOTE);
      pos->ticks = (int) floor (ticks);
      g_warn_if_fail (
        pos->ticks <
          TICKS_PER_SIXTEENTH_NOTE_DBL - 0.001);
      pos->sub_tick = ticks - pos->ticks;
      g_warn_if_fail (
        pos->sub_tick >= 0.0 &&
        pos->sub_tick < 1.0);
    }
  else
    {
      pos->bars =
        (int)
        (ticks / TRANSPORT->ticks_per_bar - 1);
      ticks =
        fmod (ticks, TRANSPORT->ticks_per_bar);
      pos->beats =
        (int)
        (ticks / TRANSPORT->ticks_per_beat - 1);
      ticks =
        fmod (ticks, TRANSPORT->ticks_per_beat);
      pos->sixteenths =
        (int)
        (ticks / TICKS_PER_SIXTEENTH_NOTE - 1);
      ticks =
        fmod (
          ticks, (double) TICKS_PER_SIXTEENTH_NOTE);
      pos->ticks = (int) floor (ticks);
      g_warn_if_fail (
        pos->ticks <
          TICKS_PER_SIXTEENTH_NOTE_DBL - 0.001);
      pos->sub_tick = ticks - pos->ticks;
      g_warn_if_fail (
        pos->sub_tick >= 0.0 &&
        pos->sub_tick < 1.0);
    }
  position_update_ticks_and_frames (pos);
}

/**
 * Calculates the midway point between the two
 * positions and sets it on pos.
 */
inline void
position_get_midway_pos (
  Position * start_pos,
  Position * end_pos,
  Position * pos) ///< position to set to
{
  double end_ticks, start_ticks, ticks_diff;
  start_ticks = position_to_ticks (start_pos);
  end_ticks = position_to_ticks (end_pos);
  ticks_diff = end_ticks - start_ticks;
  position_set_to_pos (pos, start_pos);
  position_set_tick (pos, ticks_diff / 2.0);
}

/**
 * Returns the difference in ticks between the two
 * Position's, snapped based on the given SnapGrid
 * (if any).
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
    end_pos->total_ticks -
    start_pos->total_ticks;
  int is_negative = ticks_diff < 0.0;
  POSITION_INIT_ON_STACK (diff_pos);
  position_add_ticks (
    &diff_pos, fabs (ticks_diff));
  if (sg && SNAP_GRID_ANY_SNAP(sg))
    position_snap (
      NULL, &diff_pos, NULL, NULL, sg);
  ticks_diff = diff_pos.total_ticks;
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
position_stringize_allocate (
  const Position * pos)
{
  char buf[40];
  position_stringize (pos, buf);
  return g_strdup (buf);
}

/**
 * Creates a string in the form of "0.0.0.0" from
 * the given position.
 */
void
position_stringize (
  const Position * pos,
  char *           buf)
{
  char str[40];
  sprintf (str, "%f", pos->sub_tick);
  sprintf (
    buf, "%d.%d.%d.%d.%s",
    pos->bars, pos->beats, pos->sixteenths,
    pos->ticks, &str[2]);
}

/**
 * Prints the Position in the "0.0.0.0" form.
 */
void
position_print (
  const Position * pos)
{
  char buf[60];
  position_stringize (pos, buf);
  g_message ("%s", buf);
}

/**
 * Returns the total number of bars not including
 * the current one.
 */
int
position_get_total_bars (
  const Position * pos)
{
  int bars = pos->bars - 1;

  /* if we are at the start of the bar, don't
   * count this bar */
  Position pos_at_bar;
  position_set_to_bar (&pos_at_bar, pos->bars);
  if (pos_at_bar.frames == pos->frames)
    {
      bars--;
    }

  return bars;
}

/**
 * Returns the total number of beats not including
 * the current one.
 */
int
position_get_total_beats (
  const Position * pos)
{
  return
    pos->beats +
    pos->bars * TRANSPORT->beats_per_bar - 1;
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
  double ticks = pos->total_ticks;
  position_from_ticks (pos, - ticks);
}

SERIALIZE_SRC (Position, position)
DESERIALIZE_SRC (Position, position)
PRINT_YAML_SRC (Position, position)
