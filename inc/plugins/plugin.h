// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Base plugin.
 */

#ifndef __PLUGINS_BASE_PLUGIN_H__
#define __PLUGINS_BASE_PLUGIN_H__

#include "zrythm-config.h"

#include "audio/port.h"
#include "plugins/plugin_descriptor.h"
#include "plugins/plugin_identifier.h"
#include "plugins/plugin_preset.h"
#include "settings/plugin_settings.h"
#include "utils/types.h"

/* pulled in from X11 */
#undef Bool

typedef struct Project           Project;
typedef struct Channel           Channel;
typedef struct AutomationTrack   AutomationTrack;
typedef struct _ModulatorWidget  ModulatorWidget;
typedef struct Lv2Plugin         Lv2Plugin;
typedef struct CarlaNativePlugin CarlaNativePlugin;
typedef struct MixerSelections   MixerSelections;
typedef struct _WrappedObjectWithChangeSignal
  WrappedObjectWithChangeSignal;

/**
 * @addtogroup plugins
 *
 * @{
 */

#define PLUGIN_SCHEMA_VERSION 1

#define PLUGIN_MAGIC 43198683
#define IS_PLUGIN(x) (((Plugin *) x)->magic == PLUGIN_MAGIC)
#define IS_PLUGIN_AND_NONNULL(x) (x && IS_PLUGIN (x))

/**
 * Plugin UI refresh rate limits.
 */
#define PLUGIN_MIN_REFRESH_RATE 30.f
#define PLUGIN_MAX_REFRESH_RATE 121.f

/**
 * Plugin UI scale factor limits.
 */
#define PLUGIN_MIN_SCALE_FACTOR 0.5f
#define PLUGIN_MAX_SCALE_FACTOR 4.f

#define plugin_is_in_active_project(self) \
  (self->track && track_is_in_active_project (self->track))

/** Whether the plugin is used for MIDI
 * auditioning in SampleProcessor. */
#define plugin_is_auditioner(self) \
  (self->track && track_is_auditioner (self->track))

/**
 * The base plugin
 * Inheriting plugins must have this as a child
 */
typedef struct Plugin
{
  int schema_version;

  PluginIdentifier id;

  /**
   * Pointer back to plugin in its original format.
   */
  Lv2Plugin * lv2;

  /** Pointer to Carla native plugin. */
  CarlaNativePlugin * carla;

  /** Setting this plugin was instantiated with. */
  PluginSetting * setting;

  /** Ports coming in as input. */
  Port ** in_ports;
  int     num_in_ports;
  size_t  in_ports_size;

  /* caches */
  GPtrArray * ctrl_in_ports;
  GPtrArray * audio_in_ports;
  GPtrArray * cv_in_ports;
  GPtrArray * midi_in_ports;

  /** Cache. */
  Port * midi_in_port;

  /** Outgoing ports. */
  Port ** out_ports;
  int     num_out_ports;
  size_t  out_ports_size;

  /**
   * Ports at their lilv indices.
   *
   * These point to ports in \ref Plugin.in_ports and
   * \ref Plugin.out_ports so should not be freed.
   */
  Port ** lilv_ports;
  int     num_lilv_ports;

  /**
   * Control for plugin enabled, for convenience.
   *
   * This port is already in \ref Plugin.in_ports.
   */
  Port * enabled;

  /**
   * Whether the plugin has a custom "enabled" port
   * (LV2).
   *
   * If true, bypass logic will be delegated to
   * the plugin.
   */
  Port * own_enabled_port;

  /**
   * Control for plugin gain, for convenience.
   *
   * This port is already in \ref Plugin.in_ports.
   */
  Port * gain;

  /**
   * Instrument left stereo output, for convenience.
   *
   * This port is already in \ref Plugin.out_ports
   * if instrument.
   */
  Port * l_out;
  Port * r_out;

  PluginBank ** banks;
  int           num_banks;
  size_t        banks_size;

  PluginPresetIdentifier selected_bank;
  PluginPresetIdentifier selected_preset;

  /** Whether plugin UI is opened or not. */
  bool visible;

  /** Latency reported by the Lv2Plugin, if any,
   * in samples. */
  nframes_t latency;

  /** Whether the plugin is currently instantiated
   * or not. */
  bool instantiated;

  /** Set to true if instantiation failed and the
   * plugin will be treated as disabled. */
  bool instantiation_failed;

  /** Whether the plugin is currently activated
   * or not. */
  bool activated;

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
  int ui_instantiated;

  /** Update frequency of the UI, in Hz (times
   * per second). */
  float ui_update_hz;

  /** Scale factor for drawing UIs in scale of the
   * monitor. */
  float ui_scale_factor;

  /**
   * State directory (only basename).
   *
   * Used for saving/loading the state.
   *
   * @note This is only the directory basename and
   *   should go in project/plugins/states.
   */
  char * state_dir;

  /** Whether the plugin is currently being
   * deleted. */
  bool deleting;

  /** Active preset item, if wrapped or generic
   * UI. */
  GtkWidget * active_preset_item;

  /**
   * The Plugin's window.
   *
   * This is used for both generic UIs and for
   * X11/Windows when plugins are wrapped.
   *
   * LV2 plugin UIs are only not wrapped when they
   * have external UIs. In that case, this must be
   * NULL.
   */
  GtkWindow * window;

  /** Whether show () has been called on the LV2
   * external UI. */
  bool external_ui_visible;

  /** The GdkWindow of this widget should be
   * somewhere inside \ref Plugin.window and will
   * be used for wrapping plugin UIs in. */
  GtkBox * ev_box;

  /** Vbox containing the above ev_box for wrapping,
   * or used for packing generic UI controls. */
  GtkBox * vbox;

  /** ID of the destroy signal for \ref
   * Plugin.window so that we can
   * deactivate before freeing the plugin. */
  gulong destroy_window_id;

  /** ID of the close-request signal for \ref
   * Plugin.window so that we can
   * deactivate before freeing the plugin. */
  gulong close_request_id;

  int magic;

  /** Modulator widget, if modulator. */
  ModulatorWidget * modulator_widget;

  /**
   * ID of GSource (if > 0).
   *
   * @seealso update_plugin_ui().
   */
  guint update_ui_source_id;

  /** Temporary variable to check if plugin is
   * currently undergoing deactivation. */
  bool deactivating;

  /**
   * Set to true to avoid sending multiple
   * ET_PLUGIN_STATE_CHANGED for the same plugin.
   */
  int state_changed_event_sent;

  /** Whether the plugin is used for functions. */
  bool is_function;

  /** Pointer to owner track, if any. */
  Track * track;

  /** Pointer to owner selections, if any. */
  MixerSelections * ms;

  /** Used in Gtk. */
  WrappedObjectWithChangeSignal * gobj;
} Plugin;

