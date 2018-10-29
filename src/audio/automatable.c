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
          return -128.f;
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
          return 3.f;
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


