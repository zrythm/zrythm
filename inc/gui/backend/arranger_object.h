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

#include "audio/position.h"
#include "utils/yaml.h"

typedef struct ArrangerObject ArrangerObject;
typedef struct ArrangerSelections ArrangerSelections;
typedef struct _ArrangerWidget ArrangerWidget;
typedef struct _ArrangerObjectWidget
  ArrangerObjectWidget;

/**
 * @addtogroup gui_backend
 *
 * @{
 */

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
 * The type of the object.
 */
typedef enum ArrangerObjectType
{
  /* These two are not actual object types. */
  ARRANGER_OBJECT_TYPE_NONE,
  ARRANGER_OBJECT_TYPE_ALL,

  ARRANGER_OBJECT_TYPE_REGION,
  ARRANGER_OBJECT_TYPE_MIDI_NOTE,
  ARRANGER_OBJECT_TYPE_CHORD_OBJECT,
  ARRANGER_OBJECT_TYPE_SCALE_OBJECT,
  ARRANGER_OBJECT_TYPE_MARKER,
  ARRANGER_OBJECT_TYPE_AUTOMATION_POINT,
  ARRANGER_OBJECT_TYPE_VELOCITY,
} ArrangerObjectType;

static const cyaml_strval_t
arranger_object_type_strings[] =
{
  { "None",
    ARRANGER_OBJECT_TYPE_NONE },
  { "Region",
    ARRANGER_OBJECT_TYPE_REGION },
  { "MidiNote",
    ARRANGER_OBJECT_TYPE_MIDI_NOTE },
  { "ChordObject",
    ARRANGER_OBJECT_TYPE_CHORD_OBJECT },
  { "ScaleObject",
    ARRANGER_OBJECT_TYPE_SCALE_OBJECT },
  { "Marker",
    ARRANGER_OBJECT_TYPE_MARKER },
  { "AutomationPoint",
    ARRANGER_OBJECT_TYPE_AUTOMATION_POINT },
  { "Velocity",
    ARRANGER_OBJECT_TYPE_VELOCITY },
};

typedef enum ArrangerObjectPositionType
{
  ARRANGER_OBJECT_POSITION_TYPE_START,
  ARRANGER_OBJECT_POSITION_TYPE_END,
  ARRANGER_OBJECT_POSITION_TYPE_CLIP_START,
  ARRANGER_OBJECT_POSITION_TYPE_LOOP_START,
  ARRANGER_OBJECT_POSITION_TYPE_LOOP_END,
} ArrangerObjectPositionType;

typedef enum ArrangerObjectInfoCounterpart
{
  /** This is the main (non-lane) object. */
  AOI_COUNTERPART_MAIN,
  /** This is the transient of the main object. */
  AOI_COUNTERPART_MAIN_TRANSIENT,
  /** This is the lane object. */
  AOI_COUNTERPART_LANE,
  /** This is the lane transient object. */
  AOI_COUNTERPART_LANE_TRANSIENT,
} ArrangerObjectInfoCounterpart;

/**
 * Flag do indicate how to clone the object.
 */
typedef enum ArrangerObjectCloneFlag
{
  /** Create a new Region to be added to a
   * Track as a main Region. */
  ARRANGER_OBJECT_CLONE_COPY_MAIN,

  /** Create a new Region that will not be used
   * as a main Region. */
  ARRANGER_OBJECT_CLONE_COPY,

  /** TODO */
  ARRANGER_OBJECT_CLONE_LINK
} ArrangerObjectCloneFlag;

/**
 * Provides info on whether this object is
 * transient/lane and pointers to transient/lane
 * equivalents.
 *
 * Each arranger object (Region's, MidiNote's, etc.)
 * should have a copy of this struct and indicate
 * all associated objects here. The type is used to
 * determine which object the current one is.
 */
