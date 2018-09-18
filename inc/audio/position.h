/*
 * audio/postition.h - A position on the timeline
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

#ifndef __AUDIO_POSITION_H__
#define __AUDIO_POSITION_H__

#define TICKS_PER_BAR 3840
#define TICKS_PER_BEAT 960
#define TICKS_PER_QUARTER_BEAT 240

typedef struct Position
{
  int       bars;
  int       beats;
  int       quarter_beats;
  int       ticks;              ///< for now 240 ticks per quarter beat
  int       frames;            ///< position in frames
} Position;

/**
 * Initializes given position to all 0
 */
void
position_init (Position * position);

/**
 * Sets position to given bar
 */
void
position_set_to_bar (Position * position,
                      int        bar_no);

void
position_set_bar (Position * position,
                  int      bar);

void
position_set_beat (Position * position,
                  int      beat);

void
position_set_quarter_beat (Position * position,
                  int      quarter_beat);

void
position_set_tick (Position * position,
                   int      tick);

/**
 * Sets position to target position
 */
void
position_set_to_pos (Position * position,
                     Position * target);

void
position_add_frames (Position * position,
                     int      frames);

/**
 * Notifies other parts.
 */
void
position_updated (Position * position);

#endif
