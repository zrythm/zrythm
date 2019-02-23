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

#ifndef __PLUGINS_BASE_PLUGIN_H__
#define __PLUGINS_BASE_PLUGIN_H__

#include "audio/automatable.h"
#include "audio/port.h"
#include "gui/widgets/instrument_track.h"

#include <jack/jack.h>

/* FIXME allocate dynamically */
#define MAX_IN_PORTS 400000
#define MAX_OUT_PORTS 14000
#define MAX_UNKNOWN_PORTS 4000

#define DUMMY_PLUGIN "Dummy Plugin"

typedef struct Channel Channel;



enum PluginCategory
{
  PC_UTILITY,
  PC_GENERATOR,
  PC_DELAY_REVERB,
  PC_MODULATION,
  PC_FILTER,
  PC_DISTORTION,
  PC_DYNAMICS,
  PC_HARDWARE,
  PC_AUDIO
};

typedef enum PluginProtocol
{
 PROT_LV2,
 PROT_DSSI,
 PROT_LADSPA,
 PROT_VST,
 PROT_VST3
} PluginProtocol;

typedef enum PluginArchitecture
{
  ARCH_32,
  ARCH_64
} PluginArchitecture;

/***
 * A descriptor to be implemented by all plugins
 * This will be used throughout the UI
 */
typedef struct PluginDescriptor
{
  char                 * author;
  char                 * name;
  char                 * website;
  char                 * category;           ///< one of the above
  int              num_audio_ins;             ///< # of input ports
  int              num_midi_ins;             ///< # of input ports
  int              num_audio_outs;            ///< # of output ports
  int              num_midi_outs;            ///< # of output ports
  int              num_ctrl_ins;            ///< # of input ctrls
  int              num_ctrl_outs;            ///< # of output ctrls
  PluginArchitecture   arch;               ///< architecture 32/64bit
  PluginProtocol       protocol;           ///< VST/LV2/DSSI/LADSPA...
  char                 * path;
  char                 * uri;            ///< for LV2 plugins
} PluginDescriptor;

/**
 * The base plugin
 * Inheriting plugins must have this as a child
 */
typedef struct Plugin
{
  int                  id;
  void                 * original_plugin;     ///< pointer to original plugin inheriting this base plugin
  PluginDescriptor    * descr;                 ///< descriptor
  /* ports coming in as input */
  int                  in_port_ids[MAX_IN_PORTS];
  Port                 * in_ports[MAX_IN_PORTS]; ///< cache
  int                  num_in_ports;    ///< counter
  int                  out_port_ids[MAX_OUT_PORTS];
  Port                 * out_ports[MAX_OUT_PORTS];           ///< ports going out as output
  int                  num_out_ports;    ///< counter
  int                  unknown_port_ids[MAX_UNKNOWN_PORTS];
  Port                 * unknown_ports[MAX_UNKNOWN_PORTS];           ///< ports with unknown direction
  int                  num_unknown_ports;    ///< counter
  int                  channel_id;
  Channel              * channel;        ///< pointer to channel it belongs to
  int                  processed;  ///< processed or not yet
  int                  enabled;     ///< enabled or not
  Automatable *        automatables[900]; ///< automatables
  int                  num_automatables;
} Plugin;

static const cyaml_strval_t
plugin_protocol_strings[] =
{
	{ "LV2",          PROT_LV2    },
	{ "DSSI",         PROT_DSSI   },
	{ "LADSPA",       PROT_LADSPA },
	{ "VST",          PROT_VST    },
	{ "VST3",         PROT_VST3   },
};

static const cyaml_strval_t
plugin_architecture_strings[] =
{
	{ "32-bit",       ARCH_32     },
	{ "64-bit",       ARCH_64     },
};

