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

#define _GNU_SOURCE 1  /* To pick up REG_RIP */

#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include "audio/engine.h"
#include "audio/transport.h"
#include "plugins/plugin.h"
#include "plugins/lv2_plugin.h"
#include "plugins/lv2/control.h"

#include <gtk/gtk.h>

/**
 * Handler for plugin crashes.
 */
void handler(int nSignum, siginfo_t* si, void* vcontext)
{
  g_message ("handler");

  ucontext_t* context = (ucontext_t*)vcontext;
  context->uc_mcontext.gregs[REG_RIP]++;
}

/**
 * Creates an empty plugin.
 *
 * To be filled in by the caller.
 * FIXME should be static?
 */
static Plugin *
_plugin_new ()
{
  Plugin * plugin = calloc (1, sizeof (Plugin));

  plugin->processed = 1;

  return plugin;
}

/**
 * Creates/initializes a plugin and its internal plugin (LV2, etc.)
 * using the given descriptor.
 */
Plugin *
plugin_create_from_descr (Plugin_Descriptor * descr)
{
  Plugin * plugin = _plugin_new ();
  plugin->descr = descr;
  struct sigaction action;
  memset(&action, 0, sizeof(struct sigaction));
  action.sa_flags = SA_SIGINFO;
  action.sa_sigaction = handler;
  /*sigaction(SIGSEGV, &action, NULL);*/
  switch (plugin->descr->protocol)
    {
    case PROT_LV2:

      lv2_create_from_uri (plugin, descr->uri);
      break;
    }
  return plugin;
}

/**
 * Instantiates the plugin (e.g. when adding to a channel).
 */
int
plugin_instantiate (Plugin * plugin ///< the plugin
                   )
{
  g_message ("Instantiating %s...", plugin->descr->name);
  /* TODO */
  if (plugin->descr->protocol == PROT_LV2)
    {
      LV2_Plugin *lv2 = (LV2_Plugin *) plugin->original_plugin;
      if (lv2_instantiate (lv2, NULL) < 0)
        {
          return -1;
        }
    }
  plugin->enabled = 1;
  return 0;
}


/**
 * Process plugin
 */
void
plugin_process (Plugin * plugin, nframes_t nframes)
{
  switch (plugin->descr->protocol)
    {
    case PROT_LV2:
      lv2_plugin_process ((LV2_Plugin *) plugin->original_plugin, nframes);
      break;
    case PROT_VST:

      break;

    }

  plugin->processed = 1;
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
      port_delete (port);
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
  zix_sem_wait (&AUDIO_ENGINE->port_operation_lock);

  /* disconnect all ports and free them */
  clean_ports (plugin->in_ports, &plugin->num_in_ports);
  clean_ports (plugin->out_ports, &plugin->num_out_ports);

  /* TODO other cleanup work */

  free (plugin);
  zix_sem_post (&AUDIO_ENGINE->port_operation_lock);
}
