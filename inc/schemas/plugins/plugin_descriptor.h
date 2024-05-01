// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Base plugin.
 */

#ifndef __SCHEMAS_PLUGINS_PLUGIN_DESCRIPTOR_H__
#define __SCHEMAS_PLUGINS_PLUGIN_DESCRIPTOR_H__

#include <stdbool.h>

#include "utils/yaml.h"

typedef enum ZPluginCategory_v1
{
  ZPLUGIN_CATEGORY_NONE_v1,
  Z_PLUGIN_CATEGORY_DELAY_v1,
  Z_PLUGIN_CATEGORY_REVERB_v1,
  Z_PLUGIN_CATEGORY_DISTORTION_v1,
  Z_PLUGIN_CATEGORY_WAVESHAPER_v1,
  Z_PLUGIN_CATEGORY_DYNAMICS_v1,
  Z_PLUGIN_CATEGORY_AMPLIFIER_v1,
  Z_PLUGIN_CATEGORY_COMPRESSOR_v1,
  Z_PLUGIN_CATEGORY_ENVELOPE_v1,
  Z_PLUGIN_CATEGORY_EXPANDER_v1,
  Z_PLUGIN_CATEGORY_GATE_v1,
  Z_PLUGIN_CATEGORY_LIMITER_v1,
  Z_PLUGIN_CATEGORY_FILTER_v1,
  Z_PLUGIN_CATEGORY_ALLPASS_FILTER_v1,
  Z_PLUGIN_CATEGORY_BANDPASS_FILTER_v1,
  Z_PLUGIN_CATEGORY_COMB_FILTER_v1,
  Z_PLUGIN_CATEGORY_EQ_v1,
  Z_PLUGIN_CATEGORY_MULTI_EQ_v1,
  Z_PLUGIN_CATEGORY_PARA_EQ_v1,
  Z_PLUGIN_CATEGORY_HIGHPASS_FILTER_v1,
  Z_PLUGIN_CATEGORY_LOWPASS_FILTER_v1,
  Z_PLUGIN_CATEGORY_GENERATOR_v1,
  Z_PLUGIN_CATEGORY_CONSTANT_v1,
  Z_PLUGIN_CATEGORY_INSTRUMENT_v1,
  Z_PLUGIN_CATEGORY_OSCILLATOR_v1,
  Z_PLUGIN_CATEGORY_MIDI_v1,
  Z_PLUGIN_CATEGORY_MODULATOR_v1,
  Z_PLUGIN_CATEGORY_CHORUS_v1,
  Z_PLUGIN_CATEGORY_FLANGER_v1,
  Z_PLUGIN_CATEGORY_PHASER_v1,
  Z_PLUGIN_CATEGORY_SIMULATOR_v1,
  Z_PLUGIN_CATEGORY_SIMULATOR_REVERB_v1,
  Z_PLUGIN_CATEGORY_SPATIAL_v1,
  Z_PLUGIN_CATEGORY_SPECTRAL_v1,
  Z_PLUGIN_CATEGORY_PITCH_v1,
  Z_PLUGIN_CATEGORY_UTILITY_v1,
  Z_PLUGIN_CATEGORY_ANALYZER_v1,
  Z_PLUGIN_CATEGORY_CONVERTER_v1,
  Z_PLUGIN_CATEGORY_FUNCTION_v1,
  Z_PLUGIN_CATEGORY_MIXER_v1,
} ZPluginCategory_v1;

