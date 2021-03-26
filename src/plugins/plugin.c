/*
 * Copyright (C) 2018-2021 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, version 3.
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
#include "audio/midi_event.h"
#include "audio/track.h"
#include "audio/transport.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/instrument_track.h"
#include "gui/widgets/main_window.h"
#include "plugins/cached_plugin_descriptors.h"
#include "plugins/carla_native_plugin.h"
#include "plugins/plugin.h"
#include "plugins/plugin_manager.h"
#include "plugins/plugin_gtk.h"
#include "plugins/lv2_plugin.h"
#include "plugins/lv2/lv2_gtk.h"
#include "plugins/lv2/lv2_state.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/arrays.h"
#include "utils/dialogs.h"
#include "utils/dsp.h"
#include "utils/err_codes.h"
#include "utils/gtk.h"
#include "utils/io.h"
#include "utils/flags.h"
#include "utils/math.h"
#include "utils/mem.h"
#include "utils/objects.h"
#include "utils/string.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

void
plugin_init_loaded (
  Plugin * self,
  bool     project)
{
  self->magic = PLUGIN_MAGIC;
  self->is_project = project;

  size_t max_size = 0;
  Port ** ports = NULL;
  int num_ports = 0;
  plugin_append_ports (
    self, &ports, &max_size, true, &num_ports);
  for (int i = 0; i < num_ports; i++)
    {
      ports[i]->magic = PORT_MAGIC;
      ports[i]->is_project = project;
    }

  /* set enabled/gain ports */
  for (int i = 0; i < self->num_in_ports; i++)
    {
      Port * port = self->in_ports[i];
      if (!(port->id.type == TYPE_CONTROL &&
            port->id.flags &
              PORT_FLAG_GENERIC_PLUGIN_PORT))
        continue;

      if (port->id.flags &
            PORT_FLAG_PLUGIN_ENABLED)
        {
          self->enabled = port;
        }
      if (port->id.flags &
            PORT_FLAG_PLUGIN_GAIN)
        {
          self->gain = port;
        }
    }
  g_return_if_fail (self->enabled && self->gain);

#ifdef HAVE_CARLA
  if (self->setting->open_with_carla)
    {
      self->carla =
        object_new (CarlaNativePlugin);
      self->carla->plugin = self;
      carla_native_plugin_init_loaded (self->carla);
    }
  else
    {
#endif
      switch (self->setting->descr->protocol)
        {
        case PROT_LV2:
          self->lv2 = object_new (Lv2Plugin);
          self->lv2->magic = LV2_PLUGIN_MAGIC;
          lv2_plugin_init_loaded (
            self->lv2, project);
          break;
        default:
          g_warn_if_reached ();
          break;
        }
#ifdef HAVE_CARLA
    }
#endif

  if (project)
    {
      int ret =
        plugin_instantiate (self, project, NULL);
      if (ret == 0)
        {
          plugin_activate (self, true);
        }
      else
        {
          /* disable plugin, instantiation failed */
          char * msg =
            g_strdup_printf (
              _("Instantiation failed for "
              "plugin '%s'. Disabling..."),
              self->setting->descr->name);
          g_warning ("%s", msg);
          if (ZRYTHM_HAVE_UI)
            {
              ui_show_error_message (
                MAIN_WINDOW, msg);
            }
          g_free (msg);
          self->instantiation_failed = true;
        }
    }

  /*Track * track = plugin_get_track (self);*/
  /*plugin_generate_automation_tracks (self, track);*/
}

static void
plugin_init (
  Plugin *       plugin,
  int            track_pos,
  PluginSlotType slot_type,
  int            slot)
{
  g_message (
    "%s: %s (%s) track pos %d slot %d",
    __func__, plugin->setting->descr->name,
    plugin_protocol_strings[
      plugin->setting->descr->protocol].str,
    track_pos, slot);

  plugin->in_ports_size = 1;
  plugin->out_ports_size = 1;
  plugin->id.schema_version =
    PLUGIN_IDENTIFIER_SCHEMA_VERSION;
  plugin->id.track_pos = track_pos;
  plugin->id.slot_type = slot_type;
  plugin->id.slot = slot;
  plugin->magic = PLUGIN_MAGIC;

  plugin->in_ports = object_new_n (1, Port *);
  plugin->out_ports = object_new_n (1, Port *);

  /* add enabled port */
  Port * port =
    port_new_with_type (
      TYPE_CONTROL, FLOW_INPUT,
      _("[Zrythm] Enabled"));
  port->id.comment =
    g_strdup (_("Enables or disables the plugin"));
  plugin_add_in_port (plugin, port);
  port->id.flags |=
    PORT_FLAG_PLUGIN_ENABLED;
  port->id.flags |=
    PORT_FLAG_TOGGLE;
  port->id.flags |=
    PORT_FLAG_AUTOMATABLE;
  port->id.flags |=
    PORT_FLAG_GENERIC_PLUGIN_PORT;
  port->minf = 0.f;
  port->maxf = 1.f;
  port->zerof = 0.f;
  port->deff = 1.f;
  port->control = 1.f;
  port->unsnapped_control = 1.f;
  port->carla_param_id = -1;
  plugin->enabled = port;

  /* add gain port */
  port =
    port_new_with_type (
      TYPE_CONTROL, FLOW_INPUT,
      _("[Zrythm] Gain"));
  port->id.comment = g_strdup (_("Plugin gain"));
  plugin_add_in_port (plugin, port);
  port->id.flags |=
    PORT_FLAG_PLUGIN_GAIN;
  port->id.flags |=
    PORT_FLAG_AUTOMATABLE;
  port->id.flags |=
    PORT_FLAG_GENERIC_PLUGIN_PORT;
  port->minf = 0.f;
  port->maxf = 8.f;
  port->zerof = 0.f;
  port->deff = 1.f;
  port_set_control_value (
    port, 1.f, F_NOT_NORMALIZED,
    F_NO_PUBLISH_EVENTS);
  port->carla_param_id = -1;
  plugin->gain = port;

  plugin->selected_bank.bank_idx = -1;
  plugin->selected_bank.idx = -1;
  plugin->selected_preset.bank_idx = -1;
  plugin->selected_preset.idx = -1;

  plugin_set_ui_refresh_rate (plugin);
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
          if (string_is_equal (bank->uri, uri))
            {
              return bank;
            }
        }
      else
        {
          if (string_is_equal (bank->name, name))
            {
              return bank;
            }
        }
    }

  PluginBank * bank = plugin_bank_new ();

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
  array_append (
    self->banks, self->num_banks, bank);

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

