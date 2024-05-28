// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Implementation of Plugin.
 */

#include "dsp/port_identifier.h"
#define _GNU_SOURCE 1 /* To pick up REG_RIP */

#include "zrythm-config.h"

#include <cstdlib>
#include <cstring>
#include <signal.h>

#include "actions/undo_manager.h"
#include "dsp/automation_tracklist.h"
#include "dsp/channel.h"
#include "dsp/control_port.h"
#include "dsp/engine.h"
#include "dsp/midi_event.h"
#include "dsp/track.h"
#include "dsp/transport.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/main_window.h"
#include "plugins/cached_plugin_descriptors.h"
#include "plugins/carla_native_plugin.h"
#include "plugins/plugin.h"
#include "plugins/plugin_gtk.h"
#include "plugins/plugin_manager.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/arrays.h"
#include "utils/dialogs.h"
#include "utils/dsp.h"
#include "utils/error.h"
#include "utils/file.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/io.h"
#include "utils/math.h"
#include "utils/mem.h"
#include "utils/objects.h"
#include "utils/string.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

#include "gtk_wrapper.h"

typedef enum
{
  Z_PLUGINS_PLUGIN_ERROR_CREATION_FAILED,
  Z_PLUGINS_PLUGIN_ERROR_INSTANTIATION_FAILED,
} ZPluginsPluginError;

#define Z_PLUGINS_PLUGIN_ERROR z_plugins_plugin_error_quark ()
GQuark
z_plugins_plugin_error_quark (void);
G_DEFINE_QUARK (
  z - plugins - plugin - error - quark,
  z_plugins_plugin_error)

/**
 * To be called when struct is deserialized or
 * when creating a new plugin.
 */
NONNULL static void
init_port_arrays (Plugin * self)
{
  /* create cache arrays */
  const size_t def_sz = 20;
  self->ctrl_in_ports = g_ptr_array_new_full (def_sz, NULL);
  self->audio_in_ports = g_ptr_array_new_full (def_sz, NULL);
  self->cv_in_ports = g_ptr_array_new_full (def_sz, NULL);
  self->midi_in_ports = g_ptr_array_new_full (def_sz, NULL);
}

NONNULL static void
set_stereo_outs_and_midi_in (Plugin * pl)
{
  g_return_if_fail (pl->setting && pl->setting->descr);

  /* set the L/R outputs */
  int num_audio_outs = 0;
  for (int i = 0; i < pl->num_out_ports; i++)
    {
      Port * out_port = pl->out_ports[i];
      if (out_port->id.type == ZPortType::Z_PORT_TYPE_AUDIO)
        {
          if (num_audio_outs == 0)
            {
              out_port->id.flags |= PortIdentifier::Flags::STEREO_L;
              pl->l_out = out_port;
            }
          else if (num_audio_outs == 1)
            {
              out_port->id.flags |= PortIdentifier::Flags::STEREO_R;
              pl->r_out = out_port;
            }
          num_audio_outs++;
        }
    }

  /* if mono set it as both stereo out L and
   * R */
  if (num_audio_outs == 1)
    {
      /* this code is only accessed by beta
       * projects before the change to force
       * stereo a few commits after beta 1.1.11 */
      g_warning ("should not happen with carla");
      pl->l_out->id.flags |= PortIdentifier::Flags::STEREO_R;
      pl->r_out = pl->l_out;
    }

  if (pl->setting->descr->num_audio_outs > 0)
    {
      g_return_if_fail (pl->l_out && pl->r_out);
    }

  /* set MIDI input */
  for (int i = 0; i < pl->num_in_ports; i++)
    {
      Port * port = pl->in_ports[i];
      if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags2, port->id.flags2,
          PortIdentifier::Flags2::SUPPORTS_MIDI))
        {
          pl->midi_in_port = port;
          break;
        }
    }
  if (plugin_descriptor_is_instrument (pl->setting->descr))
    {
      g_return_if_fail (pl->midi_in_port);
    }
}

static void
set_enabled_and_gain (Plugin * self)
{
  /* set enabled/gain ports */
  for (int i = 0; i < self->num_in_ports; i++)
    {
      Port * port = self->in_ports[i];
      if (
        !(port->id.type == ZPortType::Z_PORT_TYPE_CONTROL
          && ENUM_BITSET_TEST (
            PortIdentifier::Flags, port->id.flags,
            PortIdentifier::Flags::GENERIC_PLUGIN_PORT)))
        continue;

      if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags, port->id.flags,
          PortIdentifier::Flags::PLUGIN_ENABLED))
        {
          self->enabled = port;
        }
      if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags, port->id.flags,
          PortIdentifier::Flags::PLUGIN_GAIN))
        {
          self->gain = port;
        }
    }
  g_return_if_fail (self->enabled && self->gain);
}

void
plugin_init_loaded (Plugin * self, Track * track, MixerSelections * ms)
{
  self->magic = PLUGIN_MAGIC;
  self->track = track;
  self->ms = ms;

  init_port_arrays (self);

  GPtrArray * ports = g_ptr_array_new ();
  plugin_append_ports (self, ports);
  for (size_t i = 0; i < ports->len; i++)
    {
      Port * port = (Port *) g_ptr_array_index (ports, i);
      port->magic = PORT_MAGIC;
      port->plugin = self;
    }
  object_free_w_func_and_null (g_ptr_array_unref, ports);

  set_enabled_and_gain (self);

#ifdef HAVE_CARLA
  if (self->setting->open_with_carla)
    {
      self->carla = object_new (CarlaNativePlugin);
      self->carla->plugin = self;
      carla_native_plugin_init_loaded (self->carla);
    }
#endif

  if (plugin_is_in_active_project (self))
    {
      bool     was_enabled = plugin_is_enabled (self, false);
      GError * err = NULL;
      int      ret = plugin_instantiate (self, &err);
      if (ret == 0)
        {
          plugin_activate (self, true);

          plugin_set_enabled (self, was_enabled, F_NO_PUBLISH_EVENTS);
        }
      else
        {
          /* disable plugin, instantiation failed */
          HANDLE_ERROR (
            err, _ ("Instantiation failed for plugin '%s'. Disabling..."),
            self->setting->descr->name);
          self->instantiation_failed = true;
        }
    }

  /*Track * track = plugin_get_track (self);*/
  /*plugin_generate_automation_tracks (self, track);*/
}

static void
plugin_init (
  Plugin *        plugin,
  unsigned int    track_name_hash,
  ZPluginSlotType slot_type,
  int             slot)
{
  g_message (
    "%s: %s (%s) track name hash %u slot %d", __func__,
    plugin->setting->descr->name, ENUM_NAME (plugin->setting->descr->protocol),
    track_name_hash, slot);

  g_return_if_fail (
    plugin_identifier_validate_slot_type_slot_combo (slot_type, slot));

  plugin->in_ports_size = 1;
  plugin->out_ports_size = 1;
  plugin->id.track_name_hash = track_name_hash;
  plugin->id.slot_type = slot_type;
  plugin->id.slot = slot;
  plugin->magic = PLUGIN_MAGIC;

  plugin->in_ports = object_new_n (1, Port *);
  plugin->out_ports = object_new_n (1, Port *);

  /* add enabled port */
  Port * port = port_new_with_type (
    ZPortType::Z_PORT_TYPE_CONTROL, ZPortFlow::Z_PORT_FLOW_INPUT, _ ("Enabled"));
  port->id.sym = g_strdup ("enabled");
  port->id.comment = g_strdup (_ ("Enables or disables the plugin"));
  port->id.port_group = g_strdup ("[Zrythm]");
  plugin_add_in_port (plugin, port);
  port->id.flags |= PortIdentifier::Flags::PLUGIN_ENABLED;
  port->id.flags |= PortIdentifier::Flags::TOGGLE;
  port->id.flags |= PortIdentifier::Flags::AUTOMATABLE;
  port->id.flags |= PortIdentifier::Flags::GENERIC_PLUGIN_PORT;
  port->minf = 0.f;
  port->maxf = 1.f;
  port->zerof = 0.f;
  port->deff = 1.f;
  port->control = 1.f;
  port->unsnapped_control = 1.f;
  port->carla_param_id = -1;
  plugin->enabled = port;

  /* add gain port */
  port = port_new_with_type (
    ZPortType::Z_PORT_TYPE_CONTROL, ZPortFlow::Z_PORT_FLOW_INPUT, _ ("Gain"));
  port->id.sym = g_strdup ("gain");
  port->id.comment = g_strdup (_ ("Plugin gain"));
  plugin_add_in_port (plugin, port);
  port->id.flags |= PortIdentifier::Flags::PLUGIN_GAIN;
  port->id.flags |= PortIdentifier::Flags::AUTOMATABLE;
  port->id.flags |= PortIdentifier::Flags::GENERIC_PLUGIN_PORT;
  port->id.port_group = g_strdup ("[Zrythm]");
  port->minf = 0.f;
  port->maxf = 8.f;
  port->zerof = 0.f;
  port->deff = 1.f;
  port_set_control_value (port, 1.f, F_NOT_NORMALIZED, F_NO_PUBLISH_EVENTS);
  port->carla_param_id = -1;
  plugin->gain = port;

  plugin->selected_bank.bank_idx = -1;
  plugin->selected_bank.idx = -1;
  plugin_preset_identifier_init (&plugin->selected_preset);
  plugin->selected_preset.bank_idx = -1;
  plugin->selected_preset.idx = -1;

  plugin_set_ui_refresh_rate (plugin);
}