typedef struct ArrangerObjectInfo
{
  /** Main (non-lane) object. */
  ArrangerObject *       main;

  /** Transient of the main object. */
  ArrangerObject *       main_trans;

  /** Lane object. */
  ArrangerObject *       lane;

  /** Lane transient object. */
  ArrangerObject *       lane_trans;

  /** Type of object this is. */
  ArrangerObjectInfoCounterpart counterpart;
} ArrangerObjectInfo;

/**
 * Base struct for arranger objects.
 */
typedef struct ArrangerObject
{
  /** Object info. */
  ArrangerObjectInfo info;

  ArrangerObjectType type;

  /** 1 if the object is allowed to have lanes. */
  int                can_have_lanes;

  /** 1 if the object has a start and end pos */
  int                has_length;

  /** 1 if the object can loop. */
  int                can_loop;

  /** 1 if the object can fade in/out. */
  int                can_fade;

  /** 1 if the start Position is a global (timeline)
   * Position. */
  int                is_pos_global;

  /**
   * Position (or start Position if the object
   * has length).
   *
   * Midway Position between previous and next
   * AutomationPoint's, if AutomationCurve.
   */
  Position           pos;

  /** Cache, used in runtime operations. */
  Position           cache_pos;

  /** End Position, if the object has one. */
  Position           end_pos;

  /** Cache, used in runtime operations. */
  Position           cache_end_pos;

  /**
   * Start position of the clip loop.
   *
   * The first time the region plays it will start
   * playing from the clip_start_pos and then loop
   * to this position.
   */
  Position           clip_start_pos;

  /** Cache, used in runtime operations. */
  Position           cache_clip_start_pos;

  /** Loop start Position, if the object has one. */
  Position           loop_start_pos;

  /** Cache, used in runtime operations. */
  Position           cache_loop_start_pos;

  /**
   * End position of the clip loop.
   *
   * Once this is reached, the clip will go back to
   * the clip  loop start position.
   */
  Position           loop_end_pos;

  /** Cache, used in runtime operations. */
  Position           cache_loop_end_pos;

  /** Fade in position. */
  Position           fade_in_pos;

  /** Cache. */
  Position           cache_fade_in_pos;

  /** Fade in position. */
  Position           fade_out_pos;

  /** Cache. */
  Position           cache_fade_out_pos;

  /** The full rectangle this object was
   * supposed to be drawn in, in absolute
   * coordinates. */
  GdkRectangle       full_rect;

  /** The rectangle this object was last drawn in
   * (ie, after any necessary clipping),
   * in absolute coordinates. */
  GdkRectangle       draw_rect;

  /** Cache text H extents and W extents for
   * the text, if the object has any. */
  int                textw;
  int                texth;

  /** 1 when hovering over the object. */
  int                hover;

  /** Set to 1 to redraw. */
  int                redraw;

  /** Cached drawing. */
  cairo_t *          cached_cr;
  cairo_surface_t *  cached_surface;
} ArrangerObject;

static const cyaml_schema_field_t
arranger_object_fields_schema[] =
{
  CYAML_FIELD_ENUM (
    "type", CYAML_FLAG_DEFAULT,
    ArrangerObject, type,
    arranger_object_type_strings,
    CYAML_ARRAY_LEN (arranger_object_type_strings)),
	CYAML_FIELD_INT (
    "can_have_lanes", CYAML_FLAG_DEFAULT,
    ArrangerObject, can_have_lanes),
	CYAML_FIELD_INT (
    "has_length", CYAML_FLAG_DEFAULT,
    ArrangerObject, has_length),
	CYAML_FIELD_INT (
    "can_loop", CYAML_FLAG_DEFAULT,
    ArrangerObject, can_loop),
  CYAML_FIELD_MAPPING (
    "pos", CYAML_FLAG_DEFAULT,
    ArrangerObject, pos,
    position_fields_schema),
  CYAML_FIELD_MAPPING (
    "end_pos", CYAML_FLAG_DEFAULT,
    ArrangerObject, end_pos,
    position_fields_schema),
  CYAML_FIELD_MAPPING (
    "clip_start_pos", CYAML_FLAG_DEFAULT,
    ArrangerObject, clip_start_pos,
    position_fields_schema),
  CYAML_FIELD_MAPPING (
    "loop_start_pos", CYAML_FLAG_DEFAULT,
    ArrangerObject, loop_start_pos,
    position_fields_schema),
  CYAML_FIELD_MAPPING (
    "loop_end_pos", CYAML_FLAG_DEFAULT,
    ArrangerObject, loop_end_pos,
    position_fields_schema),
  CYAML_FIELD_MAPPING (
    "fade_in_pos", CYAML_FLAG_DEFAULT,
    ArrangerObject, fade_in_pos,
    position_fields_schema),
  CYAML_FIELD_MAPPING (
    "fade_out_pos", CYAML_FLAG_DEFAULT,
    ArrangerObject, fade_out_pos,
    position_fields_schema),

	CYAML_FIELD_END
};

