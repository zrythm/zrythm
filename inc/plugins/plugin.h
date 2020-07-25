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
 * Base plugin.
 */

#ifndef __PLUGINS_BASE_PLUGIN_H__
#define __PLUGINS_BASE_PLUGIN_H__

#include "zrythm-config.h"

#include "audio/port.h"
#include "plugins/carla_native_plugin.h"
#include "plugins/lv2_plugin.h"
#include "plugins/plugin_descriptor.h"
#include "plugins/plugin_identifier.h"
#include "plugins/plugin_preset.h"
#include "utils/types.h"

/* pulled in from X11 */
#undef Bool

typedef struct Channel Channel;
typedef struct VstPlugin VstPlugin;
typedef struct AutomationTrack AutomationTrack;

/**
 * @addtogroup plugins
 *
 * @{
 */

#define PLUGIN_MAGIC 43198683
#define IS_PLUGIN(tr) \
  (tr && tr->magic == PLUGIN_MAGIC)

/**
 * Plugin UI refresh rate limits.
 */
#define PLUGIN_MIN_REFRESH_RATE 30.f
#define PLUGIN_MAX_REFRESH_RATE 121.f

/**
 * The base plugin
 * Inheriting plugins must have this as a child
 */
typedef struct Plugin
{
  PluginIdentifier  id;

  /**
   * Pointer back to plugin in its original format.
   */
  Lv2Plugin *       lv2;

  /** Pointer to Carla native plugin. */
  CarlaNativePlugin * carla;

  /** Descriptor. */
  PluginDescriptor * descr;

  /** Ports coming in as input. */
  Port **           in_ports;
  int               num_in_ports;
  size_t            in_ports_size;

  /** Outgoing ports. */
  Port **           out_ports;
  int               num_out_ports;
  size_t            out_ports_size;

  /** Control for plugin enabled. */
  Port *            enabled;

  PluginBank **     banks;
  int               num_banks;
  size_t            banks_size;

  PluginPresetIdentifier selected_bank;
  PluginPresetIdentifier selected_preset;

  /** Whether plugin UI is opened or not. */
  bool              visible;

  /** Latency reported by the Lv2Plugin, if any,
   * in samples. */
  nframes_t         latency;

  /** Whether the plugin is currently instantiated
   * or not. */
  bool              instantiated;

  /** Whether the plugin is currently activated
   * or not. */
  bool              activated;

  /**
   * Whether the UI has finished instantiating.
   *
   * When instantiating a plugin UI, if it takes
   * too long there is a UI buffer overflow because
   * UI updates are sent in lv2_plugin_process.
   *
   * This should be set to false until the plugin UI
   * has finished instantiating, and if this is false
   * then no UI updates should be sent to the
   * plugin.
   */
  int               ui_instantiated;

  /** Update frequency of the UI, in Hz (times
   * per second). */
  float             ui_update_hz;

  /**
   * State directory (only basename).
   *
   * Used for saving/loading the state.
   *
   * @note This is only the directory basename and
   *   should go in project/plugins/states.
   */
  char *            state_dir;

  /** Whether the plugin is currently being
   * deleted. */
  bool              deleting;

  /** Active preset item, if wrapped or generic
   * UI. */
  GtkCheckMenuItem* active_preset_item;

  /**
   * The Plugin's window.
   *
   * This is used for both generic UIs and for
   * X11/Windows when plugins are wrapped.
   *
   * All VST plugin UIs are wrapped.
   *
   * LV2 plugin UIs
   * are only not wrapped when they have external
   * UIs. In that case, this must be NULL.
   */
  GtkWindow *       window;

  /** The GdkWindow of this widget should be
   * somewhere inside \ref Plugin.window and will
   * be used for wrapping plugin UIs in. */
  GtkEventBox *     ev_box;

  /** Vbox containing the above ev_box for wrapping,
   * or used for packing generic UI controls. */
  GtkBox *          vbox;

  /** ID of the delete-event signal for \ref
   * Plugin.window so that we can
   * deactivate before freeing the plugin. */
  gulong            delete_event_id;

  int               magic;

  bool              is_project;
} Plugin;

