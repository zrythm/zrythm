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

typedef struct SnapGrid SnapGrid;
typedef struct Track Track;
typedef struct Region Region;

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
 * Converts position bars/beats/quarter beats/ticks to frames
 */
int
position_to_frames (Position * position);

/**
 * Notifies other parts.
 */
void
position_updated (Position * position);

/**
 * Converts seconds to position and puts the result in the given Position.
 */
void
position_from_seconds (Position * position, double secs);

/**
 * Compares 2 positions.
 *
 * negative = p1 is earlier
 * 0 = equal
 * positive = p2 is earlier
 */
int
position_compare (Position * p1,
                  Position * p2);

/**
 * For debugging
 */
void
position_print (Position * pos);

int
position_to_ticks (Position * pos);

/**
 * Snaps position using given options.
 */
void
position_snap (Position * prev_pos, ///< prev pos
               Position * pos, ///< position moved to
               Track    * track, ///< track at new pos (for region moving)
               Region   * region, ///< region at new pos (for midi moving)
               SnapGrid * sg); ///< options

/**
 * Sets the end position to be 1 snap point away from the start pos.
 *
 * FIXME rename to something more meaningful.
 */
void
position_set_min_size (Position * start_pos,  ///< start position
                       Position * end_pos, ///< position to set
                       SnapGrid * snap); ///< the snap grid

/**
 * Updates frames
 */
void
position_update_frames (Position * position);

/**
 * Calculates the midway point between the two positions and sets it on pos.
 */
void
position_get_midway_pos (Position * start_pos,
                         Position * end_pos,
                         Position * pos); ///< position to set to

#endif
