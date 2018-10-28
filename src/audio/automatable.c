/*
 * audio/automatable.c - An automatable
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

#include "audio/automatable.h"
#include "audio/channel.h"
#include "audio/track.h"
#include "plugins/plugin.h"
#include "plugins/lv2_plugin.h"

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
  automatable->label = g_strdup ("Volume");
  automatable->type = AUTOMATABLE_TYPE_CHANNEL_FADER;

  return automatable;
}

Automatable *
automatable_create_mute (Track * track)
{
  Automatable * automatable = _create_blank ();

  return automatable;
}

Automatable *
automatable_create_pan (Track * track)
{
  Automatable * automatable = _create_blank ();

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
  automatable->type = AUTOMATABLE_TYPE_PLUGIN_CONTROL;
  automatable->track = plugin->channel->track;
  automatable->slot_index = channel_get_plugin_index (plugin->channel,
                                                      port->port->owner_pl);
  automatable->label = g_strdup (automatable->port->label);

  return automatable;
}

int
automatable_is_bool (Automatable * automatable)
{

  return 1;
}

int
automatable_is_float (Automatable * automatable)
{
  if (IS_AUTOMATABLE_LV2_CONTROL (automatable) &&
      automatable->control->is_logarithmic)
    {
      return 1;
    }
  return 0;
}

const float
automatable_get_minf (Automatable * automatable)
{
  if (automatable_is_float (automatable))
    {
      return lilv_node_as_float (automatable->control->min);
    }
  g_warning ("automatable %s is not float", automatable->label);
  return -1;
}

const float
automatable_get_maxf (Automatable * automatable)
{
  if (automatable_is_float (automatable))
    {
      return lilv_node_as_float (automatable->control->max);
    }
  g_warning ("automatable %s is not float", automatable->label);
  return -1;
}

void
automatable_free (Automatable * automatable)
{
  /* TODO go through every track and plugins
   * and set associated automatables to NULL */

}