static const cyaml_schema_field_t
plugin_fields_schema[] =
{
  YAML_FIELD_MAPPING_EMBEDDED (
    Plugin, id,
    plugin_identifier_fields_schema),
  YAML_FIELD_MAPPING_PTR (
    Plugin, descr,
    plugin_descriptor_fields_schema),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    Plugin, lv2,
    lv2_plugin_fields_schema),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    Plugin, carla,
    carla_native_plugin_fields_schema),
  YAML_FIELD_DYN_PTR_ARRAY_VAR_COUNT (
    Plugin, in_ports, port_schema),
  YAML_FIELD_DYN_PTR_ARRAY_VAR_COUNT (
    Plugin, out_ports, port_schema),
  YAML_FIELD_DYN_PTR_ARRAY_VAR_COUNT_OPT (
    Plugin, banks, plugin_bank_schema),
  YAML_FIELD_MAPPING_EMBEDDED (
    Plugin, selected_bank,
    plugin_preset_identifier_fields_schema),
  YAML_FIELD_MAPPING_EMBEDDED (
    Plugin, selected_preset,
    plugin_preset_identifier_fields_schema),
  YAML_FIELD_MAPPING_PTR (
    Plugin, enabled, port_fields_schema),
  YAML_FIELD_INT (
    Plugin, visible),
  YAML_FIELD_STRING_PTR_OPTIONAL (
    Plugin, state_dir),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
  plugin_schema =
{
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER_NULL_STR,
    Plugin, plugin_fields_schema),
};

void
plugin_init_loaded (
  Plugin * self,
  bool     project);

/**
 * Adds an AutomationTrack to the Plugin.
 */
void
plugin_add_automation_track (
  Plugin * self,
  AutomationTrack * at);

/**
 * Adds an in port to the plugin's list.
 */
void
plugin_add_in_port (
  Plugin * pl,
  Port *   port);

/**
 * Adds an out port to the plugin's list.
 */
void
plugin_add_out_port (
  Plugin * pl,
  Port *   port);

/**
 * Creates/initializes a plugin and its internal
 * plugin (LV2, etc.)
 * using the given descriptor.
 *
 * @param track_pos The expected position of the
 *   track the plugin will be in.
 * @param slot The expected slot the plugin will
 *   be in.
 */
Plugin *
plugin_new_from_descr (
  PluginDescriptor * descr,
  int                      track_pos,
  int                      slot);

/**
 * Create a dummy plugin for tests.
 */
Plugin *
plugin_new_dummy (
  ZPluginCategory cat,
  int            track_pos,
  int            slot);

/**
 * Sets the UI refresh rate on the Plugin.
 */
void
plugin_set_ui_refresh_rate (
  Plugin * self);

/**
 * Gets the enable/disable port for this plugin.
 */
Port *
plugin_get_enabled_port (
  Plugin * self);

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
  bool     fire_events);

/**
 * Clones the given plugin.
 *
 * @bool src_is_project Whether \ref pl is a project
 *   plugin.
 */
Plugin *
plugin_clone (
  Plugin * pl,
  bool     src_is_project);

/**
 * Activates or deactivates the plugin.
 *
 * @param activate True to activate, false to
 *   deactivate.
 */
int
plugin_activate (
  Plugin * pl,
  bool     activate);

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
  int            slot);

/**
 * Sets the channel and slot on the plugin and
 * its ports.
 */
void
plugin_set_channel_and_slot (
  Plugin *       pl,
  Channel *      ch,
  PluginSlotType slot_type,
  int            slot);

/**
 * Returns if the Plugin is an instrument or not.
 */
int
plugin_descriptor_is_instrument (
  const PluginDescriptor * descr);

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
  int            new_slot);

void
plugin_set_is_project (
  Plugin * self,
  bool     is_project);

void
plugin_append_ports (
  Plugin *  pl,
  Port ***  ports,
  int *     max_size,
  bool      is_dynamic,
  int *     size);

/**
 * Returns the escaped name of the plugin.
 */
char *
plugin_get_escaped_name (
  Plugin * pl);

/**
 * Returns the state dir as an absolute path.
 */
char *
plugin_get_abs_state_dir (
  Plugin * self,
  bool     is_backup);

/**
 * Ensures the state dir exists or creates it.
 */
void
plugin_ensure_state_dir (
  Plugin * self,
  bool     is_backup);