PluginBank *
plugin_add_bank_if_not_exists (Plugin * self, const char * uri, const char * name)
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
  plugin_identifier_copy (&bank->id.plugin_id, &self->id);
  bank->name = g_strdup (name);
  if (uri)
    bank->uri = g_strdup (uri);

  array_double_size_if_full (
    self->banks, self->num_banks, self->banks_size, PluginBank *);
  array_append (self->banks, self->num_banks, bank);

  return bank;
}

void
plugin_add_preset_to_bank (Plugin * self, PluginBank * bank, PluginPreset * preset)
{
  preset->id.idx = bank->num_presets;
  preset->id.bank_idx = bank->id.bank_idx;
  plugin_identifier_copy (&preset->id.plugin_id, &bank->id.plugin_id);

  array_double_size_if_full (
    bank->presets, bank->num_presets, bank->presets_size, PluginPreset *);
  array_append (bank->presets, bank->num_presets, preset);
}

static void
populate_banks (Plugin * self)
{
  g_message ("populating plugin banks...");

#ifdef HAVE_CARLA
  if (self->setting->open_with_carla)
    {
      carla_native_plugin_populate_banks (self->carla);
    }
#endif
}

void
plugin_set_selected_bank_from_index (Plugin * self, int idx)
{
  self->selected_bank.bank_idx = idx;
  self->selected_preset.bank_idx = idx;
  plugin_set_selected_preset_from_index (self, 0);
}

void
plugin_set_selected_preset_from_index (Plugin * self, int idx)
{
  g_return_if_fail (self->instantiated);

  self->selected_preset.idx = idx;

  g_message ("applying preset at index %d", idx);

  GError * err = NULL;
  bool     applied = false;
  if (self->setting->open_with_carla)
    {
#ifdef HAVE_CARLA
      g_return_if_fail (self->carla->host_handle);

      /* if init preset */
      if (self->selected_bank.bank_idx == 0 && idx == 0)
        {
          carla_reset_parameters (self->carla->host_handle, 0);
          applied = true;
          g_message ("applied default preset");
        }
      else
        {
          PluginPreset * pset =
            self->banks[self->selected_bank.bank_idx]->presets[idx];
          carla_set_program (
            self->carla->host_handle, 0, (uint32_t) pset->carla_program);
          applied = true;
          g_message ("applied preset '%s'", pset->name);
        }
#endif
    }

  if (!applied)
    {
      HANDLE_ERROR (err, "%s", _ ("Failed to apply preset"));
    }
}

void
plugin_set_selected_preset_by_name (Plugin * self, const char * name)
{
  g_return_if_fail (self->instantiated);

  PluginBank * bank = self->banks[self->selected_bank.bank_idx];
  for (int i = 0; i < bank->num_presets; i++)
    {
      PluginPreset * pset = bank->presets[i];
      if (string_is_equal (pset->name, name))
        {
          plugin_set_selected_preset_from_index (self, i);
          return;
        }
    }

  g_return_if_reached ();
}

/**
 * Creates/initializes a plugin and its internal
 * plugin (LV2, etc.)
 * using the given setting.
 *
 * @param track_name_hash The expected name hash
 *   of track the plugin will be in.
 * @param slot The expected slot the plugin will
 *   be in.
 */
Plugin *
plugin_new_from_setting (
  PluginSetting * setting,
  unsigned int    track_name_hash,
  ZPluginSlotType slot_type,
  int             slot,
  GError **       error)
{
  Plugin * self = object_new (Plugin);
  self->schema_version = PLUGIN_SCHEMA_VERSION;

  init_port_arrays (self);

  self->setting = plugin_setting_clone (setting, F_VALIDATE);
  setting = self->setting;
  const PluginDescriptor * descr = self->setting->descr;

  g_message (
    "%s: %s (%s) slot %d", __func__, descr->name, ENUM_NAME (descr->protocol),
    slot);

  plugin_init (self, track_name_hash, slot_type, slot);
  g_return_val_if_fail (self->gain && self->enabled, NULL);

#ifdef HAVE_CARLA
  if (setting->open_with_carla)
    {
      GError * err = NULL;
      carla_native_plugin_new_from_setting (self, &err);
      if (!self->carla)
        {
          PROPAGATE_PREFIXED_ERROR (
            error, err, "%s", _ ("Failed to get Carla plugin"));
          return NULL;
        }
    }
#endif

  /* select the init preset */
  self->selected_bank.bank_idx = 0;
  self->selected_bank.idx = -1;
  plugin_identifier_copy (&self->selected_bank.plugin_id, &self->id);
  self->selected_preset.bank_idx = 0;
  self->selected_preset.idx = 0;
  plugin_identifier_copy (&self->selected_preset.plugin_id, &self->id);

  if (!ZRYTHM_TESTING)
    {
      /* save the new setting (may have changed
       * during instantiation) */
      plugin_settings_set (S_PLUGIN_SETTINGS, self->setting, F_SERIALIZE);
    }

  return self;
}

/**
 * Create a dummy plugin for tests.
 */
Plugin *
plugin_new_dummy (ZPluginCategory cat, unsigned int track_name_hash, int slot)
{
  Plugin * self = object_new (Plugin);
  self->schema_version = PLUGIN_SCHEMA_VERSION;

  PluginDescriptor * descr = object_new (PluginDescriptor);
  descr->author = g_strdup ("Hoge");
  descr->name = g_strdup ("Dummy Plugin");
  descr->category = cat;
  descr->category_str = g_strdup ("Dummy Plugin Category");

  self->setting = plugin_setting_new_default (descr);
  plugin_descriptor_free (descr);

  plugin_init (
    self, track_name_hash, ZPluginSlotType::Z_PLUGIN_SLOT_INSERT, slot);

  return self;
}

void
plugin_append_ports (Plugin * self, GPtrArray * ports)
{
#define _ADD(port) g_ptr_array_add (ports, port)

  for (int i = 0; i < self->num_in_ports; i++)
    {
      Port * port = self->in_ports[i];
      g_return_if_fail (port);
      _ADD (port);
    }
  for (int i = 0; i < self->num_out_ports; i++)
    {
      Port * port = self->out_ports[i];
      g_return_if_fail (port);
      _ADD (port);
    }

#undef _ADD
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
  g_return_if_fail (IS_TRACK_AND_NONNULL (track));
  AutomationTracklist * atl = track_get_automation_tracklist (track);
  g_return_if_fail (atl);
  for (int i = atl->num_ats - 1; i >= 0; i--)
    {
      AutomationTrack * at = atl->ats[i];
      if (
        at->port_id.owner_type == PortIdentifier::OwnerType::PLUGIN
        || ENUM_BITSET_TEST (
          PortIdentifier::Flags, at->port_id.flags,
          PortIdentifier::Flags::PLUGIN_CONTROL))
        {
          if (
            at->port_id.plugin_id.slot == pl->id.slot
            && at->port_id.plugin_id.slot_type == pl->id.slot_type)
            {
              automation_tracklist_remove_at (atl, at, free_ats, fire_events);
            }
        }
    }
}

/**
 * Verifies that the plugin identifiers are valid.
 */
bool
plugin_validate (Plugin * self)
{
  g_return_val_if_fail (IS_PLUGIN (self), false);

  if (plugin_is_in_active_project (self))
    {
      /* assert instantiated and activated, or
       * instantiation failed */
      g_return_val_if_fail (
        self->instantiation_failed || (self->instantiated && self->activated),
        false);
    }

  return true;
}

typedef struct PluginMoveData
{
  Plugin *        pl;
  Track *         track;
  ZPluginSlotType slot_type;
  int             slot;
  bool            fire_events;
} PluginMoveData;

static void
plugin_move_data_free (void * _data)
{
  PluginMoveData * self = (PluginMoveData *) _data;
  object_zero_and_free (self);
}

static void
do_move (PluginMoveData * data)
{
  Plugin *        pl = data->pl;
  int             prev_slot = pl->id.slot;
  ZPluginSlotType prev_slot_type = pl->id.slot_type;
  Track *         prev_track = plugin_get_track (pl);
  g_return_if_fail (IS_TRACK_AND_NONNULL (prev_track));
  Channel * prev_ch = plugin_get_channel (pl);
  g_return_if_fail (IS_CHANNEL_AND_NONNULL (prev_ch));

  /* if existing plugin exists, delete it */
  Plugin * existing_pl =
    track_get_plugin_at_slot (data->track, data->slot_type, data->slot);
  if (existing_pl)
    {
      channel_remove_plugin (
        data->track->channel, data->slot_type, data->slot, F_NOT_MOVING_PLUGIN,
        F_DELETING_PLUGIN, F_NOT_DELETING_CHANNEL, F_NO_RECALC_GRAPH);
    }

  /* move plugin's automation from src to
   * dest */
  plugin_move_automation (
    pl, prev_track, data->track, data->slot_type, data->slot);

  /* remove plugin from its channel */
  channel_remove_plugin (
    prev_ch, prev_slot_type, prev_slot, F_MOVING_PLUGIN, F_NOT_DELETING_PLUGIN,
    F_NOT_DELETING_CHANNEL, F_NO_RECALC_GRAPH);

  /* add plugin to its new channel */
  channel_add_plugin (
    data->track->channel, data->slot_type, data->slot, pl, F_NO_CONFIRM,
    F_MOVING_PLUGIN, F_NO_GEN_AUTOMATABLES, F_RECALC_GRAPH, F_PUBLISH_EVENTS);

  if (data->fire_events)
    {
      EVENTS_PUSH (EventType::ET_CHANNEL_SLOTS_CHANGED, prev_ch);
      EVENTS_PUSH (EventType::ET_CHANNEL_SLOTS_CHANGED, data->track->channel);
    }
}

