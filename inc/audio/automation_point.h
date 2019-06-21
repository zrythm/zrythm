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
 * Automation point API.
 */

#ifndef __AUDIO_AUTOMATION_POINT_H__
#define __AUDIO_AUTOMATION_POINT_H__

#include "audio/position.h"
#include "gui/backend/arranger_object.h"
#include "gui/backend/arranger_object_info.h"

typedef struct AutomationTrack AutomationTrack;
typedef struct _AutomationPointWidget
AutomationPointWidget;

/**
 * @addtogroup audio
 *
 * @{
 */

#define automation_point_is_main(c) \
  arranger_object_info_is_main ( \
    &c->obj_info)

#define automation_point_is_transient(r) \
  arranger_object_info_is_transient ( \
    &r->obj_info)

/** Gets the main counterpart of the AutomationPoint. */
#define automation_point_get_main_automation_point(r) \
  ((AutomationPoint *) r->obj_info.main)

/** Gets the transient counterpart of the AutomationPoint. */
#define automation_point_get_main_trans_automation_point(r) \
  ((AutomationPoint *) r->obj_info.main_trans)

typedef enum AutomationPointCloneFlag
{
  AUTOMATION_POINT_CLONE_COPY_MAIN,
  AUTOMATION_POINT_CLONE_COPY,
} AutomationPointCloneFlag;

/**
 * An automation point living inside an AutomationTrack.
 */
typedef struct AutomationPoint
{
  /** Position in the AutomationTrack. */
  Position           pos;

  /** Cache, used in runtime operations. */
  Position           cache_pos;

  float              fvalue; ///< float value
  int                bvalue; ///< boolean value
  int                svalue; ///< step value

  /**
   * Pointer back to parent.
   */
  int                track_pos;
  int                at_index;
  AutomationTrack *  at;

  /** GUI Widget. */
  AutomationPointWidget *  widget;

  /** Index in the automation track, for faster
   * performance when getting ap before/after
   * curve. */
  int                index;

  /** Object info. */
  ArrangerObjectInfo  obj_info;
} AutomationPoint;

static const cyaml_schema_field_t
automation_point_fields_schema[] =
{
  CYAML_FIELD_MAPPING (
    "pos", CYAML_FLAG_DEFAULT,
    AutomationPoint, pos, position_fields_schema),
	CYAML_FIELD_INT (
    "svalue", CYAML_FLAG_DEFAULT,
    AutomationPoint, svalue),
	CYAML_FIELD_INT (
    "bvalue", CYAML_FLAG_DEFAULT,
    AutomationPoint, bvalue),
	CYAML_FIELD_FLOAT (
    "fvalue", CYAML_FLAG_DEFAULT,
    AutomationPoint, fvalue),
  CYAML_FIELD_INT (
    "index", CYAML_FLAG_DEFAULT,
    AutomationPoint, index),
  CYAML_FIELD_INT (
    "track_pos", CYAML_FLAG_DEFAULT,
    AutomationPoint, track_pos),
  CYAML_FIELD_INT (
    "at_index", CYAML_FLAG_DEFAULT,
    AutomationPoint, at_index),

	CYAML_FIELD_END
};

static const cyaml_schema_value_t
automation_point_schema = {
	CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
    AutomationPoint, automation_point_fields_schema),
};

void
automation_point_init_loaded (
  AutomationPoint * ap);

/**
 * Finds the automation point in the project matching
 * the params of the given one.
 */
AutomationPoint *
automation_point_find (
  AutomationPoint * src);

/**
 * Creates an AutomationPoint in the given
 * AutomationTrack at the given Position.
 *
 * @param is_main Whether this AutomationPoint will
 *   be the main counterpart (see
 *   ArrangerObjectInfo).
 */
AutomationPoint *
automation_point_new_float (
  const float         value,
  const Position *    pos,
  int                 is_main);

int
automation_point_get_y_in_px (AutomationPoint * ap);

/**
 * Updates the value and notifies interested parties.
 *
 * @param trans_only Only do transients.
 */
void
automation_point_update_fvalue (
  AutomationPoint * ap,
  float             fval,
  int               trans_only);

/**
 * Sets the AutomationTrack and the index in the
 * AutomationTrack that the AutomationPoint
 * belongs to, in all its counterparts.
 */
void
automation_point_set_automation_track_and_index (
  AutomationPoint * _ap,
  AutomationTrack * at,
  int               index);

/**
 * Returns the normalized value (0.0 to 1.0).
 */
float
automation_point_get_normalized_value (
  AutomationPoint * ap);

/**
 * Clones the atuomation point.
 */
AutomationPoint *
automation_point_clone (
  AutomationPoint * src,
  AutomationPointCloneFlag flag);

/**
 * Returns the Track this AutomationPoint is in.
 */
Track *
automation_point_get_track (
  AutomationPoint * ap);

ARRANGER_OBJ_DECLARE_GEN_WIDGET (
  AutomationPoint, automation_point);

ARRANGER_OBJ_DECLARE_SHIFT_TICKS (
  AutomationPoint, automation_point);

DECLARE_ARRANGER_OBJ_MOVE (
  AutomationPoint, automation_point);

DECLARE_ARRANGER_OBJ_SET_POS (
  AutomationPoint, automation_point);

DECLARE_IS_ARRANGER_OBJ_SELECTED (
  AutomationPoint, automation_point);

/**
 * Frees the automation point.
 */
void
automation_point_free (AutomationPoint * ap);

/**
 * @}
 */

#endif // __AUDIO_AUTOMATION_POINT_H__
