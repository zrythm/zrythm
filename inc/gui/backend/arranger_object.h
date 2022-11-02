// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Macros for arranger object backends.
 */

#ifndef __GUI_BACKEND_ARRANGER_OBJECT_H__
#define __GUI_BACKEND_ARRANGER_OBJECT_H__

#include <stdbool.h>

#include "audio/curve.h"
#include "audio/position.h"
#include "audio/region_identifier.h"
#include "utils/yaml.h"

typedef struct ArrangerObject        ArrangerObject;
typedef struct ArrangerSelections    ArrangerSelections;
typedef struct _ArrangerWidget       ArrangerWidget;
typedef struct _ArrangerObjectWidget ArrangerObjectWidget;
typedef struct UndoableAction        UndoableAction;
typedef enum ArrangerSelectionsActionEditType
  ArrangerSelectionsActionEditType;

/**
 * @addtogroup gui_backend
 *
 * @{
 */

#define ARRANGER_OBJECT_SCHEMA_VERSION 1

#define ARRANGER_OBJECT_MAGIC 347616554
#define IS_ARRANGER_OBJECT(tr) \
  (((ArrangerObject *) tr)->magic == ARRANGER_OBJECT_MAGIC \
   && ((ArrangerObject *) tr)->type \
        >= ARRANGER_OBJECT_TYPE_REGION \
   && ((ArrangerObject *) tr)->type \
        <= ARRANGER_OBJECT_TYPE_VELOCITY)
#define IS_ARRANGER_OBJECT_AND_NONNULL(x) \
  (x && IS_ARRANGER_OBJECT (x))

/**
 * Flag used in some functions.
 */
