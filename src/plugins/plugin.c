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
#include "utils/io.h"
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
  lv2_plugin_init_loaded (plgn->lv2);
  plgn->lv2->plugin = plgn;

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

  plgn->track =
    project_get_track (plgn->track_id);

  plugin_instantiate (plgn);

  for (int i = 0; i < plgn->num_automatables; i++)
    plgn->automatables[i] =
      project_get_automatable (
        plgn->automatable_ids[i]);
}

/**
 * Creates/initializes a plugin and its internal
 * plugin (LV2, etc.)
 * using the given descriptor.
 *
 * @param add_to_project Should be false when
 *   cloning.
 */
Plugin *
plugin_new_from_descr (
  PluginDescriptor * descr,
  int                add_to_project)
{
  Plugin * plugin = calloc (1, sizeof (Plugin));

  if (add_to_project)
    {
      project_add_plugin (plugin);
    }

  plugin->descr = descr;

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
      Lv2Plugin * lv2_plugin = (Lv2Plugin *) plugin->lv2;
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
 * Returns if the Plugin is an instrument or not.
 */
int
plugin_is_instrument (
  PluginDescriptor * descr)
{
  return IS_PLUGIN_DESCR_CATEGORY (
    descr,
    LV2_INSTRUMENT_PLUGIN);
}

/**
 * Instantiates the plugin (e.g. when adding to a
 * channel).
 */
int
plugin_instantiate (
  Plugin * pl)
{
  g_message ("Instantiating %s...",
             pl->descr->name);

  switch (pl->descr->protocol)
    {
    case PROT_LV2:
      {
        g_message ("state file: %s",
                   pl->lv2->state_file);
        if (lv2_instantiate (pl->lv2,
                             NULL))
          {
            g_warning ("lv2 instantiate failed");
            return -1;
          }
      }
      break;
    default:
      g_warn_if_reached ();
      return -1;
      break;
    }
  pl->enabled = 1;

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
        (Lv2Plugin *) plugin->lv2);
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
      Lv2Plugin * lv2_plugin = (Lv2Plugin *) plugin->lv2;
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
 * Returns if Plugin exists in MixerSelections.
 */
int
plugin_is_selected (
  Plugin * pl)
{
  return mixer_selections_contains_plugin (
    MIXER_SELECTIONS,
    pl);
}

/**
 * Clones the given plugin.
 */
Plugin *
plugin_clone (
  Plugin * pl)
{
  Plugin * clone;
  if (pl->descr->protocol == PROT_LV2)
    {
      /* NOTE from rgareus:
       * I think you can use   lilv_state_restore (lilv_state_new_from_instance (..), ...)
       * and skip  lilv_state_new_from_file() ; lilv_state_save ()
       * lilv_state_new_from_instance() handles files and externals, too */
      /* save state to file */
      char * tmp =
        g_strdup_printf (
          "tmp_%s_XXXXXX",
          pl->descr->name);
      char * state_dir_plugin =
        g_build_filename (PROJECT->states_dir,
                          tmp,
                          NULL);
      io_mkdir (state_dir_plugin);
      g_free (tmp);
      lv2_plugin_save_state_to_file (
        pl->lv2,
        state_dir_plugin);
      g_free (state_dir_plugin);
      if (!pl->lv2->state_file)
        {
          g_warn_if_reached ();
          return NULL;
        }

      /* create a new plugin with same descriptor */
      PluginDescriptor * descr =
        calloc (1, sizeof (PluginDescriptor));
      plugin_clone_descr (
        pl->descr,
        descr);
      clone = plugin_new_from_descr (descr, 0);
      g_return_val_if_fail (clone, NULL);
      clone->id = pl->id;

      /* set the state file on the new Lv2Plugin
       * as the state filed saved on the original
       * so that it can be used when
       * instantiating */
      clone->lv2->state_file =
        g_strdup (pl->lv2->state_file);

      /* instantiate */
      int ret = plugin_instantiate (clone);
      g_return_val_if_fail (!ret, NULL);

      /* delete the state file */
      io_remove (pl->lv2->state_file);
    }

  clone->slot = pl->slot;

  return clone;
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
        (Lv2Plugin *) plugin->lv2;
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
