/*
 * audio/automation_point.c - Automation point
 *
 * Copyright (C) 2018 Alexandros Theodotou
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

/** \file
 */

#include <math.h>

#include "audio/automatable.h"
#include "audio/automation_point.h"
#include "audio/automation_track.h"
#include "audio/channel.h"
#include "audio/port.h"
#include "audio/position.h"
#include "audio/track.h"
#include "gui/widgets/automation_point.h"
#include "gui/widgets/automation_track.h"
#include "plugins/lv2_plugin.h"
#include "plugins/plugin.h"
#include "project.h"

static AutomationPoint *
_create_new (AutomationTrack * at,
             Position *        pos)
{
  AutomationPoint * ap = calloc (1, sizeof (AutomationPoint));

  ap->at = at;
  position_set_to_pos (&ap->pos,
                       pos);
  ap->id = PROJECT->num_automation_points;
  PROJECT->automation_points[PROJECT->num_automation_points++] = ap;

  return ap;
}

/**
 * Creates automation point in given track at given Position
 */
AutomationPoint *
automation_point_new_float (AutomationTrack *   at,
                            AutomationPointType type,
                            float               value,
                            Position *          pos)
{
  AutomationPoint * ap = _create_new (at, pos);

  ap->fvalue = value;
  ap->type = AUTOMATION_POINT_VALUE;
  ap->widget = automation_point_widget_new (ap);

  return ap;
}

/**
 * Creates curviness point in given track at given Position
 */
AutomationPoint *
automation_point_new_curve (AutomationTrack *   at,
                            Position *          pos)
{
  AutomationPoint * ap = _create_new (at, pos);

  ap->curviness = 2.f;
  ap->type = AUTOMATION_POINT_CURVE;
  ap->widget = automation_point_widget_new (ap);

  return ap;
}

/**
 * Returns Y in pixels from the value based on the allocation of the
 * automation track.
 */
int
automation_point_get_y_in_px (AutomationPoint * ap)
{
  if (ap->type == AUTOMATION_POINT_VALUE)
    {
      Automatable * a = ap->at->automatable;
      float ap_max = automatable_get_maxf (a);
      float ap_range = ap_max - automatable_get_minf (a);

      /* ratio of current value in the range */
      float ap_ratio = (ap_range - (ap_max - ap->fvalue)) / ap_range;

      int allocated_h =
        gtk_widget_get_allocated_height (
          GTK_WIDGET (ap->at->widget->at_grid));
      int point = allocated_h - ap_ratio * allocated_h;
      return point;
    }
  else
    {
      AutomationPoint * prev_ap = automation_track_get_prev_ap (ap->at,
                                                                ap);
      AutomationPoint * next_ap = automation_track_get_next_ap (ap->at,
                                                                ap);
      /* ratio of current value in the range */
      float ap_ratio;
      if (ap->curviness >= AP_MID_CURVINESS)
        {
          float ap_curviness_range = AP_MAX_CURVINESS - AP_MID_CURVINESS;
          ap_ratio = (ap->curviness - AP_MID_CURVINESS) / ap_curviness_range;
          ap_ratio *= 0.5f; /* ratio is only for half */
          ap_ratio += 0.5f; /* add the missing half */
        }
      else
        {
          float ap_curviness_range = AP_MID_CURVINESS - AP_MIN_CURVINESS;
          ap_ratio = (ap->curviness - AP_MIN_CURVINESS) / ap_curviness_range;
          ap_ratio *= 0.5f; /* ratio is only for half */
        }
      int prev_ap_y_pos = automation_point_get_y_in_px (prev_ap);
      int next_ap_y_pos = automation_point_get_y_in_px (next_ap);
      int ap_max = MAX (prev_ap_y_pos, next_ap_y_pos);
      int ap_min = MIN (prev_ap_y_pos, next_ap_y_pos);
      int allocated_h = ap_max - ap_min;
      int point = ap_max - ap_ratio * allocated_h;
      return point;
    }
}

/**
 * The function.
 *
 * See https://stackoverflow.com/questions/17623152/how-map-tween-a-number-based-on-a-dynamic-curve
 */
static double
get_y_normalized (double x, double curviness, int start_at_1)
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
 */
int
automation_point_curve_get_y_px (AutomationPoint * start_ap, ///< start point (0, 0)
                           int               x, ///< x coordinate in px
                           int               width, ///< total width in px
                           int               height) ///< total height in px
{
  if (start_ap->type != AUTOMATION_POINT_VALUE)
    {
      g_error ("start ap must be a value");
    }

  /* find next curve ap & next value ap */
  AutomationPoint * curve_ap = automation_track_get_next_curve_ap (start_ap->at,
                                                                   start_ap);
  AutomationPoint * next_ap = automation_track_get_next_ap (start_ap->at,
                                                            start_ap);

  double dx = (double) x / width; /* normalized x */
  double dy;
  int ret;
  if (automation_point_get_y_in_px (next_ap) > /* if next point is lower */
      automation_point_get_y_in_px (start_ap))
    {
      dy = get_y_normalized (dx, curve_ap->curviness, 1); /* start higher */
      ret = dy * height;
      return height - ret; /* reverse the value because in pixels higher y values
                              are actually lower */
    }
  else
    {
      dy = get_y_normalized (dx, curve_ap->curviness, 0);
      ret = dy * height;
      return (height - ret) - height;
    }
}

/**
 * Updates the value and notifies interested parties.
 */
void
automation_point_update_fvalue (AutomationPoint * ap,
                                float             fval)
{
  if (ap->type != AUTOMATION_POINT_VALUE)
    {
      g_error ("Cannot update fvalue: automation point not type value");
    }

  ap->fvalue = fval;

  Automatable * a = ap->at->automatable;
  if (a->type == AUTOMATABLE_TYPE_PLUGIN_CONTROL)
    {
      Plugin * plugin = a->port->owner_pl;
      if (plugin->descr->protocol == PROT_LV2)
        {
          LV2_Plugin * lv2_plugin = (LV2_Plugin *) plugin->original_plugin;
          if (lv2_plugin->ui_instance)
            {
              Lv2ControlID * control = a->control;
              LV2_Port* port = &control->plugin->ports[control->index];
              port->control = fval;
            }
          else
            {
              lv2_gtk_set_float_control (a->control, ap->fvalue);
            }
        }
    }
  else if (a->type == AUTOMATABLE_TYPE_CHANNEL_FADER)
    {
      Channel * ch = a->track->channel;
      channel_set_fader_amp (ch, ap->fvalue);
    }
}

/**
 * Destroys the widget and frees memory.
 */
void
automation_point_free (AutomationPoint * ap)
{
  gtk_widget_destroy (GTK_WIDGET (ap->widget));
  free (ap);
}