typedef enum ArrangerObjectResizeType
{
  ARRANGER_OBJECT_RESIZE_NORMAL,
  ARRANGER_OBJECT_RESIZE_LOOP,
  ARRANGER_OBJECT_RESIZE_FADE,
  ARRANGER_OBJECT_RESIZE_STRETCH,

  /**
   * Used when we want to resize to contents
   * when BPM changes.
   *
   * Only applies to audio.
   */
  ARRANGER_OBJECT_RESIZE_STRETCH_BPM_CHANGE,
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

static const cyaml_strval_t arranger_object_type_strings[] = {
  {__ ("None"),              ARRANGER_OBJECT_TYPE_NONE        },
  { __ ("All"),              ARRANGER_OBJECT_TYPE_ALL         },
  { __ ("Region"),           ARRANGER_OBJECT_TYPE_REGION      },
  { __ ("Midi Note"),        ARRANGER_OBJECT_TYPE_MIDI_NOTE   },
  { __ ("Chord Object"),     ARRANGER_OBJECT_TYPE_CHORD_OBJECT},
  { __ ("Scale Object"),     ARRANGER_OBJECT_TYPE_SCALE_OBJECT},
  { __ ("Marker"),           ARRANGER_OBJECT_TYPE_MARKER      },
  { __ ("Automation Point"),
   ARRANGER_OBJECT_TYPE_AUTOMATION_POINT                      },
  { __ ("Velocity"),         ARRANGER_OBJECT_TYPE_VELOCITY    },
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

static const cyaml_bitdef_t arranger_object_flags_bitvals[] = {
  {.name = "non_project", .offset = 0, .bits = 1},
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
 * Base struct for arranger objects.
 */
typedef struct ArrangerObject
{
  int schema_version;

  ArrangerObjectType type;

  /** Flags. */
  ArrangerObjectFlags flags;

  /**
   * Position (or start Position if the object
   * has length).
   *
   * For audio/MIDI, the material starts at this
   * frame.
   *
   * Midway Position between previous and next
   * AutomationPoint's, if AutomationCurve.
   */
  Position pos;

  /**
   * End Position, if the object has one.
   *
   * This is exclusive of the material, i.e., the
   * data at this position is not counted (for
   * audio regions at least, TODO check for
   * others).
   */
  Position end_pos;

  /**
   * Start position of the clip loop, relative
   * to the object's start.
   *
   * The first time the region plays it will start
   * playing from the clip_start_pos and then loop
   * to this position.
   */
  Position clip_start_pos;

  /** Loop start Position, if the object has one,
   * relative to the object's start. */
  Position loop_start_pos;

  /**
   * End position of the clip loop, relative to the
   * object's start.
   *
   * Once this is reached, the clip will go back to
   * the clip  loop start position.
   */
  Position loop_end_pos;

  /** Fade in position, relative to the object's
   * start. */
  Position fade_in_pos;

  /** Fade out position, relative to the object's
   * start. */
  Position fade_out_pos;

  /** Fade in curve options. */
  CurveOptions fade_in_opts;

  /** Fade out curve options. */
  CurveOptions fade_out_opts;

  /** The full rectangle this object covers
   * including off-screen parts, in absolute
   * coordinates. */
  GdkRectangle full_rect;

  /** The rectangle this object was last drawn in
   * (ie, after any necessary clipping),
   * in absolute coordinates. */
  //GdkRectangle       draw_rect;

  /** Cache text H extents and W extents for
   * the text, if the object has any. */
  int textw;
  int texth;

  /**
   * A copy ArrangerObject corresponding to this,
   * such as when ctrl+dragging.
   *
   * This will be the clone object saved in the
   * cloned arranger selections in each arranger
   * during actions, and would get drawn separately.
   */
  ArrangerObject * transient;

  /**
   * The opposite of the above. This will be set on
   * the transient objects.
   */
  ArrangerObject * main;

  /** Whether muted or not (if applicable). */
  bool muted;

  int magic;

  /** Parent region identifier for objects that are
   * part of a region. */
  RegionIdentifier region_id;

  /**
   * Object's index in the previous lane (before
   * being moved to a new lane/track).
   *
   * Used at runtime when duplicating objects
   * in new lanes/tracks so we can put the
   * object back to its place before creating
   * new copies.
   *
   * @see arranger_selections_action_do().
   */
  int index_in_prev_lane;

  /**
   * Whether deleted with delete tool.
   *
   * This is used to simply hide these objects
   * until the action finishes so that they can
   * be cloned for the actions.
   */
  bool deleted_temporarily;

  /** 1 when hovering over the object. */
  //int                hover;

  /* ---- The following should only be used for
   * objects that really need caching, such as
   * audio regions ---- */

  /**
   * Set to true to blit the cached surface, false
   * to redraw.
   *
   * @note This is only used if \ref
   *   ArrangerObject.can_cache_drawing is true.
   */
  bool use_cache;

  /**
   * Cached cairo_t.
   *
   * Has 2 elements in case the object needs to
   * draw more than 1 copy (such as lane/track
   * regions).
   *
   * @note This is only used if \ref
   *   ArrangerObject.can_cache_drawing is true.
   */
  cairo_t * cached_cr[2];

  /**
   * Cached surface containing drawing.
   *
   * Has 2 elements in case the object needs to
   * draw more than 1 copy (such as lane/track
   * regions).
   *
   * @note This is only used if \ref
   *   ArrangerObject.can_cache_drawing is true.
   */
  cairo_surface_t * cached_surface[2];

  /** Last drawn name rectangle, if object has a
   * name. */
  GdkRectangle last_name_rect;

  /**
   * Whether part of an auditioner track. */
  bool is_auditioner;
} ArrangerObject;

static const cyaml_schema_field_t arranger_object_fields_schema[] = {
  YAML_FIELD_INT (ArrangerObject, schema_version),
  YAML_FIELD_ENUM (
    ArrangerObject,
    type,
    arranger_object_type_strings),
  CYAML_FIELD_BITFIELD (
    "flags",
    CYAML_FLAG_DEFAULT,
    ArrangerObject,
    flags,
    arranger_object_flags_bitvals,
    CYAML_ARRAY_LEN (arranger_object_flags_bitvals)),
  YAML_FIELD_INT (ArrangerObject, muted),
  YAML_FIELD_MAPPING_EMBEDDED (
    ArrangerObject,
    pos,
    position_fields_schema),
  YAML_FIELD_MAPPING_EMBEDDED (
    ArrangerObject,
    end_pos,
    position_fields_schema),
  YAML_FIELD_MAPPING_EMBEDDED (
    ArrangerObject,
    clip_start_pos,
    position_fields_schema),
  YAML_FIELD_MAPPING_EMBEDDED (
    ArrangerObject,
    loop_start_pos,
    position_fields_schema),
  YAML_FIELD_MAPPING_EMBEDDED (
    ArrangerObject,
    loop_end_pos,
    position_fields_schema),
  YAML_FIELD_MAPPING_EMBEDDED (
    ArrangerObject,
    fade_in_pos,
    position_fields_schema),
  YAML_FIELD_MAPPING_EMBEDDED (
    ArrangerObject,
    fade_out_pos,
    position_fields_schema),
  YAML_FIELD_MAPPING_EMBEDDED (
    ArrangerObject,
    fade_in_opts,
    curve_options_fields_schema),
  YAML_FIELD_MAPPING_EMBEDDED (
    ArrangerObject,
    fade_out_opts,
    curve_options_fields_schema),
  YAML_FIELD_MAPPING_EMBEDDED (
    ArrangerObject,
    region_id,
    region_identifier_fields_schema),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t arranger_object_schema = {
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
    ArrangerObject,
    arranger_object_fields_schema),
};

/**
 * Returns if the object type has a length.
 */
#define arranger_object_type_has_length(type) \
  (type == ARRANGER_OBJECT_TYPE_REGION \
   || type == ARRANGER_OBJECT_TYPE_MIDI_NOTE)

/**
 * Returns if the object type has a global position.
 */
#define arranger_object_type_has_global_pos(type) \
  (type == ARRANGER_OBJECT_TYPE_REGION \
   || type == ARRANGER_OBJECT_TYPE_SCALE_OBJECT \
   || type == ARRANGER_OBJECT_TYPE_MARKER)

#define arranger_object_type_has_name(type) \
  (type == ARRANGER_OBJECT_TYPE_REGION \
   || type == ARRANGER_OBJECT_TYPE_MARKER)

/**
 * Returns if the object is allowed to have lanes.
 */
#define arranger_object_can_have_lanes(_obj) \
  ((_obj)->type == ARRANGER_OBJECT_TYPE_REGION \
   && region_type_has_lane (((ZRegion *) _obj)->id.type))

/** Returns if the object can loop. */
#define arranger_object_type_can_loop(type) \
  (type == ARRANGER_OBJECT_TYPE_REGION)

#define arranger_object_can_fade(_obj) \
  ((_obj)->type == ARRANGER_OBJECT_TYPE_REGION \
   && region_type_can_fade (((ZRegion *) _obj)->id.type))

#define arranger_object_can_mute(_obj) \
  ((_obj)->type == ARRANGER_OBJECT_TYPE_REGION \
   || (_obj)->type == ARRANGER_OBJECT_TYPE_MIDI_NOTE)

#define arranger_object_owned_by_region(_obj) \
  ((_obj)->type == ARRANGER_OBJECT_TYPE_VELOCITY \
   || (_obj)->type == ARRANGER_OBJECT_TYPE_MIDI_NOTE \
   || (_obj)->type == ARRANGER_OBJECT_TYPE_CHORD_OBJECT \
   || (_obj)->type == ARRANGER_OBJECT_TYPE_AUTOMATION_POINT)

/** Whether or not this object supports cached
 * drawing.
 * FIXME off for now. */
#define arranger_object_can_cache_drawing(_obj) (false)

#if 0
  ((_obj)->type == ARRANGER_OBJECT_TYPE_REGION && \
    region_type_can_fade ( \
      ((ZRegion *) _obj)->id.type))
#endif

/**
 * Gets the arranger for this arranger object.
 */
ArrangerWidget *
arranger_object_get_arranger (const ArrangerObject * self);

/**
 * Sets the magic on the arranger object.
 */
void
arranger_object_set_magic (ArrangerObject * self);

/**
 * If the object is part of a ZRegion, returns it,
 * otherwise returns NULL.
 */
HOT ZRegion *
arranger_object_get_region (const ArrangerObject * const self);

/**
 * Returns a pointer to the name of the object,
 * if the object can have names.
 */
const char *
arranger_object_get_name (const ArrangerObject * self);

/**
 * Generates a human readable name for the object.
 *
 * If the object has a name, this returns a copy
 * of the name, otherwise generates something
 * appropriate.
 *
 * Must be free'd by caller.
 */
char *
arranger_object_gen_human_readable_name (
  const ArrangerObject * self);

/**
 * Generates the escaped name for the object,
 * where applicable.
 */
void
arranger_object_gen_escaped_name (const ArrangerObject * self);

/**
 * Sets the dest object's values to the main
 * src object's values.
 */
void
arranger_object_set_to_object (
  ArrangerObject * dest,
  ArrangerObject * src);

/**
 * Gets the object the ArrangerObjectInfo
 * represents.
 */
ArrangerObject *
arranger_object_get_object (ArrangerObject * self);

void
arranger_object_init (ArrangerObject * self);

/**
 * Initializes the object after loading a Project.
 */
void
arranger_object_init_loaded (ArrangerObject * self);

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
  const bool       select,
  const bool       append,
  bool             fire_events);

/**
 * Returns whether the given object is hit by the
 * given position or range.
 *
 * @param start Start position.
 * @param end End position, or NULL to only check
 *   for intersection with \ref start.
 */
NONNULL_ARGS (1)
PURE WARN_UNUSED_RESULT bool
arranger_object_is_hit (
  const ArrangerObject * self,
  const Position *       start,
  const Position *       end);

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
bool
arranger_object_is_selected (ArrangerObject * self);

/**
 * Prints debug information about the given
 * object.
 */
NONNULL
void
arranger_object_print (const ArrangerObject * self);

/**
 * Gets the mute status of the object.
 *
 * @param check_parent Whether to check parent
 *   (parent region or parent track lane if region),
 *   otherwise only whether this object itself is
 *   muted is returned. This will take the solo
 *   status of other lanes if true and if @ref self
 *   is a region that can have lanes.
 */
NONNULL
bool
arranger_object_get_muted (
  ArrangerObject * self,
  bool             check_parent);

/**
 * Sets the mute status of the object.
 */
void
arranger_object_set_muted (
  ArrangerObject * self,
  bool             muted,
  bool             fire_events);

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
 * Getter.
 */
void
arranger_object_get_fade_in_pos (
  const ArrangerObject * self,
  Position *             pos);

/**
 * Getter.
 */
void
arranger_object_get_fade_out_pos (
  const ArrangerObject * self,
  Position *             pos);

void
arranger_object_get_position_from_type (
  const ArrangerObject *     self,
  Position *                 pos,
  ArrangerObjectPositionType type);

/**
 * Callback when beginning to edit the object.
 *
 * This saves a clone of its current state to its
 * arranger.
 */
void
arranger_object_edit_begin (const ArrangerObject * self);

/**
 * Callback when finishing editing the object.
 *
 * This performs an undoable action.
 */
void
arranger_object_edit_finish (
  const ArrangerObject *           self,
  ArrangerSelectionsActionEditType type);

void
arranger_object_edit_position_finish (
  const ArrangerObject * self);

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
 * @param pos_type The type of Position to set in
 *   the ArrangerObject.
 */
HOT NONNULL WARN_UNUSED_RESULT bool
arranger_object_is_position_valid (
  const ArrangerObject * const self,
  const Position *             pos,
  ArrangerObjectPositionType   pos_type);

#define arranger_object_validate_pos \
  arranger_object_is_position_valid

/**
 * Sets the Position  all of the object's linked
 * objects (see ArrangerObjectInfo)
 *
 * @param pos The position to set to.
 * @param pos_type The type of Position to set in the
 *   ArrangerObject.
 * @param validate Validate the Position before
 *   setting it.
 */
void
arranger_object_set_position (
  ArrangerObject *           self,
  const Position *           pos,
  ArrangerObjectPositionType pos_type,
  const int                  validate);

/**
 * Returns the type as a string.
 */
const char *
arranger_object_stringize_type (ArrangerObjectType type);

/**
 * Copies the identifier from src to dest.
 */
void
arranger_object_copy_identifier (
  ArrangerObject * dest,
  ArrangerObject * src);

void
arranger_object_add_linked_region (
  ArrangerObject * self,
  ZRegion *        region);

void
arranger_object_remove_linked_region (
  ArrangerObject * self,
  ZRegion *        region);

/**
 * Moves the object by the given amount of
 * ticks.
 */
void
arranger_object_move (
  ArrangerObject * self,
  const double     ticks);

/**
 * Returns the length of the ArrangerObject (if
 * it has length) in ticks.
 *
 * (End Position - start Position).
 */
NONNULL
WARN_UNUSED_RESULT
static inline double
arranger_object_get_length_in_ticks (
  const ArrangerObject * const self)
{
  g_return_val_if_fail (
    arranger_object_type_has_length (self->type), 0);

  return self->end_pos.ticks - self->pos.ticks;
}

/**
 * Returns the length of the ArrangerObject (if
 * it has length) in frames.
 *
 * (End Position - start Position).
 */
NONNULL
WARN_UNUSED_RESULT
static inline signed_frame_t
arranger_object_get_length_in_frames (
  const ArrangerObject * const self)
{
  g_return_val_if_fail (
    arranger_object_type_has_length (self->type), 0);

  return self->end_pos.frames - self->pos.frames;
}

/**
 * Returns the length of the loop in ticks.
 */
NONNULL
static inline double
arranger_object_get_loop_length_in_ticks (
  const ArrangerObject * const self)
{
  g_return_val_if_fail (
    arranger_object_type_has_length (self->type), 0);

  return self->loop_end_pos.ticks - self->loop_start_pos.ticks;
}

/**
 * Returns the length of the loop in frames.
 */
NONNULL
HOT static inline signed_frame_t
arranger_object_get_loop_length_in_frames (
  const ArrangerObject * const self)
{
  g_return_val_if_fail (
    arranger_object_type_has_length (self->type), 0);

  return self->loop_end_pos.frames - self->loop_start_pos.frames;
}

/**
 * Updates the positions in each child recursively.
 *
 * @param from_ticks Whether to update the
 *   positions based on ticks (true) or frames
 *   (false).
 * @param action To be passed when called from an undoable
 *   action.
 */
void
arranger_object_update_positions (
  ArrangerObject * self,
  bool             from_ticks,
  bool             bpm_change,
  UndoableAction * action);

/**
 * Frees only this object.
 */
void
arranger_object_free (ArrangerObject * self);

/**
 * Resizes the object on the left side or right
 * side by given amount of ticks, for objects that
 * do not have loops (currently none? keep it as
 * reference).
 *
 * @param left 1 to resize left side, 0 to resize
 *   right side.
 * @param ticks Number of ticks to resize.
 * @param during_ui_action Whether this is called
 *   during a UI action (not at the end).
 */
void
arranger_object_resize (
  ArrangerObject *         self,
  const bool               left,
  ArrangerObjectResizeType type,
  const double             ticks,
  bool                     during_ui_action);

void
arranger_object_append_children (
  ArrangerObject * self,
  GPtrArray *      children);

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
  _obj, _val_name, _val_value) \
  g_free_and_null (_obj->_val_name); \
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
  cc, obj, val_name, val_value) \
  { \
    cc * _obj = (cc *) obj; \
    _ARRANGER_OBJECT_FREE_AND_SET_STRING ( \
      _obj, val_name, val_value); \
  }