static const cyaml_strval_t plugin_descriptor_category_strings_v1[] = {
  {"None",              ZPLUGIN_CATEGORY_NONE_v1             },
  { "Delay",            Z_PLUGIN_CATEGORY_DELAY_v1           },
  { "Reverb",           Z_PLUGIN_CATEGORY_REVERB_v1          },
  { "Distortion",       Z_PLUGIN_CATEGORY_DISTORTION_v1      },
  { "Waveshaper",       Z_PLUGIN_CATEGORY_WAVESHAPER_v1      },
  { "Dynamics",         Z_PLUGIN_CATEGORY_DYNAMICS_v1        },
  { "Amplifier",        Z_PLUGIN_CATEGORY_AMPLIFIER_v1       },
  { "Compressor",       Z_PLUGIN_CATEGORY_COMPRESSOR_v1      },
  { "Envelope",         Z_PLUGIN_CATEGORY_ENVELOPE_v1        },
  { "Expander",         Z_PLUGIN_CATEGORY_EXPANDER_v1        },
  { "Gate",             Z_PLUGIN_CATEGORY_GATE_v1            },
  { "Limiter",          Z_PLUGIN_CATEGORY_LIMITER_v1         },
  { "Filter",           Z_PLUGIN_CATEGORY_FILTER_v1          },
  { "Allpass Filter",   Z_PLUGIN_CATEGORY_ALLPASS_FILTER_v1  },
  { "Bandpass Filter",  Z_PLUGIN_CATEGORY_BANDPASS_FILTER_v1 },
  { "Comb Filter",      Z_PLUGIN_CATEGORY_COMB_FILTER_v1     },
  { "EQ",               Z_PLUGIN_CATEGORY_EQ_v1              },
  { "Multi-EQ",         Z_PLUGIN_CATEGORY_MULTI_EQ_v1        },
  { "Parametric EQ",    Z_PLUGIN_CATEGORY_PARA_EQ_v1         },
  { "Highpass Filter",  Z_PLUGIN_CATEGORY_HIGHPASS_FILTER_v1 },
  { "Lowpass Filter",   Z_PLUGIN_CATEGORY_LOWPASS_FILTER_v1  },
  { "Generator",        Z_PLUGIN_CATEGORY_GENERATOR_v1       },
  { "Constant",         Z_PLUGIN_CATEGORY_CONSTANT_v1        },
  { "Instrument",       Z_PLUGIN_CATEGORY_INSTRUMENT_v1      },
  { "Oscillator",       Z_PLUGIN_CATEGORY_OSCILLATOR_v1      },
  { "MIDI",             Z_PLUGIN_CATEGORY_MIDI_v1            },
  { "Modulator",        Z_PLUGIN_CATEGORY_MODULATOR_v1       },
  { "Chorus",           Z_PLUGIN_CATEGORY_CHORUS_v1          },
  { "Flanger",          Z_PLUGIN_CATEGORY_FLANGER_v1         },
  { "Phaser",           Z_PLUGIN_CATEGORY_PHASER_v1          },
  { "Simulator",        Z_PLUGIN_CATEGORY_SIMULATOR_v1       },
  { "Simulator Reverb", Z_PLUGIN_CATEGORY_SIMULATOR_REVERB_v1},
  { "Spatial",          Z_PLUGIN_CATEGORY_SPATIAL_v1         },
  { "Spectral",         Z_PLUGIN_CATEGORY_SPECTRAL_v1        },
  { "Pitch",            Z_PLUGIN_CATEGORY_PITCH_v1           },
  { "Utility",          Z_PLUGIN_CATEGORY_UTILITY_v1         },
  { "Analyzer",         Z_PLUGIN_CATEGORY_ANALYZER_v1        },
  { "Converter",        Z_PLUGIN_CATEGORY_CONVERTER_v1       },
  { "Function",         Z_PLUGIN_CATEGORY_FUNCTION_v1        },
  { "Mixer",            Z_PLUGIN_CATEGORY_MIXER_v1           },
};

typedef enum PluginProtocol_v1
{
  PROT_DUMMY_v1,
  PROT_LV2_v1,
  PROT_DSSI_v1,
  PROT_LADSPA_v1,
  PROT_VST_v1,
  PROT_VST3_v1,
  PROT_AU_v1,
  PROT_SFZ_v1,
  PROT_SF2_v1,
  PROT_CLAP_v1,
  PROT_JSFX_v1,
} PluginProtocol_v1;

static const cyaml_strval_t plugin_protocol_strings_v1[] = {
  {"Dummy",   PROT_DUMMY_v1 },
  { "LV2",    PROT_LV2_v1   },
  { "DSSI",   PROT_DSSI_v1  },
  { "LADSPA", PROT_LADSPA_v1},
  { "VST",    PROT_VST_v1   },
  { "VST3",   PROT_VST3_v1  },
  { "AU",     PROT_AU_v1    },
  { "SFZ",    PROT_SFZ_v1   },
  { "SF2",    PROT_SF2_v1   },
  { "CLAP",   PROT_CLAP_v1  },
  { "JSFX",   PROT_JSFX_v1  },
};

