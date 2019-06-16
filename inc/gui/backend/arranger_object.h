/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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
 * Macros for arranger object backends.
 */

#ifndef __GUI_BACKEND_ARRANGER_OBJECT_H__
#define __GUI_BACKEND_ARRANGER_OBJECT_H__

/**
 * Returns if the object is in the selections.
 */
#define DECLARE_IS_ARRANGER_OBJ_SELECTED( \
  cc,sc) \
  int \
  sc##_is_selected ( \
    cc * self)

#define DEFINE_IS_ARRANGER_OBJ_SELECTED( \
  cc,sc,selections_name,selections) \
  int \
  sc##_is_selected ( \
    cc * self) \
  { \
    return selections_name##_contains_##sc ( \
             selections, \
             sc##_get_main_##sc ( \
                self)); \
  }

/**
 * For set_pos and set_cache_pos.
 */
#define DECLARE_ARRANGER_OBJ_SET_POS(cc,sc) \
  /**
   * Sets the Position.
   *
   * @param trans_only Only do transients.
   */ \
  void \
  sc##_set_pos ( \
    cc *             sc, \
    const Position * _pos, \
    int              trans_only); \
  void \
  sc##_set_cache_pos ( \
    cc *             sc, \
    const Position * _pos) \

#define DEFINE_ARRANGER_OBJ_SET_POS(cc,sc) \
  void \
  sc##_set_pos ( \
    cc *             sc, \
    const Position * _pos, \
    int              trans_only) \
  { \
    SET_POS (sc, pos, _pos, trans_only); \
  } \
  void \
  sc##_set_cache_pos ( \
    cc *             sc, \
    const Position * _pos) \
  { \
    SET_POS (sc, cache_pos, _pos, F_NO_TRANS_ONLY); \
  }

/**
 * For set_start_pos/set_end_pos and
 * set_cache_start_pos/set_cache_end_pos.
 */
#define DECLARE_ARRANGER_OBJ_SET_POSES_W_LENGTH( \
  cc,sc) \
  /**
   * Clamps position then sets it to its counterparts.
   *
   * To be used only when resizing. For moving,
   * use *_move().
   *
   * @param trans_only Only set the transient
   *   Position's.
   * @param validate Validate the Position before
   *   setting.
   */ \
  void \
  sc##_set_start_pos ( \
    cc *             sc, \
    const Position * pos, \
    int              trans_only, \
    int              validate); \
  void \
  sc##_set_end_pos ( \
    cc *             sc, \
    const Position * pos, \
    int              trans_only, \
    int              validate); \
  void \
  sc##_set_cache_start_pos ( \
    cc *             sc, \
    const Position * pos); \
  void \
  sc##_set_cache_end_pos ( \
    cc *             sc, \
    const Position * pos)

/**
 * Definitions.
 *
 * Note: The set_start_pos and set_end_pos are left
 * to the implementation because the validation might
 * differ.
 */
#define DEFINE_ARRANGER_OBJ_SET_POSES_W_LENGTH( \
  cc,sc) \
  void \
  sc##_set_cache_start_pos ( \
    cc *             sc, \
    const Position * pos) \
  { \
    SET_POS (sc, cache_start_pos, pos, \
             F_NO_TRANS_ONLY); \
  } \
  void \
  sc##_set_cache_end_pos ( \
    cc *             sc, \
    const Position * pos) \
  { \
    SET_POS (sc, cache_end_pos, pos, \
             F_NO_TRANS_ONLY); \
  }

#define DECLARE_ARRANGER_OBJ_MOVE(cc,sc) \
  /**
   * Moves the object by the given amount of
   * ticks.
   *
   * @param use_cached_pos Add the ticks to the cached
   *   Position instead of its current Position.
   * @param trans_only Only move transients.
   * @return Whether moved or not.
   */ \
  int \
  sc##_move ( \
    cc * sc, \
    long ticks, \
    int  use_cached_pos, \
    int  trans_only)

#define DEFINE_ARRANGER_OBJ_MOVE(cc,sc) \
  int \
  sc##_move ( \
    cc * sc, \
    long ticks, \
    int  use_cached_pos, \
    int  trans_only) \
  { \
    Position tmp; \
    int moved; \
    POSITION_MOVE_BY_TICKS ( \
      tmp, use_cached_pos, sc, pos, ticks, moved, \
      trans_only); \
    return moved; \
  }


#endif