#ifdef HAVE_CARLA
  if (self->setting->open_with_carla)
    {
      carla_native_plugin_populate_banks (
        self->carla);
    }
  else
    {
#endif
      switch (self->setting->descr->protocol)
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
  self->selected_bank.schema_version =
    PLUGIN_PRESET_IDENTIFIER_SCHEMA_VERSION;
  self->selected_bank.bank_idx = 0;
  self->selected_bank.idx = -1;
  plugin_identifier_copy (
    &self->selected_bank.plugin_id, &self->id);
  self->selected_preset.schema_version =
    PLUGIN_PRESET_IDENTIFIER_SCHEMA_VERSION;
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

  if (self->setting->open_with_carla)
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
  else if (self->setting->descr->protocol == PROT_LV2)
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
 * using the given setting.
 *
 * @param track_pos The expected position of the
 *   track the plugin will be in.
 * @param slot The expected slot the plugin will
 *   be in.
 */
Plugin *
plugin_new_from_setting (
  PluginSetting * setting,
  int             track_pos,
  PluginSlotType  slot_type,
  int             slot)
{
  Plugin * self = object_new (Plugin);
  self->schema_version = PLUGIN_SCHEMA_VERSION;

  self->setting =
    plugin_setting_clone (setting, F_VALIDATE);
  setting = self->setting;
  const PluginDescriptor * descr =
    self->setting->descr;

  g_message (
    "%s: %s (%s) track pos %d slot %d",
    __func__, descr->name,
    plugin_protocol_strings[descr->protocol].str,
    track_pos, slot);

  plugin_init (self, track_pos, slot_type, slot);
  g_return_val_if_fail (
    self->gain && self->enabled, NULL);

#ifdef HAVE_CARLA
  if (setting->open_with_carla)
    {
      carla_native_plugin_new_from_setting (self);
      if (!self->carla)
        {
          g_warning ("failed to create plugin");
          return NULL;
        }
    }
  else
    {
#endif
      if (descr->protocol == PROT_LV2)
        {
          lv2_plugin_new_from_uri (
            self, descr->uri);
          g_return_val_if_fail (
            self->lv2, NULL);
        }
      else
        {
          g_return_val_if_reached (NULL);
        }
#ifdef HAVE_CARLA
    }
#endif

  /* update banks */
  populate_banks (self);

  if (!ZRYTHM_TESTING)
    {
      /* save the new setting (may have changed
       * during instantiation) */
      plugin_settings_set (
        S_PLUGIN_SETTINGS, self->setting,
        F_SERIALIZE);
    }

  return self;
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
  Plugin * self = object_new (Plugin);
  self->schema_version = PLUGIN_SCHEMA_VERSION;

  PluginDescriptor * descr =
    object_new (PluginDescriptor);
  descr->author = g_strdup ("Hoge");
  descr->name = g_strdup ("Dummy Plugin");
  descr->category = cat;
  descr->category_str =
    g_strdup ("Dummy Plugin Category");

  self->setting =
    plugin_setting_new_default (descr);
  plugin_descriptor_free (descr);

  plugin_init (
    self, track_pos, PLUGIN_SLOT_INSERT, slot);

  return self;
}

void
plugin_append_ports (
  Plugin *  pl,
  Port ***  ports,
  size_t *  max_size,
  bool      is_dynamic,
  int *     size)
{
#define _ADD(port) \
  if (is_dynamic) \
    { \
      array_double_size_if_full ( \
        *ports, (*size), (*max_size), Port *); \
    } \
  else if ((size_t) *size == *max_size) \
    { \
      g_return_if_reached (); \
    } \
  array_append ( \
    *ports, (*size), port)

  for (int i = 0; i < pl->num_in_ports; i++)
    {
      Port * port = pl->in_ports[i];
      g_return_if_fail (port);
      _ADD (port);
    }
  for (int i = 0; i < pl->num_out_ports; i++)
    {
      Port * port = pl->out_ports[i];
      g_return_if_fail (port);
      _ADD (port);
    }

#undef _ADD
}

void
plugin_set_is_project (
  Plugin * self,
  bool     is_project)
{
  self->is_project = is_project;

  size_t max_size = 20;
  Port ** ports =
    object_new_n (max_size, Port *);
  int num_ports = 0;
  Port * port;
  plugin_append_ports (
    self, &ports, &max_size, true, &num_ports);
  for (int i = 0; i < num_ports; i++)
    {
      port = ports[i];
      port_set_is_project (port, is_project);
    }
  free (ports);
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
  bool     free_ats,
  bool     fire_events)
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
                atl, at, free_ats, fire_events);
            }
        }
    }
}

/**
 * Verifies that the plugin identifiers are valid.
 */
bool
plugin_validate (
  Plugin * self)
{
  g_return_val_if_fail (IS_PLUGIN (self), false);

  /*g_return_val_if_fail (self->instantiated, false);*/

  return true;
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
  Track *        track,
  PluginSlotType slot_type,
  int            slot,
  bool           fire_events)
{
  /* confirm if another plugin exists */
  Plugin * existing_pl =
    track_get_plugin_at_slot (
      track, slot_type, slot);
  /* TODO move confirmation to widget */
#if 0
  if (existing_pl && ZRYTHM_HAVE_UI)
    {
      GtkDialog * dialog =
        dialogs_get_overwrite_plugin_dialog (
          GTK_WINDOW (MAIN_WINDOW));
      int result =
        gtk_dialog_run (dialog);
      gtk_widget_destroy (GTK_WIDGET (dialog));

      /* do nothing if not accepted */
      if (result != GTK_RESPONSE_ACCEPT)
        {
          return;
        }
    }
#endif

  int prev_slot = pl->id.slot;
  PluginSlotType prev_slot_type =
    pl->id.slot_type;
  Track * prev_track = plugin_get_track (pl);
  g_return_if_fail (
    IS_TRACK_AND_NONNULL (prev_track));
  Channel * prev_ch = plugin_get_channel (pl);
  g_return_if_fail (
    IS_CHANNEL_AND_NONNULL (prev_ch));

  /* if existing plugin exists, delete it */
  if (existing_pl)
    {
      channel_remove_plugin (
        track->channel, slot_type, slot,
        F_NOT_MOVING_PLUGIN, F_DELETING_PLUGIN,
        F_NOT_DELETING_CHANNEL, F_NO_RECALC_GRAPH);
    }

  /* move plugin's automation from src to
   * dest */
  plugin_move_automation (
    pl, prev_track, track, slot_type, slot);

  /* remove plugin from its channel */
  channel_remove_plugin (
    prev_ch, prev_slot_type, prev_slot,
    F_MOVING_PLUGIN, F_NOT_DELETING_PLUGIN,
    F_NOT_DELETING_CHANNEL, F_NO_RECALC_GRAPH);

  /* add plugin to its new channel */
  channel_add_plugin (
    track->channel, slot_type, slot, pl,
    F_NO_CONFIRM,
    F_MOVING_PLUGIN, F_NO_GEN_AUTOMATABLES,
    F_RECALC_GRAPH, F_PUBLISH_EVENTS);

  if (fire_events)
    {
      EVENTS_PUSH (
        ET_CHANNEL_SLOTS_CHANGED, prev_ch);
      EVENTS_PUSH (
        ET_CHANNEL_SLOTS_CHANGED, track->channel);
    }
}

