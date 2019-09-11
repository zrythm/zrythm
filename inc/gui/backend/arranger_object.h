/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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
 * Macros for arranger object backends.
 */

#ifndef __GUI_BACKEND_ARRANGER_OBJECT_H__
#define __GUI_BACKEND_ARRANGER_OBJECT_H__

/**
 * Flag used when updating parameters of objects.
 */
typedef enum ArrangerObjectUpdateFlag
{
  /** Update only this object. */
  AO_UPDATE_THIS,

  /** Update the transients of this object. */
  AO_UPDATE_TRANS,

  /** Update the non-transients of this object. */
  AO_UPDATE_NON_TRANS,

  /** Update all counterparts of this object. */
  AO_UPDATE_ALL,
} ArrangerObjectUpdateFlag;

/**
 * Selects the widget by adding it to the selections.
 *
 * @param cc CamelCase.
 * @param sc snake_case.
 */
#define ARRANGER_OBJ_SELECT( \
  cc,sc,selections_name,selections) \
  cc * main_##sc = \
    sc##_get_main_##sc (sc); \
  if (select) \
    { \
      selections_name##_add_##sc ( \
        selections, \
        main_##sc); \
    } \
  else \
    { \
      selections_name##_remove_##sc ( \
        selections, \
        main_##sc); \
    }

#define ARRANGER_OBJ_DECLARE_SELECT( \
  cc, sc) \
  void \
  sc##_select ( \
    cc * self, \
    int  select)

#define ARRANGER_OBJ_DEFINE_SELECT( \
  cc,sc,selections_name,selections) \
  void \
  sc##_select ( \
    cc * sc, \
    int  select) \
  { \
    ARRANGER_OBJ_SELECT ( \
      cc, sc, selections_name, selections); \
  }

/**
 * Sets the transient's values to the main
 * object's values.
 *
 * @param reset_trans 1 to reset the transient from
 *   main, 0 to reset main from transient.
 */
#define ARRANGER_OBJ_DECLARE_RESET_COUNTERPART( \
  cc, sc) \
  void \
  sc##_reset_counterpart ( \
    cc *       sc, \
    const int  reset_trans)

/**
 * Returns if the object is in the selections.
 */
#define ARRANGER_OBJ_DECLARE_IS_SELECTED( \
  cc,sc) \
  int \
  sc##_is_selected ( \
    cc * self)

#define ARRANGER_OBJ_DEFINE_IS_SELECTED( \
  cc,sc,selections_name,selections) \
  ARRANGER_OBJ_DECLARE_IS_SELECTED (cc, sc) \
  { \
    return selections_name##_contains_##sc ( \
             selections, \
             sc##_get_main_##sc ( \
                self)); \
  }

#define ARRANGER_OBJ_DECLARE_POS_GETTER( \
  cc,sc,pos_name) \
  void \
  sc##_get_##pos_name ( \
    const cc * sc, \
    Position * pos)

#define ARRANGER_OBJ_DEFINE_POS_GETTER( \
  cc,sc,pos_name) \
  ARRANGER_OBJ_DECLARE_POS_GETTER ( \
    cc, sc, pos_name) \
  { \
    position_set_to_pos ( \
      pos, &sc->pos_name); \
  }

/**
 * The setter is for use in e.g. the digital meters
 * whereas the set_pos func is used during
 * arranger actions.
 */
#define ARRANGER_OBJ_DECLARE_POS_SETTER( \
  cc,sc,pos_name) \
  void \
  sc##_##pos_name##_setter ( \
    cc *             sc, \
    const Position * pos)

/**
 * Sets the Position with the given pos_name.
 *
 * It assumes that the position has already been
 * validated.
 */
#define ARRANGER_OBJ_DECLARE_SET_POS( \
  cc,sc,pos_name) \
  void \
  sc##_set_##pos_name ( \
    cc *             sc, \
    const Position * _pos, \
    ArrangerObjectUpdateFlag _update_flag)

#define ARRANGER_OBJ_DEFINE_SET_POS( \
  cc,sc,pos_name) \
  ARRANGER_OBJ_DECLARE_SET_POS (cc, sc, pos_name) \
  { \
    SET_POS (sc, pos_name, _pos, _update_flag); \
  } \

/**
 * Sets the cached Position for use
 * in live operations like moving.
 */
