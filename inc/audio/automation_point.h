/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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
 * Automation point API.
 */

#ifndef __AUDIO_AUTOMATION_POINT_H__
#define __AUDIO_AUTOMATION_POINT_H__

#include "audio/position.h"
#include "gui/backend/arranger_object.h"

typedef struct AutomationTrack AutomationTrack;
typedef struct _AutomationPointWidget
AutomationPointWidget;

/**
 * @addtogroup audio
 *
 * @{
 */

#define automation_point_is_main(c) \
  arranger_object_is_main ( \
    (ArrangerObject *) c)
#define automation_point_is_transient(r) \
  arranger_object_is_transient ( \
    (ArrangerObject *) r)
#define automation_point_get_main(r) \
  ((AutomationPoint *) \
   arranger_object_get_main ( \
     (ArrangerObject *) r))
#define automation_point_get_main_trans(r) \
  ((AutomationPoint *) \
   arranger_object_get_main_trans ( \
     (ArrangerObject *) r))
#define automation_point_is_selected(r) \
  arranger_object_is_selected ( \
    (ArrangerObject *) r)

/**
 * An automation point inside an AutomationTrack.
 */
typedef struct AutomationPoint
{
  /** Base struct. */
  ArrangerObject  base;

  float           fvalue; ///< float value
  int             bvalue; ///< boolean value
  int             svalue; ///< step value

  /**
   * Pointer back to parent.
   */
  Region *        region;

  /** Used in clones to identify a region instead of
   * cloning the whole Region. */
  char *          region_name;

  /** Index in the region, for faster
   * performance when getting ap before/after
   * curve. */
  int             index;
} AutomationPoint;

static const cyaml_schema_field_t
automation_point_fields_schema[] =
{
  CYAML_FIELD_MAPPING (
    "base", CYAML_FLAG_DEFAULT,
    AutomationPoint, base,
    arranger_object_fields_schema),
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

	CYAML_FIELD_END
};

static const cyaml_schema_value_t
automation_point_schema = {
	CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
    AutomationPoint, automation_point_fields_schema),
};

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

/**
 * Updates the value and notifies interested parties.
 *
 * @param trans_only Only do transients.
 */
void
automation_point_update_fvalue (
  AutomationPoint * ap,
  float             fval,
  ArrangerObjectUpdateFlag update_flag);

/**
 * Sets the Region and the index in the
 * region that the AutomationPoint
 * belongs to, in all its counterparts.
 */
void
automation_point_set_region_and_index (
  AutomationPoint * _ap,
  Region *          region,
  int               index);

/**
 * Returns the normalized value (0.0 to 1.0).
 */
float
automation_point_get_normalized_value (
  AutomationPoint * ap);

int
automation_point_is_equal (
  AutomationPoint * a,
  AutomationPoint * b);

/**
 * @}
 */

#endif // __AUDIO_AUTOMATION_POINT_H__
