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
#include "utils/types.h"

typedef struct Automatable Automatable;
typedef struct AutomationTrack AutomationTrack;
typedef struct _AutomationPointWidget
AutomationPointWidget;

/**
 * @addtogroup audio
 *
 * @{
 */

#define AP_WIDGET_POINT_SIZE 6

#define AP_MAX_CURVINESS 1.0
/*#define AP_MIN_CURVINESS \
  //(1.f / AP_MAX_CURVINESS)*/
#define AP_MIN_CURVINESS 0.18
#define AP_CURVINESS_RANGE \
  (AP_MAX_CURVINESS - AP_MIN_CURVINESS)

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
 * The curve algorithm to use for curves.
 */
typedef enum AutomationPointCurveAlgorithm
{
	/** y = x^n */
	AP_CURVE_ALGORITHM_EXPONENT,
	/** y = 1 - (1 - x^n)^(1/n) */
	AP_CURVE_ALGORITHM_SUPERELLIPSE,
} AutomationPointCurveAlgorithm;

/**
 * An automation point inside an AutomationTrack.
 */
typedef struct AutomationPoint
{
  /** Base struct. */
  ArrangerObject  base;

  /** Float value (real). */
  float           fvalue;

	/** Normalized value (0 to 1) used as a cache. */
	float           normalized_val;

  /* @note These are not used at the moment. */
  int             bvalue; ///< boolean value
  int             svalue; ///< step value

	/** Whether the curve tilts upwards. */
	int             curve_up;

  /** Curviness between 0 and 1, regardless of
	 * whether it tilts up or down. */
  double          curviness;

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
	CYAML_FIELD_FLOAT (
    "normalized_val", CYAML_FLAG_DEFAULT,
    AutomationPoint, normalized_val),
  CYAML_FIELD_INT (
    "index", CYAML_FLAG_DEFAULT,
    AutomationPoint, index),
  CYAML_FIELD_INT (
    "curve_up", CYAML_FLAG_DEFAULT,
    AutomationPoint, curve_up),
	CYAML_FIELD_FLOAT (
    "curviness", CYAML_FLAG_DEFAULT,
    AutomationPoint, curviness),
  CYAML_FIELD_STRING_PTR (
    "region_name",
    CYAML_FLAG_OPTIONAL | CYAML_FLAG_POINTER,
    AutomationPoint, region_name,
   	0, CYAML_UNLIMITED),

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
  const float         normalized_val,
  const Position *    pos,
  int                 is_main);

/**
 * Updates the value and notifies interested parties.
 *
 * @param trans_only Only do transients.
 */
void
automation_point_set_fvalue (
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
 * The function to return a point on the curve.
 *
 * See https://stackoverflow.com/questions/17623152/how-map-tween-a-number-based-on-a-dynamic-curve
 *
 * @param ap The start point (0, 0).
 * @param x Normalized x.
 */
double
automation_point_get_normalized_value_in_curve (
  AutomationPoint * ap,
  double            x);

/**
 * Sets the curviness of the AutomationPoint.
 */
void
automation_point_set_curviness (
  AutomationPoint * ap,
  const curviness_t curviness,
	const int         curve_up);

/**
 * Convenience function to return the Automatable
 * that this AutomationPoint is for.
 */
Automatable *
automation_point_get_automatable (
  AutomationPoint * self);

/**
 * Convenience function to return the
 * AutomationTrack that this AutomationPoint is in.
 */
AutomationTrack *
automation_point_get_automation_track (
  AutomationPoint * self);

int
automation_point_is_equal (
  AutomationPoint * a,
  AutomationPoint * b);

/**
 * Returns Y in pixels from the value based on
 * the given height of the parent.
 */
int
automation_point_get_y (
  AutomationPoint * self,
  int               height);

/**
 * @}
 */

#endif // __AUDIO_AUTOMATION_POINT_H__
