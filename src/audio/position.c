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

/**
 * Initializes given position to all 0
 */
void
position_init (Position * position)
{
  position->bars = 0;
  position->beats = 0;
  position->quarter_beats = 0;
  position->ticks = 0;
}

static int
bar_to_frames (int bar_no)
{
  return AUDIO_ENGINE->frames_per_tick * bar_no *
    TRANSPORT->beats_per_bar * 4 * TICKS_PER_QUARTER_BEAT;
}

/**
 * Sets position to given bar
 */
void
position_set_to_bar (Position * position,
                      int        bar_no)
{
  position->bars = bar_no;
  position->beats = 0;
  position->quarter_beats = 0;
  position->ticks = 0;
  position->frames = bar_to_frames (bar_no);
}


void
position_add_frames (Position * position,
                     int      frames)
{
  position->frames += frames;
  position->ticks += frames / AUDIO_ENGINE->frames_per_tick;
  if (position->ticks > TICKS_PER_QUARTER_BEAT)
    {
      position->ticks %= TICKS_PER_QUARTER_BEAT;
      if (++position->quarter_beats > 4)
        {
          position->quarter_beats = 0;
          if (++position->beats > TRANSPORT->beats_per_bar)
            {
              position->beats = 0;
              position->bars++;
            }
        }
    }
}