/**
 * Returns the Track this ArrangerObject is in.
 */
HOT NONNULL Track *
arranger_object_get_track (const ArrangerObject * const self);

static inline const char *
arranger_object_get_type_as_string (ArrangerObjectType type)
{
  return arranger_object_type_strings[type].str;
}

void
arranger_object_post_deserialize (ArrangerObject * self);

/**
 * Validates the arranger object.
 *
 * @return True if valid.
 */
bool
arranger_object_validate (const ArrangerObject * const self);

/**
 * Validates the given name.
 *
 * @return True if valid, false otherwise.
 */
bool
arranger_object_validate_name (
  ArrangerObject * self,
  const char *     name);

/**
 * Returns the ArrangerObject matching the
 * given one.
 *
 * This should be called when we have a copy or a
 * clone, to get the actual region in the project.
 */
ArrangerObject *
arranger_object_find (ArrangerObject * obj);

/**
 * Clones the ArrangerObject.
 */
ArrangerObject *
arranger_object_clone (const ArrangerObject * self);

/**
 * Splits the given object at the given Position.
 *
 * if \ref is_project is true, it deletes the original object
 * and adds 2 new objects in the same parent (Track or
 * AutomationTrack or Region).
 *
 * @param region The ArrangerObject to split. This
 *   ArrangerObject will be deleted.
 * @param pos The Position to split at.
 * @param pos_is_local If the position is local (1) or global
 *   (0).
 * @param r1 Address to hold the pointer to the newly created
 *   ArrangerObject 1.
 * @param r2 Address to hold the pointer to the newly created
 *   ArrangerObject 2.
 * @param is_project Whether the object being passed is a
 *   project object. If true, it will be removed from the
 *   project and the child objects will be added to the
 *   project, otherwise it will be untouched and the children
 *   will be mere clones.
 */
