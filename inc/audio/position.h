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

/**
 * \file
 *
 * Position struct and API.
 */

#ifndef __AUDIO_POSITION_H__
#define __AUDIO_POSITION_H__

#include "utils/yaml.h"

/**
 * @addtogroup audio
 *
 * @{
 */

#define TICKS_PER_QUARTER_NOTE 960
#define TICKS_PER_SIXTEENTH_NOTE 240
#define TICKS_PER_QUARTER_NOTE_DBL 960.0
#define TICKS_PER_SIXTEENTH_NOTE_DBL 240.0
#define position_add_ticks(_pos, _ticks) \
  position_from_ticks ( \
    (_pos), \
    (_pos)->total_ticks + _ticks)
#define position_add_sixteenths(_pos, _s) \
  position_set_sixteenth ( \
    _pos, (_pos)->sixteenths + _s)
#define position_add_beats(_pos, _b) \
  position_set_beat ( \
    _pos, (_pos)->beats + _b)
#define position_add_bars(_pos, _b) \
  position_set_bar ( \
    _pos, (_pos)->bars + _b)
#define position_snap_simple(pos, sg) \
  position_snap (0, pos, 0, 0, sg)

/**
 * Compares 2 positions based on their frames.
 *
 * negative = p1 is earlier
 * 0 = equal
 * positive = p2 is earlier
 */
#define position_compare(p1,p2) \
  ((p1)->frames - (p2)->frames)

/** Checks if _pos is before _cmp. */
#define position_is_before(_pos,_cmp) \
  (position_compare (_pos, _cmp) < 0)

/** Checks if _pos is before or equal to _cmp. */
#define position_is_before_or_equal(_pos,_cmp) \
  (position_compare (_pos, _cmp) <= 0)

/** Checks if _pos is equal to _cmp. */
#define position_is_equal(_pos,_cmp) \
  (position_compare (_pos, _cmp) == 0)

/** Checks if _pos is after _cmp. */
#define position_is_after(_pos,_cmp) \
  (position_compare (_pos, _cmp) > 0)

/** Checks if _pos is after or equal to _cmp. */
#define position_is_after_or_equal(_pos,_cmp) \
  (position_compare (_pos, _cmp) >= 0)

/**
 * Compares 2 positions based on their total ticks.
 *
 * negative = p1 is earlier
 * 0 = equal
 * positive = p2 is earlier
 */
#define position_compare_ticks(p1,p2) \
  ((p1)->total_ticks - (p2)->total_ticks)

#define position_is_equal_ticks(p1,p2) \
  (fabs (position_compare_ticks (p1, p2)) <= \
     DBL_EPSILON)

/** Returns if _pos is after or equal to _start and
 * before _end. */
#define position_is_between(_pos,_start,_end) \
  (position_is_after_or_equal (_pos, _start) && \
   position_is_before (_pos, _end))

/** Inits the default position on the stack. */
#define POSITION_INIT_ON_STACK(name) \
  Position name = POSITION_START;

/**
 * Initializes given position.
 *
 * Assumes the given argument is a Pointer *.
 */
#define position_init(__pos) \
  *(__pos) = POSITION_START

typedef struct SnapGrid SnapGrid;
typedef struct Track Track;
typedef struct ZRegion ZRegion;

/**
 * A Position is made up of
 * bars.beats.sixteenths.ticks.
 */
typedef struct Position
{
  /* This is the size of the number of beats per
   * bar (top part of time sig) */
  int       bars;

  /**
   * The size of the beat is the the beat unit (bot
   * part of time sig).
   */
  int       beats;

  /**
   * This is always the size of a 1/16th note
   * regardless of time sig (so if bot part is 16,
   * this will always be 1).
   *
   * This is added for convenience when compared to
   * BBT, so that the user only has 240 ticks to
   * deal with for precise operations instead of 960.
   */
  int       sixteenths;

  /** 240 ticks per sixteenth. */
  int       ticks;

  /** 0.0 to 1.0 for precision. */
  double    sub_tick;

  /** Cache so we don't need to call
   * position_get_ticks. */
  double    total_ticks;

  /** Position in frames (samples). */
  long      frames;
} Position;

