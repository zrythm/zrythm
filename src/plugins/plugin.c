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

#include "zrythm-config.h"

#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include "audio/automation_tracklist.h"
#include "audio/channel.h"
#include "audio/control_port.h"
#include "audio/engine.h"
#include "audio/track.h"
#include "audio/transport.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/instrument_track.h"
#include "gui/widgets/main_window.h"
#include "plugins/plugin.h"
#include "plugins/plugin_manager.h"
#include "plugins/lv2_plugin.h"
#include "plugins/lv2/lv2_control.h"
#include "plugins/lv2/lv2_gtk.h"
#include "plugins/lv2/lv2_state.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/arrays.h"
#include "utils/dialogs.h"
#include "utils/io.h"
#include "utils/flags.h"
#include "utils/math.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

/**
 * Plugin UI refresh rate limits.
 */
#define MIN_REFRESH_RATE 30.f
#define MAX_REFRESH_RATE 121.f

void
plugin_init_loaded (
  Plugin * self)
{
  self->magic = PLUGIN_MAGIC;

#ifdef HAVE_CARLA
  if (self->descr->open_with_carla)
    {
      g_return_if_fail (self->carla);
      self->carla->plugin = self;
      carla_native_plugin_init_loaded (self->carla);
    }
  else
    {
#endif
      switch (self->descr->protocol)
        {
        case PROT_LV2:
          g_return_if_fail (self->lv2);
          self->lv2->plugin = self;
          lv2_plugin_init_loaded (self->lv2);
          break;
        default:
          g_warn_if_reached ();
          break;
        }
#ifdef HAVE_CARLA
    }
#endif

  plugin_instantiate (self);

  /*Track * track = plugin_get_track (self);*/
  /*plugin_generate_automation_tracks (self, track);*/
}

static void
plugin_init (
  Plugin * plugin,
  int      track_pos,
  int      slot)
{
  g_message (
    "%s: %s (%s) track pos %d slot %d",
    __func__, plugin->descr->name,
    plugin_protocol_strings[
      plugin->descr->protocol].str,
    track_pos, slot);

  plugin->in_ports_size = 1;
  plugin->out_ports_size = 1;
  plugin->id.track_pos = track_pos;
  plugin->id.slot = slot;
  plugin->magic = PLUGIN_MAGIC;

  plugin->in_ports =
    calloc (1, sizeof (Port *));
  plugin->out_ports =
    calloc (1, sizeof (Port *));

  /* add enabled port */
  Port * port =
    port_new_with_type (
      TYPE_CONTROL, FLOW_INPUT, _("Enabled"));
  plugin_add_in_port (plugin, port);
  port->id.flags |=
    PORT_FLAG_PLUGIN_ENABLED;
  port->id.flags |=
    PORT_FLAG_TOGGLE;
  port->id.flags |=
    PORT_FLAG_AUTOMATABLE;
  port->minf = 0.f;
  port->maxf = 1.f;
  port->zerof = 0.f;
  port->deff = 1.f;
  port->control = 1.f;
  port->carla_param_id = -1;
  plugin->enabled = port;

  plugin->selected_bank.bank_idx = -1;
  plugin->selected_bank.idx = -1;
  plugin->selected_preset.bank_idx = -1;
  plugin->selected_preset.idx = -1;
}

PluginBank *
plugin_add_bank_if_not_exists (
  Plugin * self,
  const char * uri,
  const char * name)
{
  for (int i = 0; i < self->num_banks; i++)
    {
      PluginBank * bank = self->banks[i];
      if (uri)
        {
          if (string_is_equal (bank->uri, uri, 0))
            {
              return bank;
            }
        }
      else
        {
          if (string_is_equal (bank->name, name, 0))
            {
              return bank;
            }
        }
    }

  PluginBank * bank =
    calloc (1, sizeof (PluginBank));

  bank->id.idx = -1;
  bank->id.bank_idx = self->num_banks;
  plugin_identifier_copy (
    &bank->id.plugin_id, &self->id);
  bank->name = g_strdup (name);
  if (uri)
    bank->uri = g_strdup (uri);

  array_double_size_if_full (
    self->banks, self->num_banks, self->banks_size,
    PluginBank *);
  array_append (self->banks, self->num_banks, bank);

  return bank;
}

