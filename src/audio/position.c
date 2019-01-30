/*
 * audio/position.c - position on the timeline
 *
 * Copyright (C) 2018 Alexandros Theodotou
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
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
#include "gui/widgets/midi_ruler.h"
#include "gui/widgets/piano_roll.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_ruler.h"
#include "gui/widgets/top_bar.h"
#include "project.h"

#include <gtk/gtk.h>

/**
 * Initializes given position to all 0
 */
void
position_init (Position * position)
{
  position->bars = 1;
  position->beats = 1;
  position->sixteenths = 1;
  position->ticks = 0;
}

/**
 * Converts position bars/beats/quarter beats/ticks to frames
 *
 * Note: transport must be setup by this point.
 */
int
position_to_frames (Position * position)
{
  int frames =
    AUDIO_ENGINE->frames_per_tick *
    (position->bars - 1) *
    TRANSPORT->beats_per_bar *
    TICKS_PER_BEAT;
  if (position->beats)
    frames +=
      AUDIO_ENGINE->frames_per_tick *
      (position->beats - 1) *
      TICKS_PER_BEAT;
  if (position->sixteenths)
    frames +=
      AUDIO_ENGINE->frames_per_tick *
      (position->sixteenths - 1) *
      TICKS_PER_SIXTEENTH_NOTE;
  if (position->ticks)
    frames +=
      AUDIO_ENGINE->frames_per_tick *
      position->ticks;
  return frames;
}

/**
 * Updates frames
 */
void
position_update_frames (Position * position)
{
  position->frames = position_to_frames (position);
}

/**
 * Sets position to given bar
 */
void
position_set_to_bar (Position * position,
                      int        bar)
{
  if (bar < 1)
    bar = 1;
  position->bars = bar;
  position->beats = 1;
  position->sixteenths = 1;
  position->ticks = 0;
  position->frames = position_to_frames (position);
}

void
position_set_bar (Position * position,
                  int      bar)
{
  if (bar < 1)
    bar = 1;
  position->bars = bar;
  position->frames = position_to_frames (position);
}

void
position_set_beat (Position * position,
                  int      beat)
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
  position->frames = position_to_frames (position);
}

void
position_set_sixteenth (Position * position,
                        int        sixteenth)
{
  while (sixteenth < 1 || sixteenth > SIXTEENTHS_PER_BEAT)
    {
      if (sixteenth < 1)
        {
          if (position->bars == 1 && position->beats == 1)
            {
              sixteenth = 1;
              break;
            }
          sixteenth += SIXTEENTHS_PER_BEAT;
          position_set_beat (position,
                             position->beats - 1);
        }
      else if (sixteenth > SIXTEENTHS_PER_BEAT)
        {
          sixteenth -= SIXTEENTHS_PER_BEAT;
          position_set_beat (position,
                             position->beats + 1);
        }
    }
  position->sixteenths = sixteenth;
  position->frames = position_to_frames (position);
}


void
position_set_tick (Position * position,
                  int      tick)
{
  while (tick < 0 || tick > TICKS_PER_SIXTEENTH_NOTE - 1)
    {
      if (tick < 0)
        {
          if (position->bars == 1 &&
              position->beats == 1 &&
              position->sixteenths == 1)
            {
              tick = 0;
              break;
            }
          tick += TICKS_PER_SIXTEENTH_NOTE;
          position_set_sixteenth (
            position,
            position->sixteenths - 1);
        }
      else if (tick > TICKS_PER_SIXTEENTH_NOTE - 1)
        {
          tick -= TICKS_PER_SIXTEENTH_NOTE;
          position_set_sixteenth (
            position,
            position->sixteenths + 1);
        }
    }
  position->ticks = tick;
  position->frames = position_to_frames (position);
}

/**
 * Sets position to target position
 */
void
position_set_to_pos (Position * pos,
                Position * target)
{
  position_set_bar (pos, target->bars);
  position_set_beat (pos, target->beats);
  position_set_sixteenth (pos, target->sixteenths);
  position_set_tick (pos, target->ticks);
}

void
position_add_frames (Position * position,
                     int      frames)
{
  position->frames += frames;
  position_set_tick (position,
                     position->ticks +
                       frames / AUDIO_ENGINE->frames_per_tick);
}

/**
 * Compares 2 positions.
 *
 * negative = p1 < p2
 * 0 = equal
 * positive = p1 > p2
 */
int
position_compare (Position * p1,
                  Position * p2)
{
  if (p1->bars == p2->bars &&
      p1->beats == p2->beats &&
      p1->sixteenths == p2->sixteenths &&
      p1->ticks == p2->ticks)
    return 0;
  else if ((p1->bars < p2->bars) ||
           (p1->bars == p2->bars && p1->beats < p2->beats) ||
           (p1->bars == p2->bars && p1->beats == p2->beats && p1->sixteenths < p2->sixteenths) ||
           (p1->bars == p2->bars && p1->beats == p2->beats && p1->sixteenths == p2->sixteenths && p1->ticks < p2->ticks))
    return -1;
  else
    return 1;
}

/**
 * Returns closest snap point.
 */
