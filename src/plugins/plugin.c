/*
 * Copyright (C) 2018-2020 Alexandros Theodotou <alex at zrythm dot org>
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

/**
 * \file
 *
 * Implementation of Plugin.
 */

#define _GNU_SOURCE 1  /* To pick up REG_RIP */

#include "config.h"

#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include "audio/automation_tracklist.h"
#include "audio/channel.h"
#include "audio/engine.h"
#include "audio/track.h"
#include "audio/transport.h"
#include "gui/widgets/instrument_track.h"
#include "gui/widgets/main_window.h"
#include "plugins/plugin.h"
#include "plugins/lv2_plugin.h"
#include "plugins/lv2/lv2_control.h"
#include "plugins/lv2/lv2_gtk.h"
#include "plugins/vst_plugin.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/io.h"
#include "utils/flags.h"
#include "utils/math.h"

#include <gtk/gtk.h>

/**
 * Plugin UI refresh rate limits.
 */
#define MIN_REFRESH_RATE 30.f
#define MAX_REFRESH_RATE 60.f

static void
get_automation_tracks (
  Plugin * self)
{
  g_return_if_fail (self->track);

  AutomationTracklist * atl =
    track_get_automation_tracklist (self->track);
  AutomationTrack * at;
  for (int i = 0; i < atl->num_ats; i++)
    {
      at = atl->ats[i];

      if (at->automatable->type !=
            AUTOMATABLE_TYPE_PLUGIN_CONTROL &&
          at->automatable->type !=
            AUTOMATABLE_TYPE_PLUGIN_ENABLED)
        continue;

      array_double_size_if_full (
        self->ats,
        self->num_ats,
        self->ats_size,
        AutomationTrack *);
      array_append (
        self->ats,
        self->num_ats,
        at);
    }
}

void
plugin_init_loaded (
  Plugin * self)
{
  switch (self->descr->protocol)
    {
    case PROT_LV2:
      g_return_if_fail (self->lv2);
      self->lv2->plugin = self;
      lv2_plugin_init_loaded (self->lv2);
      break;
    case PROT_VST:
      g_return_if_fail (self->vst);
      self->vst->plugin = self;
      vst_plugin_init_loaded (self->vst);
      break;
    default:
      g_warn_if_reached ();
      break;
    }

  self->ats_size = 1;
  self->ats =
    calloc (1, sizeof (AutomationTrack *));

  plugin_instantiate (self);

  get_automation_tracks (self);
}

static void
plugin_init (
  Plugin * plugin)
{
  plugin->in_ports_size = 1;
  plugin->out_ports_size = 1;
  plugin->unknown_ports_size = 1;
  plugin->ats_size = 1;
  plugin->slot = -1;

  plugin->in_ports =
    calloc (1, sizeof (Port *));
  /*plugin->in_port_ids =*/
    /*calloc (1, sizeof (PortIdentifier));*/
  plugin->out_ports =
    calloc (1, sizeof (Port *));
  /*plugin->out_port_ids =*/
    /*calloc (1, sizeof (PortIdentifier));*/
  plugin->unknown_ports =
    calloc (1, sizeof (Port *));
  /*plugin->unknown_port_ids =*/
    /*calloc (1, sizeof (PortIdentifier));*/
  plugin->ats =
    calloc (1, sizeof (AutomationTrack *));
}

/**
 * Creates/initializes a plugin and its internal
 * plugin (LV2, etc.)
 * using the given descriptor.
 */
Plugin *
plugin_new_from_descr (
  const PluginDescriptor * descr)
{
  Plugin * plugin = calloc (1, sizeof (Plugin));

  plugin->descr =
    plugin_descriptor_clone (descr);
  plugin_init (plugin);

  switch (plugin->descr->protocol)
    {
    case PROT_LV2:
      lv2_plugin_new_from_uri (
        plugin, descr->uri);
      break;
    case PROT_VST:
      vst_plugin_new_from_descriptor (
        plugin, descr);
      break;
    default:
      break;
    }

  return plugin;
}

/**
 * Removes the automation tracks associated with
 * this plugin from the automation tracklist in the
 * corresponding track.
 *
 * Used e.g. when moving plugins.
 */