void
plugin_add_preset_to_bank (
  Plugin *       self,
  PluginBank *   bank,
  PluginPreset * preset)
{
  preset->id.idx = bank->num_presets;
  preset->id.bank_idx = bank->id.bank_idx;
  plugin_identifier_copy (
    &preset->id.plugin_id, &bank->id.plugin_id);

  array_double_size_if_full (
    bank->presets, bank->num_presets,
    bank->presets_size, PluginPreset *);
  array_append (
    bank->presets, bank->num_presets, preset);
}

static void
populate_banks (
  Plugin * self)
{
  g_message ("populating plugin banks...");

  PluginDescriptor * descr = self->descr;

#ifdef HAVE_CARLA
  if (descr->open_with_carla)
    {
      carla_native_plugin_populate_banks (
        self->carla);
    }
  else
    {
#endif
      switch (descr->protocol)
        {
        case PROT_LV2:
          lv2_plugin_populate_banks (self->lv2);
          break;
        default:
          break;
        }
#ifdef HAVE_CARLA
    }
#endif

  /* select the init preset */
  self->selected_bank.bank_idx = 0;
  self->selected_bank.idx = -1;
  plugin_identifier_copy (
    &self->selected_bank.plugin_id, &self->id);
  self->selected_preset.bank_idx = 0;
  self->selected_preset.idx = 0;
  plugin_identifier_copy (
    &self->selected_preset.plugin_id, &self->id);
}

void
plugin_set_selected_bank_from_index (
  Plugin * self,
  int      idx)
{
  self->selected_bank.bank_idx = idx;
  self->selected_preset.bank_idx = idx;
  plugin_set_selected_preset_from_index (
    self, 0);
}

void
plugin_set_selected_preset_from_index (
  Plugin * self,
  int      idx)
{
  self->selected_preset.idx = idx;

  g_message (
    "applying preset at index %d", idx);

  if (self->descr->open_with_carla)
    {
#ifdef HAVE_CARLA
      /* if init preset */
      if (self->selected_bank.bank_idx == 0 &&
          idx == 0)
        {
          carla_reset_parameters (
            self->carla->host_handle, 0);
        }
      else
        {
          carla_set_program (
            self->carla->host_handle, 0,
            (uint32_t)
            self->banks[
              self->selected_bank.bank_idx]->
                presets[idx]->carla_program);
        }
#endif
    }
  else if (self->descr->protocol == PROT_LV2)
    {
      /* if init preset */
      if (self->selected_bank.bank_idx == 0 &&
          idx == 0)
        {
          /* TODO init all control ports */
        }
      else
        {
          LilvNode * pset_uri =
            lilv_new_uri (
              LILV_WORLD,
              self->banks[
                self->selected_bank.bank_idx]->
                  presets[idx]->uri);
          lv2_state_apply_preset (
            self->lv2, pset_uri);
          lilv_node_free (pset_uri);
        }
    }
}

/**
 * Creates/initializes a plugin and its internal
 * plugin (LV2, etc.)
 * using the given descriptor.
 */
Plugin *
plugin_new_from_descr (
  PluginDescriptor * descr,
  int                track_pos,
  int                slot)
{
  g_message (
    "%s: %s (%s) track pos %d slot %d",
    __func__, descr->name,
    plugin_protocol_strings[descr->protocol].str,
    track_pos, slot);

  Plugin * plugin = calloc (1, sizeof (Plugin));

  plugin->descr =
    plugin_descriptor_clone (descr);
  plugin_init (plugin, track_pos, slot);

#ifdef HAVE_CARLA
  if (descr->protocol == PROT_VST ||
      descr->protocol == PROT_VST3 ||
      descr->protocol == PROT_AU ||
      descr->protocol == PROT_SFZ ||
      descr->protocol == PROT_SF2)
    {
new_carla_plugin:
      plugin->descr->open_with_carla = true;
      carla_native_plugin_new_from_descriptor (
        plugin);
    }
  else
    {
#endif
      switch (plugin->descr->protocol)
        {
        case PROT_LV2:
          {
#ifdef HAVE_CARLA
            /* try to bridge bridgable plugins */
            if (!ZRYTHM_TESTING &&
                g_settings_get_boolean (
                  S_P_PLUGINS_UIS,
                  "bridge-unsupported"))
              {
                LilvNode * lv2_uri =
                  lilv_new_uri (
                    LILV_WORLD, descr->uri);
                const LilvPlugin * lilv_plugin =
                  lilv_plugins_get_by_uri (
                    PM_LILV_NODES.lilv_plugins,
                    lv2_uri);
                lilv_node_free (lv2_uri);
                LilvUIs * uis =
                  lilv_plugin_get_uis (lilv_plugin);
                const LilvUI * picked_ui;
                bool needs_bridging =
                  lv2_plugin_pick_ui (
                    uis, LV2_PLUGIN_UI_FOR_BRIDGING,
                    &picked_ui, NULL);
                lilv_uis_free (uis);

                if (needs_bridging &&
                    /* carla doesn't work with
                     * CV plugins */
                    descr->num_cv_ins == 0 &&
                    descr->num_cv_outs == 0)
                  {
                    goto new_carla_plugin;
                  }
              }
#endif
            lv2_plugin_new_from_uri (
              plugin, descr->uri);
          }
          break;
        default:
          break;
        }
#ifdef HAVE_CARLA
    }
#endif

  /* update banks */
  populate_banks (plugin);

  return plugin;
}