static const cyaml_schema_field_t
descriptor_fields_schema[] =
{
  CYAML_FIELD_STRING_PTR (
    "author", CYAML_FLAG_POINTER,
    PluginDescriptor, author,
   	0, CYAML_UNLIMITED),
  CYAML_FIELD_STRING_PTR (
    "name", CYAML_FLAG_POINTER,
    PluginDescriptor, name,
   	0, CYAML_UNLIMITED),
  CYAML_FIELD_STRING_PTR (
    "website", CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    PluginDescriptor, website,
   	0, CYAML_UNLIMITED),
  CYAML_FIELD_STRING_PTR (
    "category", CYAML_FLAG_POINTER,
    PluginDescriptor, category,
   	0, CYAML_UNLIMITED),
	CYAML_FIELD_INT (
    "num_audio_ins", CYAML_FLAG_DEFAULT,
	  PluginDescriptor, num_audio_ins),
	CYAML_FIELD_INT (
    "num_audio_outs", CYAML_FLAG_DEFAULT,
	  PluginDescriptor, num_audio_outs),
	CYAML_FIELD_INT (
    "num_midi_ins", CYAML_FLAG_DEFAULT,
	  PluginDescriptor, num_midi_ins),
	CYAML_FIELD_INT (
    "num_midi_outs", CYAML_FLAG_DEFAULT,
	  PluginDescriptor, num_midi_outs),
	CYAML_FIELD_INT (
    "num_ctrl_ins", CYAML_FLAG_DEFAULT,
	  PluginDescriptor, num_ctrl_ins),
	CYAML_FIELD_INT (
    "num_ctrl_outs", CYAML_FLAG_DEFAULT,
	  PluginDescriptor, num_audio_outs),
  CYAML_FIELD_ENUM (
    "arch", CYAML_FLAG_DEFAULT,
    PluginDescriptor, arch, plugin_architecture_strings,
    CYAML_ARRAY_LEN (plugin_architecture_strings)),
  CYAML_FIELD_ENUM (
    "protocol", CYAML_FLAG_DEFAULT,
    PluginDescriptor, arch, plugin_protocol_strings,
    CYAML_ARRAY_LEN (plugin_protocol_strings)),
  CYAML_FIELD_STRING_PTR (
    "path", CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    PluginDescriptor, path,
   	0, CYAML_UNLIMITED),
  CYAML_FIELD_STRING_PTR (
    "uri", CYAML_FLAG_POINTER,
    PluginDescriptor, uri,
   	0, CYAML_UNLIMITED),

	CYAML_FIELD_END
};

static const cyaml_schema_field_t
plugin_fields_schema[] =
{
  CYAML_FIELD_INT (
    "id", CYAML_FLAG_DEFAULT,
    Plugin, id),
  CYAML_FIELD_MAPPING_PTR (
    "descr", CYAML_FLAG_POINTER,
    Plugin, descr,
    descriptor_fields_schema),
  CYAML_FIELD_SEQUENCE_COUNT (
    "in_port_ids", CYAML_FLAG_DEFAULT,
    Plugin, in_port_ids, num_in_ports,
    &int_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "out_port_ids", CYAML_FLAG_DEFAULT,
    Plugin, out_port_ids, num_out_ports,
    &int_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "unknown_port_ids", CYAML_FLAG_DEFAULT,
    Plugin, unknown_port_ids, num_unknown_ports,
    &int_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_INT (
    "channel_id", CYAML_FLAG_DEFAULT,
    Plugin, channel_id),
  CYAML_FIELD_INT (
    "enabled", CYAML_FLAG_DEFAULT,
    Plugin, enabled),
  CYAML_FIELD_SEQUENCE_COUNT (
    "automatables", CYAML_FLAG_DEFAULT,
    Plugin, automatables, num_automatables,
    &automatable_schema, 0, CYAML_UNLIMITED),

	CYAML_FIELD_END
};

static const cyaml_schema_value_t
plugin_schema =
{
	CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
	  Plugin, plugin_fields_schema),
};

/**
 * Generates automatables for the plugin.
 *
 *
 * Plugin must be instantiated already.
 */
void
plugin_generate_automatables (Plugin * plugin);

/**
 * Used when loading projects.
 */
Plugin *
plugin_get_or_create_blank (int id);

/**
 * Creates/initializes a plugin and its internal plugin (LV2, etc.)
 * using the given descriptor.
 */
Plugin *
plugin_create_from_descr (PluginDescriptor * descr);

/**
 * Loads the plugin from its state file.
 */
//void
//plugin_load (Plugin * plugin);

/**
 * Instantiates the plugin (e.g. when adding to a channel)
 */
int
plugin_instantiate (Plugin * plugin);

/**
 * Process plugin
 */
void
plugin_process (Plugin * plugin);

/**
 * Process show ui
 */
void
plugin_open_ui (Plugin *plugin);

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
 * Frees given plugin, breaks all its port connections, and frees its ports
 * and other internal pointers
 */
void
plugin_free (Plugin *plugin);

#endif
