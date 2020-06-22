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

#ifndef __PLUGINS_PLUGIN_DESCRIPTOR_H__
#define __PLUGINS_PLUGIN_DESCRIPTOR_H__

#include "zrythm-config.h"

#include <stdbool.h>

#include "utils/yaml.h"

/**
 * @addtogroup plugins
 *
 * @{
 */

/**
 * Plugin category.
 */
typedef enum ZPluginCategory
{
  /** None specified. */
  ZPLUGIN_CATEGORY_NONE,
  PC_DELAY,
  PC_REVERB,
  PC_DISTORTION,
  PC_WAVESHAPER,
  PC_DYNAMICS,
  PC_AMPLIFIER,
  PC_COMPRESSOR,
  PC_ENVELOPE,
  PC_EXPANDER,
  PC_GATE,
  PC_LIMITER,
  PC_FILTER,
  PC_ALLPASS_FILTER,
  PC_BANDPASS_FILTER,
  PC_COMB_FILTER,
  PC_EQ,
  PC_MULTI_EQ,
  PC_PARA_EQ,
  PC_HIGHPASS_FILTER,
  PC_LOWPASS_FILTER,
  PC_GENERATOR,
  PC_CONSTANT,
  PC_INSTRUMENT,
  PC_OSCILLATOR,
  PC_MIDI,
  PC_MODULATOR,
  PC_CHORUS,
  PC_FLANGER,
  PC_PHASER,
  PC_SIMULATOR,
  PC_SIMULATOR_REVERB,
  PC_SPATIAL,
  PC_SPECTRAL,
  PC_PITCH,
  PC_UTILITY,
  PC_ANALYZER,
  PC_CONVERTER,
  PC_FUNCTION,
  PC_MIXER,
} ZPluginCategory;

static const cyaml_strval_t
plugin_descriptor_category_strings[] =
{
  { "None", ZPLUGIN_CATEGORY_NONE },
  { "Delay", PC_DELAY },
  { "Reverb", PC_REVERB },
  { "Distortion", PC_DISTORTION },
  { "Waveshaper", PC_WAVESHAPER },
  { "Dynamics", PC_DYNAMICS },
  { "Amplifier", PC_AMPLIFIER },
  { "Compressor", PC_COMPRESSOR },
  { "Envelope", PC_ENVELOPE },
  { "Expander", PC_EXPANDER },
  { "Gate", PC_GATE },
  { "Limiter", PC_LIMITER },
  { "Filter", PC_FILTER },
  { "Allpass Filter", PC_ALLPASS_FILTER  },
  { "Bandpass Filter", PC_BANDPASS_FILTER },
  { "Comb Filter", PC_COMB_FILTER },
  { "EQ", PC_EQ },
  { "Multi-EQ", PC_MULTI_EQ },
  { "Parametric EQ", PC_PARA_EQ },
  { "Highpass Filter", PC_HIGHPASS_FILTER },
  { "Lowpass Filter", PC_LOWPASS_FILTER },
  { "Generator", PC_GENERATOR },
  { "Constant", PC_CONSTANT },
  { "Instrument", PC_INSTRUMENT },
  { "Oscillator", PC_OSCILLATOR },
  { "MIDI", PC_MIDI },
  { "Modulator", PC_MODULATOR },
  { "Chorus", PC_CHORUS },
  { "Flanger", PC_FLANGER },
  { "Phaser", PC_PHASER },
  { "Simulator", PC_SIMULATOR },
  { "Simulator Reverb", PC_SIMULATOR_REVERB },
  { "Spatial", PC_SPATIAL },
  { "Spectral", PC_SPECTRAL },
  { "Pitch", PC_PITCH },
  { "Utility", PC_UTILITY },
  { "Analyzer", PC_ANALYZER },
  { "Converter", PC_CONVERTER },
  { "Function", PC_FUNCTION },
  { "Mixer", PC_MIXER },
};

/**
 * Plugin protocol.
 */