void
plugin_remove_ats_from_automation_tracklist (
  Plugin * pl)
{
  for (int i = 0; i < pl->num_ats; i++)
    {
      automation_tracklist_remove_at (
        &pl->track->automation_tracklist,
        pl->ats[i], F_NO_FREE);
    }
}

/**
 * Adds an AutomationTrack to the Plugin.
 */
void
plugin_add_automation_track (
  Plugin * self,
  AutomationTrack * at)
{
  g_warn_if_fail (self->track);

  array_double_size_if_full (
    self->ats,
    self->num_ats,
    self->ats_size,
    AutomationTrack *);
  array_append (
    self->ats,
    self->num_ats,
    at);
  AutomationTracklist * atl =
    track_get_automation_tracklist (self->track);
  automation_tracklist_add_at (atl, at);
}

/**
 * Sets the channel and slot on the plugin and
 * its ports.
 */
void
plugin_set_channel_and_slot (
  Plugin *  pl,
  Channel * ch,
  int       slot)
{
  pl->track = ch->track;
  pl->track_pos = ch->track->pos;
  pl->slot = slot;

  int i;
  Port * port;
  for (i = 0; i < pl->num_in_ports; i++)
    {
      port = pl->in_ports[i];
      port_set_owner_plugin (port, pl);
    }
  for (i = 0; i < pl->num_out_ports; i++)
    {
      port = pl->out_ports[i];
      port_set_owner_plugin (port, pl);
    }
  for (i = 0; i < pl->num_unknown_ports; i++)
    {
      port = pl->unknown_ports[i];
      port_set_owner_plugin (port, pl);
    }

  if (pl->descr->protocol == PROT_LV2)
    {
      lv2_plugin_update_port_identifiers (
        pl->lv2);
    }
}

/**
 * Returns if the Plugin has a supported custom
 * UI.
 */
int
plugin_has_supported_custom_ui (
  Plugin * self)
{
  switch (self->descr->protocol)
    {
    case PROT_LV2:
      break;
    case PROT_VST:
      return
        self->vst->aeffect->flags &
          effFlagsHasEditor;
      break;
    default:
      g_return_val_if_reached (-1);
      break;
    }
  g_return_val_if_reached (-1);
}

/**
 * Updates the plugin's latency.
 *
 * Calls the plugin format's get_latency()
 * function and stores the result in the plugin.
 */
void
plugin_update_latency (
  Plugin * pl)
{
  if (pl->descr->protocol == PROT_LV2)
    {
      pl->latency =
        lv2_plugin_get_latency (pl->lv2);
      g_message ("%s latency: %d samples",
                 pl->descr->name,
                 pl->latency);
    }
}

/**
 * Adds a port of the given type to the Plugin.
 */