static const cyaml_schema_value_t
arranger_object_schema = {
  CYAML_VALUE_MAPPING (CYAML_FLAG_POINTER,
    ArrangerObject, arranger_object_fields_schema),
};

/**
 * Returns the main ArrangerObject.
 */
#define arranger_object_get_main(self) \
  (((ArrangerObject *) self)->info.main)
#define arranger_object_get_main_trans(self) \
  (((ArrangerObject *) self)->info.main_trans)
#define arranger_object_get_lane(self) \
  (((ArrangerObject *) self)->info.lane)
#define arranger_object_get_lane_trans(self) \
  (((ArrangerObject *) self)->info.lane_trans)

/**
 * Returns if the object type has a length.
 */
#define arranger_object_type_has_length(type) \
  (type == ARRANGER_OBJECT_TYPE_REGION || \
   type == ARRANGER_OBJECT_TYPE_MIDI_NOTE)

#define arranger_object_info_init( \
  _self, _main, _main_trans, _lane, _lane_trans, \
  _type, _counterpart) \
  _arranger_object_info_init ( \
    _self, (ArrangerObject *) _main, \
    (ArrangerObject *) _main_trans, \
    (ArrangerObject *) _lane, \
    (ArrangerObject *) _lane_trans, _type, \
    _counterpart)

/** Initializes each object starting from the
 * main. */
#define arranger_object_info_init_main( \
  _main, _main_trans, _lane, _lane_trans) \
  _arranger_object_info_init ( \
    &_main->info, \
    _main, _main_trans, _lane, _lane_trans, \
    AOI_COUNTERPART_MAIN); \
  _arranger_object_info_init ( \
    &_main_trans->info, \
    _main, _main_trans, _lane, _lane_trans, \
    AOI_COUNTERPART_MAIN_TRANSIENT); \
  _arranger_object_info_init ( \
    &_lane->info, \
    _main, _main_trans, _lane, _lane_trans, \
    AOI_COUNTERPART_LANE); \
  _arranger_object_info_init ( \
    &_lane_trans->info, \
    _main, _main_trans, _lane, _lane_trans, \
    AOI_COUNTERPART_LANE_TRANSIENT)

/**
 * Inits the ArrangerObjectInfo with the given
 * values.
 */
static inline void
_arranger_object_info_init (
  ArrangerObjectInfo * self,
  void * main,
  void * main_trans,
  void * lane,
  void * lane_trans,
  ArrangerObjectInfoCounterpart counterpart)
{
  self->main = main;
  self->main_trans = main_trans;
  self->lane = lane;
  self->lane_trans = lane_trans;
  self->counterpart = counterpart;
}

/**
 * Returns whether the object is transient or not.
 *
 * Transient objects are objects that are used
 * during moving operations.
 */
int
arranger_object_is_transient (
  ArrangerObject * self);

/**
 * Returns whether the object is a lane object or not
 * (only applies to TimelineArrangerWidget objects.
 */
int
arranger_object_is_lane (
  ArrangerObject * self);

/**
 * Returns whether the object is a main object or not
 * (only applies to TimelineArrangerWidget objects.
 */