/**
 * Create a dummy plugin for tests.
 */
Plugin *
plugin_new_dummy (
  ZPluginCategory cat,
  int            track_pos,
  int            slot)
{
  Plugin * self = calloc (1, sizeof (Plugin));

  self->descr =
    calloc (1, sizeof (PluginDescriptor));
  PluginDescriptor * descr = self->descr;
  descr->author = g_strdup ("Hoge");
  descr->name = g_strdup ("Dummy Plugin");
  descr->category = cat;
  descr->category_str =
    g_strdup ("Dummy Plugin Category");

  plugin_init (self, track_pos, slot);

  return self;
}

/**
 * Removes the automation tracks associated with
 * this plugin from the automation tracklist in the
 * corresponding track.
 *
 * Used e.g. when moving plugins.
 *
 * @param free_ats Also free the ats.
 */
void
plugin_remove_ats_from_automation_tracklist (
  Plugin * pl,
  int      free_ats)
{
  Track * track = plugin_get_track (pl);
  AutomationTracklist * atl =
    track_get_automation_tracklist (track);
  for (int i = atl->num_ats - 1; i >= 0; i--)
    {
      AutomationTrack * at = atl->ats[i];
      if (at->port_id.owner_type ==
            PORT_OWNER_TYPE_PLUGIN ||
          at->port_id.flags &
            PORT_FLAG_PLUGIN_CONTROL)
        {
          if (at->port_id.plugin_id.slot ==
                pl->id.slot &&
              at->port_id.plugin_id.slot_type ==
                pl->id.slot_type)
            {
              automation_tracklist_remove_at (
                atl, at, free_ats);
            }
        }
    }
}

/**
 * Moves the plugin to the given slot in
 * the given channel.
 *
 * If a plugin already exists, it deletes it and
 * replaces it.
 */
void
plugin_move (
  Plugin *       pl,
  Channel *      ch,
  PluginSlotType slot_type,
  int            slot)
{
  g_return_if_fail (pl && ch);

  /* confirm if another plugin exists */
  Plugin * existing_pl = NULL;
  switch (slot_type)
    {
    case PLUGIN_SLOT_MIDI_FX:
      existing_pl = ch->midi_fx[slot];
      break;
    case PLUGIN_SLOT_INSTRUMENT:
      existing_pl = ch->instrument;
      break;
    case PLUGIN_SLOT_INSERT:
      existing_pl = ch->inserts[slot];
      break;
    }
  if (existing_pl)
    {
      GtkDialog * dialog =
        dialogs_get_overwrite_plugin_dialog (
          GTK_WINDOW (MAIN_WINDOW));
      int result =
        gtk_dialog_run (dialog);
      gtk_widget_destroy (GTK_WIDGET (dialog));

      /* do nothing if not accepted */
      if (result != GTK_RESPONSE_ACCEPT)
        return;
    }

  int prev_slot = pl->id.slot;
  PluginSlotType prev_slot_type =
    pl->id.slot_type;
  Channel * prev_ch = plugin_get_channel (pl);

  /* move plugin's automation from src to dest */
  plugin_move_automation (
    pl, prev_ch, ch, slot_type, slot);

  /* remove plugin from its channel */
  channel_remove_plugin (
    prev_ch, prev_slot_type, prev_slot,
    0, 0, F_NO_RECALC_GRAPH);

  /* add plugin to its new channel */
  channel_add_plugin (
    ch, slot_type, slot, pl, 0, 0, F_RECALC_GRAPH,
    F_PUBLISH_EVENTS);

  EVENTS_PUSH (ET_CHANNEL_SLOTS_CHANGED, prev_ch);
  EVENTS_PUSH (ET_CHANNEL_SLOTS_CHANGED, ch);
}