static void
overwrite_plugin_response_cb (
  AdwMessageDialog * dialog,
  char *             response,
  gpointer           user_data)
{
  PluginMoveData * data = (PluginMoveData *) user_data;
  if (!string_is_equal (response, "overwrite"))
    {
      return;
    }

  do_move (data);
}

void
plugin_move (
  Plugin *        pl,
  Track *         track,
  ZPluginSlotType slot_type,
  int             slot,
  bool            confirm_overwrite,
  bool            fire_events)
{
  PluginMoveData * data = object_new (PluginMoveData);
  data->pl = pl;
  data->track = track;
  data->slot_type = slot_type;
  data->slot = slot;
  data->fire_events = fire_events;

  Plugin * existing_pl = track_get_plugin_at_slot (track, slot_type, slot);
  if (existing_pl && confirm_overwrite && ZRYTHM_HAVE_UI)
    {
      AdwMessageDialog * dialog =
        dialogs_get_overwrite_plugin_dialog (GTK_WINDOW (MAIN_WINDOW));
      gtk_window_present (GTK_WINDOW (dialog));
      g_signal_connect_data (
        dialog, "response", G_CALLBACK (overwrite_plugin_response_cb), data,
        (GClosureNotify) plugin_move_data_free, G_CONNECT_DEFAULT);
      return;
    }

  do_move (data);
  plugin_move_data_free (data);
}

/**
 * Sets the channel and slot on the plugin and
 * its ports.
 */
void
plugin_set_track_and_slot (
  Plugin *        pl,
  unsigned int    track_name_hash,
  ZPluginSlotType slot_type,
  int             slot)
{
  g_return_if_fail (
    plugin_identifier_validate_slot_type_slot_combo (slot_type, slot));

  pl->id.track_name_hash = track_name_hash;
  pl->id.slot = slot;
  pl->id.slot_type = slot_type;

  for (int i = 0; i < pl->num_in_ports; i++)
    {
      Port *         port = pl->in_ports[i];
      PortIdentifier copy_id = PortIdentifier (port->id);
      port->set_owner (PortIdentifier::OwnerType::PLUGIN, pl);
      if (plugin_is_in_active_project (pl))
        {
          Track * track = plugin_get_track (pl);
          port_update_identifier (
            port, &copy_id, track, F_NO_UPDATE_AUTOMATION_TRACK);
        }
    }
  for (int i = 0; i < pl->num_out_ports; i++)
    {
      Port *         port = pl->out_ports[i];
      PortIdentifier copy_id = PortIdentifier (port->id);
      port->set_owner (PortIdentifier::OwnerType::PLUGIN, pl);
      if (plugin_is_in_active_project (pl))
        {
          Track * track = plugin_get_track (pl);
          port_update_identifier (
            port, &copy_id, track, F_NO_UPDATE_AUTOMATION_TRACK);
        }
    }
}

Track *
plugin_get_track (const Plugin * self)
{
  g_return_val_if_fail (self->track, NULL);
  return self->track;
}

Channel *
plugin_get_channel (Plugin * self)
{
  Track * track = plugin_get_track (self);
  g_return_val_if_fail (track, NULL);
  Channel * ch = track->channel;
  g_return_val_if_fail (ch, NULL);

  return ch;
}

