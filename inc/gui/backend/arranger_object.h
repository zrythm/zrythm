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

/**
 * Sets the Position pos to the earliest or latest
 * object.
 *
 * @param _array The array of objects to check.
 * @param cc Camel case.
 * @param lc Lower case.
 * @param pos_name Variable name of position to check
 * @param bef_or_aft Before or after
 * @param widg Widget to set.
 */
#define ARRANGER_OBJ_SET_GIVEN_POS_TO(_array,cc,lc,pos_name,transient,bef_or_aft,widg) \
  cc * lc; \
  for (i = 0; i < _array->num_##lc##s; i++) \
    { \
      if (transient) \
        lc = _array->lc##s[i]->obj_info.main_trans; \
      else \
        lc = _array->lc##s[i]->obj_info.main; \
      g_warn_if_fail (lc); \
      if (position_is_##bef_or_aft ( \
            &lc->pos_name, pos)) \
        { \
          position_set_to_pos ( \
            pos, &lc->pos_name); \
          widget = GTK_WIDGET (lc->widget); \
        } \
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

#define ARRANGER_OBJ_DECLARE_SHIFT_TICKS(cc,sc) \
  void \
  sc##_shift_by_ticks ( \
    cc * self, \
    long ticks)

/**
 * Shifts an object with a single Position by ticks
 * only.
 */
#define ARRANGER_OBJ_DEFINE_SHIFT_TICKS(cc,sc) \
  void \
  sc##_shift_by_ticks ( \
    cc * self, \
    long ticks) \
  { \
    if (ticks) \
      { \
        Position tmp; \
        position_set_to_pos ( \
          &tmp, &self->pos); \
        position_add_ticks (&tmp, ticks); \
        SET_POS (self, pos, (&tmp), F_NO_TRANS_ONLY); \
      } \
  }

/**
 * Shifts an object with a start and end Position
 * by ticks.
 */
#define ARRANGER_OBJ_DEFINE_SHIFT_TICKS_W_END_POS( \
  cc,sc) \
  void \
  sc##_shift_by_ticks ( \
    cc * self, \
    long ticks) \
  { \
    if (ticks) \
      { \
        Position tmp; \
        position_set_to_pos ( \
          &tmp, &self->start_pos); \
        position_add_ticks (&tmp, ticks); \
        SET_POS (self, start_pos, (&tmp), \
                 F_NO_TRANS_ONLY); \
        position_set_to_pos ( \
          &tmp, &self->end_pos); \
        position_add_ticks (&tmp, ticks); \
        SET_POS (self, end_pos, (&tmp), \
                 F_NO_TRANS_ONLY); \
      } \
  }

/**
 * Sets the object 'self' as the main object.
 */
#define ARRANGER_OBJECT_SET_AS_MAIN(caps,cc,sc) \
  cc * main_trans = \
    sc##_clone ( \
      self, caps##_CLONE_COPY); \
  cc * lane = \
    sc##_clone ( \
      self, caps##_CLONE_COPY); \
  cc * lane_trans = \
    sc##_clone ( \
      self, caps##_CLONE_COPY); \
  arranger_object_info_init_main ( \
    self, main_trans, lane, lane_trans, \
    AOI_TYPE_##caps)

/** Generates a widget for the object. */
#define ARRANGER_OBJ_DECLARE_GEN_WIDGET(cc,sc) \
  void \
  sc##_gen_widget ( \
    cc * self)

/**
 * Generates a widget for the object.
 *
 * To be used on objects without lane counterparts.
 */
#define ARRANGER_OBJ_DEFINE_GEN_WIDGET_LANELESS( \
  cc,sc) \
  void \
  sc##_gen_widget ( \
    cc * self) \
  { \
    cc * c = self; \
    for (int i = 0; i < 2; i++) \
      { \
        if (i == 0) \
          c = sc##_get_main_##sc (c); \
        else if (i == 1) \
          c = sc##_get_main_trans_##sc (c); \
        c->widget = sc##_widget_new (c); \
      } \
  }

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
#define ARRANGER_OBJ_SET_POS( \
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
#define ARRANGER_OBJ_SET_POS_WITH_LANE( \
  sc, obj, pos_name, pos, _trans_only) \
  ARRANGER_OBJ_SET_POS ( \
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

#endif