/**
 * Sets the channel and slot on the plugin and
 * its ports.
 */
void
plugin_set_channel_and_slot (
  Plugin *       pl,
  Channel *      ch,
  PluginSlotType slot_type,
  int            slot)
{
  pl->id.track_pos = ch->track_pos;
  pl->id.slot = slot;
  pl->id.slot_type = slot_type;

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

  if (
    !pl->descr->open_with_carla &&
      pl->descr->protocol == PROT_LV2)
    {
      lv2_plugin_update_port_identifiers (
        pl->lv2);
    }
}

/**
 * Returns if the Plugin has a supported custom
 * UI.
 *
 * TODO
 */
int
plugin_has_supported_custom_ui (
  Plugin * self)
{
  switch (self->descr->protocol)
    {
    case PROT_LV2:
      break;
    default:
      g_return_val_if_reached (-1);
      break;
    }
  g_return_val_if_reached (-1);
}

Track *
plugin_get_track (
  Plugin * self)
{
  g_return_val_if_fail (
    self &&
      self->id.track_pos < TRACKLIST->num_tracks,
    NULL);
  Track * track =
    TRACKLIST->tracks[self->id.track_pos];
  g_return_val_if_fail (track, NULL);

  return track;
}

Channel *
plugin_get_channel (
  Plugin * self)
{
  Track * track = plugin_get_track (self);
  g_return_val_if_fail (track, NULL);
  Channel * ch = track->channel;
  g_return_val_if_fail (ch, NULL);

  return ch;
}

Plugin *
plugin_find (
  PluginIdentifier * id)
{
  Plugin plugin;
  plugin_identifier_copy (
    &plugin.id, id);
  Channel * ch = plugin_get_channel (&plugin);
  Plugin * ret = NULL;
  switch (id->slot_type)
    {
    case PLUGIN_SLOT_MIDI_FX:
      ret = ch->midi_fx[id->slot];
      break;
    case PLUGIN_SLOT_INSTRUMENT:
      ret = ch->instrument;
      break;
    case PLUGIN_SLOT_INSERT:
      ret = ch->inserts[id->slot];
      break;
    }
  g_return_val_if_fail (ret, NULL);

  return ret;
}

void
plugin_activate (
  Plugin * pl,
  bool     activate)
{
  g_return_if_fail (pl);

  if (pl->descr->open_with_carla)
    {
#ifdef HAVE_CARLA
      carla_native_plugin_activate (
        pl->carla, activate);
#endif
    }
  else
    {
      switch (pl->descr->protocol)
        {
        case PROT_LV2:
          lv2_plugin_activate (pl->lv2, activate);
          break;
        default:
          g_warn_if_reached ();
          break;
        }
    }
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
  if (
#ifdef HAVE_CARLA
    !pl->descr->open_with_carla &&
#endif
      pl->descr->protocol == PROT_LV2)
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
  port->id.port_index = \
    pl->num_##type##_ports; \
  port_set_owner_plugin (port, pl); \
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
#undef ADD_PORT

/**
 * Moves the Plugin's automation from one Channel
 * to another.
 */
void
plugin_move_automation (
  Plugin *       pl,
  Channel *      prev_ch,
  Channel *      ch,
  PluginSlotType new_slot_type,
  int            new_slot)
{
  Track * prev_track =
    channel_get_track (prev_ch);
  AutomationTracklist * prev_atl =
    track_get_automation_tracklist (prev_track);
  Track * track = channel_get_track (ch);
  AutomationTracklist * atl =
    track_get_automation_tracklist (track);

  for (int i = prev_atl->num_ats - 1; i >= 0; i--)
    {
      AutomationTrack * at = prev_atl->ats[i];
      Port * port =
        automation_track_get_port (at);
      g_return_if_fail (IS_PORT (port));
        /*g_message (*/
          /*"before port %s", port->id.label);*/
      if (!port)
        continue;
      if (port->id.owner_type ==
            PORT_OWNER_TYPE_PLUGIN)
        {
          /*g_message ("port %s", port->id.label);*/
          Plugin * port_pl =
            port_get_plugin (port, 1);
          if (port_pl != pl)
            continue;
        }
      else
        continue;

      /* delete from prev channel */
      automation_tracklist_delete_at (
        prev_atl, at, F_NO_FREE);
      /*g_message ("deleted %s, num ats after for track %s: %d", port->id.label, prev_ch->track->name,*/
        /*prev_atl->num_ats);*/

      /* add to new channel */
      automation_tracklist_add_at (atl, at);

      /* update the automation track port
       * identifier */
      at->port_id.plugin_id.slot = new_slot;
      at->port_id.plugin_id.slot_type =
        new_slot_type;
    }
}

/**
 * Sets the UI refresh rate on the Plugin.
 */
void
plugin_set_ui_refresh_rate (
  Plugin * self)
{
  if (ZRYTHM_TESTING)
    {
      self->ui_update_hz = 30.f;
      return;
    }

  /* if no preferred refresh rate is set,
   * use the monitor's refresh rate */
  if (!g_settings_get_int (
         S_P_PLUGINS_UIS, "refresh-rate"))
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
          self->ui_update_hz, 0.f));
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
          S_P_PLUGINS_UIS, "refresh-rate");
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
 * Returns the escaped name of the plugin.
 */