static const cyaml_schema_field_t plugin_fields_schema[] = {
  YAML_FIELD_INT (Plugin, schema_version),
  YAML_FIELD_MAPPING_EMBEDDED (
    Plugin,
    id,
    plugin_identifier_fields_schema),
  YAML_FIELD_MAPPING_PTR (
    Plugin,
    setting,
    plugin_setting_fields_schema),
  YAML_FIELD_DYN_PTR_ARRAY_VAR_COUNT (
    Plugin,
    in_ports,
    port_schema),
  YAML_FIELD_DYN_PTR_ARRAY_VAR_COUNT (
    Plugin,
    out_ports,
    port_schema),
  YAML_FIELD_DYN_PTR_ARRAY_VAR_COUNT_OPT (
    Plugin,
    banks,
    plugin_bank_schema),
  YAML_FIELD_MAPPING_EMBEDDED (
    Plugin,
    selected_bank,
    plugin_preset_identifier_fields_schema),
  YAML_FIELD_MAPPING_EMBEDDED (
    Plugin,
    selected_preset,
    plugin_preset_identifier_fields_schema),
  YAML_FIELD_INT (Plugin, visible),
  YAML_FIELD_STRING_PTR_OPTIONAL (Plugin, state_dir),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t plugin_schema = {
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER_NULL_STR,
    Plugin,
    plugin_fields_schema),
};

NONNULL_ARGS (1)
void
plugin_init_loaded (
  Plugin *          self,
  Track *           track,
  MixerSelections * ms);

/**
 * Adds an AutomationTrack to the Plugin.
 */
NONNULL
void
plugin_add_automation_track (
  Plugin *          self,
  AutomationTrack * at);

/**
 * Adds an in port to the plugin's list.
 */
NONNULL
void
plugin_add_in_port (Plugin * pl, Port * port);

/**
 * Adds an out port to the plugin's list.
 */
NONNULL
void
plugin_add_out_port (Plugin * pl, Port * port);

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
NONNULL_ARGS (1)
Plugin *
plugin_new_from_setting (
  PluginSetting * setting,
  unsigned int    track_name_hash,
  PluginSlotType  slot_type,
  int             slot,
  GError **       error);

/**
 * Create a dummy plugin for tests.
 */
Plugin *
plugin_new_dummy (
  ZPluginCategory cat,
  unsigned int    track_name_hash,
  int             slot);

