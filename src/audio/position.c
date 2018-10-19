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
#include "audio/transport.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_editor.h"
#include "project/snap_grid.h"

#include <gtk/gtk.h>

/**
 * Initializes given position to all 0
 */
void
position_init (Position * position)
{
  position->bars = 1;
  position->beats = 1;
  position->quarter_beats = 1;
  position->ticks = 0;
}

/**
 * Converts position bars/beats/quarter beats/ticks to frames
 */
int
position_to_frames (Position * position)
{
  int frames = AUDIO_ENGINE->frames_per_tick * (position->bars - 1) *
    TRANSPORT->beats_per_bar * 4 * TICKS_PER_QUARTER_BEAT;
  if (position->beats)
    frames += AUDIO_ENGINE->frames_per_tick * (position->beats - 1) *
      4 * TICKS_PER_QUARTER_BEAT;
  if (position->quarter_beats)
    frames += AUDIO_ENGINE->frames_per_tick * (position->quarter_beats - 1) *
      TICKS_PER_QUARTER_BEAT;
  if (position->ticks)
    frames += AUDIO_ENGINE->frames_per_tick * position->ticks;
  return frames;
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
  position->quarter_beats = 1;
  position->ticks = 0;
  position->frames = position_to_frames (position);
  g_idle_add ((GSourceFunc) position_updated,
              position);
}

void
position_set_bar (Position * position,
                  int      bar)
{
  if (bar < 1)
    bar = 1;
  position->bars = bar;
  position->frames = position_to_frames (position);
  g_idle_add ((GSourceFunc) position_updated,
              position);
}

void
position_set_beat (Position * position,
                  int      beat)
{
  while (beat < 1 || beat > 4)
    {
      if (beat < 1)
        {
          if (position->bars == 1)
            {
              beat = 1;
              break;
            }
          beat += 4;
          position_set_bar (position,
                            position->bars - 1);
        }
      else if (beat > 4)
        {
          beat -= 4;
          position_set_bar (position,
                            position->bars + 1);
        }
    }
  position->beats = beat;
  position->frames = position_to_frames (position);
  g_idle_add ((GSourceFunc) position_updated,
              position);
}

void
position_set_quarter_beat (Position * position,
                  int      quarter_beat)
{
  while (quarter_beat < 1 || quarter_beat > 4)
    {
      if (quarter_beat < 1)
        {
          if (position->bars == 1 && position->beats == 1)
            {
              quarter_beat = 1;
              break;
            }
          quarter_beat += 4;
          position_set_beat (position,
                             position->beats - 1);
        }
      else if (quarter_beat > 4)
        {
          quarter_beat -= 4;
          position_set_beat (position,
                             position->beats + 1);
        }
    }
  position->quarter_beats = quarter_beat;
  position->frames = position_to_frames (position);
  g_idle_add ((GSourceFunc) position_updated,
              position);
}


void
position_set_tick (Position * position,
                  int      tick)
{
  while (tick < 0 || tick > TICKS_PER_QUARTER_BEAT - 1)
    {
      if (tick < 0)
        {
          if (position->bars == 1 && position->beats == 1 &&
              position->quarter_beats == 1)
            {
              tick = 0;
              break;
            }
          tick += TICKS_PER_QUARTER_BEAT;
          position_set_quarter_beat (position,
                                     position->quarter_beats - 1);
        }
      else if (tick > TICKS_PER_QUARTER_BEAT - 1)
        {
          tick -= TICKS_PER_QUARTER_BEAT;
          position_set_quarter_beat (position,
                                     position->quarter_beats + 1);
        }
    }
  position->ticks = tick;
  position->frames = position_to_frames (position);
  g_idle_add ((GSourceFunc) position_updated,
              position);
}

/**
 * Sets position to target position
 */
void
position_set_to_pos (Position * position,
                     Position * target)
{
  position_set_bar (position, target->bars);
  position_set_beat (position, target->beats);
  position_set_quarter_beat (position, target->quarter_beats);
  position_set_tick (position, target->ticks);
}

void
position_add_frames (Position * position,
                     int      frames)
{
  position->frames += frames;
  position_set_tick (position,
                     position->ticks +
                       frames / AUDIO_ENGINE->frames_per_tick);
  g_idle_add ((GSourceFunc) position_updated,
              position);
}

/**
 * Notifies other parts.
 * FIXME check what the position is first
 */