char *
plugin_get_escaped_name (
  Plugin * pl)
{
  char tmp[900];
  io_escape_dir_name (tmp, pl->descr->name);
  return g_strdup (tmp);
}

/**
 * Generates automatables for the plugin.
 *
 * Plugin must be instantiated already.
 *
 * @param track The Track this plugin belongs to.
 *   This is passed because the track might not be
 *   in the project yet so we can't fetch it
 *   through indices.
 */
void
plugin_generate_automation_tracks (
  Plugin * self,
  Track *  track)
{
  g_message (
    "generating automation tracks for %s...",
    self->descr->name);

  AutomationTracklist * atl =
    track_get_automation_tracklist (track);
  for (int i = 0; i < self->num_in_ports; i++)
  {
    Port * port = self->in_ports[i];
    if (port->id.type != TYPE_CONTROL)
      continue;

    AutomationTrack * at =
      automation_track_new (port);
    automation_tracklist_add_at (atl, at);
  }
}

/**
 * Gets the enable/disable port for this plugin.
 */
Port *
plugin_get_enabled_port (
  Plugin * self)
{
  for (int i = 0; i < self->num_in_ports; i++)
    {
      Port * port = self->in_ports[i];
      if (port->id.flags & PORT_FLAG_PLUGIN_ENABLED)
        return port;
    }
  g_return_val_if_reached (NULL);
}

/**
 * To be called when changes to the plugin
 * identifier were made, so we can update all
 * children recursively.
 */
void
plugin_update_identifier (
  Plugin * self)
{
  /* set port identifier track poses */
  int i;
  for (i = 0; i < self->num_in_ports; i++)
    {
      port_update_track_pos (
        self->in_ports[i], self->id.track_pos);
    }
  for (i = 0; i < self->num_out_ports; i++)
    {
      port_update_track_pos (
        self->out_ports[i], self->id.track_pos);
    }
}

/**
 * Sets the track and track_pos on the plugin.
 */
void
plugin_set_track_pos (
  Plugin * pl,
  int      pos)
{
  pl->id.track_pos = pos;

  plugin_update_identifier (pl);
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

  if (pl->descr->open_with_carla)
    {
#ifdef HAVE_CARLA
      carla_native_plugin_instantiate (
        pl->carla, !PROJECT->loaded);
#else
      g_return_val_if_reached (-1);
#endif
    }
  else
    {
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
        default:
          g_warn_if_reached ();
          return -1;
          break;
        }
    }
  control_port_set_val_from_normalized (
    pl->enabled, 1.f, 0);

  return 0;
}

/**
 * Prepare plugin for processing.
 */