static Position *
closest_snap_point (Position * pos, ///< position
                    Position * p1, ///< snap point 1
                    Position * p2) ///< snap point 2
{
  int frames = position_to_frames (pos);
  int p1_frames = position_to_frames (p1);
  int p2_frames = position_to_frames (p2);
  if (frames - p1_frames <=
      p2_frames - frames)
    {
      return p1;
    }
  else
    {
      return p2;
    }
}

static void
get_prev_snap_point (Position * pos, ///< the position
                     SnapGrid * sg, ///< snap grid options
                     Position * prev_snap_point) ///< position to set
{
  for (int i = sg->num_snap_points - 1; i >= 0; i--)
    {
      Position * snap_point = &sg->snap_points[i];
      if (position_compare (snap_point,
                            pos) <= 0)
        {
          position_set_to_pos (prev_snap_point,
                               snap_point);
          return;
        }
    }
}

static void
get_next_snap_point (Position * pos,
                     SnapGrid *sg,
                     Position * next_snap_point)
{
  for (int i = 0; i < sg->num_snap_points; i++)
    {
      Position * snap_point = &sg->snap_points[i];
      if (position_compare (snap_point,
                            pos) > 0)
        {
          position_set_to_pos (next_snap_point,
                               snap_point);
          return;
        }
    }
}

static void
snap_pos (Position * pos,
          SnapGrid * sg)
{
  Position prev_snap_point;
  get_prev_snap_point (pos, sg, &prev_snap_point);
  Position next_snap_point;
  get_next_snap_point (pos, sg, &next_snap_point);
  Position * csp = closest_snap_point (pos,
                                       &prev_snap_point,
                                       &next_snap_point);
  position_set_to_pos (pos,
                       csp);
}

/**
 * Sets the end position to be 1 snap point away from the start pos.
 *
 * FIXME rename to something more meaningful.
 */
void
position_set_min_size (Position * start_pos,  ///< start position
                       Position * end_pos, ///< position to set
                       SnapGrid * snap) ///< the snap grid
{
  position_set_to_pos (end_pos, start_pos);
  position_add_ticks (
    end_pos,
    snap_grid_get_note_ticks (snap->note_length,
                              snap->note_type));
}

/**
 * Snaps position using given options.
 */
void
position_snap (Position * prev_pos, ///< prev pos
               Position * pos, ///< position moved to
               Track    * track, ///< track at new pos (for region moving)
               Region   * region, ///< region at new pos (for midi moving)
               SnapGrid * sg) ///< options
{
  /* this should only be called if snap is on.
   * the check should be done before calling */
  g_assert (SNAP_GRID_ANY_SNAP (sg));

  if (sg->snap_to_grid && !sg->snap_to_grid_keep_offset &&
      !sg->snap_to_events)
    {
      snap_pos (pos, sg);
    }
  else if (!sg->snap_to_grid && sg->snap_to_grid_keep_offset &&
           !sg->snap_to_events)
    {
      g_assert (prev_pos);
      /* TODO */
      /* get closest snap point to prev_pos */

      /* get diff from closest snap point */

      /* snap pos*/

      /* add diff */
    }
}

/**
 * Converts seconds to position and puts the result in the given Position.
 * TODO
 */
void
position_from_seconds (Position * position, double secs)
{
  /*AUDIO_ENGINE->frames_per_tick;*/
  /*AUDIO_ENGINE->sample_rate;*/

}

int
position_to_ticks (Position * pos)
{
  int ticks = (pos->bars - 1) * TICKS_PER_BAR;
  if (pos->beats)
    ticks += (pos->beats - 1) *
      TICKS_PER_BEAT;
  if (pos->sixteenths)
    ticks += (pos->sixteenths - 1) *
      TICKS_PER_SIXTEENTH_NOTE;
  if (pos->ticks)
    ticks += pos->ticks;
  return ticks;
}

/**
 * Sets position to the given total tick count.
 */
void
position_from_ticks (Position * pos,
                     int        ticks)
{
  pos->bars = ticks / TICKS_PER_BAR + 1;
  ticks = ticks % TICKS_PER_BAR;
  pos->beats = ticks / TICKS_PER_BEAT + 1;
  ticks = ticks % TICKS_PER_BEAT;
  pos->sixteenths = ticks / TICKS_PER_SIXTEENTH_NOTE + 1;
  ticks = ticks % TICKS_PER_SIXTEENTH_NOTE;
  pos->ticks = ticks;
}

/**
 * Calculates the midway point between the two positions and sets it on pos.
 */
void
position_get_midway_pos (Position * start_pos,
                         Position * end_pos,
                         Position * pos) ///< position to set to
{
  int end_ticks, start_ticks, ticks_diff;
  start_ticks = position_to_ticks (start_pos);
  end_ticks = position_to_ticks (end_pos);
  ticks_diff = end_ticks - start_ticks;
  position_set_to_pos (pos, start_pos);
  position_set_tick (pos, ticks_diff / 2);
}

SERIALIZE_SRC (Position, position)
DESERIALIZE_SRC (Position, position)
PRINT_YAML_SRC (Position, position)