int
arranger_object_is_main (
  ArrangerObject * self);

/**
 * Gets the arranger for this arranger object.
 */
ArrangerWidget *
arranger_object_get_arranger (
  ArrangerObject * self);

/**
 * If the object is part of a Region, returns it,
 * otherwise returns NULL.
 */
Region *
arranger_object_get_region (
  ArrangerObject * self);

/**
 * Returns a pointer to the name of the object,
 * if the object can have names.
 */
const char *
arranger_object_get_name (
  ArrangerObject * self);

/**
 * Returns if the object represented by the
 * ArrrangerObjectInfo should be visible.
 *
 * This is counterpart-specific, ie. it checks
 * if the transient should be visible or lane
 * counterpart should be visible, etc., based on
 * what is given.
 */
int
arranger_object_should_be_visible (
  ArrangerObject * self);

/**
 * Gets the object the ArrangerObjectInfo
 * represents.
 */
ArrangerObject *
arranger_object_get_object (
  ArrangerObject * self);

/**
 * Gets the given counterpart.
 */
ArrangerObject *
arranger_object_get_counterpart (
  ArrangerObject *              self,
  ArrangerObjectInfoCounterpart counterpart);

/**
 * Initializes the object after loading a Project.
 */
void
arranger_object_init_loaded (
  ArrangerObject * self);

/**
 * Sets the widget visibility and selection state
 * to this counterpart
 * only, or to all counterparts if all is 1.
 */
void
arranger_object_set_widget_visibility_and_state (
  ArrangerObject * self,
  int              all);

/**
 * Returns the ArrangerSelections corresponding
 * to the given object type.
 */
ArrangerSelections *
arranger_object_get_selections_for_type (
  ArrangerObjectType type);

/**
 * Selects the object by adding it to its
 * corresponding selections or making it the
 * only selection.
 *
 * @param select 1 to select, 0 to deselect.
 * @param append 1 to append, 0 to make it the only
 *   selection.
 */
void
arranger_object_select (
  ArrangerObject * self,
  const int        select,
  const int        append);

#define arranger_object_reset_transient(obj) \
  arranger_object_reset_counterparts ( \
    (ArrangerObject *) obj, 1)

/**
 * Returns the number of loops in the ArrangerObject,
 * optionally including incomplete ones.
 */
int
arranger_object_get_num_loops (
  ArrangerObject * self,
  const int        count_incomplete);

/**
 * Sets the transient's values to the main
 * object's values.
 *
 * @param reset_trans 1 to reset the transient from
 *   main, 0 to reset main from transient.
 */
void
arranger_object_reset_counterparts (
  ArrangerObject * self,
  const int        reset_trans);

/**
 * Returns if the object is in the selections.
 */
int
arranger_object_is_selected (
  ArrangerObject * self);

/**
 * Prints debug information about the given
 * object.
 */
void
arranger_object_print (
  ArrangerObject * self);

/**
 * Getter.
 */
void
arranger_object_get_pos (
  const ArrangerObject * self,
  Position *             pos);

/**
 * Getter.
 */
void
arranger_object_get_end_pos (
  const ArrangerObject * self,
  Position *             pos);

/**
 * Getter.
 */
void
arranger_object_get_clip_start_pos (
  const ArrangerObject * self,
  Position *             pos);

/**
 * Getter.
 */
void
arranger_object_get_loop_start_pos (
  const ArrangerObject * self,
  Position *             pos);

/**
 * Getter.
 */
void
arranger_object_get_loop_end_pos (
  const ArrangerObject * self,
  Position *             pos);

/**
 * The setter is for use in e.g. the digital meters
 * whereas the set_pos func is used during
 * arranger actions.
 */
void
arranger_object_pos_setter (
  ArrangerObject * self,
  const Position * pos);

/**
 * The setter is for use in e.g. the digital meters
 * whereas the set_pos func is used during
 * arranger actions.
 */
void
arranger_object_end_pos_setter (
  ArrangerObject * self,
  const Position * pos);