/**
 * Sets the UI refresh rate on the Plugin.
 */
NONNULL
void
plugin_set_ui_refresh_rate (Plugin * self);

/**
 * Gets the enable/disable port for this plugin.
 */
NONNULL
Port *
plugin_get_enabled_port (Plugin * self);

/**
 * Verifies that the plugin identifiers are valid.
 */
NONNULL
bool
plugin_validate (Plugin * self);

/**
 * Prints the plugin to the buffer, if any, or to
 * the log.
 */
void
plugin_print (Plugin * self, char * buf, size_t buf_sz);

/**
 * Removes the automation tracks associated with
 * this plugin from the automation tracklist in the
 * corresponding track.
 *
 * Used e.g. when moving plugins.
 *
 * @param free_ats Also free the ats.
 */
NONNULL
void
plugin_remove_ats_from_automation_tracklist (
  Plugin * pl,
  bool     free_ats,
  bool     fire_events);

/**
 * Clones the given plugin.
 *
 * @param error To be filled if an error occurred.
 *
 * @return The cloned plugin, or NULL if an error
 *   occurred.
 */
NONNULL_ARGS (1)
Plugin *
plugin_clone (Plugin * src, GError ** error);

void
plugin_get_full_port_group_designation (
  Plugin *     self,
  const char * port_group,
  char *       buf);

NONNULL
Port *
plugin_get_port_in_group (
  Plugin *     self,
  const char * port_group,
  bool         left);

/**
 * Find corresponding port in the same port group
 * (eg, if this is left, find right and vice
 * versa).
 */
NONNULL
Port *
plugin_get_port_in_same_group (Plugin * self, Port * port);

/**
 * Activates or deactivates the plugin.
 *
 * @param activate True to activate, false to
 *   deactivate.
 */
NONNULL
int
plugin_activate (Plugin * pl, bool activate);

/**
 * Moves the plugin to the given slot in
 * the given channel.
 *
 * If a plugin already exists, it deletes it and
 * replaces it.
 */
NONNULL
void
plugin_move (
  Plugin *       pl,
  Track *        track,
  PluginSlotType slot_type,
  int            slot,
  bool           fire_events);

/**
 * Sets the channel and slot on the plugin and
 * its ports.
 */
NONNULL
void
plugin_set_track_and_slot (
  Plugin *       pl,
  unsigned int   track_name_hash,
  PluginSlotType slot_type,
  int            slot);

/**
 * Moves the Plugin's automation from one Channel
 * to another.
 */
NONNULL
void
plugin_move_automation (
  Plugin *       pl,
  Track *        prev_track,
  Track *        track,
  PluginSlotType new_slot_type,
  int            new_slot);

NONNULL
void
plugin_append_ports (Plugin * self, GPtrArray * ports);

/**
 * Exposes or unexposes plugin ports to the backend.
 *
 * @param expose Expose or not.
 * @param inputs Expose/unexpose inputs.
 * @param outputs Expose/unexpose outputs.
 */
NONNULL
void
plugin_expose_ports (
  Plugin * pl,
  bool     expose,
  bool     inputs,
  bool     outputs);

/**
 * Gets a port by its symbol.
 *
 * Only works for LV2 plugins.
 */
NONNULL
Port *
plugin_get_port_by_symbol (Plugin * pl, const char * sym);

/**
 * Gets a port by its param URI.
 *
 * Only works for LV2 plugins.
 */
NONNULL
Port *
plugin_get_port_by_param_uri (Plugin * pl, const char * uri);

/**
 * Returns the escaped name of the plugin.
 */
NONNULL
MALLOC
char *
plugin_get_escaped_name (Plugin * pl);

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
  const char * abs_state_dir);

/**
 * Returns the state dir as an absolute path.
 */
NONNULL
MALLOC
char *
plugin_get_abs_state_dir (Plugin * self, bool is_backup);

/**
 * Ensures the state dir exists or creates it.
 */
NONNULL
void
plugin_ensure_state_dir (Plugin * self, bool is_backup);

/**
 * Returns all plugins in the current project.
 */
NONNULL
void
plugin_get_all (
  Project *   prj,
  GPtrArray * arr,
  bool        check_undo_manager);

NONNULL
Channel *
plugin_get_channel (Plugin * self);

NONNULL
Track *
plugin_get_track (Plugin * self);

NONNULL
Plugin *
plugin_find (PluginIdentifier * id);

/**
 * To be called when changes to the plugin
 * identifier were made, so we can update all
 * children recursively.
 */
NONNULL
void
plugin_update_identifier (Plugin * self);

/**
 * Updates the plugin's latency.
 */