Plugin *
plugin_find (const PluginIdentifier * id)
{
  Plugin plugin;
  memset (&plugin, 0, sizeof (Plugin));
  plugin_identifier_copy (&plugin.id, id);

#if 0
  Tracklist * tracklist = NULL;
  if (plugin.is_auditioner)
    tracklist = SAMPLE_PROCESSOR->tracklist;
  else if (!plugin.is_function)
    tracklist = TRACKLIST;

  if (!tracklist)
    return NULL;
#endif

  Track * track =
    tracklist_find_track_by_name_hash (TRACKLIST, plugin.id.track_name_hash);
  g_return_val_if_fail (IS_TRACK_AND_NONNULL (track), NULL);

  Channel * ch = NULL;
  if (
    track->type != TrackType::TRACK_TYPE_MODULATOR
    || id->slot_type == ZPluginSlotType::Z_PLUGIN_SLOT_MIDI_FX
    || id->slot_type == ZPluginSlotType::Z_PLUGIN_SLOT_INSTRUMENT
    || id->slot_type == ZPluginSlotType::Z_PLUGIN_SLOT_INSERT)
    {
      ch = track->channel;
      g_return_val_if_fail (ch, NULL);
    }
  Plugin * ret = NULL;
  switch (id->slot_type)
    {
    case ZPluginSlotType::Z_PLUGIN_SLOT_MIDI_FX:
      g_return_val_if_fail (IS_CHANNEL_AND_NONNULL (ch), NULL);
      ret = ch->midi_fx[id->slot];
      break;
    case ZPluginSlotType::Z_PLUGIN_SLOT_INSTRUMENT:
      g_return_val_if_fail (IS_CHANNEL_AND_NONNULL (ch), NULL);
      ret = ch->instrument;
      break;
    case ZPluginSlotType::Z_PLUGIN_SLOT_INSERT:
      g_return_val_if_fail (IS_CHANNEL_AND_NONNULL (ch), NULL);
      ret = ch->inserts[id->slot];
      break;
    case ZPluginSlotType::Z_PLUGIN_SLOT_MODULATOR:
      g_return_val_if_fail (IS_TRACK_AND_NONNULL (track), NULL);
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
  sprintf (buf, "%s/%s/%s", track->name, self->setting->descr->name, port_group);
}

Port *
plugin_get_port_in_group (Plugin * self, const char * port_group, bool left)
{
  for (int i = 0; i < self->num_in_ports; i++)
    {
      Port * port = self->in_ports[i];
      if (
        string_is_equal (port->id.port_group, port_group)
        && ENUM_BITSET_TEST (
          PortIdentifier::Flags, port->id.flags,
          (left ? PortIdentifier::Flags::STEREO_L
                : PortIdentifier::Flags::STEREO_R)))
        {
          return port;
        }
    }
  for (int i = 0; i < self->num_out_ports; i++)
    {
      Port * port = self->out_ports[i];
      if (
        string_is_equal (port->id.port_group, port_group)
        && ENUM_BITSET_TEST (
          PortIdentifier::Flags, port->id.flags,
          (left ? PortIdentifier::Flags::STEREO_L
                : PortIdentifier::Flags::STEREO_R)))
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
plugin_get_port_in_same_group (Plugin * self, Port * port)
{
  if (!port->id.port_group)
    {
      g_message ("port %s has no port group", port->id.label);
      return NULL;
    }

  int     num_ports = 0;
  Port ** ports = NULL;
  if (port->id.flow == ZPortFlow::Z_PORT_FLOW_INPUT)
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

      if (string_is_equal (port->id.port_group, cur_port->id.port_group) && ((ENUM_BITSET_TEST (PortIdentifier::Flags, cur_port->id.flags, PortIdentifier::Flags::STEREO_L) && ENUM_BITSET_TEST (PortIdentifier::Flags, port->id.flags, PortIdentifier::Flags::STEREO_R)) || (ENUM_BITSET_TEST (PortIdentifier::Flags, cur_port->id.flags, PortIdentifier::Flags::STEREO_R) && ENUM_BITSET_TEST (PortIdentifier::Flags, port->id.flags, PortIdentifier::Flags::STEREO_L))))
        {
          return cur_port;
        }
    }

  return NULL;
}

char *
plugin_generate_window_title (Plugin * self)
{
  g_return_val_if_fail (self->setting->descr, NULL);
  g_return_val_if_fail (plugin_is_in_active_project (self), NULL);

  Track * track = self->track;

  const PluginSetting * setting = self->setting;

  const char * track_name = track ? track->name : "";
  const char * plugin_name = setting->descr->name;
  g_return_val_if_fail (track_name && plugin_name, NULL);

  char bridge_mode[100];
  strcpy (bridge_mode, "");
  if (setting->bridge_mode != CarlaBridgeMode::None)
    {
      sprintf (bridge_mode, " - bridge: %s", ENUM_NAME (setting->bridge_mode));
    }

  char slot[100];
  sprintf (slot, "#%d", self->id.slot + 1);
  if (self->id.slot_type == ZPluginSlotType::Z_PLUGIN_SLOT_INSTRUMENT)
    {
      strcpy (slot, "instrument");
    }

  char title[500];
  sprintf (
    title, "%s (%s %s%s%s)", plugin_name, track_name, slot,
    /* assume all plugins use carla for now */
    "",
    /*setting->open_with_carla ? " carla" : "",*/
    bridge_mode);

  return g_strdup (title);
}

/**
 * Activates or deactivates the plugin.
 *
 * @param activate True to activate, false to
 *   deactivate.
 */
int
plugin_activate (Plugin * pl, bool activate)
{
  g_return_val_if_fail (IS_PLUGIN (pl), -1);

  if ((pl->activated && activate) || (!pl->activated && !activate))
    {
      /* nothing to do */
      g_message ("%s: nothing to do", __func__);
      return 0;
    }

  if (activate && !pl->instantiated)
    {
      g_critical ("plugin %s not instantiated", pl->setting->descr->name);
      return -1;
    }

  if (!activate)
    {
      pl->deactivating = true;
    }

  if (pl->setting->open_with_carla)
    {
#ifdef HAVE_CARLA
      int ret = carla_native_plugin_activate (pl->carla, activate);
      g_return_val_if_fail (ret == 0, ret);
#endif
    }

  pl->activated = activate;
  pl->deactivating = false;

  return 0;
}

/**
 * Cleans up an instantiated but not activated
 * plugin.
 */
int
plugin_cleanup (Plugin * self)
{
  g_message ("Cleaning up %s...", self->setting->descr->name);

  if (!self->activated && self->instantiated)
    {
      if (self->setting->open_with_carla)
        {
#ifdef HAVE_CARLA
#  if 0
          carla_native_plugin_close (
            self->carla);
#  endif
#endif
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
plugin_update_latency (Plugin * pl)
{
  if (pl->setting->open_with_carla)
    {
#ifdef HAVE_CARLA
      pl->latency = carla_native_plugin_get_latency (pl->carla);
#endif
    }

  g_message ("%s latency: %d samples", pl->setting->descr->name, pl->latency);
}

/**
 * Adds a port of the given type to the Plugin.
 */
#define ADD_PORT(type) \
  while (pl->num_##type##_ports >= (int) pl->type##_ports_size) \
    { \
      if (pl->type##_ports_size == 0) \
        pl->type##_ports_size = 1; \
      else \
        pl->type##_ports_size *= 2; \
      pl->type##_ports = static_cast<Port **> (g_realloc ( \
        pl->type##_ports, sizeof (Port *) * pl->type##_ports_size)); \
    } \
  port->id.port_index = pl->num_##type##_ports; \
  port->set_owner (PortIdentifier::OwnerType::PLUGIN, pl); \
  array_append (pl->type##_ports, pl->num_##type##_ports, port)

/**
 * Adds an in port to the plugin's list.
 */
void
plugin_add_in_port (Plugin * pl, Port * port)
{
  ADD_PORT (in);
#if 0
  g_debug (
    "added input port %s to plugin %s at index %d",
    port->id.label, pl->descr->name,
    port->id.port_index);
#endif
}

/**
 * Adds an out port to the plugin's list.
 */
void
plugin_add_out_port (Plugin * pl, Port * port)
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
  Plugin *        pl,
  Track *         prev_track,
  Track *         track,
  ZPluginSlotType new_slot_type,
  int             new_slot)
{
  g_message (
    "moving plugin '%s' automation from "
    "%s to %s -> %s:%d",
    pl->setting->descr->name, prev_track->name, track->name,
    ENUM_NAME (new_slot_type), new_slot);

  AutomationTracklist * prev_atl = track_get_automation_tracklist (prev_track);
  g_return_if_fail (prev_atl);
  AutomationTracklist * atl = track_get_automation_tracklist (track);
  g_return_if_fail (atl);

  unsigned int name_hash = track_get_name_hash (track);
  for (int i = prev_atl->num_ats - 1; i >= 0; i--)
    {
      AutomationTrack * at = prev_atl->ats[i];
      Port *            port = Port::find_from_identifier (&at->port_id);
      if (!port)
        continue;
      g_return_if_fail (IS_PORT (port));
      if (port->id.owner_type == PortIdentifier::OwnerType::PLUGIN)
        {
          Plugin * port_pl = port_get_plugin (port, 1);
          if (port_pl != pl)
            continue;
        }
      else
        continue;

      g_return_if_fail (port->at == at);

      /* delete from prev channel */
      int num_regions_before = at->num_regions;
      automation_tracklist_remove_at (
        prev_atl, at, F_NO_FREE, F_NO_PUBLISH_EVENTS);

      /* add to new channel */
      automation_tracklist_add_at (atl, at);
      g_warn_if_fail (
        at == atl->ats[at->index] && at->num_regions == num_regions_before);

      /* update the automation track port
       * identifier */
      at->port_id.plugin_id.slot = new_slot;
      at->port_id.plugin_id.slot_type = new_slot_type;
      at->port_id.plugin_id.track_name_hash = name_hash;

      g_warn_if_fail (at->port_id.port_index == port->id.port_index);
    }
}

/**
 * Sets the UI refresh rate on the Plugin.
 */
void
plugin_set_ui_refresh_rate (Plugin * self)
{
  if (ZRYTHM_TESTING || ZRYTHM_GENERATING_PROJECT)
    {
      self->ui_update_hz = 30.f;
      self->ui_scale_factor = 1.f;
      goto return_refresh_rate_and_scale_factor;
    }

  /* if no preferred refresh rate is set,
   * use the monitor's refresh rate */
  if (g_settings_get_int (S_P_PLUGINS_UIS, "refresh-rate"))
    {
      /* Use user-specified UI update rate. */
      self->ui_update_hz =
        (float) g_settings_get_int (S_P_PLUGINS_UIS, "refresh-rate");
    }
  else
    {
      self->ui_update_hz = (float) z_gtk_get_primary_monitor_refresh_rate ();
      g_debug (
        "refresh rate returned by GDK: %.01f", (double) self->ui_update_hz);
    }

  /* if no preferred scale factor is set,
   * use the monitor's scale factor */
  {
    float scale_factor_setting =
      (float) g_settings_get_double (S_P_PLUGINS_UIS, "scale-factor");
    if (scale_factor_setting >= 0.5f)
      {
        /* use user-specified scale factor */
        self->ui_scale_factor = scale_factor_setting;
      }
    else
      {
        /* set the scale factor */
        self->ui_scale_factor =
          (float) z_gtk_get_primary_monitor_scale_factor ();
        g_debug (
          "scale factor returned by GDK: %.01f", (double) self->ui_scale_factor);
      }
  }

  /* clamp the refresh rate to sensible limits */
  if (
    self->ui_update_hz < PLUGIN_MIN_REFRESH_RATE
    || self->ui_update_hz > PLUGIN_MAX_REFRESH_RATE)
    {
      g_warning (
        "Invalid refresh rate of %.01f received, "
        "clamping to reasonable bounds",
        (double) self->ui_update_hz);
      self->ui_update_hz = CLAMP (
        self->ui_update_hz, PLUGIN_MIN_REFRESH_RATE, PLUGIN_MAX_REFRESH_RATE);
    }

  /* clamp the scale factor to sensible limits */
  if (
    self->ui_scale_factor < PLUGIN_MIN_SCALE_FACTOR
    || self->ui_scale_factor > PLUGIN_MAX_SCALE_FACTOR)
    {
      g_warning (
        "Invalid scale factor of %.01f received, "
        "clamping to reasonable bounds",
        (double) self->ui_scale_factor);
      self->ui_scale_factor = CLAMP (
        self->ui_scale_factor, PLUGIN_MIN_SCALE_FACTOR, PLUGIN_MAX_SCALE_FACTOR);
    }

return_refresh_rate_and_scale_factor:
  g_message ("refresh rate set to %f", (double) self->ui_update_hz);
  g_message ("scale factor set to %f", (double) self->ui_scale_factor);
}

/**
 * Returns the escaped name of the plugin.
 */
char *
plugin_get_escaped_name (Plugin * pl)
{
  const PluginDescriptor * descr = pl->setting->descr;
  g_return_val_if_fail (descr, NULL);

  char tmp[strlen (descr->name) + 200];
  io_escape_dir_name (tmp, descr->name);
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
plugin_generate_automation_tracks (Plugin * self, Track * track)
{
  g_message (
    "generating automation tracks for %s...", self->setting->descr->name);

  AutomationTracklist * atl = track_get_automation_tracklist (track);
  for (int i = 0; i < self->num_in_ports; i++)
    {
      Port * port = self->in_ports[i];
      if (
        port->id.type != ZPortType::Z_PORT_TYPE_CONTROL
        || !(ENUM_BITSET_TEST (
          PortIdentifier::Flags, port->id.flags,
          PortIdentifier::Flags::AUTOMATABLE)))
        continue;

      AutomationTrack * at = automation_track_new (port);
      automation_tracklist_add_at (atl, at);
    }
}

/**
 * Gets the enable/disable port for this plugin.
 */
Port *
plugin_get_enabled_port (Plugin * self)
{
  for (int i = 0; i < self->num_in_ports; i++)
    {
      Port * port = self->in_ports[i];
      if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags, port->id.flags,
          PortIdentifier::Flags::PLUGIN_ENABLED)
        && ENUM_BITSET_TEST (
          PortIdentifier::Flags, port->id.flags,
          PortIdentifier::Flags::GENERIC_PLUGIN_PORT))
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
plugin_update_identifier (Plugin * self)
{
  g_return_if_fail (self->track);

  /* set port identifier track poses */
  for (int i = 0; i < self->num_in_ports; i++)
    {
      Port * port = self->in_ports[i];
      port_update_track_name_hash (port, self->track, self->id.track_name_hash);
      port->id.plugin_id = self->id;
    }
  for (int i = 0; i < self->num_out_ports; i++)
    {
      Port * port = self->out_ports[i];
      port_update_track_name_hash (port, self->track, self->id.track_name_hash);
      port->id.plugin_id = self->id;
    }
}

/**
 * Sets the track name hash on the plugin.
 */
void
plugin_set_track_name_hash (Plugin * pl, unsigned int track_name_hash)
{
  pl->id.track_name_hash = track_name_hash;

  plugin_update_identifier (pl);
}

int
plugin_instantiate (Plugin * self, GError ** error)
{
  g_return_val_if_fail (self->setting, -1);
  const PluginDescriptor * descr = self->setting->descr;
  g_return_val_if_fail (descr, -1);

  g_message ("Instantiating plugin '%s'...", descr->name);

  set_enabled_and_gain (self);

  plugin_set_ui_refresh_rate (self);

  if (!PROJECT->loaded)
    {
      g_return_val_if_fail (self->state_dir, -1);
    }
  g_message ("state dir: %s", self->state_dir);

  if (self->setting->open_with_carla)
    {
#ifdef HAVE_CARLA
      GError * err = NULL;
      int      ret = carla_native_plugin_instantiate (
        self->carla, !PROJECT->loaded, self->state_dir ? true : false, &err);
      if (ret != 0)
        {
          PROPAGATE_PREFIXED_ERROR_LITERAL (
            error, err, _ ("Carla plugin instantiation failed"));
          return -1;
        }

      /* save the state */
      bool success =
        carla_native_plugin_save_state (self->carla, false, NULL, &err);
      if (!success)
        {
          PROPAGATE_PREFIXED_ERROR_LITERAL (
            error, err, _ ("Failed saving Carla plugin state"));
          return -1;
        }
#else
      g_return_val_if_reached (-1);
#endif
    }
  g_return_val_if_fail (IS_PORT_AND_NONNULL (self->enabled), -1);
  control_port_set_val_from_normalized (self->enabled, 1.f, 0);

  /* set the L/R outputs */
  set_stereo_outs_and_midi_in (self);

  /* update banks */
  populate_banks (self);

  self->instantiated = true;

  return 0;
}

/**
 * Prepare plugin for processing.
 */
void
plugin_prepare_process (Plugin * self)
{
  for (size_t i = 0; i < self->audio_in_ports->len; i++)
    {
      Port * port = (Port *) g_ptr_array_index (self->audio_in_ports, i);
      port_clear_buffer (AUDIO_ENGINE, port);
    }
  for (size_t i = 0; i < self->cv_in_ports->len; i++)
    {
      Port * port = (Port *) g_ptr_array_index (self->cv_in_ports, i);
      port_clear_buffer (AUDIO_ENGINE, port);
    }
  for (size_t i = 0; i < self->midi_in_ports->len; i++)
    {
      Port * port = (Port *) g_ptr_array_index (self->midi_in_ports, i);
      port_clear_buffer (AUDIO_ENGINE, port);
    }

  for (int i = 0; i < self->num_out_ports; i++)
    {
      port_clear_buffer (AUDIO_ENGINE, self->out_ports[i]);
    }
}

/**
 * Process plugin.
 */
void
plugin_process (Plugin * plugin, const EngineProcessTimeInfo * const time_nfo)
{
  if (!plugin_is_enabled (plugin, true) && !plugin->own_enabled_port)
    {
      plugin_process_passthrough (plugin, time_nfo);
      return;
    }

  if (!plugin->instantiated || !plugin->activated)
    {
      return;
    }

  /* if has MIDI input port */
  if (plugin->setting->descr->num_midi_ins > 0)
    {
      /* if recording, write MIDI events to the region TODO */

      /* if there is a midi note in this buffer range TODO */
      /* add midi events to input port */
    }

#ifdef HAVE_CARLA
  if (plugin->setting->open_with_carla)
    {
      carla_native_plugin_process (plugin->carla, time_nfo);
    }
#endif

    /* turn off any trigger input controls */
    /* (only used in direct LV2 implementation which is now disabled, so skip
     * this for now to avoid unnecessary loops) */
#if 0
  for (size_t i = 0; i < plugin->ctrl_in_ports->len; i++)
    {
      Port * port = (Port *) g_ptr_array_index (plugin->ctrl_in_ports, i);
      if (
        ENUM_BITSET_TEST(PortIdentifier::Flags,port->id.flags,PortIdentifier::Flags::TRIGGER)
        && !math_floats_equal (port->control, 0.f))
        {
          port_set_control_value (port, 0.f, 0, 1);
        }
    }
#endif

  /* if plugin has gain, apply it */
  if (!math_floats_equal_epsilon (plugin->gain->control, 1.f, 0.001f))
    {
      for (int i = 0; i < plugin->num_out_ports; i++)
        {
          Port * port = plugin->out_ports[i];
          if (port->id.type != ZPortType::Z_PORT_TYPE_AUDIO)
            continue;

          /* if close to 0 set it to the denormal prevention val */
          if (math_floats_equal_epsilon (plugin->gain->control, 0.f, 0.00001f))
            {
              dsp_fill (
                &port->buf[time_nfo->local_offset],
                DENORMAL_PREVENTION_VAL (AUDIO_ENGINE), time_nfo->nframes);
            }
          /* otherwise just apply gain */
          else
            {
              dsp_mul_k2 (
                &port->buf[time_nfo->local_offset], plugin->gain->control,
                time_nfo->nframes);
            }
        }
    }
}

/**
 * Prints the plugin to the buffer, if any, or to
 * the log.
 */
void
plugin_print (Plugin * self, char * buf, size_t buf_sz)
{
  if (buf)
    {
      Track * track = plugin_is_in_active_project (self) ? self->track : NULL;
      snprintf (
        buf, buf_sz, "%s (%d):%s:%d - %s", track ? track->name : "<no track>",
        track ? track->pos : -1, ENUM_NAME (self->id.slot_type), self->id.slot,
        self->setting->descr->name);
    }
}

/**
 * Sets caches for processing.
 */
void
plugin_set_caches (Plugin * self)
{

#define PREPARE_ARRAY(arr) g_ptr_array_remove_range (arr, 0, arr->len)

  PREPARE_ARRAY (self->ctrl_in_ports);
  PREPARE_ARRAY (self->audio_in_ports);
  PREPARE_ARRAY (self->cv_in_ports);
  PREPARE_ARRAY (self->midi_in_ports);

#undef PREPARE_ARRAY

  for (int i = 0; i < self->num_in_ports; i++)
    {
      Port * port = self->in_ports[i];
      switch (port->id.type)
        {
        case ZPortType::Z_PORT_TYPE_CONTROL:
          g_ptr_array_add (self->ctrl_in_ports, port);
          break;
        case ZPortType::Z_PORT_TYPE_AUDIO:
          g_ptr_array_add (self->audio_in_ports, port);
          break;
        case ZPortType::Z_PORT_TYPE_CV:
          g_ptr_array_add (self->cv_in_ports, port);
          break;
        case ZPortType::Z_PORT_TYPE_EVENT:
          g_ptr_array_add (self->midi_in_ports, port);
          break;
        default:
          break;
        }
    }
}

/**
 * Shows plugin ui and sets window close callback
 */
void
plugin_open_ui (Plugin * self)
{
  g_return_if_fail (IS_PLUGIN (self));
  g_return_if_fail (self->setting->descr);
  g_return_if_fail (plugin_is_in_active_project (self));

  char pl_str[700];
  plugin_print (self, pl_str, 700);
  g_debug ("opening plugin UI [%s]", pl_str);

  PluginSetting * setting = self->setting;

  if (self->instantiation_failed)
    {
      g_message (
        "plugin %s instantiation failed, no UI to "
        "open",
        pl_str);
      return;
    }

  /* if plugin already has a window (either generic
   * or LV2 non-carla and non-external UI) */
  if (GTK_IS_WINDOW (self->window))
    {
      /* present it */
      g_debug ("presenting plugin [%s] window %p", pl_str, self->window);
      gtk_window_present (GTK_WINDOW (self->window));
    }
  else
    {
      bool generic_ui = setting->force_generic_ui;

      /* handle generic UIs, then carla custom,
       * then LV2 custom */
      if (generic_ui)
        {
          g_debug ("creating and opening generic UI");
          plugin_gtk_create_window (self);
          plugin_gtk_open_generic_ui (self, F_PUBLISH_EVENTS);
        }
      else if (setting->open_with_carla)
        {
#ifdef HAVE_CARLA
          carla_native_plugin_open_ui (self->carla, 1);
#endif
        }
    }
}

/**
 * Returns if Plugin exists in MixerSelections.
 */
bool
plugin_is_selected (Plugin * pl)
{
  g_return_val_if_fail (IS_PLUGIN (pl), false);

  return mixer_selections_contains_plugin (MIXER_SELECTIONS, pl);
}

/**
 * Selects the plugin in the MixerSelections.
 *
 * @param select Select or deselect.
 * @param exclusive Whether to make this the only
 *   selected plugin or add it to the selections.
 */
void
plugin_select (Plugin * self, bool select, bool exclusive)
{
  g_return_if_fail (IS_PLUGIN (self));
  g_return_if_fail (self->setting->descr);
  g_return_if_fail (plugin_is_in_active_project (self));

  if (exclusive)
    {
      mixer_selections_clear (MIXER_SELECTIONS, F_PUBLISH_EVENTS);
    }

  Track * track = plugin_get_track (self);
  g_return_if_fail (IS_TRACK_AND_NONNULL (track));

  if (select)
    {
      mixer_selections_add_slot (
        MIXER_SELECTIONS, track, self->id.slot_type, self->id.slot, F_NO_CLONE,
        F_PUBLISH_EVENTS);
    }
  else
    {
      mixer_selections_remove_slot (
        MIXER_SELECTIONS, self->id.slot, self->id.slot_type, F_PUBLISH_EVENTS);
    }
}

/**
 * Copies the state directory from the given source
 * plugin to the given destination plugin's state
 * directory.
 *
 * @param is_backup Whether this is a backup
 *   project. Used for calculating the absolute
 *   path to the state dir.
 * @param abs_state_dir If passed, the state will
 *   be saved inside this directory instead of the
 *   plugin's state directory. Used when saving
 *   presets.
 */
int
plugin_copy_state_dir (
  Plugin *     self,
  Plugin *     src,
  bool         is_backup,
  const char * abs_state_dir)
{
  char * dir_to_use = NULL;
  if (abs_state_dir)
    {
      dir_to_use = g_strdup (abs_state_dir);
    }
  else
    {
      dir_to_use = plugin_get_abs_state_dir (self, is_backup, true);
    }
  char ** files_in_dir = io_get_files_in_dir (dir_to_use, false);
  g_return_val_if_fail (!files_in_dir, -1);

  char * src_dir_to_use = plugin_get_abs_state_dir (src, is_backup, false);

  GError * err = NULL;
  bool     success = io_copy_dir (
    dir_to_use, src_dir_to_use, F_FOLLOW_SYMLINKS, F_RECURSIVE, &err);
  if (!success)
    {
      HANDLE_ERROR_LITERAL (err, "Failed to copy state directory");
      return -1;
    }

  g_free (src_dir_to_use);
  g_free (dir_to_use);

  g_warn_if_fail (self->state_dir);

  return 0;
}

/**
 * Returns the state dir as an absolute path.
 */
char *
plugin_get_abs_state_dir (Plugin * self, bool is_backup, bool create_if_not_exists)
{
  if (create_if_not_exists)
    {
      GError * err = NULL;
      bool     success = plugin_ensure_state_dir (self, is_backup, &err);
      if (!success)
        {
          HANDLE_ERROR_LITERAL (err, "Failed to create plugin state directory");
          return NULL;
        }
    }
  else
    {
      g_return_val_if_fail (self->state_dir, NULL);
    }

  char * parent_dir = project_get_path (
    PROJECT, ProjectPath::PROJECT_PATH_PLUGIN_STATES, is_backup);
  char * full_path = g_build_filename (parent_dir, self->state_dir, NULL);

  g_free (parent_dir);

  return full_path;
}

/**
 * Ensures the state dir exists or creates it.
 *
 * @return Whether successful.
 */
bool
plugin_ensure_state_dir (Plugin * self, bool is_backup, GError ** error)
{
  if (self->state_dir)
    {
      char * parent_dir = project_get_path (
        PROJECT, ProjectPath::PROJECT_PATH_PLUGIN_STATES, is_backup);
      char * abs_state_dir =
        g_build_filename (parent_dir, self->state_dir, NULL);
      GError * err = NULL;
      bool     success = io_mkdir (abs_state_dir, &err);
      if (!success)
        {
          PROPAGATE_PREFIXED_ERROR (
            error, err, "Failed to create state directory %s", abs_state_dir);
          return false;
        }
      g_free (parent_dir);
      g_free (abs_state_dir);
      return true;
    }

  char * escaped_name = plugin_get_escaped_name (self);
  g_return_val_if_fail (escaped_name, false);
  char * parent_dir = project_get_path (
    PROJECT, ProjectPath::PROJECT_PATH_PLUGIN_STATES, is_backup);
  g_return_val_if_fail (parent_dir, false);
  char * tmp = g_strdup_printf ("%s_XXXXXX", escaped_name);
  char * abs_state_dir_template = g_build_filename (parent_dir, tmp, NULL);
  if (!abs_state_dir_template)
    {
      g_critical ("Failed to build filename using '%s' / '%s'", parent_dir, tmp);
      return false;
    }
  GError * err = NULL;
  bool     success = io_mkdir (parent_dir, &err);
  if (!success)
    {
      PROPAGATE_PREFIXED_ERROR (
        error, err, "Failed to create directory %s", parent_dir);
      return false;
    }
  char * abs_state_dir = g_mkdtemp (abs_state_dir_template);
  if (!abs_state_dir)
    {
      g_critical (
        "Failed to make state dir using template "
        "%s: %s",
        abs_state_dir_template, strerror (errno));
      return false;
    }
  self->state_dir = g_path_get_basename (abs_state_dir);
  g_debug ("set plugin state dir to %s", self->state_dir);
  g_free (escaped_name);
  g_free (parent_dir);
  g_free (tmp);
  g_free (abs_state_dir);

  return true;
}

/**
 * Returns all plugins in the current project.
 */
void
plugin_get_all (Project * prj, GPtrArray * arr, bool check_undo_manager)
{
  for (int i = 0; i < prj->tracklist->num_tracks; i++)
    {
      Track * track = prj->tracklist->tracks[i];
      track_get_plugins (track, arr);
    }

  if (check_undo_manager)
    {
      undo_manager_get_plugins (prj->undo_manager, arr);
    }
}

/**
 * Clones the given plugin.
 *
 * @param error To be filled if an error occurred.
 *
 * @return The cloned plugin, or NULL if an error
 *   occurred.
 */
Plugin *
plugin_clone (Plugin * src, GError ** error)
{
  g_return_val_if_fail (!error || !*error, NULL);
  g_return_val_if_fail (IS_PLUGIN (src), NULL);

  char buf[800];
  plugin_print (src, buf, 800);

  Plugin * self = NULL;
  g_debug ("[0/5] cloning plugin '%s'", buf);

  /* save the state of the original plugin */
  g_message ("[1/5] saving state of source plugin (if instantiated)");
  if (src->instantiated)
    {
      if (src->setting->open_with_carla)
        {
#ifdef HAVE_CARLA
          GError * err = NULL;
          bool     success = carla_native_plugin_save_state (
            src->carla, F_NOT_BACKUP, NULL, &err);
          if (!success)
            {
              PROPAGATE_PREFIXED_ERROR_LITERAL (
                error, err, _ ("Failed saving Carla plugin state"));
              return NULL;
            }
#else
          g_return_val_if_reached (NULL);
#endif
        }
      g_message ("saved source plugin state to %s", src->state_dir);
    }

  /* create a new plugin with same descriptor */
  g_message ("[2/5] creating new plugin with same setting");
  GError * err = NULL;
  self = plugin_new_from_setting (
    src->setting, src->id.track_name_hash, src->id.slot_type, src->id.slot,
    &err);
  if (!self || !self->carla)
    {
      PROPAGATE_PREFIXED_ERROR (
        error, err, _ ("Failed to create plugin clone for %s"), buf);
      return NULL;
    }

  /* copy ports */
  g_message ("[3/5] copying ports from source plugin");
  self->enabled = NULL;
  self->gain = NULL;
  for (int i = 0; i < self->num_in_ports; i++)
    {
      object_free_w_func_and_null (port_free, self->in_ports[i]);
    }
  for (int i = 0; i < self->num_out_ports; i++)
    {
      object_free_w_func_and_null (port_free, self->out_ports[i]);
    }
  self->in_ports = static_cast<Port **> (
    g_realloc_n (self->in_ports, (size_t) src->num_in_ports, sizeof (Port *)));
  self->out_ports = static_cast<Port **> (g_realloc_n (
    self->out_ports, (size_t) src->num_out_ports, sizeof (Port *)));
  for (int i = 0; i < src->num_in_ports; i++)
    {
      self->in_ports[i] = port_clone (src->in_ports[i]);
      self->in_ports[i]->set_owner (PortIdentifier::OwnerType::PLUGIN, self);
    }
  for (int i = 0; i < src->num_out_ports; i++)
    {
      self->out_ports[i] = port_clone (src->out_ports[i]);
      self->out_ports[i]->set_owner (PortIdentifier::OwnerType::PLUGIN, self);
    }
  self->num_in_ports = src->num_in_ports;
  self->num_out_ports = src->num_out_ports;
  set_enabled_and_gain (self);

  /* copy the state directory */
  g_message ("[4/5] copying state directory from source plugin");
  plugin_copy_state_dir (self, src, F_NOT_BACKUP, NULL);

  g_message ("[5/5] done");

  g_return_val_if_fail (src->num_in_ports || src->num_out_ports, NULL);

  g_return_val_if_fail (self, NULL);
  plugin_identifier_copy (&self->id, &src->id);
  self->magic = PLUGIN_MAGIC;
  self->visible = src->visible;

  /* verify same number of inputs and outputs */
  g_return_val_if_fail (src->num_in_ports == self->num_in_ports, NULL);
  g_return_val_if_fail (src->num_out_ports == self->num_out_ports, NULL);

  return self;
}

bool
plugin_is_enabled (Plugin * self, bool check_track)
{
  if (!control_port_is_toggled (self->enabled))
    return false;

  if (check_track)
    {
      Track * track = plugin_get_track (self);
      g_return_val_if_fail (track, false);
      return track_is_enabled (track);
    }
  else
    {
      return true;
    }
}

void
plugin_set_enabled (Plugin * self, bool enabled, bool fire_events)
{
  g_return_if_fail (self->instantiated);

  port_set_control_value (
    self->enabled, enabled ? 1.f : 0.f, false, fire_events);

  if (fire_events)
    {
      EVENTS_PUSH (EventType::ET_PLUGIN_STATE_CHANGED, self);
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
  Plugin *                            self,
  const EngineProcessTimeInfo * const time_nfo)
{
  int last_audio_idx = 0;
  int last_midi_idx = 0;
  for (int i = 0; i < self->num_in_ports; i++)
    {
      bool   goto_next = false;
      Port * in_port = self->in_ports[i];
      switch (in_port->id.type)
        {
        case ZPortType::Z_PORT_TYPE_AUDIO:
          for (int j = last_audio_idx; j < self->num_out_ports; j++)
            {
              Port * out_port = self->out_ports[j];
              if (out_port->id.type == ZPortType::Z_PORT_TYPE_AUDIO)
                {
                  /* copy */
                  dsp_copy (
                    &out_port->buf[time_nfo->local_offset],
                    &in_port->buf[time_nfo->local_offset], time_nfo->nframes);

                  last_audio_idx = j + 1;
                  goto_next = true;
                  break;
                }
              if (goto_next)
                continue;
            }
          break;
        case ZPortType::Z_PORT_TYPE_EVENT:
          for (int j = last_midi_idx; j < self->num_out_ports; j++)
            {
              Port * out_port = self->out_ports[j];
              if (
                out_port->id.type == ZPortType::Z_PORT_TYPE_EVENT
                && ENUM_BITSET_TEST (
                  PortIdentifier::Flags2, out_port->id.flags2,
                  PortIdentifier::Flags2::SUPPORTS_MIDI))
                {
                  /* copy */
                  midi_events_append (
                    out_port->midi_events, in_port->midi_events,
                    time_nfo->local_offset, time_nfo->nframes, false);

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
plugin_close_ui (Plugin * self)
{
  g_return_if_fail (ZRYTHM_HAVE_UI);
  g_return_if_fail (plugin_is_in_active_project (self));

  if (self->instantiation_failed)
    {
      g_message (
        "plugin %s instantiation failed, "
        "no UI to close",
        self->setting->descr->name);
      return;
    }

  g_return_if_fail (self->instantiated);

  plugin_gtk_close_ui (self);

#ifdef HAVE_CARLA
  bool generic_ui = self->setting->force_generic_ui;
  if (!generic_ui && self->setting->open_with_carla)
    {
      g_message ("closing carla plugin UI");
      carla_native_plugin_open_ui (self->carla, false);
    }
#endif

  /* run events immediately otherwise freed
   * plugin might be accessed by event manager */
  event_manager_process_now (EVENT_MANAGER);

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
          if (port->id.type == ZPortType::Z_PORT_TYPE_EVENT)
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
          if (port->id.type == ZPortType::Z_PORT_TYPE_EVENT)
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
plugin_connect_to_plugin (Plugin * src, Plugin * dest)
{
  int   i, j, last_index, num_ports_to_connect;
  Port *in_port, *out_port;

  /* get actual port counts */
  int num_src_audio_outs = 0;
  for (i = 0; i < src->num_out_ports; i++)
    {
      Port * port = src->out_ports[i];
      if (port->id.type == ZPortType::Z_PORT_TYPE_AUDIO)
        num_src_audio_outs++;
    }

  int num_dest_audio_ins = 0;
  for (i = 0; i < dest->num_in_ports; i++)
    {
      Port * port = dest->in_ports[i];
      if (port->id.type == ZPortType::Z_PORT_TYPE_AUDIO)
        num_dest_audio_ins++;
    }

  if (num_src_audio_outs == 1 && num_dest_audio_ins == 1)
    {
      last_index = 0;
      for (i = 0; i < src->num_out_ports; i++)
        {
          out_port = src->out_ports[i];

          if (out_port->id.type == ZPortType::Z_PORT_TYPE_AUDIO)
            {
              for (j = 0; j < dest->num_in_ports; j++)
                {
                  in_port = dest->in_ports[j];

                  if (in_port->id.type == ZPortType::Z_PORT_TYPE_AUDIO)
                    {
                      port_connect (out_port, in_port, 1);
                      goto done1;
                    }
                }
            }
        }
done1:;
    }
  else if (num_src_audio_outs == 1 && num_dest_audio_ins > 1)
    {
      /* plugin is mono and next plugin is
       * not mono, so connect the mono out to
       * each input */
      for (i = 0; i < src->num_out_ports; i++)
        {
          out_port = src->out_ports[i];

          if (out_port->id.type == ZPortType::Z_PORT_TYPE_AUDIO)
            {
              for (j = 0; j < dest->num_in_ports; j++)
                {
                  in_port = dest->in_ports[j];

                  if (in_port->id.type == ZPortType::Z_PORT_TYPE_AUDIO)
                    {
                      port_connect (out_port, in_port, 1);
                    }
                }
              break;
            }
        }
    }
  else if (num_src_audio_outs > 1 && num_dest_audio_ins == 1)
    {
      /* connect multi-output channel into mono by
       * only connecting to the first input channel
       * found */
      for (i = 0; i < dest->num_in_ports; i++)
        {
          in_port = dest->in_ports[i];

          if (in_port->id.type == ZPortType::Z_PORT_TYPE_AUDIO)
            {
              for (j = 0; j < src->num_out_ports; j++)
                {
                  out_port = src->out_ports[j];

                  if (out_port->id.type == ZPortType::Z_PORT_TYPE_AUDIO)
                    {
                      port_connect (out_port, in_port, 1);
                      goto done2;
                    }
                }
              break;
            }
        }
done2:;
    }
  else if (num_src_audio_outs > 1 && num_dest_audio_ins > 1)
    {
      /* connect to as many audio outs this
       * plugin has, or until we can't connect
       * anymore */
      num_ports_to_connect = MIN (num_src_audio_outs, num_dest_audio_ins);
      last_index = 0;
      int ports_connected = 0;
      for (i = 0; i < src->num_out_ports; i++)
        {
          out_port = src->out_ports[i];

          if (out_port->id.type == ZPortType::Z_PORT_TYPE_AUDIO)
            {
              for (; last_index < dest->num_in_ports; last_index++)
                {
                  in_port = dest->in_ports[last_index];
                  if (in_port->id.type == ZPortType::Z_PORT_TYPE_AUDIO)
                    {
                      port_connect (out_port, in_port, 1);
                      last_index++;
                      ports_connected++;
                      break;
                    }
                }
              if (ports_connected == num_ports_to_connect)
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

      if (
        out_port->id.type == ZPortType::Z_PORT_TYPE_EVENT
        && ENUM_BITSET_TEST (
          PortIdentifier::Flags2, out_port->id.flags2,
          PortIdentifier::Flags2::SUPPORTS_MIDI))
        {
          for (j = 0; j < dest->num_in_ports; j++)
            {
              in_port = dest->in_ports[j];

              if (
                in_port->id.type == ZPortType::Z_PORT_TYPE_EVENT
                && ENUM_BITSET_TEST (
                  PortIdentifier::Flags2, in_port->id.flags2,
                  PortIdentifier::Flags2::SUPPORTS_MIDI))
                {
                  port_connect (out_port, in_port, 1);
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
plugin_connect_to_prefader (Plugin * pl, Channel * ch)
{
  g_return_if_fail (pl->instantiated || pl->instantiation_failed);

  Track *   track = channel_get_track (ch);
  ZPortType type = track->out_signal_type;

  if (type == ZPortType::Z_PORT_TYPE_EVENT)
    {
      for (int i = 0; i < pl->num_out_ports; i++)
        {
          Port * out_port = pl->out_ports[i];
          if (
            out_port->id.type == ZPortType::Z_PORT_TYPE_EVENT
            && ENUM_BITSET_TEST (
              PortIdentifier::Flags2, out_port->id.flags2,
              PortIdentifier::Flags2::SUPPORTS_MIDI)
            && out_port->id.flow == ZPortFlow::Z_PORT_FLOW_OUTPUT)
            {
              port_connect (out_port, ch->midi_out, 1);
            }
        }
    }
  else if (type == ZPortType::Z_PORT_TYPE_AUDIO)
    {
      if (pl->l_out && pl->r_out)
        {
          port_connect (pl->l_out, ch->prefader->stereo_in->l, true);
          port_connect (pl->r_out, ch->prefader->stereo_in->r, true);
        }
    }
}

/**
 * Disconnect the automatic connections from the
 * Plugin to the Channel's prefader (if last
 * Plugin).
 */
void
plugin_disconnect_from_prefader (Plugin * pl, Channel * ch)
{
  int       i;
  Port *    out_port;
  Track *   track = channel_get_track (ch);
  ZPortType type = track->out_signal_type;

  for (i = 0; i < pl->num_out_ports; i++)
    {
      out_port = pl->out_ports[i];
      if (
        type == ZPortType::Z_PORT_TYPE_AUDIO
        && out_port->id.type == ZPortType::Z_PORT_TYPE_AUDIO)
        {
          if (ports_connected (out_port, ch->prefader->stereo_in->l))
            port_disconnect (out_port, ch->prefader->stereo_in->l);
          if (ports_connected (out_port, ch->prefader->stereo_in->r))
            port_disconnect (out_port, ch->prefader->stereo_in->r);
        }
      else if (
        type == ZPortType::Z_PORT_TYPE_EVENT
        && out_port->id.type == ZPortType::Z_PORT_TYPE_EVENT
        && ENUM_BITSET_TEST (
          PortIdentifier::Flags2, out_port->id.flags2,
          PortIdentifier::Flags2::SUPPORTS_MIDI))
        {
          if (ports_connected (out_port, ch->prefader->midi_in))
            port_disconnect (out_port, ch->prefader->midi_in);
        }
    }
}

/**
 * Disconnect the automatic connections from the
 * given source Plugin to the given destination
 * Plugin.
 */
void
plugin_disconnect_from_plugin (Plugin * src, Plugin * dest)
{
  int   i, j, last_index, num_ports_to_connect;
  Port *in_port, *out_port;

  /* get actual port counts */
  int num_src_audio_outs = 0;
  for (i = 0; i < src->num_out_ports; i++)
    {
      Port * port = src->out_ports[i];
      if (port->id.type == ZPortType::Z_PORT_TYPE_AUDIO)
        num_src_audio_outs++;
    }

  int num_dest_audio_ins = 0;
  for (i = 0; i < dest->num_in_ports; i++)
    {
      Port * port = dest->in_ports[i];
      if (port->id.type == ZPortType::Z_PORT_TYPE_AUDIO)
        num_dest_audio_ins++;
    }

  if (num_src_audio_outs == 1 && num_dest_audio_ins == 1)
    {
      last_index = 0;
      for (i = 0; i < src->num_out_ports; i++)
        {
          out_port = src->out_ports[i];

          if (out_port->id.type == ZPortType::Z_PORT_TYPE_AUDIO)
            {
              for (j = 0; j < dest->num_in_ports; j++)
                {
                  in_port = dest->in_ports[j];

                  if (in_port->id.type == ZPortType::Z_PORT_TYPE_AUDIO)
                    {
                      port_disconnect (out_port, in_port);
                      goto done1;
                    }
                }
            }
        }
done1:;
    }
  else if (num_src_audio_outs == 1 && num_dest_audio_ins > 1)
    {
      /* plugin is mono and next plugin is
       * not mono, so disconnect the mono out from
       * each input */
      for (i = 0; i < src->num_out_ports; i++)
        {
          out_port = src->out_ports[i];

          if (out_port->id.type == ZPortType::Z_PORT_TYPE_AUDIO)
            {
              for (j = 0; j < dest->num_in_ports; j++)
                {
                  in_port = dest->in_ports[j];

                  if (in_port->id.type == ZPortType::Z_PORT_TYPE_AUDIO)
                    {
                      port_disconnect (out_port, in_port);
                    }
                }
              break;
            }
        }
    }
  else if (num_src_audio_outs > 1 && num_dest_audio_ins == 1)
    {
      /* disconnect multi-output channel from mono
       * by disconnecting to the first input channel
       * found */
      for (i = 0; i < dest->num_in_ports; i++)
        {
          in_port = dest->in_ports[i];

          if (in_port->id.type == ZPortType::Z_PORT_TYPE_AUDIO)
            {
              for (j = 0; j < src->num_out_ports; j++)
                {
                  out_port = src->out_ports[j];

                  if (out_port->id.type == ZPortType::Z_PORT_TYPE_AUDIO)
                    {
                      port_disconnect (out_port, in_port);
                      goto done2;
                    }
                }
              break;
            }
        }
done2:;
    }
  else if (num_src_audio_outs > 1 && num_dest_audio_ins > 1)
    {
      /* connect to as many audio outs this
       * plugin has, or until we can't connect
       * anymore */
      num_ports_to_connect = MIN (num_src_audio_outs, num_dest_audio_ins);
      last_index = 0;
      int ports_disconnected = 0;
      for (i = 0; i < src->num_out_ports; i++)
        {
          out_port = src->out_ports[i];

          if (out_port->id.type == ZPortType::Z_PORT_TYPE_AUDIO)
            {
              for (; last_index < dest->num_in_ports; last_index++)
                {
                  in_port = dest->in_ports[last_index];
                  if (in_port->id.type == ZPortType::Z_PORT_TYPE_AUDIO)
                    {
                      port_disconnect (out_port, in_port);
                      last_index++;
                      ports_disconnected++;
                      break;
                    }
                }
              if (ports_disconnected == num_ports_to_connect)
                break;
            }
        }
    }

  /* disconnect MIDI connections */
  for (i = 0; i < src->num_out_ports; i++)
    {
      out_port = src->out_ports[i];

      if (
        out_port->id.type == ZPortType::Z_PORT_TYPE_EVENT
        && ENUM_BITSET_TEST (
          PortIdentifier::Flags2, out_port->id.flags2,
          PortIdentifier::Flags2::SUPPORTS_MIDI))
        {
          for (j = 0; j < dest->num_in_ports; j++)
            {
              in_port = dest->in_ports[j];

              if (
                in_port->id.type == ZPortType::Z_PORT_TYPE_EVENT
                && ENUM_BITSET_TEST (
                  PortIdentifier::Flags2, in_port->id.flags2,
                  PortIdentifier::Flags2::SUPPORTS_MIDI))
                {
                  port_disconnect (out_port, in_port);
                }
            }
        }
    }
}

/**
 * To be called immediately when a channel or plugin is deleted.
 *
 * A call to plugin_free can be made at any point later just to free the
 * resources.
 */
void
plugin_disconnect (Plugin * self)
{
  g_message ("disconnecting plugin %s...", self->setting->descr->name);

  self->deleting = 1;

  if (plugin_is_in_active_project (self))
    {
      if (self->visible && ZRYTHM_HAVE_UI)
        plugin_close_ui (self);

      /* disconnect all ports */
      ports_disconnect (self->in_ports, self->num_in_ports, true);
      ports_disconnect (self->out_ports, self->num_out_ports, true);
      g_message (
        "%s: DISCONNECTED ALL PORTS OF %s %d %d", __func__,
        self->setting->descr->name, self->num_in_ports, self->num_out_ports);

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
        "disconnect",
        self->setting->descr->name);

      self->visible = false;
    }

  g_message ("finished disconnecting plugin %s", self->setting->descr->name);
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
plugin_delete_state_files (Plugin * self)
{
  g_message (
    "deleting state files for plugin %s (%s)", self->setting->descr->name,
    self->state_dir);

  g_return_if_fail (g_path_is_absolute (self->state_dir));

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
plugin_expose_ports (Plugin * pl, bool expose, bool inputs, bool outputs)
{
  if (inputs)
    {
      for (int i = 0; i < pl->num_in_ports; i++)
        {
          Port * port = pl->in_ports[i];

          bool is_exposed = port_is_exposed_to_backend (port);
          if (expose && !is_exposed)
            {
              port_set_expose_to_backend (port, true);
            }
          else if (!expose && is_exposed)
            {
              port_set_expose_to_backend (port, false);
            }
        }
    }
  if (outputs)
    {
      for (int i = 0; i < pl->num_out_ports; i++)
        {
          Port * port = pl->out_ports[i];

          bool is_exposed = port_is_exposed_to_backend (port);
          if (expose && !is_exposed)
            {
              port_set_expose_to_backend (port, true);
            }
          else if (!expose && is_exposed)
            {
              port_set_expose_to_backend (port, false);
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
plugin_get_port_by_symbol (Plugin * pl, const char * sym)
{
  g_return_val_if_fail (
    IS_PLUGIN (pl)
      && pl->setting->descr->protocol == ZPluginProtocol::Z_PLUGIN_PROTOCOL_LV2,
    NULL);

  for (int i = 0; i < pl->num_in_ports; i++)
    {
      Port * port = pl->in_ports[i];
      g_return_val_if_fail (IS_PORT_AND_NONNULL (port), NULL);

      if (string_is_equal (port->id.sym, sym))
        {
          return port;
        }
    }
  for (int i = 0; i < pl->num_out_ports; i++)
    {
      Port * port = pl->out_ports[i];
      g_return_val_if_fail (IS_PORT_AND_NONNULL (port), NULL);

      if (string_is_equal (port->id.sym, sym))
        {
          return port;
        }
    }

  g_warning ("failed to find port with symbol %s", sym);
  return NULL;
}

/**
 * Frees given plugin, frees its ports
 * and other internal pointers
 */
void
plugin_free (Plugin * self)
{
  g_return_if_fail (IS_PLUGIN (self));

  g_return_if_fail (!self->visible);

  g_debug ("freeing plugin %s", self->setting->descr->name);

#ifdef HAVE_CARLA
  object_free_w_func_and_null (carla_native_plugin_free, self->carla);
#endif

  for (int i = 0; i < self->num_in_ports; i++)
    {
      Port * port = self->in_ports[i];
      object_free_w_func_and_null (port_free, port);
    }
  for (int i = 0; i < self->num_out_ports; i++)
    {
      Port * port = self->out_ports[i];
      object_free_w_func_and_null (port_free, port);
    }

  object_free_w_func_and_null (g_ptr_array_unref, self->ctrl_in_ports);
  object_free_w_func_and_null (g_ptr_array_unref, self->audio_in_ports);
  object_free_w_func_and_null (g_ptr_array_unref, self->cv_in_ports);
  object_free_w_func_and_null (g_ptr_array_unref, self->midi_in_ports);

  object_zero_and_free (self);
}