#define \
  ARRANGER_OBJ_DECLARE_SET_CACHE_POS( \
  cc, sc, pos_name) \
  void \
  sc##_set_cache_##pos_name ( \
    cc * sc, \
    const Position * pos) \

#define \
  ARRANGER_OBJ_DEFINE_SET_CACHE_POS( \
  cc, sc, pos_name) \
  ARRANGER_OBJ_DECLARE_SET_CACHE_POS( \
    cc,sc,pos_name) \
  { \
    SET_POS (sc, cache_##pos_name, pos, \
             AO_UPDATE_ALL); \
  }

/**
 * For set_pos and set_cache_pos and the getter /
 * setter.
 */
#define ARRANGER_OBJ_DECLARE_SET_POSES(cc,sc) \
  ARRANGER_OBJ_DECLARE_SET_POS ( \
    cc, sc, pos); \
  ARRANGER_OBJ_DECLARE_SET_CACHE_POS ( \
    cc, sc, pos); \
  ARRANGER_OBJ_DECLARE_POS_GETTER ( \
    cc, sc, pos); \
  ARRANGER_OBJ_DECLARE_POS_SETTER ( \
    cc, sc, pos)

#define ARRANGER_OBJ_DEFINE_SET_POSES(cc,sc) \
  ARRANGER_OBJ_DEFINE_SET_POS ( \
    cc, sc, pos); \
  ARRANGER_OBJ_DEFINE_SET_CACHE_POS ( \
    cc, sc, pos); \
  ARRANGER_OBJ_DEFINE_POS_GETTER ( \
    cc, sc, pos)

/**
 * For set_start_pos/set_end_pos and
 * set_cache_start_pos/set_cache_end_pos.
 */
#define ARRANGER_OBJ_DECLARE_SET_POSES_W_LENGTH( \
  cc,sc) \
  ARRANGER_OBJ_DECLARE_SET_POS ( \
    cc, sc, start_pos); \
  ARRANGER_OBJ_DECLARE_SET_CACHE_POS ( \
    cc, sc, start_pos); \
  ARRANGER_OBJ_DECLARE_POS_GETTER ( \
    cc, sc, start_pos); \
  ARRANGER_OBJ_DECLARE_POS_SETTER ( \
    cc, sc, start_pos); \
  ARRANGER_OBJ_DECLARE_SET_POS ( \
    cc, sc, end_pos); \
  ARRANGER_OBJ_DECLARE_SET_CACHE_POS ( \
    cc, sc, end_pos); \
  ARRANGER_OBJ_DECLARE_POS_GETTER ( \
    cc, sc, end_pos); \
  ARRANGER_OBJ_DECLARE_POS_SETTER ( \
    cc, sc, end_pos)

/**
 * Definitions.
 *
 * Note: The set_start_pos and set_end_pos are left
 * to the implementation because the validation might
 * differ.
 */
#define ARRANGER_OBJ_DEFINE_SET_POSES_W_LENGTH( \
  cc,sc) \
  ARRANGER_OBJ_DEFINE_SET_POS ( \
    cc, sc, start_pos); \
  ARRANGER_OBJ_DEFINE_SET_CACHE_POS ( \
    cc, sc, start_pos); \
  ARRANGER_OBJ_DEFINE_POS_GETTER ( \
    cc, sc, start_pos); \
  ARRANGER_OBJ_DEFINE_SET_POS ( \
    cc, sc, end_pos); \
  ARRANGER_OBJ_DEFINE_SET_CACHE_POS ( \
    cc, sc, end_pos); \
  ARRANGER_OBJ_DEFINE_POS_GETTER ( \
    cc, sc, end_pos)

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


/**
 * Moves the object by the given amount of
 * ticks.
 *
 * @param use_cached_pos Add the ticks to the cached
 *   Position instead of its current Position.
 * @param trans_only Only move transients.
 */
#define ARRANGER_OBJ_DECLARE_MOVE(cc,sc) \
  void \
  sc##_move ( \
    cc * sc, \
    const long ticks, \
    int  use_cached_pos, \
    ArrangerObjectUpdateFlag update_flag)

#define ARRANGER_OBJ_DEFINE_MOVE(cc,sc) \
  ARRANGER_OBJ_DECLARE_MOVE (cc, sc) \
  { \
    Position tmp; \
    POSITION_MOVE_BY_TICKS ( \
      tmp, use_cached_pos, sc, pos, ticks, \
      update_flag); \
  }

