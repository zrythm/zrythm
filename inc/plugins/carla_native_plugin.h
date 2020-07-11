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

} CarlaNativePlugin;

static const cyaml_schema_field_t
  carla_native_plugin_fields_schema[] =
{
  /** Not really needed but cyaml fails to load if
   * nothing is here. */
  YAML_FIELD_INT (
    CarlaNativePlugin, carla_plugin_id),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
  carla_native_plugin_schema =
{
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER, CarlaNativePlugin,
    carla_native_plugin_fields_schema),
};

#ifdef HAVE_CARLA

void
carla_native_plugin_init_loaded (
  CarlaNativePlugin * self);

/**
 * Creates an instance of a CarlaNativePlugin inside
 * the given Plugin.
 *
 * The given Plugin must have its descriptor filled
 * in.
 */
void
carla_native_plugin_new_from_descriptor (
  Plugin * plugin);

/**
 * Returns a filled in descriptor from the
 * CarlaCachedPluginInfo.
 */
PluginDescriptor *
carla_native_plugin_get_descriptor_from_cached (
  const CarlaCachedPluginInfo * info,
  PluginType              type);

/**
 * Saves the state inside the given directory.
 */
int
carla_native_plugin_save_state (
  CarlaNativePlugin * self,
  bool                is_backup);

/**
 * Loads the state from the given file or from
 * its state file.
 */
void
carla_native_plugin_load_state (
  CarlaNativePlugin * self,
  char *              abs_path);

void
carla_native_plugin_populate_banks (
  CarlaNativePlugin * self);

/**
 * Instantiates the plugin.
 *
 * @param loading Whether loading an existing plugin
 *   or not.
 * @ret 0 if no errors, non-zero if errors.
 */
int
carla_native_plugin_instantiate (
  CarlaNativePlugin * self,
  bool                loading);

char *
carla_native_plugin_get_abs_state_file_path (
  CarlaNativePlugin * self,
  bool                is_backup);

/**
 * Processes the plugin for this cycle.
 */
void
carla_native_plugin_proces (
  CarlaNativePlugin * self,
  const long          g_start_frames,
  const nframes_t     nframes);

/**
 * Shows or hides the UI.
 */
void
carla_native_plugin_open_ui (
  CarlaNativePlugin * self,
  bool                show);

/**
 * Returns the plugin Port corresponding to the
 * given parameter.
 */
Port *
carla_native_plugin_get_port_from_param_id (
  CarlaNativePlugin * self,
  const uint32_t      id);

/**
 * Returns the MIDI out port.
 */
Port *
carla_native_plugin_get_midi_out_port (
  CarlaNativePlugin * self);

float
carla_native_plugin_get_param_value (
  CarlaNativePlugin * self,
  const uint32_t      id);

void
carla_native_plugin_set_param_value (
  CarlaNativePlugin * self,
  const uint32_t      id,
  float               val);

int
carla_native_plugin_activate (
  CarlaNativePlugin * self,
  bool                activate);

void
carla_native_plugin_close (
  CarlaNativePlugin * self);

/**
 * Deactivates, cleanups and frees the instance.
 */
void
carla_native_plugin_free (
  CarlaNativePlugin * self);
#endif // HAVE_CARLA

/**
 * @}
 */

#endif