/**
 * The setter is for use in e.g. the digital meters
 * whereas the set_pos func is used during
 * arranger actions.
 */
void
arranger_object_clip_start_pos_setter (
  ArrangerObject * self,
  const Position * pos);

/**
 * The setter is for use in e.g. the digital meters
 * whereas the set_pos func is used during
 * arranger actions.
 */
void
arranger_object_loop_start_pos_setter (
  ArrangerObject * self,
  const Position * pos);

/**
 * The setter is for use in e.g. the digital meters
 * whereas the set_pos func is used during
 * arranger actions.
 */
void
arranger_object_loop_end_pos_setter (
  ArrangerObject * self,
  const Position * pos);

/**
 * Returns if the given Position is valid.
 *
 * @param pos The position to set to.
 * @param pos_type The type of Position to set in the
 *   ArrangerObject.
 * @param cached Set to 1 to validate based on the
 *   cached positions instead of the main ones.
 * @param validate Validate the Position before
 *   setting it.
 */
int
arranger_object_is_position_valid (
  ArrangerObject *           self,
  const Position *           pos,
  ArrangerObjectPositionType pos_type,
  const int                  cached);

/**
 * Sets the Position  all of the object's linked
 * objects (see ArrangerObjectInfo)
 *
 * @param pos The position to set to.
 * @param pos_type The type of Position to set in the
 *   ArrangerObject.
 * @param cached Set to 1 to set the cached positions
 *   instead of the main ones.
 * @param validate Validate the Position before
 *   setting it.
 * @param update_flag Flag to indicate which
 *   counterparts to move.
 */
void
arranger_object_set_position (
  ArrangerObject *           self,
  const Position *           pos,
  ArrangerObjectPositionType pos_type,
  const int                  cached,
  const int                  validate,
  ArrangerObjectUpdateFlag   update_flag);

/**
 * Returns the type as a string.
 */
const char *
arranger_object_stringize_type (
  ArrangerObjectType type);

/**
 * Moves the object by the given amount of
 * ticks.
 *
 * @param use_cached_pos Add the ticks to the cached
 *   Position instead of its current Position.
 * @param update_flag Flag to indicate which
 *   counterparts to move.
 */
void
arranger_object_move (
  ArrangerObject *         self,
  const long               ticks,
  const int                use_cached_pos,
  ArrangerObjectUpdateFlag update_flag);

/**
 * Sets the given object as the main object and
 * clones it into its other counterparts.
 *
 * The type must be set before calling this function.
 */
void
arranger_object_set_as_main (
  ArrangerObject *   self);

/**
 * Returns the length of the ArrangerObject (if
 * it has length) in ticks.
 *
 * (End Position - start Position).
 */
long
arranger_object_get_length_in_ticks (
  ArrangerObject * self);

/**
 * Returns the length of the ArrangerObject (if
 * it has length) in frames.
 *
 * (End Position - start Position).
 */
long
arranger_object_get_length_in_frames (
  ArrangerObject * self);

/**
 * Returns the length of the loop in ticks.
 */
long
arranger_object_get_loop_length_in_ticks (
  ArrangerObject * self);

/**
 * Returns the length of the loop in ticks.
 */
long
arranger_object_get_loop_length_in_frames (
  ArrangerObject * self);

/**
 * Updates the frames of each position in each child
 * recursively.
 */
void
arranger_object_update_frames (
  ArrangerObject * self);

/**
 * Generates a widget for each of the object's
 * counterparts.
 */
//void
//arranger_object_gen_widget (
  //ArrangerObject * self);

/**
 * Frees each object stored in obj_info.
 */
void
arranger_object_free_all (
  ArrangerObject * self);

/**
 * Frees only this object.
 */
void
arranger_object_free (
  ArrangerObject * self);

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
ArrangerObject *
arranger_object_get_visible_counterpart (
  ArrangerObject * self);