typedef enum PluginProtocol
{
  /** Dummy protocol for tests. */
  PROT_DUMMY,
  PROT_LV2,
  PROT_DSSI,
  PROT_LADSPA,
  PROT_VST,
  PROT_VST3,
  PROT_AU,
  PROT_SFZ,
  PROT_SF2,
} PluginProtocol;

static const cyaml_strval_t
plugin_protocol_strings[] =
{
  { __("Dummy"),    PROT_DUMMY  },
  { "LV2",          PROT_LV2    },
  { "DSSI",         PROT_DSSI   },
  { "LADSPA",       PROT_LADSPA },
  { "VST",          PROT_VST    },
  { "VST3",         PROT_VST3   },
  { "AU",           PROT_AU     },
  { "SFZ",          PROT_SFZ    },
  { "SF2",          PROT_SF2    },
};

/**
 * 32 or 64 bit.
 */
typedef enum PluginArchitecture
{
  ARCH_32,
  ARCH_64
} PluginArchitecture;

static const cyaml_strval_t
plugin_architecture_strings[] =
{
  { "32-bit",       ARCH_32     },
  { "64-bit",       ARCH_64     },
};

/**
 * Carla bridge mode.
 */
typedef enum CarlaBridgeMode
{
  CARLA_BRIDGE_NONE,
  CARLA_BRIDGE_UI,
  CARLA_BRIDGE_FULL,
} CarlaBridgeMode;

static const cyaml_strval_t
carla_bridge_mode_strings[] =
{
  { "None", CARLA_BRIDGE_NONE },
  { "UI",   CARLA_BRIDGE_UI   },
  { "Full", CARLA_BRIDGE_FULL },
};

/***
 * A descriptor to be implemented by all plugins
 * This will be used throughout the UI
 */
typedef struct PluginDescriptor
{
  char *           author;
  char *           name;
  char *           website;
  ZPluginCategory   category;
  /** Lv2 plugin subcategory. */
  char *           category_str;
  /** Number of audio input ports. */
  int              num_audio_ins;
  /** Number of MIDI input ports. */
  int              num_midi_ins;
  /** Number of audio output ports. */
  int              num_audio_outs;
  /** Number of MIDI output ports. */
  int              num_midi_outs;
  /** Number of input control (plugin param)
   * ports. */
  int              num_ctrl_ins;
  /** Number of output control (plugin param)
   * ports. */
  int              num_ctrl_outs;
  /** Number of input CV ports. */
  int              num_cv_ins;
  /** Number of output CV ports. */
  int              num_cv_outs;
  /** Architecture (32/64bit). */
  PluginArchitecture arch;
  /** Plugin protocol (Lv2/DSSI/LADSPA/VST...). */
  PluginProtocol   protocol;
  /** Path, if not an Lv2Plugin which uses URIs. */
  char *           path;
  /** Lv2Plugin URI. */
  char *           uri;

  /** 1 if this plugin is to be instantiated
   * through Carla. */
  bool             open_with_carla;

  /** Carla bridge mode. */
  CarlaBridgeMode  bridge_mode;

  /** Whether the plugin needs to be bridged
   * through Carla (only used for LV2 plugins
   * having Gtk2 UIs). */
  //bool           needs_bridging;

  /** Used for VST. */
  int64_t          unique_id;

  /** Hash of the plugin's bundle (.so/.ddl for VST)
   * used when caching PluginDescriptor's, obtained
   * using g_file_hash(). */
  unsigned int     ghash;
} PluginDescriptor;