void
plugin_prepare_process (
  Plugin * self)
{
  for (int i = 0; i < self->num_in_ports; i++)
    {
      port_clear_buffer (self->in_ports[i]);
    }
  for (int i = 0; i < self->num_out_ports; i++)
    {
      port_clear_buffer (self->out_ports[i]);
    }
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
  if (!plugin_is_enabled (plugin))
    {
      plugin_process_passthrough (
        plugin, g_start_frames, local_offset,
        nframes);
      return;
    }

  /* if has MIDI input port */
  if (plugin->descr->num_midi_ins > 0)
    {
      /* if recording, write MIDI events to the
       * region TODO */

        /* if there is a midi note in this buffer
         * range TODO */
        /* add midi events to input port */
    }

#ifdef HAVE_CARLA
  if (plugin->descr->open_with_carla)
    {
      carla_native_plugin_proces (
        plugin->carla, g_start_frames, nframes);
    }
  else
    {
#endif
      switch (plugin->descr->protocol)
        {
        case PROT_LV2:
          lv2_plugin_process (
            plugin->lv2, g_start_frames, nframes);
          break;
        default:
          break;
        }
#ifdef HAVE_CARLA
    }
#endif

  /* turn off any trigger input controls */
  for (int i = 0; i < plugin->num_in_ports; i++)
    {
      Port * port = plugin->in_ports[i];
      if (port->id.type == TYPE_CONTROL &&
          port->id.flags &
            PORT_FLAG_TRIGGER &&
          !math_floats_equal (port->control, 0.f))
        {
          port_set_control_value (port, 0.f, 0, 1);
        }
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
  if (plugin->descr->open_with_carla)
    {
#ifdef HAVE_CARLA
      carla_native_plugin_open_ui (
        plugin->carla, 1);
#endif
    }
  else
    {
      if (GTK_IS_WINDOW (plugin->window))
        {
          gtk_window_present (
            GTK_WINDOW (plugin->window));
          gtk_window_set_transient_for (
            GTK_WINDOW (plugin->window),
            (GtkWindow *) MAIN_WINDOW);
        }
      else
        {
          switch (plugin->descr->protocol)
            {
            case PROT_LV2:
              {
                Lv2Plugin * lv2_plugin =
                  plugin->lv2;
                if (lv2_plugin->has_external_ui &&
                    lv2_plugin->external_ui_widget)
                  {
                    lv2_plugin->
                      external_ui_widget->
                        show (
                        lv2_plugin->
                          external_ui_widget);
                  }
                else
                  {
                    lv2_gtk_open_ui (lv2_plugin);
                  }
              }
              break;
            default:
              break;
            }
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
    MIXER_SELECTIONS, pl);
}

/**
 * Generates a state directory path for the plugin.
 *
 * @param mkdir Create the directory at the path.
 *
 * @return The path. Must be free'd by caller with
 *   g_free().
 */
char *
plugin_generate_state_dir (
  Plugin * pl,
  bool     mkdir)
{
  char * escaped_name =
    plugin_get_escaped_name (pl);
  char * tmp =
    g_strdup_printf (
      "tmp_%s_XXXXXX", escaped_name);
  char * states_dir =
    project_get_states_dir (
      PROJECT, PROJECT->backup_dir != NULL);
  char * state_dir_plugin =
    g_build_filename (
      states_dir, tmp, NULL);
  g_free (states_dir);
  g_free (tmp);
  g_free (escaped_name);

  if (mkdir)
    io_mkdir (state_dir_plugin);

  return state_dir_plugin;
}

/**
 * Clones the given plugin.
 */
Plugin *
plugin_clone (
  Plugin * pl)
{
  Plugin * clone = NULL;
#ifdef HAVE_CARLA
  if (pl->descr->open_with_carla)
    {
      /* create a new plugin with same descriptor */
      clone =
        plugin_new_from_descr (
          pl->descr, pl->id.track_pos,
          pl->id.slot);
      g_return_val_if_fail (
        clone && clone->carla, NULL);

      /* instantiate */
      int ret = plugin_instantiate (clone);
      g_return_val_if_fail (!ret, NULL);

      /* save the state of the original plugin */
      char * state_dir_pl =
        plugin_generate_state_dir (
          pl, true);
      carla_native_plugin_save_state (
        pl->carla, state_dir_pl);

      /* load the state to the new plugin. */
      carla_native_plugin_load_state (
        clone->carla, state_dir_pl);

      g_free (state_dir_pl);
    }
  else
    {
#endif
      if (pl->descr->protocol == PROT_LV2)
        {
          /* NOTE from rgareus:
           * I think you can use   lilv_state_restore (lilv_state_new_from_instance (..), ...)
           * and skip  lilv_state_new_from_file() ; lilv_state_save ()
           * lilv_state_new_from_instance() handles files and externals, too */

          /* save state to file */
          char * state_dir_plugin =
            plugin_generate_state_dir (
              pl, true);
          lv2_plugin_save_state_to_file (
            pl->lv2, state_dir_plugin);
          g_free (state_dir_plugin);
          g_return_val_if_fail (
            pl->lv2->state_file, NULL);

          /* create a new plugin with same descriptor */
          clone =
            plugin_new_from_descr (
              pl->descr, pl->id.track_pos,
              pl->id.slot);

          /* set the state file on the new Lv2Plugin
           * as the state filed saved on the original
           * so that it can be used when
           * instantiating */
          clone->lv2->state_file =
            g_strdup (pl->lv2->state_file);

          /* instantiate */
          int ret = plugin_instantiate (clone);
          g_return_val_if_fail (!ret, NULL);

          g_return_val_if_fail (
            clone && clone->lv2 &&
            clone->lv2->num_ports ==
              pl->lv2->num_ports,
            NULL);

          /* delete the state file */
          io_remove (pl->lv2->state_file);
        }
#ifdef HAVE_CARLA
    }
#endif
  g_return_val_if_fail (
    pl->num_in_ports || pl->num_out_ports, NULL);

  g_return_val_if_fail (clone, NULL);
  plugin_identifier_copy (
    &clone->id, &pl->id);
  clone->magic = PLUGIN_MAGIC;
  clone->visible = pl->visible;

  return clone;
}

/**
 * Returns whether the plugin is enabled.
 */
bool
plugin_is_enabled (
  Plugin * self)
{
  return control_port_is_toggled (self->enabled);
}

void
plugin_set_enabled (
  Plugin * self,
  bool     enabled,
  bool     fire_events)
{
  port_set_control_value (
    self->enabled, enabled ? 1.f : 0.f, false,
    fire_events);

  if (fire_events)
    {
      EVENTS_PUSH (ET_PLUGIN_STATE_CHANGED, self);
    }
}

/**
 * Processes the plugin by passing through the
 * input to its output.
 *
 * This is called when the plugin is bypassed.
 */
void
plugin_process_passthrough (
  Plugin * self,
  const long      g_start_frames,
  const nframes_t  local_offset,
  const nframes_t nframes)
{
  int last_audio_idx = 0;
  int last_midi_idx = 0;
  for (int i = 0; i < self->num_in_ports; i++)
    {
      bool goto_next = false;
      Port * in_port = self->in_ports[i];
      switch (in_port->id.type)
        {
        case TYPE_AUDIO:
          for (int j = last_audio_idx;
               j < self->num_out_ports;
               j++)
            {
              Port * out_port = self->out_ports[j];
              if (out_port->id.type == TYPE_AUDIO)
                {
                  /* copy */
                  memcpy (
                    &out_port->buf[local_offset],
                    &in_port->buf[local_offset],
                    sizeof (float) * nframes);

                  last_audio_idx = j + 1;
                  goto_next = true;
                  break;
                }
              if (goto_next)
                continue;
            }
          break;
        case TYPE_EVENT:
          for (int j = last_midi_idx;
               j < self->num_out_ports;
               j++)
            {
              Port * out_port = self->out_ports[j];
              if (out_port->id.type == TYPE_EVENT)
                {
                  /* copy */
                  midi_events_append (
                    in_port->midi_events,
                    out_port->midi_events,
                    local_offset,
                    nframes, false);

                  last_midi_idx = j + 1;
                  goto_next = true;
                  break;
                }
              if (goto_next)
                continue;
            }
          break;
        default:
          break;
        }
    }
}

/**
 * hides plugin ui
 */
void
plugin_close_ui (Plugin *plugin)
{
#ifdef HAVE_CARLA
  if (plugin->descr->open_with_carla)
    {
      carla_native_plugin_open_ui (
        plugin->carla, false);
      g_message ("closing carla plugin UI");
    }
  else
    {
      g_message ("closing non-carla plugin UI");
#endif
      if (GTK_IS_WINDOW (plugin->window))
        {
          g_signal_handler_disconnect (
            plugin->window,
            plugin->delete_event_id);
        }

      switch (plugin->descr->protocol)
        {
        case PROT_LV2:
          lv2_gtk_close_ui (plugin->lv2);
          break;
        default:
          g_return_if_reached ();
          break;
        }
#ifdef HAVE_CARLA
    }
#endif
}

/**
 * Returns the event ports in the plugin.
 *
 * @param ports Array to fill in. Must be large
 *   enough.
 *
 * @return The number of ports in the array.
 */
int
plugin_get_event_ports (
  Plugin * pl,
  Port **  ports,
  int      input)
{
  g_return_val_if_fail (pl && ports, -1);

  int index = 0;

  if (input)
    {
      for (int i = 0; i < pl->num_in_ports; i++)
        {
          Port * port = pl->in_ports[i];
          if (port->id.type == TYPE_EVENT)
            {
              ports[index++] = port;
            }
        }
    }
  else
    {
      for (int i = 0; i < pl->num_out_ports; i++)
        {
          Port * port = pl->out_ports[i];
          if (port->id.type == TYPE_EVENT)
            {
              ports[index++] = port;
            }
        }
    }

  return index;
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

          if (out_port->id.type == TYPE_AUDIO)
            {
              for (j = 0;
                   j < dest->num_in_ports; j++)
                {
                  in_port = dest->in_ports[j];

                  if (in_port->id.type == TYPE_AUDIO)
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

          if (out_port->id.type == TYPE_AUDIO)
            {
              for (j = 0;
                   j < dest->num_in_ports;
                   j++)
                {
                  in_port = dest->in_ports[j];

                  if (in_port->id.type == TYPE_AUDIO)
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

          if (in_port->id.type == TYPE_AUDIO)
            {
              for (j = 0;
                   j < src->num_out_ports; j++)
                {
                  out_port = src->out_ports[j];

                  if (out_port->id.type == TYPE_AUDIO)
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

          if (out_port->id.type == TYPE_AUDIO)
            {
              for (;
                   last_index <
                     dest->num_in_ports;
                   last_index++)
                {
                  in_port =
                    dest->in_ports[last_index];
                  if (in_port->id.type == TYPE_AUDIO)
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

      if (out_port->id.type == TYPE_EVENT)
        {
          for (j = 0;
               j < dest->num_in_ports; j++)
            {
              in_port = dest->in_ports[j];

              if (in_port->id.type == TYPE_EVENT)
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
  Track * track =
    channel_get_track (ch);
  PortType type = track->out_signal_type;

  if (type == TYPE_EVENT)
    {
      for (i = 0; i < pl->num_out_ports; i++)
        {
          out_port = pl->out_ports[i];
          if (out_port->id.type ==
                TYPE_EVENT &&
              out_port->id.flow ==
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

              if (out_port->id.type ==
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

              if (out_port->id.type !=
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
  Track * track = channel_get_track (ch);
  PortType type = track->out_signal_type;

  for (i = 0; i < pl->num_out_ports; i++)
    {
      out_port = pl->out_ports[i];
      if (type == TYPE_AUDIO &&
          out_port->id.type == TYPE_AUDIO)
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
               out_port->id.type ==
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

          if (out_port->id.type == TYPE_AUDIO)
            {
              for (j = 0;
                   j < dest->num_in_ports; j++)
                {
                  in_port = dest->in_ports[j];

                  if (in_port->id.type == TYPE_AUDIO)
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

          if (out_port->id.type == TYPE_AUDIO)
            {
              for (j = 0;
                   j < dest->num_in_ports;
                   j++)
                {
                  in_port = dest->in_ports[j];

                  if (in_port->id.type == TYPE_AUDIO)
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

          if (in_port->id.type == TYPE_AUDIO)
            {
              for (j = 0;
                   j < src->num_out_ports; j++)
                {
                  out_port = src->out_ports[j];

                  if (out_port->id.type == TYPE_AUDIO)
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

          if (out_port->id.type == TYPE_AUDIO)
            {
              for (;
                   last_index <
                     dest->num_in_ports;
                   last_index++)
                {
                  in_port =
                    dest->in_ports[last_index];
                  if (in_port->id.type == TYPE_AUDIO)
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

      if (out_port->id.type == TYPE_EVENT)
        {
          for (j = 0;
               j < dest->num_in_ports; j++)
            {
              in_port = dest->in_ports[j];

              if (in_port->id.type ==
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
 * To be called immediately when a channel or
 * plugin is deleted.
 *
 * A call to plugin_free can be made at any point
 * later just to free the resources.
 */
void
plugin_disconnect (
  Plugin * self)
{
  self->deleting = 1;

  /* disconnect all ports */
  ports_disconnect (
    self->in_ports,
    self->num_in_ports, 1);
  ports_disconnect (
    self->out_ports,
    self->num_out_ports, 1);
  g_message (
    "%s: DISCONNECTED ALL PORTS OF %s %d %d",
    __func__, self->descr->name,
    self->num_in_ports,
    self->num_out_ports);

#ifdef HAVE_CARLA
  if (self->descr->open_with_carla)
    {
      carla_native_plugin_close (self->carla);
    }
#endif
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

  free (plugin);
}

SERIALIZE_SRC (Plugin, plugin);
DESERIALIZE_SRC (Plugin, plugin);
