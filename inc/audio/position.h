/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "utils/yaml.h"

#define TICKS_PER_QUARTER_NOTE 960
#define TICKS_PER_SIXTEENTH_NOTE 240
/**
 * Regarding calculation:
 * TICKS_PER_QUARTER_NOTE * 4 to get the ticks per full note.
 * Divide by beat unit (e.g. if beat unit is 2, it means it
 * is a 1/2th note, so multiply 1/2 with the ticks per note
 */
#define TICKS_PER_BEAT \
  ((TICKS_PER_QUARTER_NOTE * 4) / \
   transport_get_beat_unit (TRANSPORT))
#define TICKS_PER_BAR \
  (TICKS_PER_BEAT * TRANSPORT->beats_per_bar)
#define SIXTEENTHS_PER_BEAT \
  (16 / transport_get_beat_unit (TRANSPORT))
#define position_add_ticks(position, _ticks) \
  position_set_tick (position, \
                     (position)->ticks + _ticks)
#define position_add_sixteenths(position, _s) \
  position_set_sixteenth (position, \
                     (position)->sixteenths + _s)
#define position_add_beats(position, _b) \
  position_set_beat (position, \
                     (position)->beats + _b)
#define position_add_bars(position, _b) \
  position_set_bar (position, \
                     (position)->bars + _b)
#define position_snap_simple(pos, sg) \
  position_snap (0, \
                 pos, \
                 0, \
                 0, \
                 sg)

typedef struct SnapGrid SnapGrid;
typedef struct Track Track;
typedef struct Region Region;

typedef struct Position
{
  int       bars; ///< this is the size of the number of beats per bar (top part of time sig)

  /**
   * The size of the beat is the the beat unit (bot part
   * of time sig).
   */
  int       beats;

  /*
   * This is always the size of a 1/16th note regardless
   * of time sig (so if bot part is 16, this will always be
   * 1). This is added for convenience when compared to BBT,
   * so that the user only has 240 ticks to deal with for
   * precise operations instead of 960.
   */
  int       sixteenths;

  int       ticks; ///< 240 ticks per sixteen

  long      frames; ///< position in frames (samples)
} Position;

static const cyaml_schema_field_t
  position_fields_schema[] =
{
	CYAML_FIELD_INT (
    "bars", CYAML_FLAG_DEFAULT,
    Position, bars),
	CYAML_FIELD_INT (
    "beats", CYAML_FLAG_DEFAULT,
    Position, beats),
	CYAML_FIELD_INT (
    "sixteenths", CYAML_FLAG_DEFAULT,
    Position, sixteenths),
	CYAML_FIELD_INT (
    "ticks", CYAML_FLAG_DEFAULT,
    Position, ticks),

	CYAML_FIELD_END
};

static const cyaml_schema_value_t
  position_schema =
{
	CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
    Position, position_fields_schema),
};

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
                  int        bar);

void
position_set_beat (Position * position,
                   int        beat);

void
position_set_sixteenth (Position * position,
                        int        sixteenth);

void
position_set_tick (Position * position,
                   int        tick);

/**
 * Sets position to target position
 */
void
position_set_to_pos (Position * position,
                     Position * target);

void
position_add_frames (Position * position,
                     long       frames);

/**
 * Converts position bars/beats/quarter beats/ticks to frames
 */
long
position_to_frames (Position * position);

/**
 * Converts seconds to position and puts the result in the given Position.
 */
void
position_from_seconds (Position * position,
                       double     secs);

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

int
position_to_ticks (Position * pos);

/**
 * Sets position to the given total tick count.
 */
void
position_from_ticks (Position * pos,
                     long       ticks);

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

/**
 * Creates a string in the form of "0.0.0.0" from
 * the given position.
 *
 * Must be free'd by caller.
 */
char *
position_stringize (Position * pos);

SERIALIZE_INC (Position, position)
DESERIALIZE_INC (Position, position)
PRINT_YAML_INC (Position, position)

#endif
