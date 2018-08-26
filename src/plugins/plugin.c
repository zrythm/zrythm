/*
 * plugins/plugin.c - a base plugin for plugins (lv2/vst/ladspa/etc.)
 *                         to inherit from
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
 * Implementation of Plugin. */

#include <stdlib.h>
#include <string.h>

#include "plugins/plugin.h"
#include "plugins/lv2_plugin.h"

#include <gtk/gtk.h>

/**
 * Creates an empty plugin.
 *
 * To be filled in by the caller.
 */
Plugin *
plugin_new ()
{
  Plugin * plugin = calloc (1, sizeof (Plugin));

  plugin->processed = 0;

  return plugin;
}

/**
 * Creates a dummy plugin.
 *
 * Used when filling up channel slots. A dummy plugin has 2 output ports
 * and 2 input ports and a MIDI input.
 */
Plugin *
plugin_new_dummy (Channel * channel    ///< channel it belongs to
            )
{
  Plugin * plugin = plugin_new ();

  plugin->descr.author = "Zrythm";
  plugin->descr.name = DUMMY_PLUGIN;
  plugin->descr.website = "http://alextee.online/zrythm";
  plugin->descr.category = "PC_UTILITY";
  plugin->descr.num_audio_ins = 2;
  plugin->descr.num_midi_ins = 1;
  plugin->descr.num_audio_outs = 2;
  plugin->descr.num_midi_outs = 1;

  plugin->num_in_ports = 0;
  plugin->in_ports[plugin->num_in_ports++] = port_new ();
  plugin->in_ports[plugin->num_in_ports++] = port_new ();

  plugin->num_out_ports = 0;
  plugin->out_ports[plugin->num_out_ports++] = port_new ();
  plugin->out_ports[plugin->num_out_ports++] = port_new ();

  /* FIXME is this really needed */
  plugin->num_unknown_ports = 0;

  plugin->processed = 0;

  plugin->original_plugin = NULL;

  plugin->channel = channel;

  return plugin;
}

/**
 * Instantiates the plugin (e.g. when adding to a channel).
 */
int
plugin_instantiate (Plugin * plugin ///< the plugin
                   )
{
  g_message ("Instantiating %s...", plugin->descr.name);
  /* TODO */
  if (plugin->descr.protocol == PROT_LV2)
    {
      LV2_Plugin *lv2 = (LV2_Plugin *) plugin->original_plugin;
      lv2_instantiate (lv2, NULL);
    }
}


/**
 * Process plugin
 */
void
plugin_process (Plugin * plugin)
{
  /* TODO */
  switch (plugin->descr.protocol)
    {
    case PROT_LV2:
      break;
    }
}

/**
 * Returns whether given plugin is a dummy plugin or not.
 */
int
plugin_is_dummy (Plugin *plugin)
{
  return (!strcmp (plugin->descr.name, DUMMY_PLUGIN));
}

/**
 * Disconnects all connected ports from each port in the given array and
 * frees them.
 */
static void
clean_ports (Port ** array, int * size)
{
  /* go through each port */
  for (int i = 0; i < (* size); i++)
    {
      Port * port = array[i];

      if (port->flow == FLOW_INPUT) /* disconnect incoming ports */
        {
          for (int j = 0; j < port->num_srcs; j++)
            {
              port_disconnect (port->srcs[j], port);
            }
        }
        else if (port->flow == FLOW_OUTPUT) /* disconnect outgoing ports */
          {
            for (int j = 0; j < port->num_dests; j++)
              {
                /* disconnect outgoing ports */
                port_disconnect (port, port->dests[j]);
              }
          }
      free (port);
    }
  (* size) = 0;
}


/**
 * Frees given plugin, breaks all its port connections, and frees its ports
 * and other internal pointers
 */
void
plugin_free (Plugin *plugin)
{
  /* disconnect all ports and free them */
  clean_ports (plugin->in_ports, &plugin->num_in_ports);
  clean_ports (plugin->out_ports, &plugin->num_out_ports);

  /* TODO other cleanup work */

  free (plugin);
}
