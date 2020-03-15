/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/curve.h"
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

#define ARRANGER_OBJECT_MAGIC 347616554
#define IS_ARRANGER_OBJECT(tr) \
  (tr && \
   ((ArrangerObject *) tr)->magic == \
     ARRANGER_OBJECT_MAGIC)

/**
 * Flag used in some functions.
 */
typedef enum ArrangerObjectResizeType
{
  ARRANGER_OBJECT_RESIZE_NORMAL,
  ARRANGER_OBJECT_RESIZE_LOOP,
  ARRANGER_OBJECT_RESIZE_FADE,
} ArrangerObjectResizeType;

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

/**
 * ArrangerObject flags.
 */
typedef enum ArrangerObjectFlags
{
  /** This object is not a project object, but an
   * object used temporarily eg. when undoing/
   * redoing. */
  ARRANGER_OBJECT_FLAG_NON_PROJECT = 1 << 0,

} ArrangerObjectFlags;

static const cyaml_bitdef_t
arranger_object_flags_bitvals[] =
{
  { .name = "non_project", .offset =  0, .bits =  1 },
};

typedef enum ArrangerObjectPositionType
{
  ARRANGER_OBJECT_POSITION_TYPE_START,
  ARRANGER_OBJECT_POSITION_TYPE_END,
  ARRANGER_OBJECT_POSITION_TYPE_CLIP_START,
  ARRANGER_OBJECT_POSITION_TYPE_LOOP_START,
  ARRANGER_OBJECT_POSITION_TYPE_LOOP_END,
  ARRANGER_OBJECT_POSITION_TYPE_FADE_IN,
  ARRANGER_OBJECT_POSITION_TYPE_FADE_OUT,
} ArrangerObjectPositionType;

/**
 * Flag do indicate how to clone the object.
 */
typedef enum ArrangerObjectCloneFlag
{
  /** Create a new ZRegion to be added to a
   * Track as a main Region. */
  ARRANGER_OBJECT_CLONE_COPY_MAIN,

  /** Create a new ZRegion that will not be used
   * as a main Region. */
  ARRANGER_OBJECT_CLONE_COPY,

  /** TODO */
  ARRANGER_OBJECT_CLONE_LINK
} ArrangerObjectCloneFlag;

/**
 * Base struct for arranger objects.
 */
typedef struct ArrangerObject
{
  ArrangerObjectType type;

  /** Flags. */
  ArrangerObjectFlags  flags;

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

  /** Fade in curve options. */
  CurveOptions       fade_in_opts;

  /** Fade out curve options. */
  CurveOptions       fade_out_opts;

  /** The full rectangle this object covers
   * including off-screen parts, in absolute
   * coordinates. */
  GdkRectangle       full_rect;

  /** The rectangle this object was last drawn in
   * (ie, after any necessary clipping),
   * in absolute coordinates. */
  //GdkRectangle       draw_rect;

  /** Cache text H extents and W extents for
   * the text, if the object has any. */
  int                textw;
  int                texth;

  /**
   * A copy ArrangerObject corresponding to this,
   * such as when ctrl+dragging.
   *
   * This will be the clone object saved in the
   * cloned arranger selections in each arranger
   * during actions, and would get drawn separately.
   */
  ArrangerObject *   transient;

  /**
   * The opposite of the above. This will be set on
   * the transient objects.
   */
  ArrangerObject *   main;

  int                magic;

  /** 1 when hovering over the object. */
  //int                hover;

  /** Set to 1 to redraw. */
  //int                redraw;

  /** Cached drawing. */
  //cairo_t *          cached_cr;
  //cairo_surface_t *  cached_surface;
} ArrangerObject;

