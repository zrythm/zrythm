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

/** \file
 */

#include <math.h>

#include "audio/automatable.h"
#include "audio/automation_curve.h"
#include "audio/automation_region.h"
#include "audio/channel.h"
#include "audio/port.h"
#include "audio/position.h"
#include "audio/track.h"
#include "gui/widgets/automation_curve.h"
#include "plugins/lv2_plugin.h"
#include "plugins/plugin.h"
#include "project.h"

void
automation_curve_init_loaded (
  AutomationCurve * ac)
{
  /*ac->at =*/
  /*TODO */
    /*project_get_automation_track (ac->at_id);*/

}

static AutomationCurve *
_create_new (
  Region *         region,
  const Position * pos)
{
  AutomationCurve * self =
    calloc (1, sizeof (AutomationCurve));

  self->region = region;
  position_set_to_pos (&self->pos,
                       pos);

  return self;
}

/**
 * Creates an AutomationCurve.
 *
 * @param region The Region, used to figure out the
 * AutomationCurve type.
 */
AutomationCurve *
automation_curve_new (
  Region *         region,
  const Position * pos)
{
  AutomationCurve * ac =
    _create_new (region, pos);

  ac->curviness = 1.f;
  switch (ac->region->at->automatable->type)
    {
    case AUTOMATABLE_TYPE_PLUGIN_CONTROL:
    case AUTOMATABLE_TYPE_CHANNEL_FADER:
    case AUTOMATABLE_TYPE_CHANNEL_PAN:
      /*TODO check */
      ac->type = AUTOMATION_CURVE_TYPE_FLOAT;
      break;
    case AUTOMATABLE_TYPE_PLUGIN_ENABLED:
    case AUTOMATABLE_TYPE_CHANNEL_MUTE:
      ac->type = AUTOMATION_CURVE_TYPE_BOOL;
      break;
    }
  ac->widget = automation_curve_widget_new (ac);

  return ac;
}

/**
 * Returns Y in pixels from the value based on the
 * allocation of the automation arranger.
 */
int
automation_curve_get_y_in_px (
  AutomationCurve * ac)
{
  AutomationPoint * prev_ap =
    automation_region_get_ap_before_curve (
      ac->region, ac);
  AutomationPoint * next_ap =
    automation_region_get_ap_after_curve (
      ac->region, ac);

  /* ratio of current value in the range */
  float ap_ratio;
  if (ac->curviness >= AP_MID_CURVINESS)
    {
      float ap_curviness_range =
        AP_MAX_CURVINESS - AP_MID_CURVINESS;
      ap_ratio =
        (ac->curviness - AP_MID_CURVINESS) /
        ap_curviness_range;
      ap_ratio *= 0.5f; /* ratio is only for half */
      ap_ratio += 0.5f; /* add the missing half */
    }
  else
    {
      float ap_curviness_range =
        AP_MID_CURVINESS - AP_MIN_CURVINESS;
      ap_ratio =
        (ac->curviness - AP_MIN_CURVINESS) /
        ap_curviness_range;
      ap_ratio *= 0.5f; /* ratio is only for half */
    }
  int prev_ap_y_pos =
    automation_point_get_y_in_px (prev_ap);
  int next_ap_y_pos =
    automation_point_get_y_in_px (next_ap);
  int ap_max = MAX (prev_ap_y_pos, next_ap_y_pos);
  int ap_min = MIN (prev_ap_y_pos, next_ap_y_pos);
  int allocated_h = ap_max - ap_min;
  int point = ap_max - ap_ratio * allocated_h;

  return point;
}

/**
 * TODO add description.
 *
 * See https://stackoverflow.com/questions/17623152/how-map-tween-a-number-based-on-a-dynamic-curve
 * @param x X-coordinate.
 * @param curviness Curviness variable.
 * @param start_at_1 Start at lower point.
 */
double
automation_curve_get_y_normalized (
  double x,
  double curviness,
  int    start_at_1)
{
  if (start_at_1)
    {
      double val = pow (1.0 - pow (x, curviness), (1.0 / curviness));
      return val;
    }
  else
    {
      double val = pow (1.0 - pow (x, 1.0 / curviness), (1.0 / (1.0 / curviness)));
      return 1.0 - val;
    }
}

/**
 * The function to return a point on the curve.
 *
 * See https://stackoverflow.com/questions/17623152/how-map-tween-a-number-based-on-a-dynamic-curve
 *
 * @param ac The start point (0, 0).
 * @param x X-coordinate in px.
 * @param width Total width in px.
 * @param height Total height in px.
 */
double
automation_curve_get_y_px (
  AutomationCurve * ac,
  double            x,
  double            width,
  double            height)
{
  /* find next curve ap & next value ap */
  AutomationPoint * prev_ap =
    automation_region_get_ap_before_curve (
      ac->region, ac);
  AutomationPoint * next_ap =
    automation_region_get_ap_after_curve (
      ac->region, ac);

  double dx = x / width; /* normalized x */
  double dy;
  double ret;

  /* if next point is lower */
  if (automation_point_get_y_in_px (next_ap) >
      automation_point_get_y_in_px (prev_ap))
    {
      /* start higher */
      dy =
        automation_curve_get_y_normalized (
          dx, ac->curviness, 1);
      ret = dy * height;

      /* reverse the value because in pixels
       * higher y values are actually lower */
      return height - ret;
    }
  else
    {
      dy =
        automation_curve_get_y_normalized (
          dx, ac->curviness, 0);
      ret = dy * height;
      return - ret;
    }
}

/**
 * Sets the curviness of the AutomationCurve.
 */
void
automation_curve_set_curviness (
  AutomationCurve * ac,
  float             curviness)
{
  if (ac->curviness == curviness)
    return;

  ac->curviness = curviness;
  if (ac->widget)
    ac->widget->cache = 0;
}

/**
 * Destroys the widget and frees memory.
 */
void
automation_curve_free (AutomationCurve * ac)
{
  if (GTK_IS_WIDGET (ac->widget))
    gtk_widget_destroy (GTK_WIDGET (ac->widget));

  free (ac);
}