/**
 * Resizes the object on the left side or right
 * side by given amount of ticks, for objects that
 * do not have loops (currently none? keep it as
 * reference).
 *
 * @param left 1 to resize left side, 0 to resize
 *   right side.
 * @param ticks Number of ticks to resize.
 * @param loop Whether this is a loop-resize (1) or
 *   a normal resize (0). Only used if the object
 *   can have loops.
 * @param update_flag ArrangerObjectUpdateFlag.
 */
void
arranger_object_resize (
  ArrangerObject * self,
  const int        left,
  const int        loop,
  const long       ticks,
  ArrangerObjectUpdateFlag update_flag);

/**
 * Adds the given ticks to each included object.
 */
void
arranger_object_add_ticks_to_children (
  ArrangerObject * self,
  const long       ticks);

/**
 * Updates an arranger object's value in all
 * counterparts specified by the update_flag.
 *
 * @param cc CamelCase (eg, Region).
 * @param obj The object.
 * @param val_name The struct member name to set
 *   the primitive value to.
 * @param val_value The value to store.
 * @param update_flag The ArrangerObjectUpdateFlag.
 */
#define arranger_object_set_primitive( \
  cc,obj,val_name,val_value,update_flag) \
  { \
    cc * _obj = (cc *) obj; \
    ArrangerObject * ar_obj = \
      (ArrangerObject *) obj; \
    switch (update_flag) \
      { \
      case AO_UPDATE_THIS: \
        _obj->val_name = val_value; \
        break; \
      case AO_UPDATE_TRANS: \
        _obj = \
          (cc *) \
          arranger_object_get_main_trans (ar_obj); \
        _obj->val_name = val_value; \
        _obj = \
          (cc *) \
          arranger_object_get_lane_trans (ar_obj); \
        _obj->val_name = val_value; \
        break; \
      case AO_UPDATE_NON_TRANS: \
        _obj = \
          (cc *) \
          arranger_object_get_main (ar_obj); \
        _obj->val_name = val_value; \
        _obj = \
          (cc *) \
          arranger_object_get_lane (ar_obj); \
        _obj->val_name = val_value; \
        break; \
      case AO_UPDATE_ALL: \
        _obj = \
          (cc *) \
          arranger_object_get_main (ar_obj); \
        _obj->val_name = val_value; \
        _obj = \
          (cc *) \
          arranger_object_get_lane (ar_obj); \
        _obj->val_name = val_value; \
        _obj = \
          (cc *) \
          arranger_object_get_main_trans (ar_obj); \
        _obj->val_name = val_value; \
        _obj = \
          (cc *) \
          arranger_object_get_lane_trans (ar_obj); \
        _obj->val_name = val_value; \
        break; \
      } \
  }

/**
 * Not to be used anywhere else.
 */
#define _ARRANGER_OBJECT_FREE_AND_SET_STRING( \
  _obj,_val_name,_val_value) \
  if (_obj->_val_name) \
    g_free (_obj->_val_name); \
  _obj->_val_name = g_strdup (_val_value)

/**
 * Updates an arranger object's value in all
 * counterparts specified by the update_flag.
 *
 * @param cc CamelCase (eg, Region).
 * @param obj The object.
 * @param val_name The struct member name to set
 *   the primitive value to.
 * @param val_value The value to store.
 * @param update_flag The ArrangerObjectUpdateFlag.
 */