/**
 * Moves the object by the given amount of ticks.
 *
 * @param use_cached_pos Add the ticks to the cached
 *   Position instead of its current Position.
 * @param trans_only Only do transients.
 */
#define ARRANGER_OBJ_DECLARE_MOVE_W_LENGTH(cc,sc) \
  ARRANGER_OBJ_DECLARE_MOVE (cc, sc)

#define ARRANGER_OBJ_DEFINE_MOVE_W_LENGTH(cc,sc) \
  ARRANGER_OBJ_DECLARE_MOVE_W_LENGTH (cc, sc) \
  { \
    Position tmp; \
    POSITION_MOVE_BY_TICKS_W_LENGTH ( \
      tmp, use_cached_pos, sc, ticks, \
      update_flag); \
  }

#define ARRANGER_OBJ_DECLARE_SHIFT_TICKS(cc,sc) \
  void \
  sc##_shift_by_ticks ( \
    cc * self, \
    long ticks, \
    ArrangerObjectUpdateFlag update_flag)

/**
 * Shifts an object with a single Position by ticks
 * only.
 */
#define ARRANGER_OBJ_DEFINE_SHIFT_TICKS(cc,sc) \
  ARRANGER_OBJ_DECLARE_SHIFT_TICKS (cc, sc) \
  { \
    Position tmp; \
    POSITION_MOVE_BY_TICKS ( \
      tmp, F_NO_USE_CACHED, self, pos, ticks, \
      update_flag); \
  }

/**
 * Shifts an object with a start and end Position
 * by ticks.
 */
#define ARRANGER_OBJ_DEFINE_SHIFT_TICKS_W_LENGTH( \
  cc,sc) \
  ARRANGER_OBJ_DECLARE_SHIFT_TICKS (cc, sc) \
  { \
    Position tmp; \
    POSITION_MOVE_BY_TICKS ( \
      tmp, F_NO_USE_CACHED, self, start_pos, ticks, \
      update_flag); \
    POSITION_MOVE_BY_TICKS ( \
      tmp, F_NO_USE_CACHED, self, end_pos, ticks, \
      update_flag); \
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

/**
 * Generates a widget for the object.
 *
 * To be used on objects without lane counterparts.
 */
#define ARRANGER_OBJ_DECLARE_GEN_WIDGET(cc,sc) \
  void \
  sc##_gen_widget ( \
    cc * self)

#define ARRANGER_OBJ_DEFINE_GEN_WIDGET_LANELESS( \
  cc,sc) \
  ARRANGER_OBJ_DECLARE_GEN_WIDGET (cc, sc) \
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
 * @param _update_flag Update flag
 */
#define ARRANGER_OBJ_SET_POS( \
  sc, obj, pos_name, pos, _update_flag) \
  switch (_update_flag) \
  { \
  case AO_UPDATE_THIS: \
    position_set_to_pos ( \
      &obj->pos_name, pos); \
    break; \
  case AO_UPDATE_TRANS: \
    position_set_to_pos ( \
      &sc##_get_main_trans_##sc (obj)-> \
      pos_name, pos); \
    break; \
  case AO_UPDATE_NON_TRANS: \
    position_set_to_pos ( \
      &sc##_get_main_##sc (obj)-> \
      pos_name, pos); \
    break; \
  case AO_UPDATE_ALL: \
    position_set_to_pos ( \
      &sc##_get_main_trans_##sc (obj)-> \
      pos_name, pos); \
    position_set_to_pos ( \
      &sc##_get_main_##sc (obj)-> \
      pos_name, pos); \
    break; \
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
 * @param _update_flag ArrangerObjectUpdateFlag.
 */
#define ARRANGER_OBJ_SET_POS_WITH_LANE( \
  sc, obj, pos_name, pos, _update_flag) \
  ARRANGER_OBJ_SET_POS ( \
    sc, obj, pos_name, pos, _update_flag); \
  switch (_update_flag) \
  { \
  case AO_UPDATE_TRANS: \
    position_set_to_pos ( \
      &sc##_get_lane_trans_##sc (obj)-> \
      pos_name, pos); \
    break; \
  case AO_UPDATE_NON_TRANS: \
    position_set_to_pos ( \
      &sc##_get_lane_##sc (obj)-> \
      pos_name, pos); \
    break; \
  case AO_UPDATE_ALL: \
    position_set_to_pos ( \
      &sc##_get_lane_trans_##sc (obj)-> \
      pos_name, pos); \
    position_set_to_pos ( \
      &sc##_get_lane_##sc (obj)-> \
      pos_name, pos); \
    break; \
  default: \
    break; \
  }