typedef enum PluginArchitecture_v1
{
  Z_PLUGIN_ARCHITECTURE_32_v1,
  Z_PLUGIN_ARCHITECTURE_64_v1,
} PluginArchitecture_v1;

static const cyaml_strval_t plugin_architecture_strings_v1[] = {
  {"32-bit",  Z_PLUGIN_ARCHITECTURE_32_v1},
  { "64-bit", Z_PLUGIN_ARCHITECTURE_64_v1},
};

typedef enum ZCarlaBridgeMode_v1
{
  Z_CARLA_BRIDGE_NONE_v1,
  Z_CARLA_BRIDGE_UI_v1,
  Z_CARLA_BRIDGE_FULL_v1,
} ZCarlaBridgeMode_v1;

static const cyaml_strval_t carla_bridge_mode_strings_v1[] = {
  {"None",  Z_CARLA_BRIDGE_NONE_v1},
  { "UI",   Z_CARLA_BRIDGE_UI_v1  },
  { "Full", Z_CARLA_BRIDGE_FULL_v1},
};

/***
 * A descriptor to be implemented by all plugins
 * This will be used throughout the UI
 */
typedef struct PluginDescriptor_v1
{
  int                   schema_version;
  char *                author;
  char *                name;
  char *                website;
  ZPluginCategory_v1    category;
  char *                category_str;
  int                   num_audio_ins;
  int                   num_midi_ins;
  int                   num_audio_outs;
  int                   num_midi_outs;
  int                   num_ctrl_ins;
  int                   num_ctrl_outs;
  int                   num_cv_ins;
  int                   num_cv_outs;
  PluginArchitecture_v1 arch;
  PluginProtocol_v1     protocol;
  char *                path;
  char *                uri;
  int64_t               unique_id;
  ZCarlaBridgeMode_v1   min_bridge_mode;
  bool                  has_custom_ui;
  unsigned int          ghash;
} PluginDescriptor_v1;

static const cyaml_schema_field_t plugin_descriptor_fields_schema_v1[] = {
  YAML_FIELD_INT (PluginDescriptor_v1, schema_version),
  YAML_FIELD_STRING_PTR_OPTIONAL (PluginDescriptor_v1, author),
  YAML_FIELD_STRING_PTR_OPTIONAL (PluginDescriptor_v1, name),
  YAML_FIELD_STRING_PTR_OPTIONAL (PluginDescriptor_v1, website),
  YAML_FIELD_ENUM (
    PluginDescriptor_v1,
    category,
    plugin_descriptor_category_strings_v1),
  YAML_FIELD_STRING_PTR_OPTIONAL (PluginDescriptor_v1, category_str),
  YAML_FIELD_INT (PluginDescriptor_v1, num_audio_ins),
  YAML_FIELD_INT (PluginDescriptor_v1, num_audio_outs),
  YAML_FIELD_INT (PluginDescriptor_v1, num_midi_ins),
  YAML_FIELD_INT (PluginDescriptor_v1, num_midi_outs),
  YAML_FIELD_INT (PluginDescriptor_v1, num_ctrl_ins),
  YAML_FIELD_INT (PluginDescriptor_v1, num_ctrl_outs),
  YAML_FIELD_INT (PluginDescriptor_v1, num_cv_ins),
  YAML_FIELD_INT (PluginDescriptor_v1, num_cv_outs),
  YAML_FIELD_UINT (PluginDescriptor_v1, unique_id),
  YAML_FIELD_ENUM (PluginDescriptor_v1, arch, plugin_architecture_strings_v1),
  YAML_FIELD_ENUM (PluginDescriptor_v1, protocol, plugin_protocol_strings_v1),
  YAML_FIELD_STRING_PTR_OPTIONAL (PluginDescriptor_v1, path),
  YAML_FIELD_STRING_PTR_OPTIONAL (PluginDescriptor_v1, uri),
  YAML_FIELD_ENUM (
    PluginDescriptor_v1,
    min_bridge_mode,
    carla_bridge_mode_strings_v1),
  YAML_FIELD_INT (PluginDescriptor_v1, has_custom_ui),
  YAML_FIELD_UINT (PluginDescriptor_v1, ghash),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t plugin_descriptor_schema_v1 = {
  YAML_VALUE_PTR (PluginDescriptor_v1, plugin_descriptor_fields_schema_v1),
};

#endif