#define arranger_object_set_string( \
  cc,obj,val_name,val_value, update_flag) \
  { \
    cc * _obj = (cc *) obj; \
    ArrangerObject * ar_obj = \
      (ArrangerObject *) obj; \
    switch (update_flag) \
      { \
      case AO_UPDATE_THIS: \
        _ARRANGER_OBJECT_FREE_AND_SET_STRING( \
          _obj,val_name,val_value); \
        break; \
      case AO_UPDATE_TRANS: \
        _obj = \
          (cc *) \
          arranger_object_get_main_trans (ar_obj); \
        _ARRANGER_OBJECT_FREE_AND_SET_STRING( \
          _obj,val_name,val_value); \
        _obj = \
          (cc *) \
          arranger_object_get_lane_trans (ar_obj); \
        _ARRANGER_OBJECT_FREE_AND_SET_STRING( \
          _obj,val_name,val_value); \
        break; \
      case AO_UPDATE_NON_TRANS: \
        _obj = \
          (cc *) \
          arranger_object_get_main (ar_obj); \
        _ARRANGER_OBJECT_FREE_AND_SET_STRING( \
          _obj,val_name,val_value); \
        _obj = \
          (cc *) \
          arranger_object_get_lane (ar_obj); \
        _ARRANGER_OBJECT_FREE_AND_SET_STRING( \
          _obj,val_name,val_value); \
        break; \
      case AO_UPDATE_ALL: \
        _obj = \
          (cc *) \
          arranger_object_get_main (ar_obj); \
        _ARRANGER_OBJECT_FREE_AND_SET_STRING( \
          _obj,val_name,val_value); \
        _obj = \
          (cc *) \
          arranger_object_get_lane (ar_obj); \
        _ARRANGER_OBJECT_FREE_AND_SET_STRING( \
          _obj,val_name,val_value); \
        _obj = \
          (cc *) \
          arranger_object_get_main_trans (ar_obj); \
        _ARRANGER_OBJECT_FREE_AND_SET_STRING( \
          _obj,val_name,val_value); \
        _obj = \
          (cc *) \
          arranger_object_get_lane_trans (ar_obj); \
        _ARRANGER_OBJECT_FREE_AND_SET_STRING( \
          _obj,val_name,val_value); \
        break; \
      } \
  }

/**
 * Returns the Track this ArrangerObject is in.
 */
Track *
arranger_object_get_track (
  ArrangerObject * self);

/**
 * Returns the widget type for the given
 * ArrangerObjectType.
 */
//GType
//arranger_object_get_widget_type_for_type (
  //ArrangerObjectType type);

void
arranger_object_post_deserialize (
  ArrangerObject * self);

/**
 * Validates the given Position.
 *
 * @return 1 if valid, 0 otherwise.
 */
int
arranger_object_validate_pos (
  ArrangerObject *           self,
  const Position *           pos,
  ArrangerObjectPositionType type);

/**
 * Returns the ArrangerObject matching the
 * given one.
 *
 * This should be called when we have a copy or a
 * clone, to get the actual region in the project.
 */
ArrangerObject *
arranger_object_find (
  ArrangerObject * obj);

/**
 * Clone the ArrangerObject.
 *
 * Creates a new region and either links to the
 * original or copies every field.
 */
ArrangerObject *
arranger_object_clone (
  ArrangerObject *        self,
  ArrangerObjectCloneFlag clone_flag);

/**
 * Splits the given object at the given Position,
 * deletes the original object and adds 2 new
 * objects in the same parent (Track or
 * AutomationTrack or Region).
 *
 * The given object must be the main object, as this
 * will create 2 new main objects.
 *
 * @param region The ArrangerObject to split. This
 *   ArrangerObject will be deleted.
 * @param pos The Position to split at.
 * @param pos_is_local If the position is local (1)
 *   or global (0).
 * @param r1 Address to hold the pointer to the
 *   newly created ArrangerObject 1.
 * @param r2 Address to hold the pointer to the
 *   newly created ArrangerObject 2.
 */
void
arranger_object_split (
  ArrangerObject *  self,
  const Position *  pos,
  const int         pos_is_local,
  ArrangerObject ** r1,
  ArrangerObject ** r2);

/**
 * Undoes what arranger_object_split() did.
 */
void
arranger_object_unsplit (
  ArrangerObject *         r1,
  ArrangerObject *         r2,
  ArrangerObject **        region);

/**
 * Sets the name of the object, if the object can
 * have a name.
 */
void
arranger_object_set_name (
  ArrangerObject *         self,
  const char *             name,
  ArrangerObjectUpdateFlag flag);

/**
 * @}
 */

#endif