static const cyaml_schema_field_t
  arranger_object_fields_schema[] =
{
  YAML_FIELD_ENUM (
    ArrangerObject, type,
    arranger_object_type_strings),
  CYAML_FIELD_BITFIELD (
    "flags", CYAML_FLAG_DEFAULT,
    ArrangerObject, flags,
    arranger_object_flags_bitvals,
    CYAML_ARRAY_LEN (
      arranger_object_flags_bitvals)),
  YAML_FIELD_MAPPING_EMBEDDED (
    ArrangerObject, pos,
    position_fields_schema),
  YAML_FIELD_MAPPING_EMBEDDED (
    ArrangerObject, end_pos,
    position_fields_schema),
  YAML_FIELD_MAPPING_EMBEDDED (
    ArrangerObject, clip_start_pos,
    position_fields_schema),
  YAML_FIELD_MAPPING_EMBEDDED (
    ArrangerObject, loop_start_pos,
    position_fields_schema),
  YAML_FIELD_MAPPING_EMBEDDED (
    ArrangerObject, loop_end_pos,
    position_fields_schema),
  YAML_FIELD_MAPPING_EMBEDDED (
    ArrangerObject, fade_in_pos,
    position_fields_schema),
  YAML_FIELD_MAPPING_EMBEDDED (
    ArrangerObject, fade_out_pos,
    position_fields_schema),
  YAML_FIELD_MAPPING_EMBEDDED (
    ArrangerObject, fade_in_opts,
    curve_options_fields_schema),
  YAML_FIELD_MAPPING_EMBEDDED (
    ArrangerObject, fade_out_opts,
    curve_options_fields_schema),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
  arranger_object_schema =
{
  CYAML_VALUE_MAPPING (CYAML_FLAG_POINTER,
    ArrangerObject, arranger_object_fields_schema),
};

/**
 * Returns if the object type has a length.
 */
#define arranger_object_type_has_length(type) \
  (type == ARRANGER_OBJECT_TYPE_REGION || \
   type == ARRANGER_OBJECT_TYPE_MIDI_NOTE)

/**
 * Returns if the object type has a global position.
 */
#define arranger_object_type_has_global_pos(type) \
  (type == ARRANGER_OBJECT_TYPE_REGION || \
   type == ARRANGER_OBJECT_TYPE_SCALE_OBJECT || \
   type == ARRANGER_OBJECT_TYPE_MARKER)

/**
 * Returns if the object is allowed to have lanes.
 */
#define arranger_object_can_have_lanes(_obj) \
  ((_obj)->type == ARRANGER_OBJECT_TYPE_REGION && \
    region_type_has_lane ( \
      ((ZRegion *) _obj)->id.type))

/** Returns if the object can loop. */
#define arranger_object_type_can_loop(type) \
  (type == ARRANGER_OBJECT_TYPE_REGION)

#define arranger_object_can_fade(_obj) \
  ((_obj)->type == ARRANGER_OBJECT_TYPE_REGION && \
    region_type_can_fade ( \
      ((ZRegion *) _obj)->id.type))

/**
 * Gets the arranger for this arranger object.
 */
ArrangerWidget *
arranger_object_get_arranger (
  ArrangerObject * self);

/**
 * If the object is part of a ZRegion, returns it,
 * otherwise returns NULL.
 */
ZRegion *
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
 * Sets the dest object's values to the main
 * src object's values.
 */
void
arranger_object_set_to_object (
  ArrangerObject * dest,
  ArrangerObject * src);

/**
 * Returns if the lane counterpart should be visible.
 */
int
arranger_object_should_lane_be_visible (
  ArrangerObject * self);

/**
 * Returns if the cached object should be visible, ie,
 * while copy- moving (ctrl+drag) we want to show both
 * the object at its original position and the current
 * object.
 *
 * This refers to the object at its original position.
 */
int
arranger_object_should_orig_be_visible (
  ArrangerObject * self);

/**
 * Gets the object the ArrangerObjectInfo
 * represents.
 */
ArrangerObject *
arranger_object_get_object (
  ArrangerObject * self);

void
arranger_object_init (
  ArrangerObject * self);

/**
 * Initializes the object after loading a Project.
 */
void
arranger_object_init_loaded (
  ArrangerObject * self);

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

/**
 * Returns the number of loops in the ArrangerObject,
 * optionally including incomplete ones.
 */
int
arranger_object_get_num_loops (
  ArrangerObject * self,
  const int        count_incomplete);

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
 */
void
arranger_object_set_position (
  ArrangerObject *           self,
  const Position *           pos,
  ArrangerObjectPositionType pos_type,
  const int                  cached,
  const int                  validate);

/**
 * Returns the type as a string.
 */
const char *
arranger_object_stringize_type (
  ArrangerObjectType type);

/**
 * Copies the identifier from src to dest.
 */
void
arranger_object_copy_identifier (
  ArrangerObject * dest,
  ArrangerObject * src);

/**
 * Moves the object by the given amount of
 * ticks.
 *
 * @param use_cached_pos Add the ticks to the cached
 *   Position instead of its current Position.
 */
void
arranger_object_move (
  ArrangerObject *         self,
  const double             ticks,
  const int                use_cached_pos);

/**
 * Returns the length of the ArrangerObject (if
 * it has length) in ticks.
 *
 * (End Position - start Position).
 */
double
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
double
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
 * Frees only this object.
 */
void
arranger_object_free (
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
 */
void
arranger_object_resize (
  ArrangerObject *         self,
  const int                left,
  ArrangerObjectResizeType type,
  const double             ticks);

/**
 * Adds the given ticks to each included object.
 */
void
arranger_object_add_ticks_to_children (
  ArrangerObject * self,
  const double     ticks);

/**
 * Not to be used anywhere besides below.
 */
#define _ARRANGER_OBJECT_FREE_AND_SET_STRING( \
  _obj,_val_name,_val_value) \
  if (_obj->_val_name) \
    g_free (_obj->_val_name); \
  _obj->_val_name = g_strdup (_val_value)

/**
 * Updates an arranger object's string value.
 *
 * @param cc CamelCase (eg, Region).
 * @param obj The object.
 * @param val_name The struct member name to set
 *   the primitive value to.
 * @param val_value The value to store.
 */
#define arranger_object_set_string( \
  cc,obj,val_name,val_value) \
  { \
    cc * _obj = (cc *) obj; \
    _ARRANGER_OBJECT_FREE_AND_SET_STRING( \
      _obj,val_name,val_value); \
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
  ArrangerObject * self,
  const char *     name,
  int              fire_events);

/**
 * @}
 */

#endif
