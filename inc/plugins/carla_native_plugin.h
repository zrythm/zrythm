/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
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
 * Code related to Carla plugins.
 */

#include "zrythm-config.h"

#ifndef __PLUGINS_CARLA_NATIVE_PLUGIN_H__
#define __PLUGINS_CARLA_NATIVE_PLUGIN_H__

#ifdef HAVE_CARLA
#include <CarlaNativePlugin.h>
#include <CarlaUtils.h>
#endif

typedef struct PluginDescriptor PluginDescriptor;
typedef void * CarlaPluginHandle;

/**
 * @addtogroup plugins
 *
 * @{
 */

#define CARLA_STATE_FILENAME "state.carla"

/**
 * The type of the Carla plugin.
 */
typedef enum CarlaPluginType
{
  CARLA_PLUGIN_NONE,
  CARLA_PLUGIN_RACK,
  CARLA_PLUGIN_PATCHBAY,
  CARLA_PLUGIN_PATCHBAY16,
  CARLA_PLUGIN_PATCHBAY32,
  CARLA_PLUGIN_PATCHBAY64,
} CarlaPluginType;

typedef struct CarlaNativePlugin
{
#ifdef HAVE_CARLA
  NativePluginHandle native_plugin_handle;
  NativeHostDescriptor native_host_descriptor;
  const NativePluginDescriptor * native_plugin_descriptor;

  CarlaHostHandle  host_handle;

  //uint32_t                 num_midi_events;
  //NativeMidiEvent          midi_events[200];
  NativeTimeInfo   time_info;
#endif

  /** Pointer back to Plugin. */
  Plugin *         plugin;

  /** Plugin ID inside carla engine. */
  unsigned int     carla_plugin_id;

  /** Whether ports are already created or not. */
  bool             ports_created;

  /** GTK tick callback. */
  guint            tick_cb;

} CarlaNativePlugin;

#ifdef HAVE_CARLA

NONNULL
void
carla_native_plugin_init_loaded (
  CarlaNativePlugin * self);

/**
 * Creates an instance of a CarlaNativePlugin inside
 * the given Plugin.
 *
 * The given Plugin must have its descriptor
 * filled in.
 *
 * @return Non-zero if fail.
 */
NONNULL
int
carla_native_plugin_new_from_setting (
  Plugin * plugin);

/**
 * Returns a filled in descriptor from the
 * CarlaCachedPluginInfo.
 */
NONNULL
PluginDescriptor *
carla_native_plugin_get_descriptor_from_cached (
  const CarlaCachedPluginInfo * info,
  PluginType              type);

/**
 * Saves the state inside the standard state
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
carla_native_plugin_save_state (
  CarlaNativePlugin * self,
  bool                is_backup,
  const char *        abs_state_dir);

/**
 * Loads the state from the given file or from
 * its state file.
 */
void
carla_native_plugin_load_state (
  CarlaNativePlugin * self,
  const char *        abs_path);

NONNULL
void
carla_native_plugin_populate_banks (
  CarlaNativePlugin * self);

/**
 * Instantiates the plugin.
 *
 * @param loading Whether loading an existing plugin
 *   or not.
 * @param use_state_file Whether to use the plugin's
 *   state file to instantiate the plugin.
 *
 * @return 0 if no errors, non-zero if errors.
 */
NONNULL
int
carla_native_plugin_instantiate (
  CarlaNativePlugin * self,
  bool                loading,
  bool                use_state_file);

NONNULL
char *
carla_native_plugin_get_abs_state_file_path (
  CarlaNativePlugin * self,
  bool                is_backup);

/**
 * Processes the plugin for this cycle.
 */
HOT
NONNULL
void
carla_native_plugin_process (
  CarlaNativePlugin * self,
  const long          g_start_frames,
  const nframes_t  local_offset,
  const nframes_t     nframes);

/**
 * Shows or hides the UI.
 */
NONNULL
void
carla_native_plugin_open_ui (
  CarlaNativePlugin * self,
  bool                show);

/**
 * Returns the plugin Port corresponding to the
 * given parameter.
 */
NONNULL
Port *
carla_native_plugin_get_port_from_param_id (
  CarlaNativePlugin * self,
  const uint32_t      id);

/**
 * Returns the MIDI out port.
 */
NONNULL
Port *
carla_native_plugin_get_midi_out_port (
  CarlaNativePlugin * self);

NONNULL
float
carla_native_plugin_get_param_value (
  CarlaNativePlugin * self,
  const uint32_t      id);

/**
 * Called from port_set_control_value() to send
 * the value to carla.
 *
 * @param val Real value (ie, not normalized).
 */
NONNULL
void
carla_native_plugin_set_param_value (
  CarlaNativePlugin * self,
  const uint32_t      id,
  float               val);

NONNULL
int
carla_native_plugin_activate (
  CarlaNativePlugin * self,
  bool                activate);

NONNULL
void
carla_native_plugin_close (
  CarlaNativePlugin * self);

bool
carla_native_plugin_has_custom_ui (
  PluginDescriptor * descr);

/**
 * Deactivates, cleanups and frees the instance.
 */
NONNULL
void
carla_native_plugin_free (
  CarlaNativePlugin * self);

#endif /* HAVE_CARLA */

/**
 * @}
 */

#endif
