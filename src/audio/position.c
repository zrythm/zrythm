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

#include "audio/engine.h"
#include "audio/position.h"
#include "audio/transport.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_editor.h"
#include "gui/widgets/timeline.h"

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
static int
position_to_frames (Position * position)
{
  int frames = AUDIO_ENGINE->frames_per_tick * position->bars *
    TRANSPORT->beats_per_bar * 4 * TICKS_PER_QUARTER_BEAT;
  if (position->beats)
    frames += AUDIO_ENGINE->frames_per_tick * position->beats *
      4 * TICKS_PER_QUARTER_BEAT;
  if (position->quarter_beats)
    frames += AUDIO_ENGINE->frames_per_tick * position->quarter_beats *
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
  /*if (position->ticks > TICKS_PER_QUARTER_BEAT)*/
    /*{*/
      /*position->ticks %= TICKS_PER_QUARTER_BEAT;*/
      /*if (++position->quarter_beats > 4)*/
        /*{*/
          /*position->quarter_beats = 0;*/
          /*if (++position->beats > TRANSPORT->beats_per_bar)*/
            /*{*/
              /*position->beats = 0;*/
              /*position->bars++;*/
            /*}*/
        /*}*/
    /*}*/
  g_idle_add ((GSourceFunc) position_updated,
              position);
}

/**
 * Notifies other parts.
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