void
position_updated (Position * position)
{
  if (MAIN_WINDOW)
    {
      if (MAIN_WINDOW->digital_transport)
        {
          gtk_widget_queue_draw (
                  GTK_WIDGET (MAIN_WINDOW->digital_transport));
        }
      if (MAIN_WINDOW->ruler)
        {
          gtk_widget_queue_draw (
                  GTK_WIDGET (MAIN_WINDOW->ruler));
        }
      if (MAIN_WINDOW->timeline)
        {
          gtk_widget_queue_draw (
                  GTK_WIDGET (MAIN_WINDOW->timeline->bg));
        }
      if (MAIN_WINDOW->midi_editor)
        {
          if (MAIN_WINDOW->midi_editor->midi_ruler)
            {
              gtk_widget_queue_draw (
                      GTK_WIDGET (MAIN_WINDOW->midi_editor->midi_ruler));
            }
          if (MAIN_WINDOW->midi_editor->midi_arranger)
            {
              gtk_widget_queue_draw (
                      GTK_WIDGET (MAIN_WINDOW->midi_editor->midi_arranger->bg));
            }
        }
    }
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
      p1->quarter_beats == p2->quarter_beats &&
      p1->ticks == p2->ticks)
    return 0;
  else if ((p1->bars < p2->bars) ||
           (p1->bars == p2->bars && p1->beats < p2->beats) ||
           (p1->bars == p2->bars && p1->beats == p2->beats && p1->quarter_beats < p2->quarter_beats) ||
           (p1->bars == p2->bars && p1->beats == p2->beats && p1->quarter_beats == p2->quarter_beats && p1->ticks < p2->ticks))
    return -1;
  else
    return 1;
}

/**
 * Returns difference in position.
 *
 * 1 if pos > comp_pos,
 * -1 if pos < comp_pos,
 * 0 if equal
 */
/*int*/
/*position_get_diff (Position * pos, ///< Position*/
                   /*Position * comp_pos, ///< pos to compare to*/
                   /*Position * diff) ///< (OUT) difference in Position)*/
/*{*/
  /*int ret = position_compare (pos, comp_pos);*/
  /*diff->bars = 0;*/
  /*diff->beats = 0;*/
  /*diff->quarter_beats = 0;*/
  /*diff->ticks = 0;*/
  /*if (ret < 0)*/
    /*{*/
      /*int frames_diff = position_to_frames (comp_pos) - position_to_frames (pos);*/
      /*position_add_frames (diff, frames_diff);*/
    /*}*/
  /*else if (ret > 0)*/
    /*{*/
      /*int frames_diff = position_to_frames (pos) - position_to_frames (comp_pos);*/
      /*position_add_frames (diff, frames_diff);*/
    /*}*/
  /*return ret;*/
/*}*/

/**
 * Returns closest snap point.
 */
static Position *
closest_snap_point (Position * pos, ///< position
                    Position * p1, ///< snap point 1
                    Position * p2) ///< snap point 2
{
  int frames = position_to_frames (pos);
  if (frames - position_to_frames (p1) <=
      position_to_frames (p2) - frames)
    {
      return p1;
    }
  else
    {
      return p2;
    }
}

static void
snap_pos (Position * pos,
          SnapGrid * sg)
{
  Position prev_snap_point;
  Position next_snap_point;
  prev_snap_point.bars = 1;
  prev_snap_point.beats = 1;
  prev_snap_point.quarter_beats = 1;
  prev_snap_point.ticks = 0;
  next_snap_point.bars = 1;
  next_snap_point.beats = 1;
  next_snap_point.quarter_beats = 1;
  next_snap_point.ticks = 0;
  if (sg->grid_density == 0) /* 1/1 */
    {
      prev_snap_point.bars = pos->bars;
      next_snap_point.bars = pos->bars + 1;
    }
  else if (sg->grid_density == 1) /* 1/2 */
    {
      prev_snap_point.bars = pos->bars;
      if (pos->beats >= 3)
        {
          prev_snap_point.beats = 3;
          next_snap_point.bars = pos->bars + 1;
          next_snap_point.beats = 1;
        }
      else
        {
          prev_snap_point.beats = 1;
          next_snap_point.bars = pos->bars;
          next_snap_point.beats = 3;
        }
    }
  else if (sg->grid_density == 2) /* 1/4 */
    {
      prev_snap_point.bars = pos->bars;
      prev_snap_point.beats = pos->beats;
      position_set_beat (&next_snap_point,
                         pos->beats + 1);
    }
  /*g_message ("pos %d.%d.%d.%d",*/
         /*pos->bars, pos->beats,*/
         /*pos->quarter_beats, pos->ticks);*/
  /*g_message ("prev snap %d.%d.%d.%d",*/
         /*prev_snap_point.bars, prev_snap_point.beats,*/
         /*prev_snap_point.quarter_beats, prev_snap_point.ticks);*/
  /*g_message ("next snap %d.%d.%d.%d",*/
         /*next_snap_point.bars, next_snap_point.beats,*/
         /*next_snap_point.quarter_beats, next_snap_point.ticks);*/
  Position * csp = closest_snap_point (pos,
                                                      &prev_snap_point,
                                                      &next_snap_point);
  /*g_message ("csp %d.%d.%d.%d",*/
         /*csp->bars, csp->beats,*/
         /*csp->quarter_beats, csp->ticks);*/
  position_set_to_pos (pos,
                       csp);
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
  if (sg->snap_to_grid)
    {
      if (sg->snap_keep_offset && prev_pos)
        {
          /* get closest snap point to prev_pos */

          /* get diff from closest snap point */

          /* snap pos*/

          /* add diff */

        }
      else
        {
          /* just snap pos */
          snap_pos (pos, sg);
        }

    }

}
