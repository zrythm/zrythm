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
#include "audio/instrument_track.h"
#include "audio/port.h"
#include "audio/position.h"
#include "audio/track.h"
#include "gui/widgets/automation_point.h"
#include "gui/widgets/automation_track.h"
#include "gui/widgets/automation_curve.h"
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
                            float               value,
                            Position *          pos)
{
  AutomationPoint * ap = _create_new (at, pos);

  ap->fvalue = value;
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
  Automatable * a = ap->at->automatable;
  float ap_max = automatable_get_maxf (a);
  float ap_range = ap_max - automatable_get_minf (a);

  /* ratio of current value in the range */
  float ap_ratio = (ap_range - (ap_max - ap->fvalue)) / ap_range;

  int allocated_h =
    gtk_widget_get_allocated_height (
      GTK_WIDGET (ap->at->widget));
  int point = allocated_h - ap_ratio * allocated_h;
  return point;
}

/**
 * Updates the value and notifies interested parties.
 */
void
automation_point_update_fvalue (AutomationPoint * ap,
                                float             fval)
{
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
      Channel * ch = track_get_channel (a->track);
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
