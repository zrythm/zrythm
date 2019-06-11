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
#define position_add_ticks(position, _ticks) \
  position_from_ticks ( \
    position, \
    (position)->total_ticks + _ticks)
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
 * Sets the Position by the name of pos_name in
 * all of the object's linked objects (see
 * ArrangerObjectInfo)
 *
 * It assumes that the object has members called
 * *_get_main_trans_*, etc.
 *
 * @param sc snake_case of object's name (e.g.
 *   region).
 * @param _trans_only Only do transients.
 */
#define POSITION_SET_ARRANGER_OBJ_POS( \
  sc, obj, pos_name, pos, _trans_only) \
  if (!_trans_only) \
    { \
      position_set_to_pos ( \
        &sc##_get_main_##sc (obj)-> \
        pos_name, pos); \
    } \
  position_set_to_pos ( \
    &sc##_get_main_trans_##sc (obj)-> \
    pos_name, pos)

/**
 * Sets the Position by the name of pos_name in
 * all of the object's linked objects (see
 * ArrangerObjectInfo)
 *
 * It assumes that the object has members called
 * *_get_main_trans_*, etc.
 *
 * @param sc snake_case of object's name (e.g.
 *   region).
 * @param trans_only Only set transient positions.
 */
#define POSITION_SET_ARRANGER_OBJ_POS_WITH_LANE( \
  sc, obj, pos_name, pos, _trans_only) \
  POSITION_SET_ARRANGER_OBJ_POS ( \
    sc, obj, pos_name, pos, _trans_only); \
  position_set_to_pos ( \
    &sc##_get_lane_trans_##sc (obj)-> \
    pos_name, pos); \
  if (!_trans_only) \
    { \
      position_set_to_pos ( \
        &sc##_get_lane_##sc (obj)-> \
        pos_name, pos); \
    }

/** Start Position to be used in calculations. */
#define DEFINE_START_POS \
  static const Position __start_pos = { \
    .bars = 1, \
    .beats = 1, \
    .sixteenths = 1, \
    .ticks = 0, \
    .total_ticks = 0, \
    .frames = 0 }; \
  static const Position * START_POS = &__start_pos;

/**
 * Moves the Position of an object only has a
 * start position defined by the argument pos_name
 * and a cache position
 * named cache_##pos_name by the given amount of
 * ticks.
 *
 * This also assumes that there is a SET_POS
 * defined. See audio/chord_object.c for an example.
 *
 * This doesn't allow the start position to be
 * less than 1.1.1.0.
 *
 * @param _use_cached An int variable set to 1 for
 *   using the cached positions or 0 for moving the
 *   normal positions.
 * @param _obj The object.
 * @param _pos_name The name of the position.
 * @param _ticks The number of ticks to move by.
 * @param _tmp_pos A Position variable to use for
 *   calculations so we don't create one in the
 *   macro.
 * @param _moved An int variable to hold if the
 *   move was made or not.
 * @param trans_only Move transients only.
 */
#define POSITION_MOVE_BY_TICKS( \
  _tmp_pos,_use_cached,_obj,_pos_name,_ticks, \
  _moved,_trans_only) \
  if (_use_cached) \
    position_set_to_pos ( \
      &_tmp_pos, &_obj->cache_##_pos_name); \
  else \
    position_set_to_pos ( \
      &_tmp_pos, &_obj->_pos_name); \
  position_add_ticks ( \
    &_tmp_pos, _ticks); \
  SET_POS (_obj, _pos_name, &_tmp_pos, \
           _trans_only); \
  moved = 1

/**
 * Moves the Position of an object that has a start
 * and end position named start_pos and end_pos and
 * cached positions named cache_start_pos and
 * cache_end_pos by given amount of ticks.
 *
 * This also assumes that there is a SET_POS
 * defined. See audio/region.c for an example.
 *
 * This doesn't allow the start position to be
 * less than 1.1.1.0.
 *
 * @param _use_cached An int variable set to 1 for
 *   using the cached positions or 0 for moving the
 *   normal positions.
 * @param _obj The object.
 * @param _ticks The number of ticks to move by.
 * @param _tmp_pos A Position variable to use for
 *   calculations so we don't create one in the
 *   macro.
 * @param _moved An int variable to hold if the
 *   move was made or not.
 * @param trans_only Move transients only.
 */
#define POSITION_MOVE_BY_TICKS_W_LENGTH( \
  _tmp_pos,_use_cached,_obj,_ticks,_moved, \
  _trans_only) \
  /* start pos */ \
  POSITION_MOVE_BY_TICKS ( \
    _tmp_pos, _use_cached, _obj, start_pos, \
    _ticks, _moved, _trans_only); \
  /* end pos */ \
  POSITION_MOVE_BY_TICKS ( \
    _tmp_pos, _use_cached, _obj, end_pos, \
    _ticks, _moved, _trans_only); \
  moved = 1

typedef struct SnapGrid SnapGrid;
typedef struct Track Track;
typedef struct Region Region;

/**
 * A Position is made up of
 * bars.beats.sixteenths.ticks.
 */
typedef struct Position
{
  int       bars; ///< this is the size of the number of beats per bar (top part of time sig)

  /**
   * The size of the beat is the the beat unit (bot part
   * of time sig).
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

  /** Cache so we don't need to call
   * position_get_ticks. */
  long      total_ticks;

  /** Position in frames (samples). */
  long      frames;
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
  int        tick);

/**
 * Sets position to target position
 */
void
position_set_to_pos (
  Position * position,
  const Position * target);

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

static inline void
position_from_frames (
  Position * pos,
  long       frames)
{
  position_init (pos);
  position_add_frames (
    pos, frames);
}

/**
 * Compares 2 positions.
 *
 * negative = p1 is earlier
 * 0 = equal
 * positive = p2 is earlier
 */
int
position_compare (
  const Position * p1,
  const Position * p2);

long
position_to_ticks (
  const Position * pos);

/**
 * Sets position to the given total tick count.
 */
void
position_from_ticks (
  Position * pos,
  long       ticks);

/**
 * Snaps position using given options.
 *
 * NOTE: Does not do negative Positions.
 *
 * @param prev_pos Previous Position.
 * @param pos Position moved to.
 * @param track Track at new Position (for Region
 *   moving) FIXME needed?.
 * @param region Region at new Position (for
 *   MidiNote moving) FIXME needed?.
 * @param sg SnapGrid options.
 */
void
position_snap (
  const Position * prev_pos,
  Position * pos,
  Track    * track,
  Region   * region,
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
  Position * start_pos,
  Position * end_pos,
  SnapGrid * snap);

/**
 * Updates frames
 */
void
position_update_frames (Position * position);

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
long
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
position_stringize (
  const Position * pos);

/**
 * Prints the Position in the "0.0.0.0" form.
 */
void
position_print_simple (
  const Position * pos);

SERIALIZE_INC (Position, position)
DESERIALIZE_INC (Position, position)
PRINT_YAML_INC (Position, position)

/**
 * @}
 */

#endif
