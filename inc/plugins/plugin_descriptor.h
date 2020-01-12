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

#include "utils/yaml.h"

/**
 * @addtogroup plugins
 *
 * @{
 */

/**
 * Plugin category.
 */
typedef enum PluginCategory
{
  /** None specified. */
  PLUGIN_CATEGORY_NONE,
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
} PluginCategory;

/**
 * Plugin protocol.
 */
typedef enum PluginProtocol
{
 PROT_LV2,
 PROT_DSSI,
 PROT_LADSPA,
 PROT_VST,
 PROT_VST3,
} PluginProtocol;

/**
 * 32 or 64 bit.
 */
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
  char *           author;
  char *           name;
  char *           website;
  PluginCategory   category;
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
  PluginArchitecture   arch;
  /** Plugin protocol (Lv2/DSSI/LADSPA/VST...). */
  PluginProtocol       protocol;
  /** Path, if not an Lv2Plugin which uses URIs. */
  char                 * path;
  /** Lv2Plugin URI. */
  char                 * uri;

  /** Hash of the plugin's bundle (.so/.ddl for VST)
   * used when caching PluginDescriptor's, obtained
   * using g_file_hash(). */
  unsigned int         ghash;
} PluginDescriptor;

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
  CYAML_FIELD_STRING_PTR (
    "category_str",
    CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    PluginDescriptor, category_str,
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
    PluginDescriptor, num_ctrl_outs),
  CYAML_FIELD_INT (
    "num_cv_ins", CYAML_FLAG_DEFAULT,
    PluginDescriptor, num_cv_ins),
  CYAML_FIELD_INT (
    "num_cv_outs", CYAML_FLAG_DEFAULT,
    PluginDescriptor, num_cv_outs),
  CYAML_FIELD_ENUM (
    "arch", CYAML_FLAG_DEFAULT,
    PluginDescriptor, arch,
    plugin_architecture_strings,
    CYAML_ARRAY_LEN (plugin_architecture_strings)),
  CYAML_FIELD_ENUM (
    "protocol", CYAML_FLAG_DEFAULT,
    PluginDescriptor, protocol,
    plugin_protocol_strings,
    CYAML_ARRAY_LEN (plugin_protocol_strings)),
  CYAML_FIELD_STRING_PTR (
    "path", CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    PluginDescriptor, path,
     0, CYAML_UNLIMITED),
  CYAML_FIELD_STRING_PTR (
    "uri", CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    PluginDescriptor, uri,
     0, CYAML_UNLIMITED),
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
 * Returns the PluginCategory matching the given
 * string.
 */
PluginCategory
plugin_descriptor_string_to_category (
  const char * str);

void
plugin_descriptor_free (
  PluginDescriptor * self);

/**
 * @}
 */

#endif
