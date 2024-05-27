// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Automation point API.
 */

#ifndef __AUDIO_AUTOMATION_POINT_H__
#define __AUDIO_AUTOMATION_POINT_H__

#include "dsp/curve.h"
#include "dsp/position.h"
#include "dsp/region_identifier.h"
#include "gui/backend/arranger_object.h"
#include "utils/types.h"

typedef struct Port                   Port;
typedef struct AutomationTrack        AutomationTrack;
typedef struct _AutomationPointWidget AutomationPointWidget;

/**
 * @addtogroup dsp
 *
 * @{
 */

#define AP_WIDGET_POINT_SIZE 6

#define automation_point_is_selected(r) \
  arranger_object_is_selected ((ArrangerObject *) r)

/**
 * Used for caching.
 */
typedef struct AutomationPointDrawSettings
{
  float        normalized_val;
  CurveOptions curve_opts;
  GdkRectangle draw_rect;
} AutomationPointDrawSettings;

/**
 * An automation point inside an AutomationTrack.
 *
 * @extends ArrangerObject
 */
typedef struct AutomationPoint
{
  /** Base struct. */
  ArrangerObject base;

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

  /** Temporary string used with StringEntryDialogWidget. */
  char * tmp_str;
} AutomationPoint;

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

const char *
automation_point_get_fvalue_as_string (AutomationPoint * self);

void
automation_point_set_fvalue_with_action (
  AutomationPoint * self,
  const char *      fval_str);

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
 * See
 * https://stackoverflow.com/questions/17623152/how-map-tween-a-number-based-on-a-dynamic-curve
 *
 * @param ap The start point (0, 0).
 * @param region region The automation region (if known),
 *   otherwise the non-cached region will be used.
 * @param x Normalized x.
 */
HOT double
automation_point_get_normalized_value_in_curve (
  AutomationPoint * self,
  Region *          region,
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
automation_point_get_port (const AutomationPoint * const self);

/**
 * Convenience function to return the
 * AutomationTrack that this AutomationPoint is in.
 */
AutomationTrack *
automation_point_get_automation_track (const AutomationPoint * const self);

int
automation_point_is_equal (AutomationPoint * a, AutomationPoint * b);

/**
 * Returns if the curve of the AutomationPoint
 * curves upwards as you move right on the x axis.
 */
NONNULL bool
automation_point_curves_up (AutomationPoint * self);

/**
 * @}
 */

#endif // __AUDIO_AUTOMATION_POINT_H__