NONNULL
void
plugin_update_latency (Plugin * pl);

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
NONNULL
void
plugin_generate_automation_tracks (
  Plugin * plugin,
  Track *  track);

/**
 * Prepare plugin for processing.
 */
HOT NONNULL OPTIMIZE_O3 void
plugin_prepare_process (Plugin * self);

/**
 * Instantiates the plugin (e.g. when adding to a
 * channel)
 */
NONNULL_ARGS (1)
int
plugin_instantiate (
  Plugin *    self,
  LilvState * state,
  GError **   error);

/**
 * Sets the track name hash on the plugin.
 */
NONNULL
void
plugin_set_track_name_hash (
  Plugin *     pl,
  unsigned int track_name_hash);

/**
 * Process plugin.
 */
NONNULL
HOT void
plugin_process (
  Plugin *                            plugin,
  const EngineProcessTimeInfo * const time_nfo);

NONNULL
MALLOC
char *
plugin_generate_window_title (Plugin * plugin);

/**
 * Process show ui
 */
NONNULL
void
plugin_open_ui (Plugin * plugin);

/**
 * Returns if Plugin exists in MixerSelections.
 */
NONNULL
bool
plugin_is_selected (Plugin * pl);

/**
 * Selects the plugin in the MixerSelections.
 *
 * @param select Select or deselect.
 * @param exclusive Whether to make this the only
 *   selected plugin or add it to the selections.
 */
NONNULL
void
plugin_select (Plugin * self, bool select, bool exclusive);

/**
 * Returns whether the plugin is enabled.
 *
 * @param check_track Whether to check if the track
 *   is enabled as well.
 */
NONNULL
bool
plugin_is_enabled (Plugin * self, bool check_track);

NONNULL
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
HOT NONNULL void
plugin_process_passthrough (
  Plugin *                            self,
  const EngineProcessTimeInfo * const time_nfo);

/**
 * Returns the event ports in the plugin.
 *
 * @param ports Array to fill in. Must be large
 *   enough.
 *
 * @return The number of ports in the array.
 */
NONNULL
int
plugin_get_event_ports (Plugin * pl, Port ** ports, int input);

/**
 * Process hide ui
 */
NONNULL
void
plugin_close_ui (Plugin * plugin);

/**
 * (re)Generates automatables for the plugin.
 */
NONNULL
void
plugin_update_automatables (Plugin * plugin);

PluginBank *
plugin_add_bank_if_not_exists (
  Plugin *     self,
  const char * uri,
  const char * name);

NONNULL
void
plugin_add_preset_to_bank (
  Plugin *       self,
  PluginBank *   bank,
  PluginPreset * preset);

NONNULL
void
plugin_set_selected_bank_from_index (Plugin * self, int idx);

NONNULL
void
plugin_set_selected_preset_from_index (Plugin * self, int idx);

NONNULL
void
plugin_set_selected_preset_by_name (
  Plugin *     self,
  const char * name);

/**
 * Sets caches for processing.
 */
NONNULL
void
plugin_set_caches (Plugin * self);

/**
 * Connect the output Ports of the given source
 * Plugin to the input Ports of the given
 * destination Plugin.
 *
 * Used when automatically connecting a Plugin
 * in the Channel strip to the next Plugin.
 */
NONNULL
void
plugin_connect_to_plugin (Plugin * src, Plugin * dest);

/**
 * Disconnect the automatic connections from the
 * given source Plugin to the given destination
 * Plugin.
 */
NONNULL
void
plugin_disconnect_from_plugin (Plugin * src, Plugin * dest);

/**
 * Connects the Plugin's output Port's to the
 * input Port's of the given Channel's prefader.
 *
 * Used when doing automatic connections.
 */
NONNULL
void
plugin_connect_to_prefader (Plugin * pl, Channel * ch);

/**
 * Disconnect the automatic connections from the
 * Plugin to the Channel's prefader (if last
 * Plugin).
 */
NONNULL
void
plugin_disconnect_from_prefader (Plugin * pl, Channel * ch);

/**
 * To be called immediately when a channel or plugin
 * is deleted.
 *
 * A call to plugin_free can be made at any point
 * later just to free the resources.
 */
NONNULL
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
NONNULL
void
plugin_delete_state_files (Plugin * self);

/**
 * Cleans up an instantiated but not activated
 * plugin.
 */
NONNULL
int
plugin_cleanup (Plugin * self);

/**
 * Frees given plugin, breaks all its port connections, and frees its ports
 * and other internal pointers
 */
NONNULL
void
plugin_free (Plugin * plugin);

/**
 * @}
 */

#endif