#define ADD_PORT(type) \
  while (pl->num_##type##_ports >= \
        (int) pl->type##_ports_size) \
    { \
      if (pl->type##_ports_size == 0) \
        pl->type##_ports_size = 1; \
      else \
        pl->type##_ports_size *= 2; \
      pl->type##_ports = \
        realloc ( \
          pl->type##_ports, \
          sizeof (Port *) * \
            pl->type##_ports_size); \
    } \
  port->identifier.port_index = \
    pl->num_##type##_ports; \
  array_append ( \
    pl->type##_ports, \
    pl->num_##type##_ports, \
    port)

/**
 * Adds an in port to the plugin's list.
 */
void
plugin_add_in_port (
  Plugin * pl,
  Port *   port)
{
  ADD_PORT (in);
}

/**
 * Adds an out port to the plugin's list.
 */
void
plugin_add_out_port (
  Plugin * pl,
  Port *   port)
{
  ADD_PORT (out);
}

/**
 * Adds an unknown port to the plugin's list.
 */
void
plugin_add_unknown_port (
  Plugin * pl,
  Port *   port)
{
  ADD_PORT (unknown);
}
#undef ADD_PORT

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
  for (i = 0; i < prev_atl->num_ats; i++)
    {
      at = prev_atl->ats[i];

      Port * port = at->automatable->port;
      if (!port)
        continue;
      if (port->identifier.owner_type ==
            PORT_OWNER_TYPE_PLUGIN)
        {
          Plugin * port_pl =
            port_get_plugin (port, 1);
          if (port_pl != pl)
            continue;
        }

      /* delete from prev channel */
      automation_tracklist_delete_at (
        prev_atl, at, F_NO_FREE);

      /* add to new channel */
      automation_tracklist_add_at (
        atl, at);
    }
}

/**
 * Sets the UI refresh rate on the Plugin.
 */
void
plugin_set_ui_refresh_rate (
  Plugin * self)
{
  /* if no preferred refresh rate is set,
   * use the monitor's refresh rate */
  if (!g_settings_get_int (
         S_PREFERENCES, "plugin-ui-refresh-rate"))
    {
      GdkDisplay * display =
        gdk_display_get_default ();
      g_warn_if_fail (display);
      GdkMonitor * monitor =
        gdk_display_get_primary_monitor (display);
      g_warn_if_fail (monitor);
      self->ui_update_hz =
        (float)
        /* divide by 1000 because gdk returns the
         * value in milli-Hz */
          gdk_monitor_get_refresh_rate (monitor) /
        1000.f;
      g_warn_if_fail (
        !math_floats_equal (
          self->ui_update_hz, 0.f, 0.001f));
      g_message (
        "refresh rate returned by GDK: %.01f",
        (double) self->ui_update_hz);
    }
  else
    {
      /* Use user-specified UI update rate. */
      self->ui_update_hz =
        (float)
        g_settings_get_int (
          S_PREFERENCES, "plugin-ui-refresh-rate");
    }

  /* clamp the refresh rate to sensible limits */
  if (self->ui_update_hz < MIN_REFRESH_RATE ||
      self->ui_update_hz > MAX_REFRESH_RATE)
    {
      g_warning (
        "Invalid refresh rate of %.01f received, "
        "clamping to reasonable bounds",
        (double) self->ui_update_hz);
      self->ui_update_hz =
        CLAMP (
          self->ui_update_hz, MIN_REFRESH_RATE,
          MAX_REFRESH_RATE);
    }
}

/**
 * Generates automatables for the plugin.
 *
 *
 * Plugin must be instantiated already.
 */
void
plugin_generate_automation_tracks (
  Plugin * plugin)
{
  g_message ("generating automatables for %s...",
             plugin->descr->name);

  AutomationTrack * at;
  Automatable * a;

  /* add plugin enabled automatable */
  a =
    automatable_create_plugin_enabled (plugin);
  at = automation_track_new (a);
  plugin_add_automation_track (
    plugin, at);

  /* add plugin control automatables */
  switch (plugin->descr->protocol)
    {
    case PROT_LV2:
      {
        Lv2Plugin * lv2_plugin = plugin->lv2;
        for (int j = 0;
             j < lv2_plugin->controls.n_controls;
             j++)
          {
            Lv2Control * control =
              lv2_plugin->controls.controls[j];
            a =
              automatable_create_lv2_control (
                plugin, control);
            at = automation_track_new (a);
            plugin_add_automation_track (
              plugin, at);
          }
      }
      break;
    case PROT_VST:
      {
        VstPlugin * vst = plugin->vst;
        g_return_if_fail (vst && vst->aeffect);
        for (int i = 0;
             i < plugin->num_in_ports; i++)
          {
            Port * port = plugin->in_ports[i];
            if (port->identifier.type !=
                  TYPE_CONTROL)
              continue;

            a =
              automatable_create_vst_control (
                plugin, port);
            at = automation_track_new (a);
            plugin_add_automation_track (
              plugin, at);
          }
      }
      break;
    default:
      g_warning (
        "%s: Plugin protocol not supported yet",
        __func__);
      break;
    }
}

/**
 * Sets the track and track_pos on the plugin.
 */
void
plugin_set_track (
  Plugin * pl,
  Track * tr)
{
  g_return_if_fail (tr);
  pl->track = tr;
  pl->track_pos = tr->pos;

  /* set port identifier track poses */
  int i;
  for (i = 0; i < pl->num_in_ports; i++)
    {
      /*pl->in_port_ids[i].track_pos = tr->pos;*/
    }
  for (i = 0; i < pl->num_out_ports; i++)
    {
      /*pl->out_port_ids[i].track_pos = tr->pos;*/
    }
  for (i = 0; i < pl->num_unknown_ports; i++)
    {
      /*pl->unknown_port_ids[i].track_pos = tr->pos;*/
    }
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

  plugin_set_ui_refresh_rate (pl);

  switch (pl->descr->protocol)
    {
    case PROT_LV2:
      {
        g_message ("state file: %s",
                   pl->lv2->state_file);
        if (lv2_plugin_instantiate (
              pl->lv2, NULL))
          {
            g_warning ("lv2 instantiate failed");
            return -1;
          }
      }
      break;
    case PROT_VST:
      if (vst_plugin_instantiate (
            pl->vst, !PROJECT->loaded))
        {
          g_warning (
            "VST plugin instantiation failed");
          return -1;
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
 * Process plugin.
 *
 * @param g_start_frames The global start frames.
 * @param nframes The number of frames to process.
 */
void
plugin_process (
  Plugin *        plugin,
  const long      g_start_frames,
  const nframes_t  local_offset,
  const nframes_t nframes)
{
  /* if has MIDI input port */
  if (plugin->descr->num_midi_ins > 0)
    {
      /* if recording, write MIDI events to the region TODO */

        /* if there is a midi note in this buffer range TODO */
          /* add midi events to input port */
    }

  switch (plugin->descr->protocol)
    {
    case PROT_LV2:
      lv2_plugin_process (
        plugin->lv2, g_start_frames, nframes);
      break;
    case PROT_VST:
      vst_plugin_process (
        plugin->vst, g_start_frames, local_offset,
        nframes);
      break;
    default:
      break;
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
  switch (plugin->descr->protocol)
    {
    case PROT_LV2:
      {
        Lv2Plugin * lv2_plugin = plugin->lv2;
        if (GTK_IS_WINDOW (lv2_plugin->window))
          {
            gtk_window_present (
              GTK_WINDOW (lv2_plugin->window));
            gtk_window_set_transient_for (
              GTK_WINDOW (lv2_plugin->window),
              (GtkWindow *) MAIN_WINDOW);
          }
        else
          {
            lv2_gtk_open_ui (lv2_plugin);
          }
      }
      break;
    case PROT_VST:
      {
        VstPlugin * vst_plugin = plugin->vst;
        if (GTK_IS_WINDOW (vst_plugin->gtk_window_parent))
          {
            gtk_window_present (
              GTK_WINDOW (vst_plugin->gtk_window_parent));
            gtk_window_set_transient_for (
              GTK_WINDOW (vst_plugin->gtk_window_parent),
              (GtkWindow *) MAIN_WINDOW);
          }
        else
          {
            vst_plugin_open_ui (vst_plugin);
          }
      }
      break;
    default:
      break;
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
  Plugin * clone = NULL;
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
      char * states_dir =
        project_get_states_dir (
          PROJECT, PROJECT->backup_dir != NULL);
      char * state_dir_plugin =
        g_build_filename (states_dir,
                          tmp,
                          NULL);
      g_free (states_dir);
      io_mkdir (state_dir_plugin);
      g_free (tmp);
      lv2_plugin_save_state_to_file (
        pl->lv2,
        state_dir_plugin);
      g_free (state_dir_plugin);
      g_return_val_if_fail (
        pl->lv2->state_file, NULL);

      /* create a new plugin with same descriptor */
      clone = plugin_new_from_descr (pl->descr);
      g_return_val_if_fail (
        clone && clone->lv2, NULL);

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
  else if (pl->descr->protocol == PROT_VST)
    {
      /* create a new plugin with same descriptor */
      clone = plugin_new_from_descr (pl->descr);
      g_return_val_if_fail (
        clone && clone->vst, NULL);

      /* instantiate */
      int ret = plugin_instantiate (clone);
      g_return_val_if_fail (!ret, NULL);

      /* copy the parameter values from the
       * original plugin */
      vst_plugin_copy_params (clone->vst, pl->vst);
    }
  g_return_val_if_fail (
    pl->num_in_ports || pl->num_out_ports, NULL);

  g_return_val_if_fail (clone, NULL);
  clone->slot = pl->slot;
  clone->track_pos = pl->track_pos;

  return clone;
}

/**
 * hides plugin ui
 */
void
plugin_close_ui (Plugin *plugin)
{
  switch (plugin->descr->protocol)
    {
    case PROT_LV2:
      if (GTK_IS_WIDGET (
            plugin->lv2->window))
        g_signal_handler_disconnect (
          plugin->lv2->window,
          plugin->lv2->delete_event_id);
      lv2_gtk_close_ui (plugin->lv2);
      break;
    case PROT_VST:
      if (GTK_IS_WIDGET (
            plugin->vst->gtk_window_parent))
        g_signal_handler_disconnect (
          plugin->vst->gtk_window_parent,
          plugin->vst->delete_event_id);
      vst_plugin_close_ui (plugin->vst);
      break;
    default:
      g_return_if_reached ();
      break;
    }
}

/**
 * Connect the output Ports of the given source
 * Plugin to the input Ports of the given
 * destination Plugin.
 *
 * Used when automatically connecting a Plugin
 * in the Channel strip to the next Plugin.
 */
void
plugin_connect_to_plugin (
  Plugin * src,
  Plugin * dest)
{
  int i, j, last_index, num_ports_to_connect;
  Port * in_port, * out_port;

  if (src->descr->num_audio_outs == 1 &&
      dest->descr->num_audio_ins == 1)
    {
      last_index = 0;
      for (i = 0; i < src->num_out_ports; i++)
        {
          out_port = src->out_ports[i];

          if (out_port->identifier.type == TYPE_AUDIO)
            {
              for (j = 0;
                   j < dest->num_in_ports; j++)
                {
                  in_port = dest->in_ports[j];

                  if (in_port->identifier.type == TYPE_AUDIO)
                    {
                      port_connect (
                        out_port,
                        in_port, 1);
                      goto done1;
                    }
                }
            }
        }
done1:
      ;
    }
  else if (src->descr->num_audio_outs == 1 &&
           dest->descr->num_audio_ins > 1)
    {
      /* plugin is mono and next plugin is
       * not mono, so connect the mono out to
       * each input */
      for (i = 0; i < src->num_out_ports; i++)
        {
          out_port = src->out_ports[i];

          if (out_port->identifier.type == TYPE_AUDIO)
            {
              for (j = 0;
                   j < dest->num_in_ports;
                   j++)
                {
                  in_port = dest->in_ports[j];

                  if (in_port->identifier.type == TYPE_AUDIO)
                    {
                      port_connect (
                        out_port,
                        in_port, 1);
                    }
                }
              break;
            }
        }
    }
  else if (src->descr->num_audio_outs > 1 &&
           dest->descr->num_audio_ins == 1)
    {
      /* connect multi-output channel into mono by
       * only connecting to the first input channel
       * found */
      for (i = 0; i < dest->num_in_ports; i++)
        {
          in_port = dest->in_ports[i];

          if (in_port->identifier.type == TYPE_AUDIO)
            {
              for (j = 0;
                   j < src->num_out_ports; j++)
                {
                  out_port = src->out_ports[j];

                  if (out_port->identifier.type == TYPE_AUDIO)
                    {
                      port_connect (
                        out_port,
                        in_port, 1);
                      goto done2;
                    }
                }
              break;
            }
        }
done2:
      ;
    }
  else if (src->descr->num_audio_outs > 1 &&
           dest->descr->num_audio_ins > 1)
    {
      /* connect to as many audio outs this
       * plugin has, or until we can't connect
       * anymore */
      num_ports_to_connect =
        MIN (src->descr->num_audio_outs,
             dest->descr->num_audio_ins);
      last_index = 0;
      int ports_connected = 0;
      for (i = 0; i < src->num_out_ports; i++)
        {
          out_port = src->out_ports[i];

          if (out_port->identifier.type == TYPE_AUDIO)
            {
              for (;
                   last_index <
                     dest->num_in_ports;
                   last_index++)
                {
                  in_port =
                    dest->in_ports[last_index];
                  if (in_port->identifier.type == TYPE_AUDIO)
                    {
                      port_connect (
                        out_port,
                        in_port, 1);
                      last_index++;
                      ports_connected++;
                      break;
                    }
                }
              if (ports_connected ==
                  num_ports_to_connect)
                break;
            }
        }
    }

  /* connect prev midi outs to next midi ins */
  /* this connects only one midi out to all of the
   * midi ins of the next plugin */
  for (i = 0; i < src->num_out_ports; i++)
    {
      out_port = src->out_ports[i];

      if (out_port->identifier.type == TYPE_EVENT)
        {
          for (j = 0;
               j < dest->num_in_ports; j++)
            {
              in_port = dest->in_ports[j];

              if (in_port->identifier.type == TYPE_EVENT)
                {
                  port_connect (
                    out_port,
                    in_port, 1);
                }
            }
          break;
        }
    }
}

/**
 * Connects the Plugin's output Port's to the
 * input Port's of the given Channel's prefader.
 *
 * Used when doing automatic connections.
 */
void
plugin_connect_to_prefader (
  Plugin *  pl,
  Channel * ch)
{
  int i, last_index;
  Port * out_port;
  PortType type = ch->track->out_signal_type;

  if (type == TYPE_EVENT)
    {
      for (i = 0; i < pl->num_out_ports; i++)
        {
          out_port = pl->out_ports[i];
          if (out_port->identifier.type ==
                TYPE_EVENT &&
              out_port->identifier.flow ==
                FLOW_OUTPUT)
            {
              port_connect (
                out_port, ch->midi_out, 1);
            }
        }
    }
  else if (type == TYPE_AUDIO)
    {
      if (pl->descr->num_audio_outs == 1)
        {
          /* if mono find the audio out and connect to
           * both stereo out L and R */
          for (i = 0; i < pl->num_out_ports; i++)
            {
              out_port = pl->out_ports[i];

              if (out_port->identifier.type ==
                    TYPE_AUDIO)
                {
                  port_connect (
                    out_port,
                    ch->prefader.stereo_in->l, 1);
                  port_connect (
                    out_port,
                    ch->prefader.stereo_in->r, 1);
                  break;
                }
            }
        }
      else if (pl->descr->num_audio_outs > 1)
        {
          last_index = 0;

          for (i = 0; i < pl->num_out_ports; i++)
            {
              out_port = pl->out_ports[i];

              if (out_port->identifier.type !=
                    TYPE_AUDIO)
                continue;

              if (last_index == 0)
                {
                  port_connect (
                    out_port,
                    ch->prefader.stereo_in->l, 1);
                  last_index++;
                }
              else if (last_index == 1)
                {
                  port_connect (
                    out_port,
                    ch->prefader.stereo_in->r, 1);
                  break;
                }
            }
        }
    }
}

/**
 * Disconnect the automatic connections from the
 * Plugin to the Channel's prefader (if last
 * Plugin).
 */
void
plugin_disconnect_from_prefader (
  Plugin *  pl,
  Channel * ch)
{
  int i;
  Port * out_port;
  PortType type = ch->track->out_signal_type;

  for (i = 0; i < pl->num_out_ports; i++)
    {
      out_port = pl->out_ports[i];
      if (type == TYPE_AUDIO &&
          out_port->identifier.type == TYPE_AUDIO)
        {
          if (ports_connected (
                out_port,
                ch->prefader.stereo_in->l))
            port_disconnect (
              out_port,
              ch->prefader.stereo_in->l);
          if (ports_connected (
                out_port,
                ch->prefader.stereo_in->r))
            port_disconnect (
              out_port,
              ch->prefader.stereo_in->r);
        }
      else if (type == TYPE_EVENT &&
               out_port->identifier.type ==
                 TYPE_EVENT)
        {
          if (ports_connected (
                out_port,
                ch->prefader.midi_in))
            port_disconnect (
              out_port,
              ch->prefader.midi_in);
        }
    }
}

/**
 * Disconnect the automatic connections from the
 * given source Plugin to the given destination
 * Plugin.
 */
void
plugin_disconnect_from_plugin (
  Plugin * src,
  Plugin * dest)
{
  int i, j, last_index, num_ports_to_connect;
  Port * in_port, * out_port;

  if (src->descr->num_audio_outs == 1 &&
      dest->descr->num_audio_ins == 1)
    {
      last_index = 0;
      for (i = 0; i < src->num_out_ports; i++)
        {
          out_port = src->out_ports[i];

          if (out_port->identifier.type == TYPE_AUDIO)
            {
              for (j = 0;
                   j < dest->num_in_ports; j++)
                {
                  in_port = dest->in_ports[j];

                  if (in_port->identifier.type == TYPE_AUDIO)
                    {
                      port_disconnect (
                        out_port,
                        in_port);
                      goto done1;
                    }
                }
            }
        }
done1:
      ;
    }
  else if (src->descr->num_audio_outs == 1 &&
           dest->descr->num_audio_ins > 1)
    {
      /* plugin is mono and next plugin is
       * not mono, so disconnect the mono out from
       * each input */
      for (i = 0; i < src->num_out_ports; i++)
        {
          out_port = src->out_ports[i];

          if (out_port->identifier.type == TYPE_AUDIO)
            {
              for (j = 0;
                   j < dest->num_in_ports;
                   j++)
                {
                  in_port = dest->in_ports[j];

                  if (in_port->identifier.type == TYPE_AUDIO)
                    {
                      port_disconnect (
                        out_port,
                        in_port);
                    }
                }
              break;
            }
        }
    }
  else if (src->descr->num_audio_outs > 1 &&
           dest->descr->num_audio_ins == 1)
    {
      /* disconnect multi-output channel from mono
       * by disconnecting to the first input channel
       * found */
      for (i = 0; i < dest->num_in_ports; i++)
        {
          in_port = dest->in_ports[i];

          if (in_port->identifier.type == TYPE_AUDIO)
            {
              for (j = 0;
                   j < src->num_out_ports; j++)
                {
                  out_port = src->out_ports[j];

                  if (out_port->identifier.type == TYPE_AUDIO)
                    {
                      port_disconnect (
                        out_port,
                        in_port);
                      goto done2;
                    }
                }
              break;
            }
        }
done2:
      ;
    }
  else if (src->descr->num_audio_outs > 1 &&
           dest->descr->num_audio_ins > 1)
    {
      /* connect to as many audio outs this
       * plugin has, or until we can't connect
       * anymore */
      num_ports_to_connect =
        MIN (src->descr->num_audio_outs,
             dest->descr->num_audio_ins);
      last_index = 0;
      int ports_disconnected = 0;
      for (i = 0; i < src->num_out_ports; i++)
        {
          out_port = src->out_ports[i];

          if (out_port->identifier.type == TYPE_AUDIO)
            {
              for (;
                   last_index <
                     dest->num_in_ports;
                   last_index++)
                {
                  in_port =
                    dest->in_ports[last_index];
                  if (in_port->identifier.type == TYPE_AUDIO)
                    {
                      port_disconnect (
                        out_port,
                        in_port);
                      last_index++;
                      ports_disconnected++;
                      break;
                    }
                }
              if (ports_disconnected ==
                  num_ports_to_connect)
                break;
            }
        }
    }

  /* disconnect MIDI connections */
  for (i = 0; i < src->num_out_ports; i++)
    {
      out_port = src->out_ports[i];

      if (out_port->identifier.type == TYPE_EVENT)
        {
          for (j = 0;
               j < dest->num_in_ports; j++)
            {
              in_port = dest->in_ports[j];

              if (in_port->identifier.type ==
                    TYPE_EVENT)
                {
                  port_disconnect (
                    out_port,
                    in_port);
                }
            }
        }
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
    "DISCONNECTED ALL PORTS OF %p PLUGIN %d %d",
    plugin,
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

  ports_remove (
    plugin->in_ports,
    &plugin->num_in_ports);
  ports_remove (
    plugin->out_ports,
    &plugin->num_out_ports);

  /* delete automation tracks */
  plugin_remove_ats_from_automation_tracklist (
    plugin);
  for (int i = 0; i < plugin->num_ats; i++)
    {
      automation_track_free (plugin->ats[i]);
    }
  free (plugin->ats);

  free (plugin);
}

SERIALIZE_SRC (Plugin, plugin);
DESERIALIZE_SRC (Plugin, plugin);
