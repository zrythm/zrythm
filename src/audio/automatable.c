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

#include "audio/automatable.h"
#include "audio/automation_track.h"
#include "audio/automation_tracklist.h"
#include "audio/channel.h"
#include "audio/track.h"
#include "plugins/plugin.h"
#include "plugins/lv2_plugin.h"
#include "utils/math.h"

#include <gtk/gtk.h>

static Automatable *
_create_blank ()
{
  Automatable * automatable = calloc (1, sizeof (Automatable));
  return automatable;
}

Automatable *
automatable_create_fader (Channel * channel)
{
  Automatable * automatable = _create_blank ();

  automatable->track = channel->track;
  automatable->track_id = channel->track->id;
  automatable->label = g_strdup ("Volume");
  automatable->type = AUTOMATABLE_TYPE_CHANNEL_FADER;

  return automatable;
}

Automatable *
automatable_create_pan (Channel * channel)
{
  Automatable * automatable = _create_blank ();

  automatable->track = channel->track;
  automatable->track_id = channel->track->id;
  automatable->label = g_strdup ("Pan");
  automatable->type = AUTOMATABLE_TYPE_CHANNEL_PAN;

  return automatable;
}

Automatable *
automatable_create_mute (Channel * channel)
{
  Automatable * automatable = _create_blank ();

  automatable->track = channel->track;
  automatable->track_id = channel->track->id;
  automatable->label = g_strdup ("Mute");
  automatable->type = AUTOMATABLE_TYPE_CHANNEL_MUTE;

  return automatable;
}

Automatable *
automatable_create_lv2_control (Plugin *       plugin,
                                Lv2ControlID * control)
{
  Automatable * automatable = _create_blank ();

  automatable->control = control;
  LV2_Port * port = &control->plugin->ports[control->index];
  automatable->port = port->port;
  automatable->port_id = port->port->id;
  /*automatable->index = control->index;*/
  automatable->type = AUTOMATABLE_TYPE_PLUGIN_CONTROL;
  automatable->track = plugin->channel->track;
  automatable->track_id =
    plugin->channel->track->id;
  automatable->slot_index =
    channel_get_plugin_index (plugin->channel,
                              plugin);
  automatable->label = g_strdup (automatable->port->label);

  return automatable;
}

Automatable *
automatable_create_plugin_enabled (
  Plugin * plugin)
{
  Automatable * automatable = _create_blank ();

  automatable->type = AUTOMATABLE_TYPE_PLUGIN_ENABLED;
  automatable->track = plugin->channel->track;
  automatable->track_id =
    plugin->channel->track->id;
  automatable->slot_index =
    channel_get_plugin_index (plugin->channel,
                              plugin);
  automatable->label = g_strdup ("Enable/disable");

  return automatable;
}

int
automatable_is_bool (Automatable * automatable)
{

  return 0;
}

int
automatable_is_float (Automatable * a)
{
  if (IS_AUTOMATABLE_LV2_CONTROL (a))
    {
      if (a->control->value_type == a->control->plugin->forge.Float)
        {
          return 1;
        }
    }
  else if (IS_AUTOMATABLE_CH_FADER (a))
    {
      return 1;
    }
  return 0;
}

const float
automatable_get_minf (Automatable * a)
{
  if (automatable_is_float (a))
    {
      if (IS_AUTOMATABLE_CH_FADER (a))
        {
          return 0.f;
        }
      else if (IS_AUTOMATABLE_LV2_CONTROL (a))
        {
          return lilv_node_as_float (a->control->min);
        }
    }
  g_warning ("automatable %s is not float", a->label);
  return -1;
}

const float
automatable_get_maxf (Automatable * a)
{
  if (automatable_is_float (a))
    {
      if (IS_AUTOMATABLE_CH_FADER (a))
        {
          return 1.0f;
        }
      else if (IS_AUTOMATABLE_LV2_CONTROL (a))
        {
          return lilv_node_as_float (a->control->max);
        }
    }
  g_warning ("automatable %s is not float", a->label);
  return -1;
}

/**
 * Returns max - min for the float automatable
 */
const float
automatable_get_sizef (Automatable * automatable)
{
  return (automatable_get_maxf (automatable) -
          automatable_get_minf (automatable));
}

void
automatable_free (Automatable * automatable)
{
  /* TODO go through every track and plugins
   * and set associated automatables to NULL */

}

/**
 * Gets the current value of the parameter the
 * automatable is for.
 *
 * This does not consider the automation track, it
 * only looks in the actual parameter for its
 * current value.
 */
float
automatable_get_val (Automatable * a)
{
  if (a->type == AUTOMATABLE_TYPE_PLUGIN_CONTROL)
    {
      Plugin * plugin = a->port->owner_pl;
      if (plugin->descr->protocol == PROT_LV2)
        {
          /*Lv2Plugin * lv2_plugin = (Lv2Plugin *) plugin->original_plugin;*/
          /*if (lv2_plugin->ui_instance)*/
            /*{*/
              Lv2ControlID * control = a->control;
              LV2_Port* port = &control->plugin->ports[control->index];
              return port->control;
            /*}*/
          /*else*/
            /*{*/
              /*return a->control;*/
            /*}*/
        }
    }
  else if (a->type == AUTOMATABLE_TYPE_CHANNEL_FADER)
    {
      Channel * ch = track_get_channel (a->track);
      return ch->fader_amp;
    }
  return 0;
}

/**
 * Updates the actual value.
 *
 * The given value is always a normalized 0.0-1.0
 * value and must be translated to the actual value
 * before setting it.
 */
void
automatable_set_val (Automatable * a,
                     float         val)
{
  if (a->type == AUTOMATABLE_TYPE_PLUGIN_CONTROL)
    {
      Plugin * plugin = a->port->owner_pl;
      if (plugin->descr->protocol == PROT_LV2)
        {
          Lv2Plugin * lv2_plugin = (Lv2Plugin *) plugin->original_plugin;
          if (lv2_plugin->ui_instance)
            {
              Lv2ControlID * control = a->control;
              LV2_Port* port = &control->plugin->ports[control->index];
              port->control = val;
            }
          else
            {
              lv2_gtk_set_float_control (
                a->control, val);
            }
        }
    }
  else if (a->type == AUTOMATABLE_TYPE_CHANNEL_FADER)
    {
      Channel * ch = track_get_channel (a->track);
      double dval =
        math_get_amp_val_from_fader (val);
      g_message ("setting channel fader to %f",
                 dval);
      channel_set_fader_amp (
        ch,
        dval);
    }

}

/**
 * Gets automation track for given automatable, if any.
 */
AutomationTrack *
automatable_get_automation_track (Automatable * automatable)
{
  Track * track = automatable->track;
  AutomationTracklist * automation_tracklist =
    track_get_automation_tracklist (track);

  for (int i = 0;
       i < automation_tracklist->num_automation_tracks;
       i++)
    {
      AutomationTrack * at =
        automation_tracklist->automation_tracks[i];
      if (at->automatable == automatable)
        {
          return at;
        }
    }
  return NULL;
}

