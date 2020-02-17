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

#include "config.h"

#include "audio/automatable.h"
#include "audio/port.h"
#ifdef HAVE_CARLA
#include "plugins/carla_native_plugin.h"
#endif
#include "plugins/lv2_plugin.h"
#include "plugins/plugin_descriptor.h"
#include "plugins/plugin_identifier.h"
#include "plugins/vst_plugin.h"
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
 * The base plugin
 * Inheriting plugins must have this as a child
 */
typedef struct Plugin
{
  PluginIdentifier     id;

  /**
   * Pointer back to plugin in its original format.
   */
  Lv2Plugin *          lv2;

  /** VST plugin. */
  VstPlugin *          vst;

#ifdef HAVE_CARLA
  /** Pointer to Carla native plugin. */
  CarlaNativePlugin *  carla;
#endif

  /** Descriptor. */
  PluginDescriptor *   descr;

  /** Ports coming in as input. */
  Port **             in_ports;
  int                 num_in_ports;
  size_t              in_ports_size;

  /** Outgoing ports. */
  Port **             out_ports;
  int                 num_out_ports;
  size_t              out_ports_size;

  /** Control for plugin enabled. */
  Port *              enabled;

  /** Whether plugin UI is opened or not. */
  int                 visible;

  /** The latency in samples. */
  nframes_t           latency;

  /**
   * UI has been instantiated or not.
   *
   * When instantiating a plugin UI, if it takes
   * too long there is a UI buffer overflow because
   * UI updates are sent in lv2_plugin_process.
   *
   * This should be set to 0 until the plugin UI
   * has finished instantiating, and if this is 0
   * then no UI updates should be sent to the
   * plugin.
   */
  int                  ui_instantiated;

  /** Update frequency of the UI, in Hz (times
   * per second). */
  float                ui_update_hz;

  /** Plugin is in deletion. */
  int                  deleting;

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
  GtkWindow *          window;

  /** The GdkWindow of this widget should be
   * somewhere inside \ref Plugin.window and will
   * be used for wrapping plugin UIs in. */
  GtkEventBox *        ev_box;

  /** Vbox containing the above ev_box for wrapping,
   * or used for packing generic UI controls. */
  GtkBox *             vbox;

  /** ID of the delete-event signal for \ref
   * Plugin.window so that we can
   * deactivate before freeing the plugin. */
  gulong             delete_event_id;

#ifdef _WOE32
  /** A black decoration-less window that follows
   * the wrapped plugin's window on Windows. */
  GtkWindow *          black_window;

  /**
   * Set to 1 to avoid recursive signals
   * being fired.
   *
   * See plugin_gtk.c
   */
  int                black_window_shown_manually;
#endif

  int                 magic;
} Plugin;

static const cyaml_schema_field_t
plugin_fields_schema[] =
{
  CYAML_FIELD_MAPPING (
    "id", CYAML_FLAG_DEFAULT,
    Plugin, id,
    plugin_identifier_fields_schema),
  CYAML_FIELD_MAPPING_PTR (
    "descr", CYAML_FLAG_POINTER,
    Plugin, descr,
    plugin_descriptor_fields_schema),
  CYAML_FIELD_MAPPING_PTR (
    "lv2", CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    Plugin, lv2,
    lv2_plugin_fields_schema),
  CYAML_FIELD_MAPPING_PTR (
    "vst", CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    Plugin, vst,
    vst_plugin_fields_schema),
  CYAML_FIELD_SEQUENCE_COUNT (
    "in_ports",
    CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    Plugin, in_ports, num_in_ports,
    &port_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "out_ports",
    CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    Plugin, out_ports, num_out_ports,
    &port_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_MAPPING_PTR (
    "enabled",
    CYAML_FLAG_POINTER,
    Plugin, enabled, port_fields_schema),
  CYAML_FIELD_INT (
    "visible", CYAML_FLAG_DEFAULT,
    Plugin, visible),

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
  Plugin * self);

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
  const PluginDescriptor * descr,
  int                      track_pos,
  int                      slot);

/**
 * Create a dummy plugin for tests.
 */
Plugin *
plugin_new_dummy (
  PluginCategory cat,
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
 */
void
plugin_remove_ats_from_automation_tracklist (
  Plugin * pl);

/**
 * Clones the given plugin.
 */
Plugin *
plugin_clone (
  Plugin * pl);

/**
 * Sets the channel and slot on the plugin and
 * its ports.
 */
void
plugin_set_channel_and_slot (
  Plugin *  pl,
  Channel * ch,
  int       slot);

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
  Plugin *  pl,
  Channel * prev_ch,
  Channel * ch,
  int       new_slot);

/**
 * Returns if the Plugin has a supported custom
 * UI.
 */
int
plugin_has_supported_custom_ui (
  Plugin * self);

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
 * Loads the plugin from its state file.
 */
//void
//plugin_load (Plugin * plugin);

/**
 * Instantiates the plugin (e.g. when adding to a
 * channel)
 */
int
plugin_instantiate (Plugin * plugin);

/**
 * Sets the track and track_pos on the plugin.
 */
void
plugin_set_track (
  Plugin * pl,
  Track * tr);

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
