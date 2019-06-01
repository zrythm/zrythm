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
#include "gui/backend/arranger_object_info.h"

typedef struct AutomationTrack AutomationTrack;
typedef struct _AutomationPointWidget
AutomationPointWidget;

/**
 * @addtogroup audio
 *
 * @{
 */

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
 * Creates automation point in given track at given Position
 */
AutomationPoint *
automation_point_new_float (
  AutomationTrack *   at,
  float               value,
  Position *          pos);

int
automation_point_get_y_in_px (AutomationPoint * ap);

/**
 * Updates the value and notifies interested parties.
 */
void
automation_point_update_fvalue (
  AutomationPoint * ap,
  float             fval);

/**
 * Returns the normalized value (0.0 to 1.0).
 */
float
automation_point_get_normalized_value (
  AutomationPoint * ap);

/**
 * Returns the Track this AutomationPoint is in.
 */
Track *
automation_point_get_track (
  AutomationPoint * ap);

/**
 * Frees the automation point.
 */
void
automation_point_free (AutomationPoint * ap);

/**
 * @}
 */

#endif // __AUDIO_AUTOMATION_POINT_H__