static const cyaml_schema_field_t
plugin_descriptor_fields_schema[] =
{
  CYAML_FIELD_STRING_PTR (
    "author",
    CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    PluginDescriptor, author,
     0, CYAML_UNLIMITED),
  CYAML_FIELD_STRING_PTR (
    "name",
    CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    PluginDescriptor, name,
     0, CYAML_UNLIMITED),
  CYAML_FIELD_STRING_PTR (
    "website",
    CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    PluginDescriptor, website,
     0, CYAML_UNLIMITED),
  YAML_FIELD_ENUM (
    PluginDescriptor, category,
    plugin_descriptor_category_strings),
  CYAML_FIELD_STRING_PTR (
    "category_str",
    CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    PluginDescriptor, category_str,
     0, CYAML_UNLIMITED),
  YAML_FIELD_INT (
    PluginDescriptor, num_audio_ins),
  YAML_FIELD_INT (
    PluginDescriptor, num_audio_outs),
  YAML_FIELD_INT (
    PluginDescriptor, num_midi_ins),
  YAML_FIELD_INT (
    PluginDescriptor, num_midi_outs),
  YAML_FIELD_INT (
    PluginDescriptor, num_ctrl_ins),
  YAML_FIELD_INT (
    PluginDescriptor, num_ctrl_outs),
  YAML_FIELD_INT (
    PluginDescriptor, num_cv_ins),
  CYAML_FIELD_UINT (
    "unique_id", CYAML_FLAG_DEFAULT,
    PluginDescriptor, unique_id),
  YAML_FIELD_INT (
    PluginDescriptor, num_cv_outs),
  YAML_FIELD_INT (
    PluginDescriptor, open_with_carla),
  //YAML_FIELD_INT (
    //PluginDescriptor, needs_bridging),
  YAML_FIELD_ENUM (
    PluginDescriptor, arch,
    plugin_architecture_strings),
  YAML_FIELD_ENUM (
    PluginDescriptor, protocol,
    plugin_protocol_strings),
#if 0
  YAML_FIELD_ENUM (
    PluginDescriptor, bridge_mode,
    carla_bridge_mode_strings),
#endif
  YAML_FIELD_STRING_PTR_OPTIONAL (
    PluginDescriptor, path),
  YAML_FIELD_STRING_PTR_OPTIONAL (
    PluginDescriptor, uri),
  CYAML_FIELD_UINT (
    "ghash", CYAML_FLAG_DEFAULT,
    PluginDescriptor, ghash),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
plugin_descriptor_schema =
{
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
    PluginDescriptor,
    plugin_descriptor_fields_schema),
};

const char *
plugin_protocol_to_str (
  PluginProtocol prot);

/**
 * Clones the plugin descriptor.
 */
void
plugin_descriptor_copy (
  const PluginDescriptor * src,
  PluginDescriptor * dest);

/**
 * Clones the plugin descriptor.
 */
PluginDescriptor *
plugin_descriptor_clone (
  const PluginDescriptor * src);

/**
 * Returns if the Plugin is an instrument or not.
 */
int
plugin_descriptor_is_instrument (
  const PluginDescriptor * descr);

/**
 * Returns if the Plugin is an effect or not.
 */
int
plugin_descriptor_is_effect (
  PluginDescriptor * descr);

/**
 * Returns if the Plugin is a modulator or not.
 */
int
plugin_descriptor_is_modulator (
  PluginDescriptor * descr);

/**
 * Returns if the Plugin is a midi modifier or not.
 */
int
plugin_descriptor_is_midi_modifier (
  PluginDescriptor * descr);

/**
 * Returns the ZPluginCategory matching the given
 * string.
 */
ZPluginCategory
plugin_descriptor_string_to_category (
  const char * str);

char *
plugin_descriptor_category_to_string (
  ZPluginCategory category);

/**
 * Returns if the given plugin identifier can be
 * dropped in a slot of the given type.
 */
bool
plugin_descriptor_is_valid_for_slot_type (
  PluginDescriptor * self,
  int                slot_type,
  int                track_type);

/**
 * Returns if the Plugin has a supported custom
 * UI.
 */
bool
plugin_descriptor_has_custom_ui (
  PluginDescriptor * self);

void
plugin_descriptor_free (
  PluginDescriptor * self);

/**
 * @}
 */

#endif