Channel *
plugin_get_channel (
  Plugin * self);

Track *
plugin_get_track (
  Plugin * self);

Plugin *
plugin_find (
  PluginIdentifier * id);

/**
 * To be called when changes to the plugin
 * identifier were made, so we can update all
 * children recursively.
 */
void
plugin_update_identifier (
  Plugin * self);

/**
 * Updates the plugin's latency.
 */
void
plugin_update_latency (
  Plugin * pl);

/**
 * Generates automatables for the plugin.
 *
 * The plugin must be instantiated already.
 *
 * @param track The Track this plugin belongs to.
 *   This is passed because the track might not be
 *   in the project yet so we can't fetch it
 *   through indices.
 */
void
plugin_generate_automation_tracks (
  Plugin * plugin,
  Track *  track);

/**
 * Prepare plugin for processing.
 */
void
plugin_prepare_process (
  Plugin * self);

/**
 * Loads the plugin from its state file.
 */
//void
//plugin_load (Plugin * plugin);

/**
 * Instantiates the plugin (e.g. when adding to a
 * channel)
 *
 * @param project Whether this is a project plugin
 *   (as opposed to a clone used in actions).
 */
int
plugin_instantiate (
  Plugin *    self,
  bool        project,
  LilvState * state);

/**
 * Sets the track and track_pos on the plugin.
 */
void
plugin_set_track_pos (
  Plugin * pl,
  int      pos);

/**
 * Process plugin.
 *
 * @param g_start_frames The global start frames.
 * @param nframes The number of frames to process.
 */
void
plugin_process (
  Plugin *    plugin,
  const long  g_start_frames,
  const nframes_t  local_offset,
  const nframes_t   nframes);

char *
plugin_generate_window_title (
  Plugin * plugin);

/**
 * Process show ui
 */
void
plugin_open_ui (
  Plugin *plugin);

/**
 * Returns if Plugin exists in MixerSelections.
 */
int
plugin_is_selected (
  Plugin * pl);

/**
 * Returns whether the plugin is enabled.
 */
bool
plugin_is_enabled (
  Plugin * self);

void
plugin_set_enabled (
  Plugin * self,
  bool     enabled,
  bool     fire_events);

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
  const nframes_t nframes);

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
  int      input);

/**
 * Process hide ui
 */
void
plugin_close_ui (Plugin *plugin);

/**
 * (re)Generates automatables for the plugin.
 */
void
plugin_update_automatables (Plugin * plugin);

PluginBank *
plugin_add_bank_if_not_exists (
  Plugin * self,
  const char * uri,
  const char * name);

void
plugin_add_preset_to_bank (
  Plugin *       self,
  PluginBank *   bank,
  PluginPreset * preset);

void
plugin_set_selected_bank_from_index (
  Plugin * self,
  int      idx);

void
plugin_set_selected_preset_from_index (
  Plugin * self,
  int      idx);

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
  Plugin * dest);

/**
 * Disconnect the automatic connections from the
 * given source Plugin to the given destination
 * Plugin.
 */
void
plugin_disconnect_from_plugin (
  Plugin * src,
  Plugin * dest);

/**
 * Connects the Plugin's output Port's to the
 * input Port's of the given Channel's prefader.
 *
 * Used when doing automatic connections.
 */
void
plugin_connect_to_prefader (
  Plugin *  pl,
  Channel * ch);

/**
 * Disconnect the automatic connections from the
 * Plugin to the Channel's prefader (if last
 * Plugin).
 */
void
plugin_disconnect_from_prefader (
  Plugin *  pl,
  Channel * ch);

/**
 * To be called immediately when a channel or plugin
 * is deleted.
 *
 * A call to plugin_free can be made at any point
 * later just to free the resources.
 */
void
plugin_disconnect (Plugin * plugin);

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
  Plugin * self);

/**
 * Cleans up an instantiated but not activated
 * plugin.
 */
int
plugin_cleanup (
  Plugin * self);

/**
 * Frees given plugin, breaks all its port connections, and frees its ports
 * and other internal pointers
 */
void
plugin_free (Plugin *plugin);

/**
 * @}
 */

SERIALIZE_INC (Plugin, plugin);
DESERIALIZE_INC (Plugin, plugin);

#endif