/**
 * Frees each object stored in obj_info
 * (declaration).
 *
 * @param sc snake_case.
 * @param cc CamelCase.
 */
#define ARRANGER_OBJ_DECLARE_FREE_ALL_LANELESS( \
  cc, sc) \
  void \
  sc##_free_all (cc * self)

#define ARRANGER_OBJ_DEFINE_FREE_ALL_LANELESS( \
  cc, sc) \
  ARRANGER_OBJ_DECLARE_FREE_ALL_LANELESS (cc, sc) \
  { \
    cc * sc; \
    sc = (cc *) self->obj_info.main; \
    if (sc) \
      sc##_free (sc); \
    sc = (cc *) self->obj_info.main_trans; \
    if (sc) \
      sc##_free (sc); \
    sc = (cc *) self->obj_info.lane; \
    if (sc) \
      sc##_free (sc); \
    sc = (cc *) self->obj_info.lane_trans; \
    if (sc) \
      sc##_free (sc); \
  }

/**
 * Returns the visible counterpart (ie, the
 * transient or the non transient) of the object.
 *
 * Used for example when moving a Region to
 * allocate the MidiNote's based on the transient
 * Region's position instead of the main Region.
 *
 * Only checks the main/main-trans.
 */
#define ARRANGER_OBJ_DECLARE_GET_VISIBLE( \
  cc, sc) \
  cc * \
  sc##_get_visible (cc * self)
#define ARRANGER_OBJ_DEFINE_GET_VISIBLE( \
  cc, sc) \
  ARRANGER_OBJ_DECLARE_GET_VISIBLE (cc, sc) \
  { \
    self = sc##_get_main_##sc (self); \
    if (!arranger_object_info_should_be_visible ( \
          &self->obj_info)) \
      self = sc##_get_main_trans_##sc (self);\
    return self; \
  }

/**
 * Resizes the object on the left side or right
 * side by given amount of ticks, for objects that
 * do not have loops (currently none? keep it as
 * reference).
 *
 * @param left 1 to resize left side, 0 to resize
 *   right side.
 * @param ticks Number of ticks to resize.
 * @param update_flag ArrangerObjectUpdateFlag.
 */
#define ARRANGER_OBJ_DECLARE_RESIZE_NO_LOOP( \
  cc, sc) \
  void \
  sc##_resize ( \
    cc * sc, \
    int  left, \
    long ticks, \
    ArrangerObjectUpdateFlag update_flag)

#define ARRANGER_OBJ_DEFINE_RESIZE_NO_LOOP( \
  cc, sc) \
  ARRANGER_OBJ_DECLARE_RESIZE_NO_LOOP (cc, sc) \
  { \
    Position tmp; \
    if (left) \
      { \
        POSITION_MOVE_BY_TICKS ( \
          tmp, F_NO_USE_CACHED, sc, start_pos, \
          ticks, update_flag); \
      } \
    else \
      { \
        POSITION_MOVE_BY_TICKS ( \
          tmp, F_NO_USE_CACHED, sc, end_pos, \
          ticks, update_flag); \
      } \
  }

/**
 * Declaration for validation of a Position.
 *
 * Returns 1 if valid, 0 if invalid.
 */
#define ARRANGER_OBJ_DECLARE_VALIDATE_POS( \
  cc,sc,pos_name) \
  int \
  sc##_validate_##pos_name ( \
    const cc * sc, \
    const Position * pos)

/**
 * Updates an arranger object's value in all
 * counterparts specified by the update_flag.
 *
 * @param cc CamelCase.
 * @param obj The object.
 * @param val_name The struct member name.
 * @param val_value The value to store.
 * @param update_flag The ArrangerObjectUpdateFlag.
 */
