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
  PC_DELAY_v1,
  PC_REVERB_v1,
  PC_DISTORTION_v1,
  PC_WAVESHAPER_v1,
  PC_DYNAMICS_v1,
  PC_AMPLIFIER_v1,
  PC_COMPRESSOR_v1,
  PC_ENVELOPE_v1,
  PC_EXPANDER_v1,
  PC_GATE_v1,
  PC_LIMITER_v1,
  PC_FILTER_v1,
  PC_ALLPASS_FILTER_v1,
  PC_BANDPASS_FILTER_v1,
  PC_COMB_FILTER_v1,
  PC_EQ_v1,
  PC_MULTI_EQ_v1,
  PC_PARA_EQ_v1,
  PC_HIGHPASS_FILTER_v1,
  PC_LOWPASS_FILTER_v1,
  PC_GENERATOR_v1,
  PC_CONSTANT_v1,
  PC_INSTRUMENT_v1,
  PC_OSCILLATOR_v1,
  PC_MIDI_v1,
  PC_MODULATOR_v1,
  PC_CHORUS_v1,
  PC_FLANGER_v1,
  PC_PHASER_v1,
  PC_SIMULATOR_v1,
  PC_SIMULATOR_REVERB_v1,
  PC_SPATIAL_v1,
  PC_SPECTRAL_v1,
  PC_PITCH_v1,
  PC_UTILITY_v1,
  PC_ANALYZER_v1,
  PC_CONVERTER_v1,
  PC_FUNCTION_v1,
  PC_MIXER_v1,
} ZPluginCategory_v1;

static const cyaml_strval_t plugin_descriptor_category_strings_v1[] = {
  {"None",              ZPLUGIN_CATEGORY_NONE_v1},
  { "Delay",            PC_DELAY_v1             },
  { "Reverb",           PC_REVERB_v1            },
  { "Distortion",       PC_DISTORTION_v1        },
  { "Waveshaper",       PC_WAVESHAPER_v1        },
  { "Dynamics",         PC_DYNAMICS_v1          },
  { "Amplifier",        PC_AMPLIFIER_v1         },
  { "Compressor",       PC_COMPRESSOR_v1        },
  { "Envelope",         PC_ENVELOPE_v1          },
  { "Expander",         PC_EXPANDER_v1          },
  { "Gate",             PC_GATE_v1              },
  { "Limiter",          PC_LIMITER_v1           },
  { "Filter",           PC_FILTER_v1            },
  { "Allpass Filter",   PC_ALLPASS_FILTER_v1    },
  { "Bandpass Filter",  PC_BANDPASS_FILTER_v1   },
  { "Comb Filter",      PC_COMB_FILTER_v1       },
  { "EQ",               PC_EQ_v1                },
  { "Multi-EQ",         PC_MULTI_EQ_v1          },
  { "Parametric EQ",    PC_PARA_EQ_v1           },
  { "Highpass Filter",  PC_HIGHPASS_FILTER_v1   },
  { "Lowpass Filter",   PC_LOWPASS_FILTER_v1    },
  { "Generator",        PC_GENERATOR_v1         },
  { "Constant",         PC_CONSTANT_v1          },
  { "Instrument",       PC_INSTRUMENT_v1        },
  { "Oscillator",       PC_OSCILLATOR_v1        },
  { "MIDI",             PC_MIDI_v1              },
  { "Modulator",        PC_MODULATOR_v1         },
  { "Chorus",           PC_CHORUS_v1            },
  { "Flanger",          PC_FLANGER_v1           },
  { "Phaser",           PC_PHASER_v1            },
  { "Simulator",        PC_SIMULATOR_v1         },
  { "Simulator Reverb", PC_SIMULATOR_REVERB_v1  },
  { "Spatial",          PC_SPATIAL_v1           },
  { "Spectral",         PC_SPECTRAL_v1          },
  { "Pitch",            PC_PITCH_v1             },
  { "Utility",          PC_UTILITY_v1           },
  { "Analyzer",         PC_ANALYZER_v1          },
  { "Converter",        PC_CONVERTER_v1         },
  { "Function",         PC_FUNCTION_v1          },
  { "Mixer",            PC_MIXER_v1             },
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
  ARCH_32_v1,
  ARCH_64_v1,
} PluginArchitecture_v1;

static const cyaml_strval_t plugin_architecture_strings_v1[] = {
  {"32-bit",  ARCH_32_v1},
  { "64-bit", ARCH_64_v1},
};

typedef enum CarlaBridgeMode_v1
{
  CARLA_BRIDGE_NONE_v1,
  CARLA_BRIDGE_UI_v1,
  CARLA_BRIDGE_FULL_v1,
} CarlaBridgeMode_v1;

static const cyaml_strval_t carla_bridge_mode_strings_v1[] = {
  {"None",  CARLA_BRIDGE_NONE_v1},
  { "UI",   CARLA_BRIDGE_UI_v1  },
  { "Full", CARLA_BRIDGE_FULL_v1},
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
  CarlaBridgeMode_v1    min_bridge_mode;
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
