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

/** \file
 * Implementation of Plugin. */

#define _GNU_SOURCE 1  /* To pick up REG_RIP */

#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include "audio/automation_tracklist.h"
#include "audio/channel.h"
#include "audio/engine.h"
#include "audio/track.h"
#include "audio/transport.h"
#include "gui/widgets/instrument_track.h"
#include "plugins/plugin.h"
#include "plugins/lv2_plugin.h"
#include "plugins/lv2/control.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/flags.h"

#include <gtk/gtk.h>

/**
 * Handler for plugin crashes.
 */
/*void handler(int nSignum, siginfo_t* si, void* vcontext)*/
/*{*/
  /*g_message ("handler");*/

  /*ucontext_t* context = (ucontext_t*)vcontext;*/
  /*context->uc_mcontext.gregs[REG_RIP]++;*/
/*}*/

void
plugin_init_loaded (Plugin * plgn)
{
  lv2_plugin_init_loaded (plgn->original_plugin);
  plgn->original_plugin->plugin = plgn;

  for (int i = 0; i < plgn->num_in_ports; i++)
    plgn->in_ports[i] =
      project_get_port (plgn->in_port_ids[i]);

  for (int i = 0; i < plgn->num_out_ports; i++)
    plgn->out_ports[i] =
      project_get_port (plgn->out_port_ids[i]);

  for (int i = 0;
       i < plgn->num_unknown_ports; i++)
    plgn->unknown_ports[i] =
      project_get_port (plgn->unknown_port_ids[i]);

  plgn->channel =
    project_get_channel (plgn->channel_id);

  plugin_instantiate (plgn);

  for (int i = 0; i < plgn->num_automatables; i++)
    plgn->automatables[i] =
      project_get_automatable (
        plgn->automatable_ids[i]);
}

/**
 * Creates an empty plugin.
 *
 * To be filled in by the caller.
 */
static Plugin *
_plugin_new ()
{
  Plugin * plugin = calloc (1, sizeof (Plugin));

  /*g_atomic_int_set (&plugin->processed, 1);*/

  project_add_plugin (plugin);

  return plugin;
}

/**
 * Creates/initializes a plugin and its internal plugin (LV2, etc.)
 * using the given descriptor.
 */
Plugin *
plugin_create_from_descr (PluginDescriptor * descr)
{
  Plugin * plugin = _plugin_new ();
  plugin->descr = descr;
  /*struct sigaction action;*/
  /*memset(&action, 0, sizeof(struct sigaction));*/
  /*action.sa_flags = SA_SIGINFO;*/
  /*action.sa_sigaction = handler;*/
  /*sigaction(SIGSEGV, &action, NULL);*/
  if (plugin->descr->protocol == PROT_LV2)
    {
      lv2_create_from_uri (plugin, descr->uri);
    }
  return plugin;
}

/**
 * Clones the plugin descriptor.
 */
void
plugin_clone_descr (
  PluginDescriptor * src,
  PluginDescriptor * dest)
{
  dest->author = g_strdup (src->author);
  dest->name = g_strdup (src->name);
  dest->website = g_strdup (src->website);
  dest->category = g_strdup (src->category);
  dest->num_audio_ins = src->num_audio_ins;
  dest->num_midi_ins = src->num_midi_ins;
  dest->num_audio_outs = src->num_audio_outs;
  dest->num_midi_outs = src->num_midi_outs;
  dest->num_ctrl_ins = src->num_ctrl_ins;
  dest->num_ctrl_outs = src->num_ctrl_outs;
  dest->arch = src->arch;
  dest->protocol = src->protocol;
  dest->path = g_strdup (src->path);
  dest->uri = g_strdup (src->uri);
}

/**
 * Loads the plugin from its state file.
 */
/*void*/
/*plugin_load (Plugin * plugin)*/
/*{*/
  /*switch (plugin->descr->protocol)*/
    /*{*/
    /*case PROT_LV2:*/

      /*lv2_load_from_state (plugin, descr->uri);*/
      /*break;*/
    /*}*/
  /*return plugin;*/

/*}*/

/**
 * Moves the Plugin's automation from one Channel
 * to another.
 */
void
plugin_move_automation (
  Plugin *  pl,
  Channel * prev_ch,
  Channel * ch)
{
  AutomationTracklist * prev_atl =
    &prev_ch->track->automation_tracklist;
  AutomationTracklist * atl =
    &ch->track->automation_tracklist;

  int i;
  AutomationTrack * at;
  AutomationLane * al;
  for (i = 0; i < prev_atl->num_automation_tracks;
       i++)
    {
      at = prev_atl->automation_tracks[i];

      if (!at->automatable->port ||
          at->automatable->port->owner_pl != pl)
        continue;

      /* delete from prev channel */
      automation_tracklist_delete_automation_track (
        prev_atl, at, F_NO_FREE);

      /* add to new channel */
      automation_tracklist_add_automation_track (
        atl, at);
    }
  for (i = 0; i < prev_atl->num_automation_lanes;
       i++)
    {
      al = prev_atl->automation_lanes[i];

      if (!al->at->automatable->port ||
          al->at->automatable->port->owner_pl != pl)
        continue;

      /* delete from prev channel */
      automation_tracklist_delete_automation_lane (
        prev_atl, al, F_NO_FREE);

      /* add to new channel */
      automation_tracklist_add_automation_lane (
        atl, al);
    }
}