#define ARRANGER_OBJ_SET_PRIMITIVE_VAL( \
  cc,obj,val_name,val_value, update_flag) \
  switch (update_flag) \
    { \
    case AO_UPDATE_THIS: \
      obj->val_name = val_value; \
      break; \
    case AO_UPDATE_TRANS: \
      obj = (cc *) obj->obj_info.main_trans; \
      obj->val_name = val_value; \
      obj = (cc *) obj->obj_info.lane_trans; \
      obj->val_name = val_value; \
      break; \
    case AO_UPDATE_NON_TRANS: \
      obj = (cc *) obj->obj_info.main; \
      obj->val_name = val_value; \
      obj = (cc *) obj->obj_info.lane; \
      obj->val_name = val_value; \
      break; \
    case AO_UPDATE_ALL: \
      obj = (cc *) obj->obj_info.main; \
      obj->val_name = val_value; \
      obj = (cc *) obj->obj_info.lane; \
      obj->val_name = val_value; \
      obj = (cc *) obj->obj_info.main_trans; \
      obj->val_name = val_value; \
      obj = (cc *) obj->obj_info.lane_trans; \
      obj->val_name = val_value; \
      break; \
    }

/**
 * Declares the minimal funcs for a movable arranger
 * object without length, such as ChordObject's and
 * AutomationPoint's.
 */
#define ARRANGER_OBJ_DECLARE_MOVABLE(cc,sc) \
  ARRANGER_OBJ_DECLARE_RESET_COUNTERPART (cc, sc); \
  ARRANGER_OBJ_DECLARE_SELECT (cc, sc); \
  ARRANGER_OBJ_DECLARE_IS_SELECTED (cc, sc); \
  ARRANGER_OBJ_DECLARE_SET_POSES (cc, sc); \
  ARRANGER_OBJ_DECLARE_MOVE (cc, sc); \
  ARRANGER_OBJ_DECLARE_SHIFT_TICKS (cc, sc); \
  ARRANGER_OBJ_DECLARE_GEN_WIDGET (cc, sc); \
  ARRANGER_OBJ_DECLARE_GET_VISIBLE (cc, sc); \
  ARRANGER_OBJ_DECLARE_VALIDATE_POS (cc, sc, pos)

#define ARRANGER_OBJ_DEFINE_MOVABLE( \
  cc,sc,selections_name,selections) \
  ARRANGER_OBJ_DEFINE_SELECT ( \
    cc,sc,selections_name,selections); \
  ARRANGER_OBJ_DEFINE_IS_SELECTED ( \
    cc,sc,selections_name,selections); \
  ARRANGER_OBJ_DEFINE_SET_POSES (cc, sc); \
  ARRANGER_OBJ_DEFINE_MOVE (cc, sc); \
  ARRANGER_OBJ_DEFINE_SHIFT_TICKS (cc, sc); \
  ARRANGER_OBJ_DEFINE_GET_VISIBLE (cc, sc)

/**
 * Declares the minimal funcs for a movable arranger
 * object with length, such as MidiNote's and
 * Region's.
 */
#define ARRANGER_OBJ_DECLARE_MOVABLE_W_LENGTH( \
  cc,sc) \
  ARRANGER_OBJ_DECLARE_RESET_COUNTERPART (cc, sc); \
  ARRANGER_OBJ_DECLARE_SELECT (cc, sc); \
  ARRANGER_OBJ_DECLARE_IS_SELECTED (cc, sc); \
  ARRANGER_OBJ_DECLARE_SET_POSES_W_LENGTH (cc, sc); \
  ARRANGER_OBJ_DECLARE_MOVE_W_LENGTH (cc, sc); \
  ARRANGER_OBJ_DECLARE_SHIFT_TICKS (cc, sc); \
  ARRANGER_OBJ_DECLARE_GEN_WIDGET (cc, sc); \
  ARRANGER_OBJ_DECLARE_GET_VISIBLE (cc, sc); \
  ARRANGER_OBJ_DECLARE_VALIDATE_POS ( \
    cc, sc, start_pos); \
  ARRANGER_OBJ_DECLARE_VALIDATE_POS ( \
    cc, sc, end_pos)

#define ARRANGER_OBJ_DEFINE_MOVABLE_W_LENGTH( \
  cc,sc,selections_name,selections) \
  ARRANGER_OBJ_DEFINE_SELECT ( \
    cc,sc,selections_name,selections); \
  ARRANGER_OBJ_DEFINE_IS_SELECTED ( \
    cc,sc,selections_name,selections); \
  ARRANGER_OBJ_DEFINE_SET_POSES_W_LENGTH (cc, sc); \
  ARRANGER_OBJ_DEFINE_MOVE_W_LENGTH (cc, sc); \
  ARRANGER_OBJ_DEFINE_SHIFT_TICKS_W_LENGTH ( \
    cc, sc); \
  ARRANGER_OBJ_DEFINE_GET_VISIBLE (cc, sc); \

#endif