/**
 * Sets the channel and slot on the plugin and
 * its ports.
 */
void
plugin_set_track_and_slot (
  Plugin *       pl,
  int            track_pos,
  PluginSlotType slot_type,
  int            slot)
{
  pl->id.track_pos = track_pos;
  pl->id.slot = slot;
  pl->id.slot_type = slot_type;

  int i;
  Port * port;
  for (i = 0; i < pl->num_in_ports; i++)
    {
      port = pl->in_ports[i];
      port_set_owner_plugin (port, pl);
      if (pl->is_project)
        {
          Track * track = plugin_get_track (pl);
          port_update_identifier (
            port, track,
            F_NO_UPDATE_AUTOMATION_TRACK);
        }
    }
  for (i = 0; i < pl->num_out_ports; i++)
    {
      port = pl->out_ports[i];
      port_set_owner_plugin (port, pl);
      if (pl->is_project)
        {
          Track * track = plugin_get_track (pl);
          port_update_identifier (
            port, track,
            F_NO_UPDATE_AUTOMATION_TRACK);
        }
    }
}

Track *
plugin_get_track (
  Plugin * self)
{
  g_return_val_if_fail (
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
  Track * track = plugin_get_track (&plugin);
  g_return_val_if_fail (
    IS_TRACK_AND_NONNULL (track), NULL);

  Channel * ch = NULL;
  if (track->type != TRACK_TYPE_MODULATOR ||
      id->slot_type == PLUGIN_SLOT_MIDI_FX ||
      id->slot_type == PLUGIN_SLOT_INSTRUMENT ||
      id->slot_type == PLUGIN_SLOT_INSERT)
    {
      ch = plugin_get_channel (&plugin);
      g_return_val_if_fail (ch, NULL);
    }
  Plugin * ret = NULL;
  switch (id->slot_type)
    {
    case PLUGIN_SLOT_MIDI_FX:
      g_return_val_if_fail (
        IS_CHANNEL_AND_NONNULL (ch), NULL);
      ret = ch->midi_fx[id->slot];
      break;
    case PLUGIN_SLOT_INSTRUMENT:
      g_return_val_if_fail (
        IS_CHANNEL_AND_NONNULL (ch), NULL);
      ret = ch->instrument;
      break;
    case PLUGIN_SLOT_INSERT:
      g_return_val_if_fail (
        IS_CHANNEL_AND_NONNULL (ch), NULL);
      ret = ch->inserts[id->slot];
      break;
    case PLUGIN_SLOT_MODULATOR:
      g_return_val_if_fail (
        IS_TRACK_AND_NONNULL (track), NULL);
      ret = track->modulators[id->slot];
      break;
    default:
      g_return_val_if_reached (NULL);
      break;
    }
  g_return_val_if_fail (ret, NULL);

  return ret;
}

void
plugin_get_full_port_group_designation (
  Plugin *     self,
  const char * port_group,
  char *       buf)
{
  Track * track = plugin_get_track (self);
  g_return_if_fail (track);
  sprintf (
    buf, "%s/%s/%s",
    track->name, self->setting->descr->name, port_group);
}

Port *
plugin_get_port_in_group (
  Plugin *     self,
  const char * port_group,
  bool         left)
{
  for (int i = 0; i < self->num_in_ports; i++)
    {
      Port * port = self->in_ports[i];
      if (string_is_equal (
            port->id.port_group, port_group) &&
          port->id.flags &
            (left ?
               PORT_FLAG_STEREO_L :
               PORT_FLAG_STEREO_R))
        {
          return port;
        }
    }
  for (int i = 0; i < self->num_out_ports; i++)
    {
      Port * port = self->out_ports[i];
      if (string_is_equal (
            port->id.port_group, port_group) &&
          port->id.flags &
            (left ?
               PORT_FLAG_STEREO_L :
               PORT_FLAG_STEREO_R))
        {
          return port;
        }
    }

  return NULL;
}

/**
 * Find corresponding port in the same port group
 * (eg, if this is left, find right and vice
 * versa).
 */
Port *
plugin_get_port_in_same_group (
  Plugin * self,
  Port *   port)
{
  if (!port->id.port_group)
    {
      g_message (
        "port %s has no port group",
        port->id.label);
      return NULL;
    }

  int num_ports = 0;
  Port ** ports = NULL;
  if (port->id.flow == FLOW_INPUT)
    {
      num_ports = self->num_in_ports;
      ports = self->in_ports;
    }
  else
    {
      num_ports = self->num_out_ports;
      ports = self->out_ports;
    }

  for (int i = 0; i < num_ports; i++)
    {
      Port * cur_port = ports[i];

      if (port == cur_port)
        {
          continue;
        }

      if (string_is_equal (
            port->id.port_group,
            cur_port->id.port_group) &&
          ((cur_port->id.flags &
              PORT_FLAG_STEREO_L &&
            port->id.flags & PORT_FLAG_STEREO_R) ||
           (cur_port->id.flags &
              PORT_FLAG_STEREO_R &&
            port->id.flags & PORT_FLAG_STEREO_L)))
        {
          return cur_port;
        }
    }

  return NULL;
}

char *
plugin_generate_window_title (
  Plugin * plugin)
{
  g_return_val_if_fail (
    plugin->setting->descr, NULL);

  Track * track = plugin_get_track (plugin);
  g_return_val_if_fail (
    IS_TRACK_AND_NONNULL (track), NULL);

  const PluginSetting * setting = plugin->setting;

  const char* track_name = track->name;
  const char* plugin_name = setting->descr->name;
  g_return_val_if_fail (
    track_name && plugin_name, NULL);

  char bridge_mode[100];
  strcpy (bridge_mode, "");
  if (setting->bridge_mode != CARLA_BRIDGE_NONE)
    {
      sprintf (
        bridge_mode, _(" - bridge: %s"),
        carla_bridge_mode_strings[
          setting->bridge_mode].str);
    }

  char slot[100];
  sprintf (
    slot, "#%d", plugin->id.slot + 1);
  if (plugin->id.slot_type ==
        PLUGIN_SLOT_INSTRUMENT)
    {
      strcpy (slot, _("instrument"));
    }

  char title[500];
  sprintf (
    title,
    "%s (%s %s%s%s)",
    plugin_name, track_name, slot,
    setting->open_with_carla ? " carla" : "",
    bridge_mode);

  switch (plugin->setting->descr->protocol)
    {
    case PROT_LV2:
      if (!plugin->setting->open_with_carla &&
          plugin->lv2->preset)
        {
          Lv2Plugin * lv2 = plugin->lv2;
          const char* preset_label =
            lilv_state_get_label (lv2->preset);
          g_return_val_if_fail (preset_label, NULL);
          char preset_part[500];
          sprintf (
            preset_part, " - %s",
            preset_label);
          strcat (
            title, preset_part);
        }
      break;
    default:
      break;
    }

  return g_strdup (title);
}

/**
 * Activates or deactivates the plugin.
 *
 * @param activate True to activate, false to
 *   deactivate.
 */
int
plugin_activate (
  Plugin * pl,
  bool     activate)
{
  g_return_val_if_fail (
    IS_PLUGIN (pl), ERR_OBJECT_IS_NULL);

  if ((pl->activated && activate) ||
      (!pl->activated && !activate))
    {
      /* nothing to do */
      return 0;
    }

  if (activate && !pl->instantiated)
    {
      g_critical (
        "plugin %s not instantiated",
        pl->setting->descr->name);
      return -1;
    }

  if (pl->setting->open_with_carla)
    {
#ifdef HAVE_CARLA
      int ret =
        carla_native_plugin_activate (
          pl->carla, activate);
      g_return_val_if_fail (ret == 0, ret);
#endif
    }
  else
    {
      switch (pl->setting->descr->protocol)
        {
        case PROT_LV2:
          {
            int ret =
              lv2_plugin_activate (
                pl->lv2, activate);
            g_return_val_if_fail (ret == 0, ret);
          }
          break;
        default:
          g_warn_if_reached ();
          break;
        }
    }

  pl->activated = activate;

  return 0;
}

/**
 * Cleans up an instantiated but not activated
 * plugin.
 */
int
plugin_cleanup (
  Plugin * self)
{
  g_message (
    "Cleaning up %s...", self->setting->descr->name);

  if (!self->activated && self->instantiated)
    {
      if (self->setting->open_with_carla)
        {
#ifdef HAVE_CARLA
#if 0
          /* TODO */
          int ret =
            carla_native_plugin_cleanup (
              self->carla, activate);
          g_return_val_if_fail (ret == 0, ret);
#endif
#endif
        }
      else
        {
          switch (self->setting->descr->protocol)
            {
            case PROT_LV2:
              {
                int ret =
                  lv2_plugin_cleanup (self->lv2);
                g_return_val_if_fail (
                  ret == 0, ret);
              }
              break;
            default:
              g_warn_if_reached ();
              break;
            }
        }
    }

  self->instantiated = false;
  g_message ("done");

  return 0;
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
    !pl->setting->open_with_carla &&
#endif
      pl->setting->descr->protocol == PROT_LV2)
    {
      pl->latency =
        lv2_plugin_get_latency (pl->lv2);
      g_message ("%s latency: %d samples",
                 pl->setting->descr->name,
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
        g_realloc ( \
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
  /*g_message (*/
    /*"added input port %s to plugin %s at index %d",*/
    /*port->id.label, pl->descr->name,*/
    /*port->id.port_index);*/
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
  /*g_message (*/
    /*"added output port %s to plugin %s at index %d",*/
    /*port->id.label, pl->descr->name,*/
    /*port->id.port_index);*/
}
#undef ADD_PORT

/**
 * Moves the Plugin's automation from one Channel
 * to another.
 */
void
plugin_move_automation (
  Plugin *       pl,
  Track *        prev_track,
  Track *        track,
  PluginSlotType new_slot_type,
  int            new_slot)
{
  g_message (
    "moving plugin '%s' automation from "
    "%s to %s -> slot#%d",
    pl->setting->descr->name, prev_track->name,
    track->name, new_slot);

  AutomationTracklist * prev_atl =
    track_get_automation_tracklist (prev_track);
  AutomationTracklist * atl =
    track_get_automation_tracklist (track);

  for (int i = prev_atl->num_ats - 1; i >= 0; i--)
    {
      AutomationTrack * at = prev_atl->ats[i];
      Port * port = automation_track_get_port (at);
      g_return_if_fail (IS_PORT (port));
      if (!port)
        continue;
      if (port->id.owner_type ==
            PORT_OWNER_TYPE_PLUGIN)
        {
          Plugin * port_pl =
            port_get_plugin (port, 1);
          if (port_pl != pl)
            continue;
        }
      else
        continue;

      g_return_if_fail (port->at == at);

      /* delete from prev channel */
      int num_regions_before = at->num_regions;
      automation_tracklist_remove_at (
        prev_atl, at, F_NO_FREE,
        F_NO_PUBLISH_EVENTS);

      /* add to new channel */
      automation_tracklist_add_at (atl, at);
      g_warn_if_fail (
        at == atl->ats[at->index] &&
        at->num_regions == num_regions_before);

      /* update the automation track port
       * identifier */
      at->port_id.plugin_id.slot = new_slot;
      at->port_id.plugin_id.slot_type =
        new_slot_type;
      at->port_id.plugin_id.track_pos = track->pos;

      g_warn_if_fail (
        at->port_id.port_index ==
        port->id.port_index);
    }
}

/**
 * Sets the UI refresh rate on the Plugin.
 */
void
plugin_set_ui_refresh_rate (
  Plugin * self)
{
  g_message ("setting refresh rate...");

  if (ZRYTHM_TESTING || ZRYTHM_GENERATING_PROJECT)
    {
      self->ui_update_hz = 30.f;
      self->ui_scale_factor = 1.f;
      return;
    }

  /* if no preferred refresh rate is set,
   * use the monitor's refresh rate */
  if (g_settings_get_int (
        S_P_PLUGINS_UIS, "refresh-rate"))
    {
      /* Use user-specified UI update rate. */
      self->ui_update_hz =
        (float)
        g_settings_get_int (
          S_P_PLUGINS_UIS, "refresh-rate");
    }
  else
    {
      self->ui_update_hz =
        (float)
        z_gtk_get_primary_monitor_refresh_rate ();
      g_message (
        "refresh rate returned by GDK: %.01f",
        (double) self->ui_update_hz);
    }

  /* if no preferred scale factor is set,
   * use the monitor's scale factor */
  float scale_factor_setting =
    (float)
    g_settings_get_double (
      S_P_PLUGINS_UIS, "scale-factor");
  if (scale_factor_setting >= 0.5f)
    {
      /* use user-specified scale factor */
      self->ui_scale_factor = scale_factor_setting;
    }
  else
    {
      /* set the scale factor */
      self->ui_scale_factor =
         (float)
         z_gtk_get_primary_monitor_scale_factor ();
      g_message (
        "scale factor returned by GDK: %.01f",
        (double) self->ui_scale_factor);
    }

  /* clamp the refresh rate to sensible limits */
  if (self->ui_update_hz < PLUGIN_MIN_REFRESH_RATE ||
      self->ui_update_hz > PLUGIN_MAX_REFRESH_RATE)
    {
      g_warning (
        "Invalid refresh rate of %.01f received, "
        "clamping to reasonable bounds",
        (double) self->ui_update_hz);
      self->ui_update_hz =
        CLAMP (
          self->ui_update_hz,
          PLUGIN_MIN_REFRESH_RATE,
          PLUGIN_MAX_REFRESH_RATE);
    }

  /* clamp the scale factor to sensible limits */
  if (self->ui_scale_factor <
        PLUGIN_MIN_SCALE_FACTOR ||
      self->ui_scale_factor >
        PLUGIN_MAX_SCALE_FACTOR)
    {
      g_warning (
        "Invalid scale factor of %.01f received, "
        "clamping to reasonable bounds",
        (double) self->ui_scale_factor);
      self->ui_scale_factor =
        CLAMP (
          self->ui_scale_factor,
          PLUGIN_MIN_SCALE_FACTOR,
          PLUGIN_MAX_SCALE_FACTOR);
    }

  g_message ("refresh rate set to %f",
    (double) self->ui_update_hz);
  g_message ("scale factor set to %f",
    (double) self->ui_scale_factor);
}

/**
 * Returns the escaped name of the plugin.
 */
char *
plugin_get_escaped_name (
  Plugin * pl)
{
  g_return_val_if_fail (pl->setting->descr, NULL);

  char tmp[900];
  io_escape_dir_name (tmp, pl->setting->descr->name);
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
    self->setting->descr->name);

  AutomationTracklist * atl =
    track_get_automation_tracklist (track);
  for (int i = 0; i < self->num_in_ports; i++)
  {
    Port * port = self->in_ports[i];
    if (port->id.type != TYPE_CONTROL ||
        !(port->id.flags & PORT_FLAG_AUTOMATABLE))
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
      if (port->id.flags &
            PORT_FLAG_PLUGIN_ENABLED &&
          port->id.flags &
            PORT_FLAG_GENERIC_PLUGIN_PORT)
        {
          return port;
        }
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
      Port * port = self->in_ports[i];
      port_update_track_pos (
        port, NULL, self->id.track_pos);
      port->id.plugin_id = self->id;
    }
  for (i = 0; i < self->num_out_ports; i++)
    {
      Port * port = self->out_ports[i];
      port_update_track_pos (
        port, NULL, self->id.track_pos);
      port->id.plugin_id = self->id;
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
 * channel)
 *
 * @param project Whether this is a project plugin
 *   (as opposed to a clone used in actions).
 */
int
plugin_instantiate (
  Plugin *    pl,
  bool        project,
  LilvState * state)
{
  g_return_val_if_fail (
    pl->setting && pl->setting->descr, -1);

  g_message ("Instantiating %s...",
             pl->setting->descr->name);

  plugin_set_ui_refresh_rate (pl);

  if (!PROJECT->loaded)
    {
      g_return_val_if_fail (pl->state_dir, -1);
    }
  g_message ("state dir: %s", pl->state_dir);

  if (pl->setting->open_with_carla)
    {
#ifdef HAVE_CARLA
      int ret =
        carla_native_plugin_instantiate (
          pl->carla, !PROJECT->loaded,
          pl->state_dir ? true : false);
      if (ret != 0)
        {
          g_warning (
            "carla plugin instantiation failed");
          return -1;
        }

      /* save the state */
      carla_native_plugin_save_state (
        pl->carla, false);
#else
      g_return_val_if_reached (-1);
#endif
    }
  else
    {
      switch (pl->setting->descr->protocol)
        {
        case PROT_LV2:
          {
            pl->lv2->plugin = pl;
            if (lv2_plugin_instantiate (
                  pl->lv2, project,
                  pl->state_dir ? true : false,
                  NULL, state))
              {
                g_warning (
                  "lv2 instantiate failed");
                return -1;
              }
            else
              {
                pl->instantiated = true;
              }
            g_warn_if_fail (pl->lv2->instance);
            /* save the state */
            lv2_state_save_to_file (
              pl->lv2, F_NOT_BACKUP);
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

  pl->instantiated = true;

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
  if (!plugin_is_enabled (plugin) &&
      !plugin->own_enabled_port)
    {
      plugin_process_passthrough (
        plugin, g_start_frames, local_offset,
        nframes);
      return;
    }

  if (!plugin->instantiated || !plugin->activated)
    {
      return;
    }

  /* if has MIDI input port */
  if (plugin->setting->descr->num_midi_ins > 0)
    {
      /* if recording, write MIDI events to the
       * region TODO */

        /* if there is a midi note in this buffer
         * range TODO */
        /* add midi events to input port */
    }

#ifdef HAVE_CARLA
  if (plugin->setting->open_with_carla)
    {
      carla_native_plugin_process (
        plugin->carla, g_start_frames,
        local_offset, nframes);
    }
  else
    {
#endif
      switch (plugin->setting->descr->protocol)
        {
        case PROT_LV2:
          lv2_plugin_process (
            plugin->lv2, g_start_frames,
            local_offset, nframes);
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

  /* if plugin has gain, apply it */
  if (!math_floats_equal_epsilon (
        plugin->gain->control, 1.f, 0.001f))
    {
      for (int i = 0; i < plugin->num_out_ports;
           i++)
        {
          Port * port = plugin->out_ports[i];
          if (port->id.type != TYPE_AUDIO)
            continue;

          /* if close to 0 set it to the denormal
           * prevention val */
          if (math_floats_equal_epsilon (
                plugin->gain->control, 0.f,
                0.00001f))
            {
              dsp_fill (
                &port->buf[local_offset],
                DENORMAL_PREVENTION_VAL,
                nframes);
            }
          /* otherwise just apply gain */
          else
            {
              dsp_mul_k2 (
                &port->buf[local_offset],
                plugin->gain->control, nframes);
            }
        }
    }
}

/**
 * Shows plugin ui and sets window close callback
 */
void
plugin_open_ui (
  Plugin * self)
{
  g_debug ("opening plugin UI");

  g_return_if_fail (
    IS_PLUGIN (self) && self->setting->descr);

  PluginSetting * setting = self->setting;
  const PluginDescriptor * descr = setting->descr;

  if (self->instantiation_failed)
    {
      g_message (
        "plugin %s instantiation failed, no UI to "
        "open",
        descr->name);
      return;
    }

  /* show error if LV2 UI type is deprecated */
  if (self->is_project &&
      descr->protocol == PROT_LV2 &&
      (!setting->open_with_carla ||
       setting->bridge_mode != CARLA_BRIDGE_FULL))
    {
      char * deprecated_uri =
        lv2_plugin_has_deprecated_ui (descr->uri);
      if (deprecated_uri)
        {
          char msg[1200];
          sprintf (
            msg,
            _("%s <%s> has a deprecated UI "
            "type:\n  %s\n"
            "If the UI does not load, please try "
            "instantiating the plugin in full-"
            "bridged mode, and report this to the "
            "author:\n  %s <%s>"),
            descr->name,
            descr->uri,
            deprecated_uri, descr->author,
            descr->website);
          ui_show_error_message (
            MAIN_WINDOW, msg);
          g_free (deprecated_uri);
        }
    }

  /* if plugin already has a window (either generic
   * or LV2 non-carla and non-external UI) */
  if (GTK_IS_WINDOW (self->window))
    {
      /* present it */
      gtk_window_present (
        GTK_WINDOW (self->window));
    }
  else
    {
      bool generic_ui = setting->force_generic_ui;

      /* handle generic UIs, then carla custom,
       * then LV2 custom */
      if (generic_ui)
        {
          plugin_gtk_create_window (self);
          plugin_gtk_open_generic_ui (self);
        }
      else if (setting->open_with_carla)
        {
#ifdef HAVE_CARLA
          carla_native_plugin_open_ui (
            self->carla, 1);
#endif
        }
      else if (descr->protocol == PROT_LV2)
        {
          plugin_gtk_create_window (self);
          lv2_gtk_open_ui (self->lv2);
        }
    }
}

/**
 * Returns if Plugin exists in MixerSelections.
 */
bool
plugin_is_selected (
  Plugin * pl)
{
  g_return_val_if_fail (IS_PLUGIN (pl), false);

  return mixer_selections_contains_plugin (
    MIXER_SELECTIONS, pl);
}

/**
 * Selects the plugin in the MixerSelections.
 *
 * @param select Select or deselect.
 * @param exclusive Whether to make this the only
 *   selected plugin or add it to the selections.
 */
void
plugin_select (
  Plugin * pl,
  bool     select,
  bool     exclusive)
{
  g_return_if_fail (
    IS_PLUGIN (pl) && pl->is_project);

  if (exclusive)
    {
      mixer_selections_clear (
        MIXER_SELECTIONS, F_PUBLISH_EVENTS);
    }

  Track * track = plugin_get_track (pl);
  g_return_if_fail (IS_TRACK_AND_NONNULL (track));

  if (select)
    {
      mixer_selections_add_slot (
        MIXER_SELECTIONS, track, pl->id.slot_type,
        pl->id.slot, F_NO_CLONE);
    }
  else
    {
      mixer_selections_remove_slot (
        MIXER_SELECTIONS, pl->id.slot,
        pl->id.slot_type, F_PUBLISH_EVENTS);
    }
}

/**
 * Returns the state dir as an absolute path.
 */
char *
plugin_get_abs_state_dir (
  Plugin * self,
  bool     is_backup)
{
  plugin_ensure_state_dir (self, is_backup);

  char * parent_dir =
    project_get_path (
      PROJECT, PROJECT_PATH_PLUGIN_STATES,
      is_backup);
  char * full_path =
    g_build_filename (
      parent_dir, self->state_dir, NULL);

  g_free (parent_dir);

  return full_path;
}

/**
 * Ensures the state dir exists or creates it.
 */
void
plugin_ensure_state_dir (
  Plugin * self,
  bool     is_backup)
{
  if (self->state_dir)
    {
      char * parent_dir =
        project_get_path (
          PROJECT, PROJECT_PATH_PLUGIN_STATES,
          is_backup);
      char * abs_state_dir =
        g_build_filename (
          parent_dir, self->state_dir, NULL);
      io_mkdir (abs_state_dir);
      g_free (parent_dir);
      g_free (abs_state_dir);
      return;
    }

  char * escaped_name =
    plugin_get_escaped_name (self);
  char * parent_dir =
    project_get_path (
      PROJECT, PROJECT_PATH_PLUGIN_STATES,
      is_backup);
  char * tmp =
    g_strdup_printf (
      "%s_XXXXXX", escaped_name);
  char * abs_state_dir_template =
    g_build_filename (parent_dir, tmp, NULL);
  io_mkdir (parent_dir);
  char * abs_state_dir =
    g_mkdtemp (abs_state_dir_template);
  if (!abs_state_dir)
    {
      g_critical (
        "Failed to make state dir: %s",
        strerror (errno));
    }
  self->state_dir =
    g_path_get_basename (abs_state_dir);
  g_free (escaped_name);
  g_free (parent_dir);
  g_free (tmp);
  g_free (abs_state_dir);
}

/**
 * Clones the given plugin.
 *
 * @bool src_is_project Whether the given plugin
 *   is a project plugin.
 */
Plugin *
plugin_clone (
  Plugin * pl,
  bool     src_is_project)
{
  g_return_val_if_fail (IS_PLUGIN (pl), NULL);

  Plugin * clone = NULL;
#ifdef HAVE_CARLA
  if (pl->setting->open_with_carla)
    {
      /* create a new plugin with same descriptor */
      clone =
        plugin_new_from_setting (
          pl->setting, pl->id.track_pos,
          pl->id.slot_type, pl->id.slot);
      g_return_val_if_fail (
        clone && clone->carla, NULL);

      /* instantiate */
      int ret =
        plugin_instantiate (clone, false, NULL);
      g_return_val_if_fail (ret == 0, NULL);

      /* also instantiate the source, if not
       * already instantiated, so its state can
       * be ready */
      if (!pl->instantiated)
        {
          ret =
            plugin_instantiate (
              pl, src_is_project, NULL);
          g_return_val_if_fail (
            ret == 0 &&
            pl->num_in_ports ==
              clone->num_in_ports, NULL);
        }

      /* save the state of the original plugin */
      carla_native_plugin_save_state (
        pl->carla, F_NOT_BACKUP);

      /* load the state to the new plugin. */
      char * state_file_abs_path =
        carla_native_plugin_get_abs_state_file_path  (
          pl->carla, F_NOT_BACKUP);
      carla_native_plugin_load_state (
        clone->carla, state_file_abs_path);

      /* create a new state dir and save the state
       * for the clone */
      plugin_ensure_state_dir (
        clone, F_NOT_BACKUP);
      carla_native_plugin_save_state (
        clone->carla, F_NOT_BACKUP);

      /* cleanup the source if it wasnt in the
       * project */
      if (!src_is_project)
        {
          plugin_cleanup (pl);
        }
    }
  else
    {
#endif
      if (pl->setting->descr->protocol == PROT_LV2)
        {
          /* if src plugin not instantiated,
           * instantiate it */
          if (!pl->instantiated)
            {
              g_message (
                "source plugin not instantiated, "
                "instantiating it...");
              int ret =
                plugin_instantiate (
                  pl, src_is_project, NULL);
              g_return_val_if_fail (
                ret == 0, NULL);
            }
          g_return_val_if_fail (
            pl->instantiated &&
            pl->lv2->instance, NULL);

          /* Make a state */
          LilvState * state =
            lv2_state_save_to_file (
              pl->lv2, F_NOT_BACKUP);

          /* create a new plugin with same
           * descriptor */
          clone =
            plugin_new_from_setting (
              pl->setting, pl->id.track_pos,
              pl->id.slot_type, pl->id.slot);
          g_return_val_if_fail (
            IS_PLUGIN_AND_NONNULL (clone), NULL);

          /* instantiate using the state */
          int ret =
            plugin_instantiate (
              clone, false, state);
          g_return_val_if_fail (!ret, NULL);

          /* verify */
          g_return_val_if_fail (
            clone && clone->lv2 &&
            clone->num_lilv_ports ==
              pl->num_lilv_ports,
            NULL);

          /* free the state */
          lilv_state_free (state);

          /* create a new state dir and save the
           * state  for the clone */
          plugin_ensure_state_dir (
            clone, F_NOT_BACKUP);
          lv2_state_save_to_file (
            clone->lv2, F_NOT_BACKUP);

          /* cleanup the source if it wasnt in the
           * project */
          if (!src_is_project)
            {
              plugin_cleanup (pl);
            }
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

  /* set generic port values since they are not
   * saved in the state */
  port_set_control_value (
    clone->enabled, pl->enabled->control,
    F_NOT_NORMALIZED, F_NO_PUBLISH_EVENTS);
  port_set_control_value (
    clone->gain, pl->gain->control,
    F_NOT_NORMALIZED, F_NO_PUBLISH_EVENTS);

  /* verify same number of inputs and outputs */
  g_return_val_if_fail (
    pl->num_in_ports == clone->num_in_ports, NULL);
  g_return_val_if_fail (
    pl->num_out_ports == clone->num_out_ports,
    NULL);

  /* copy plugin id to each port */

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
                  dsp_copy (
                    &out_port->buf[local_offset],
                    &in_port->buf[local_offset],
                    nframes);

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
              if (out_port->id.type ==
                    TYPE_EVENT &&
                  out_port->id.flags &
                    PORT_FLAG2_SUPPORTS_MIDI)
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
plugin_close_ui (
  Plugin * self)
{
  g_return_if_fail (ZRYTHM_HAVE_UI);

  if (self->instantiation_failed)
    {
      g_message (
        "plugin %s instantiation failed, "
        "no UI to close",
        self->setting->descr->name);
      return;
    }

  plugin_gtk_close_ui (self);

#ifdef HAVE_CARLA
  bool generic_ui = self->setting->force_generic_ui;
  if (!generic_ui && self->setting->open_with_carla)
    {
      g_message ("closing carla plugin UI");
      carla_native_plugin_open_ui (
        self->carla, false);
    }
#endif

  self->visible = false;
}

#if 0
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
#endif

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

  if (src->setting->descr->num_audio_outs == 1 &&
      dest->setting->descr->num_audio_ins == 1)
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
  else if (src->setting->descr->num_audio_outs == 1 &&
           dest->setting->descr->num_audio_ins > 1)
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
  else if (src->setting->descr->num_audio_outs > 1 &&
           dest->setting->descr->num_audio_ins == 1)
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
  else if (src->setting->descr->num_audio_outs > 1 &&
           dest->setting->descr->num_audio_ins > 1)
    {
      /* connect to as many audio outs this
       * plugin has, or until we can't connect
       * anymore */
      num_ports_to_connect =
        MIN (src->setting->descr->num_audio_outs,
             dest->setting->descr->num_audio_ins);
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

      if (out_port->id.type == TYPE_EVENT &&
          out_port->id.flags2 &
            PORT_FLAG2_SUPPORTS_MIDI)
        {
          for (j = 0;
               j < dest->num_in_ports; j++)
            {
              in_port = dest->in_ports[j];

              if (in_port->id.type == TYPE_EVENT &&
                  in_port->id.flags2 &
                    PORT_FLAG2_SUPPORTS_MIDI)
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
              out_port->id.flags2 &
                PORT_FLAG2_SUPPORTS_MIDI &&
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
      if (pl->setting->descr->num_audio_outs == 1)
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
                    ch->prefader->stereo_in->l, 1);
                  port_connect (
                    out_port,
                    ch->prefader->stereo_in->r, 1);
                  break;
                }
            }
        }
      else if (pl->setting->descr->num_audio_outs > 1)
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
                    ch->prefader->stereo_in->l, 1);
                  last_index++;
                }
              else if (last_index == 1)
                {
                  port_connect (
                    out_port,
                    ch->prefader->stereo_in->r, 1);
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
                ch->prefader->stereo_in->l))
            port_disconnect (
              out_port,
              ch->prefader->stereo_in->l);
          if (ports_connected (
                out_port,
                ch->prefader->stereo_in->r))
            port_disconnect (
              out_port,
              ch->prefader->stereo_in->r);
        }
      else if (type == TYPE_EVENT &&
               out_port->id.type ==
                 TYPE_EVENT &&
               out_port->id.flags2 &
                 PORT_FLAG2_SUPPORTS_MIDI)
        {
          if (ports_connected (
                out_port,
                ch->prefader->midi_in))
            port_disconnect (
              out_port,
              ch->prefader->midi_in);
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

  if (src->setting->descr->num_audio_outs == 1 &&
      dest->setting->descr->num_audio_ins == 1)
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
  else if (src->setting->descr->num_audio_outs == 1 &&
           dest->setting->descr->num_audio_ins > 1)
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
  else if (src->setting->descr->num_audio_outs > 1 &&
           dest->setting->descr->num_audio_ins == 1)
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
  else if (src->setting->descr->num_audio_outs > 1 &&
           dest->setting->descr->num_audio_ins > 1)
    {
      /* connect to as many audio outs this
       * plugin has, or until we can't connect
       * anymore */
      num_ports_to_connect =
        MIN (src->setting->descr->num_audio_outs,
             dest->setting->descr->num_audio_ins);
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

      if (out_port->id.type == TYPE_EVENT &&
          out_port->id.flags2 &
            PORT_FLAG2_SUPPORTS_MIDI)
        {
          for (j = 0;
               j < dest->num_in_ports; j++)
            {
              in_port = dest->in_ports[j];

              if (in_port->id.type ==
                    TYPE_EVENT &&
                  in_port->id.flags2 &
                    PORT_FLAG2_SUPPORTS_MIDI)
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
  g_message (
    "disconnecting plugin %s...",
    self->setting->descr->name);

  self->deleting = 1;

  if (self->visible)
    {
      plugin_close_ui (self);
    }

  if (self->is_project)
    {
      /* disconnect all ports */
      ports_disconnect (
        self->in_ports,
        self->num_in_ports, true);
      ports_disconnect (
        self->out_ports,
        self->num_out_ports, true);
      g_message (
        "%s: DISCONNECTED ALL PORTS OF %s %d %d",
        __func__, self->setting->descr->name,
        self->num_in_ports,
        self->num_out_ports);

#ifdef HAVE_CARLA
      if (self->setting->open_with_carla)
        {
          carla_native_plugin_close (self->carla);
        }
#endif
    }
  else
    {
      g_debug (
        "%s is not a project plugin, skipping "
        "disconnect", self->setting->descr->name);
    }

  g_message (
    "finished disconnecting plugin %s",
    self->setting->descr->name);
}

/**
 * Deletes any state files associated with this
 * plugin.
 *
 * This should be called when a plugin instance is
 * removed from the project (including undo stacks)
 * to remove any files not needed anymore.
 */
void
plugin_delete_state_files (
  Plugin * self)
{
  g_message (
    "deleting state files for plugin %s (%s)",
    self->setting->descr->name, self->state_dir);

  g_return_if_fail (
    g_path_is_absolute (self->state_dir));

  io_rmdir (self->state_dir, true);
}

/**
 * Exposes or unexposes plugin ports to the backend.
 *
 * @param expose Expose or not.
 * @param inputs Expose/unexpose inputs.
 * @param outputs Expose/unexpose outputs.
 */
void
plugin_expose_ports (
  Plugin * pl,
  bool     expose,
  bool     inputs,
  bool     outputs)
{
  if (inputs)
    {
      for (int i = 0; i < pl->num_in_ports; i++)
        {
          Port * port = pl->in_ports[i];

          bool is_exposed =
            port_is_exposed_to_backend (port);
          if (expose && !is_exposed)
            {
              port_set_expose_to_backend (
                port, true);
            }
          else if (!expose && is_exposed)
            {
              port_set_expose_to_backend (
                port, false);
            }
        }
    }
  if (outputs)
    {
      for (int i = 0; i < pl->num_out_ports; i++)
        {
          Port * port = pl->out_ports[i];

          bool is_exposed =
            port_is_exposed_to_backend (port);
          if (expose && !is_exposed)
            {
              port_set_expose_to_backend (
                port, true);
            }
          else if (!expose && is_exposed)
            {
              port_set_expose_to_backend (
                port, false);
            }
        }
    }
}

/**
 * Gets a port by its symbol.
 *
 * Only works for LV2 plugins.
 */
Port *
plugin_get_port_by_symbol (
  Plugin *     pl,
  const char * sym)
{
  g_return_val_if_fail (
    IS_PLUGIN (pl) &&
      pl->setting->descr->protocol == PROT_LV2,
    NULL);

  for (int i = 0; i < pl->num_in_ports; i++)
    {
      Port * port = pl->in_ports[i];
      g_return_val_if_fail (
        IS_PORT_AND_NONNULL (port), NULL);

      if (string_is_equal (port->id.sym, sym))
        {
          return port;
        }
    }
  for (int i = 0; i < pl->num_out_ports; i++)
    {
      Port * port = pl->out_ports[i];
      g_return_val_if_fail (
        IS_PORT_AND_NONNULL (port), NULL);

      if (string_is_equal (port->id.sym, sym))
        {
          return port;
        }
    }

  g_critical (
    "failed to find port with symbol %s", sym);
  return NULL;
}

Port *
plugin_get_port_by_param_uri (
  Plugin *     pl,
  const char * uri)
{
  g_return_val_if_fail (
    IS_PLUGIN (pl) &&
      pl->setting->descr->protocol == PROT_LV2 &&
      !pl->setting->open_with_carla,
    NULL);

  g_return_val_if_fail (pl->lv2, NULL);

  for (int i = 0; i < pl->num_in_ports; i++)
    {
      Port * port = pl->in_ports[i];

      if (string_is_equal (port->id.uri, uri))
        {
          return port;
        }
    }

  g_critical (
    "failed to find port with parameter URI <%s>",
    uri);
  return NULL;
}

/**
 * Frees given plugin, frees its ports
 * and other internal pointers
 */
void
plugin_free (
  Plugin * self)
{
  g_return_if_fail (IS_PLUGIN (self));

  g_return_if_fail (!self->visible);

  g_debug (
    "freeing plugin %s",
    self->setting->descr->name);

  object_free_w_func_and_null (
    lv2_plugin_free, self->lv2);
#ifdef HAVE_CARLA
  object_free_w_func_and_null (
    carla_native_plugin_free, self->carla);
#endif

  for (int i = 0; i < self->num_in_ports; i++)
    {
      Port * port = self->in_ports[i];
      object_free_w_func_and_null (
        port_free, port);
    }
  for (int i = 0; i < self->num_out_ports; i++)
    {
      Port * port = self->out_ports[i];
      object_free_w_func_and_null (
        port_free, port);
    }

  object_zero_and_free (self->lilv_ports);

  object_zero_and_free (self);
}