/**
 * Generates automatables for the plugin.
 *
 *
 * Plugin must be instantiated already.
 */
void
plugin_generate_automatables (Plugin * plugin)
{
  g_message ("generating automatables for %s...",
             plugin->descr->name);

  /* add plugin enabled automatable */
  array_append (
    plugin->automatables,
    plugin->num_automatables,
    automatable_create_plugin_enabled (plugin));
  plugin->automatable_ids[
    plugin->num_automatables - 1] =
    plugin->automatables[
      plugin->num_automatables - 1]->id;

  /* add plugin control automatables */
  if (plugin->descr->protocol == PROT_LV2)
    {
      Lv2Plugin * lv2_plugin = (Lv2Plugin *) plugin->original_plugin;
      for (int j = 0; j < lv2_plugin->controls.n_controls; j++)
        {
          Lv2Control * control =
            lv2_plugin->controls.controls[j];
          array_append (
            plugin->automatables,
            plugin->num_automatables,
            automatable_create_lv2_control (
              plugin, control));
          plugin->automatable_ids[
            plugin->num_automatables - 1] =
            plugin->automatables[
              plugin->num_automatables - 1]->id;
        }
    }
  else
    {
      g_warning ("Plugin protocol not supported yet (gen automatables)");
    }
}


/**
 * Instantiates the plugin (e.g. when adding to a
 * channel).
 */
int
plugin_instantiate (
  Plugin * plugin)
{
  g_message ("Instantiating %s...",
             plugin->descr->name);

  switch (plugin->descr->protocol)
    {
    case PROT_LV2:
      {
        Lv2Plugin *lv2 =
          (Lv2Plugin *) plugin->original_plugin;
        if (lv2_instantiate (lv2, NULL) < 0)
          {
            g_warning ("lv2 instantiate failed");
            return -1;
          }
      }
      break;
    default:
      g_warn_if_reached ();
      break;
    }
  plugin->enabled = 1;

  return 0;
}

/**
 * Process plugin
 */
void
plugin_process (Plugin * plugin)
{

  /* if has MIDI input port */
  if (plugin->descr->num_midi_ins > 0)
    {
      /* if recording, write MIDI events to the region TODO */

        /* if there is a midi note in this buffer range TODO */
          /* add midi events to input port */
    }

  if (plugin->descr->protocol == PROT_LV2)
    {
      lv2_plugin_process (
        (Lv2Plugin *) plugin->original_plugin);
    }

  /*g_atomic_int_set (&plugin->processed, 1);*/
  /*zix_sem_post (&plugin->processed_sem);*/
}

/**
 * shows plugin ui and sets window close callback
 */
void
plugin_open_ui (Plugin *plugin)
{
  if (plugin->descr->protocol == PROT_LV2)
    {
      Lv2Plugin * lv2_plugin = (Lv2Plugin *) plugin->original_plugin;
      if (GTK_IS_WINDOW (lv2_plugin->window))
        {
          gtk_window_present (
            GTK_WINDOW (lv2_plugin->window));
        }
      else
        {
          lv2_open_ui (lv2_plugin);
        }
    }
}


/**
 * hides plugin ui
 */
void
plugin_close_ui (Plugin *plugin)
{
  if (plugin->descr->protocol == PROT_LV2)
    {
      Lv2Plugin * lv2_plugin =
        (Lv2Plugin *) plugin->original_plugin;
      if (GTK_IS_WINDOW (lv2_plugin->window))
        gtk_window_close (
          GTK_WINDOW (lv2_plugin->window));
      else
        lv2_close_ui (lv2_plugin);
    }
}

/**
 * To be called immediately when a channel or plugin
 * is deleted.
 *
 * A call to plugin_free can be made at any point
 * later just to free the resources.
 */
void
plugin_disconnect (Plugin * plugin)
{
  plugin->deleting = 1;

  /* disconnect all ports */
  ports_disconnect (
    plugin->in_ports,
    plugin->num_in_ports, 1);
  ports_disconnect (
    plugin->out_ports,
    plugin->num_out_ports, 1);
  g_message (
    "DISCONNECTED ALL PORTS OF PLUGIN %d %d",
    plugin->num_in_ports,
    plugin->num_out_ports);
}

/**
 * Frees given plugin, frees its ports
 * and other internal pointers
 */
void
plugin_free (Plugin *plugin)
{
  g_message ("FREEING PLUGIN %s",
             plugin->descr->name);
  g_warn_if_fail (plugin);
  project_remove_plugin (plugin);

  ports_remove (
    plugin->in_ports,
    &plugin->num_in_ports);
  ports_remove (
    plugin->out_ports,
    &plugin->num_out_ports);

  /* delete automatables */
  for (int i = 0; i < plugin->num_automatables; i++)
    {
      Automatable * automatable =
        plugin->automatables[i];
      project_remove_automatable (automatable);
      automatable_free (automatable);
    }

  free (plugin);
}