void
arranger_object_split (
  ArrangerObject *  self,
  const Position *  pos,
  const bool        pos_is_local,
  ArrangerObject ** r1,
  ArrangerObject ** r2,
  bool              is_project);

/**
 * Undoes what arranger_object_split() did.
 */
NONNULL_ARGS (1, 2)
void
arranger_object_unsplit (
  ArrangerObject *  r1,
  ArrangerObject *  r2,
  ArrangerObject ** obj,
  bool              fire_events);

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
 * Changes the name and adds an action to the
 * undo stack.
 *
 * Calls arranger_object_set_name() internally.
 */
void
arranger_object_set_name_with_action (
  ArrangerObject * self,
  const char *     name);

/**
 * Sets the end position of the ArrangerObject and
 * also sets the loop end and fade out so that
 * they are at the end.
 */
void
arranger_object_set_start_pos_full_size (
  ArrangerObject * obj,
  Position *       pos);

/**
 * Sets the end position of the ArrangerObject and
 * also sets the loop end and fade out to that
 * position.
 */
void
arranger_object_set_end_pos_full_size (
  ArrangerObject * obj,
  Position *       pos);

/**
 * Appends the ArrangerObject to where it belongs
 * in the project (eg, a Track), without taking
 * into account its previous index (eg, before
 * deletion if undoing).
 */
NONNULL
void
arranger_object_add_to_project (
  ArrangerObject * obj,
  bool             fire_events);

/**
 * Inserts the ArrangerObject where it belongs in
 * the project (eg, a Track).
 *
 * This function assumes that the object already
 * knows the index where it should be inserted
 * in its parent.
 *
 * This is mostly used when undoing.
 */
void
arranger_object_insert_to_project (ArrangerObject * obj);

/**
 * Removes the object from its parent in the
 * project.
 */
void
arranger_object_remove_from_project (ArrangerObject * obj);

/**
 * Returns whether the arranger object is part of
 * a frozen track.
 */
bool
arranger_object_is_frozen (ArrangerObject * obj);

/**
 * Returns whether the given object is deletable
 * or not (eg, start marker).
 */
bool
arranger_object_is_deletable (const ArrangerObject * obj);

bool
arranger_object_is_renamable (const ArrangerObject * obj);

/**
 * Removes the child from the given object.
 */
void
arranger_object_remove_child (
  ArrangerObject * self,
  ArrangerObject * child);

/**
 * @}
 */

#endif
