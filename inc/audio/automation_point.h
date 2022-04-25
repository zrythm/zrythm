// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Automation point API.
 */

#ifndef __AUDIO_AUTOMATION_POINT_H__
#define __AUDIO_AUTOMATION_POINT_H__

#include "audio/curve.h"
#include "audio/position.h"
#include "audio/region_identifier.h"
#include "gui/backend/arranger_object.h"
#include "utils/types.h"

typedef struct Port            Port;
typedef struct AutomationTrack AutomationTrack;
typedef struct _AutomationPointWidget
  AutomationPointWidget;

/**
 * @addtogroup audio
 *
 * @{
 */

#define AUTOMATION_POINT_SCHEMA_VERSION 1

#define AP_WIDGET_POINT_SIZE 6

#define automation_point_is_selected(r) \
  arranger_object_is_selected ( \
    (ArrangerObject *) r)

/**
 * Used for caching.
 */
typedef struct AutomationPointDrawSettings
{
  float        fvalue;
  CurveOptions curve_opts;
  GdkRectangle draw_rect;
} AutomationPointDrawSettings;

/**
 * An automation point inside an AutomationTrack.
 */
typedef struct AutomationPoint
{
  /** Base struct. */
  ArrangerObject base;

  int schema_version;

  /** Float value (real). */
  float fvalue;

  /** Normalized value (0 to 1) used as a cache. */
  float normalized_val;

  CurveOptions curve_opts;

  /** Index in the region. */
  int index;

  /** Last settings used for drawing in editor. */
  AutomationPointDrawSettings last_settings;

  /** Last settings used for drawing in timeline. */
  AutomationPointDrawSettings last_settings_tl;

  /** Cached cairo node to reuse when drawing in the
   * editor. */
  GskRenderNode * cairo_node;

  /** Cached cairo node to reuse when drawing in the
   * timeline. */
  GskRenderNode * cairo_node_tl;
} AutomationPoint;

static const cyaml_schema_field_t
  automation_point_fields_schema[] = {
    YAML_FIELD_MAPPING_EMBEDDED (
      AutomationPoint,
      base,
      arranger_object_fields_schema),
    YAML_FIELD_INT (AutomationPoint, schema_version),
    YAML_FIELD_FLOAT (AutomationPoint, fvalue),
    YAML_FIELD_FLOAT (AutomationPoint, normalized_val),
    YAML_FIELD_INT (AutomationPoint, index),
    YAML_FIELD_MAPPING_EMBEDDED (
      AutomationPoint,
      curve_opts,
      curve_options_fields_schema),

    CYAML_FIELD_END
  };

static const cyaml_schema_value_t
  automation_point_schema = {
    YAML_VALUE_PTR (
      AutomationPoint,
      automation_point_fields_schema),
  };

/**
 * Creates an AutomationPoint in the given
 * AutomationTrack at the given Position.
 */
AutomationPoint *
automation_point_new_float (
  const float      value,
  const float      normalized_val,
  const Position * pos);

/**
 * Sets the value from given real or normalized
 * value and notifies interested parties.
 *
 * @param is_normalized Whether the given value is
 *   normalized.
 */
void
automation_point_set_fvalue (
  AutomationPoint * self,
  float             real_val,
  bool              is_normalized,
  bool              pub_events);

/**
 * Sets the ZRegion and the index in the
 * region that the AutomationPoint
 * belongs to, in all its counterparts.
 */
void
automation_point_set_region_and_index (
  AutomationPoint * _ap,
  ZRegion *         region,
  int               index);

/**
 * The function to return a point on the curve.
 *
 * See https://stackoverflow.com/questions/17623152/how-map-tween-a-number-based-on-a-dynamic-curve
 *
 * @param ap The start point (0, 0).
 * @param x Normalized x.
 */
HOT double
automation_point_get_normalized_value_in_curve (
  AutomationPoint * ap,
  double            x);

/**
 * Sets the curviness of the AutomationPoint.
 */
void
automation_point_set_curviness (
  AutomationPoint * ap,
  const curviness_t curviness);

/**
 * Convenience function to return the control port
 * that this AutomationPoint is for.
 */
Port *
automation_point_get_port (
  const AutomationPoint * const self);

/**
 * Convenience function to return the
 * AutomationTrack that this AutomationPoint is in.
 */
AutomationTrack *
automation_point_get_automation_track (
  const AutomationPoint * const self);

PURE int
automation_point_is_equal (
  AutomationPoint * a,
  AutomationPoint * b);

/**
 * Returns if the curve of the AutomationPoint
 * curves upwards as you move right on the x axis.
 */
NONNULL
bool
automation_point_curves_up (AutomationPoint * self);

/**
 * @}
 */

#endif // __AUDIO_AUTOMATION_POINT_H__