static const cyaml_schema_field_t
  position_fields_schema[] =
{
  YAML_FIELD_INT (
    Position, bars),
  YAML_FIELD_INT (
    Position, beats),
  YAML_FIELD_INT (
    Position, sixteenths),
  YAML_FIELD_INT (
    Position, ticks),
  YAML_FIELD_FLOAT (
    Position, sub_tick),
  YAML_FIELD_FLOAT (
    Position, total_ticks),
  YAML_FIELD_INT (
    Position, frames),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
  position_schema =
{
  YAML_VALUE_PTR (
    Position, position_fields_schema),
};

/** Start Position to be used in calculations. */
static const Position POSITION_START =
{
  .bars = 1,
  .beats = 1,
  .sixteenths = 1,
  .ticks = 0,
  .total_ticks = 0,
  .sub_tick = 0.0,
  .frames = 0
};

/**
 * Sets position to given bar
 */
void
position_set_to_bar (
  Position * position,
  int        bar_no);

void
position_set_bar (
  Position * position,
  int        bar);

void
position_set_beat (
  Position * position,
  int        beat);

void
position_set_sixteenth (
  Position * position,
  int        sixteenth);

/**
 * Sets the tick of the Position.
 *
 * If the tick exceeds the max ticks allowed on
 * the positive or negative axis, it calls
 * position_set_sixteenth until it is within range./
 *
 * This function can handle both positive and
 * negative Positions. Negative positions start at
 * -1.-1.-1.-1 (one tick before zero) and positive
 * Positions start at zero (1.1.1.0).
 */
void
position_set_tick (
  Position * position,
  double     tick);

/**
 * Sorts an array of Position's.
 */
void
position_sort_array (
  Position *   array,
  const size_t size);

/**
 * Sets position to target position
 *
 * Assumes each position is Position *.
 */
#define position_set_to_pos(_pos,_target) \
  *(_pos) = *(_target)

/**
 * Adds the frames to the position and updates
 * the rest of the fields, and makes sure the
 * frames are still accurate.
 */
void
position_add_frames (
  Position * pos,
  const long frames);

/**
 * Converts position bars/beats/quarter beats/ticks to frames
 */
long
position_to_frames (
  const Position * position);

/**
 * Converts seconds to position and puts the result
 * in the given Position.
 */
void
position_from_seconds (
  Position * position,
  double     secs);

static inline void
position_from_frames (
  Position * pos,
  long       frames)
{
  position_init (pos);
  position_add_frames (pos, frames);
}

/**
 * Returns the Position in milliseconds.
 */
long
position_to_ms (
  const Position * pos);

long
position_ms_to_frames (
  long ms);

void
position_add_ms (
  Position * pos,
  long       ms);

void
position_add_minutes (
  Position * pos,
  int        mins);

void
position_add_seconds (
  Position * pos,
  long       seconds);

double
position_to_ticks (
  const Position * pos);

/**
 * Sets position to the given total tick count.
 */
void
position_from_ticks (
  Position * pos,
  double     ticks);

/**
 * Snaps position using given options.
 *
 * NOTE: Does not do negative Positions.
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
  const SnapGrid * sg);

/**
 * Sets the end position to be 1 snap point away
 * from the start pos.
 *
 * FIXME rename to something more meaningful.
 *
 * @param start_pos Start Position.
 * @param end_pos End Position.
 * @param snap SnapGrid.
 */
void
position_set_min_size (
  const Position * start_pos,
  Position * end_pos,
  SnapGrid * snap);

/**
 * Updates frames
 */
void
position_update_ticks_and_frames (
  Position * position);

/**
 * Calculates the midway point between the two
 * Positions and sets it on pos.
 *
 * @param pos Position to set to.
 */
void
position_get_midway_pos (
  Position * start_pos,
  Position * end_pos,
  Position * pos);

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
  const SnapGrid * sg);

/**
 * Creates a string in the form of "0.0.0.0" from
 * the given position.
 *
 * Must be free'd by caller.
 */
char *
position_stringize_allocate (
  const Position * pos);

/**
 * Creates a string in the form of "0.0.0.0" from
 * the given position.
 */
void
position_stringize (
  const Position * pos,
  char *           buf);

/**
 * Prints the Position in the "0.0.0.0" form.
 */
void
position_print (
  const Position * pos);

/**
 * Returns the total number of bars not including
 * the current one.
 */
int
position_get_total_bars (
  const Position * pos);

/**
 * Returns the total number of beats not including
 * the current one.
 */
int
position_get_total_beats (
  const Position * pos);

/**
 * Changes the sign of the position.
 *
 * For example, 4.2.1.21 would become -4.2.1.21.
 */
void
position_change_sign (
  Position * pos);

SERIALIZE_INC (Position, position)
DESERIALIZE_INC (Position, position)
PRINT_YAML_INC (Position, position)

/**
 * @}
 */

#endif
