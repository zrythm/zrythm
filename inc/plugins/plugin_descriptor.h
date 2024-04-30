// SPDX-FileCopyrightText: © 2018-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

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

#include <glib/gi18n.h>

#include <CarlaBackend.h>
#include <gio/gio.h>

typedef struct _WrappedObjectWithChangeSignal WrappedObjectWithChangeSignal;

/**
 * @addtogroup plugins
 *
 * @{
 */

#define PLUGIN_DESCRIPTOR_SCHEMA_VERSION 1

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

#if 0
static const char * plugin_descriptor_category_strings[] = {
  "None",
  "Delay",
  "Reverb",
  "Distortion",
  "Waveshaper",
  "Dynamics",
  "Amplifier",
  "Compressor",
  "Envelope",
  "Expander",
  "Gate",
  "Limiter",
  "Filter",
  "Allpass Filter",
  "Bandpass Filter",
  "Comb Filter",
  "EQ",
  "Multi-EQ",
  "Parametric EQ",
  "Highpass Filter",
  "Lowpass Filter",
  "Generator",
  "Constant",
  "Instrument",
  "Oscillator",
  "MIDI",
  "Modulator",
  "Chorus",
  "Flanger",
  "Phaser",
  "Simulator",
  "Simulator Reverb",
  "Spatial",
  "Spectral",
  "Pitch",
  "Utility",
  "Analyzer",
  "Converter",
  "Function",
  "Mixer",
};
#endif

/**
 * Plugin protocol.
 */
typedef enum ZPluginProtocol
{
  /** Dummy protocol for tests. */
  Z_PLUGIN_PROTOCOL_DUMMY,
  Z_PLUGIN_PROTOCOL_LV2,
  Z_PLUGIN_PROTOCOL_DSSI,
  Z_PLUGIN_PROTOCOL_LADSPA,
  Z_PLUGIN_PROTOCOL_VST,
  Z_PLUGIN_PROTOCOL_VST3,
  Z_PLUGIN_PROTOCOL_AU,
  Z_PLUGIN_PROTOCOL_SFZ,
  Z_PLUGIN_PROTOCOL_SF2,
  Z_PLUGIN_PROTOCOL_CLAP,
  Z_PLUGIN_PROTOCOL_JSFX,
} ZPluginProtocol;

static const char * plugin_protocol_strings[] = {
  "Dummy", "LV2", "DSSI", "LADSPA", "VST",  "VST3",
  "AU",    "SFZ", "SF2",  "CLAP",   "JSFX",
};

/**
 * 32 or 64 bit.
 */
typedef enum PluginArchitecture
{
  ARCH_32,
  ARCH_64
} PluginArchitecture;

/**
 * Carla bridge mode.
 */
typedef enum CarlaBridgeMode
{
  CARLA_BRIDGE_NONE,
  CARLA_BRIDGE_UI,
  CARLA_BRIDGE_FULL,
} CarlaBridgeMode;

static const char * carla_bridge_mode_strings[] = {
  "None",
  "UI",
  "Full",
};

/***
 * A descriptor to be implemented by all plugins
 * This will be used throughout the UI
 */
typedef struct PluginDescriptor
{
  int             schema_version;
  char *          author;
  char *          name;
  char *          website;
  ZPluginCategory category;
  /** Lv2 plugin subcategory. */
  char * category_str;
  /** Number of audio input ports. */
  int num_audio_ins;
  /** Number of MIDI input ports. */
  int num_midi_ins;
  /** Number of audio output ports. */
  int num_audio_outs;
  /** Number of MIDI output ports. */
  int num_midi_outs;
  /** Number of input control (plugin param) ports. */
  int num_ctrl_ins;
  /** Number of output control (plugin param) ports. */
  int num_ctrl_outs;
  /** Number of input CV ports. */
  int num_cv_ins;
  /** Number of output CV ports. */
  int num_cv_outs;
  /** Architecture (32/64bit). */
  PluginArchitecture arch;
  /** Plugin protocol (Lv2/DSSI/LADSPA/VST/etc.). */
  ZPluginProtocol protocol;
  /** Path, if not an Lv2Plugin which uses URIs. */
  char * path;
  /** Lv2Plugin URI. */
  char * uri;

  /** Used for VST. */
  int64_t unique_id;

  /** Minimum required bridge mode. */
  CarlaBridgeMode min_bridge_mode;

  bool has_custom_ui;

  /**
   * Hash of the plugin's bundle (.so/.ddl for VST) used when caching
   * PluginDescriptor's, obtained using g_file_hash().
   *
   * @deprecated Kept so that older projects still work.
   */
  unsigned int ghash;

  /** SHA1 of the file (replaces ghash). */
  char * sha1;

  /** Used in Gtk. */
  WrappedObjectWithChangeSignal * gobj;
} PluginDescriptor;

PluginDescriptor *
plugin_descriptor_new (void);

const char *
plugin_protocol_to_str (ZPluginProtocol prot);

ZPluginProtocol
plugin_protocol_from_str (const char * str);

static inline const char *
plugin_protocol_get_icon_name (ZPluginProtocol prot)
{
  const char * icon = NULL;
  switch (prot)
    {
    case Z_PLUGIN_PROTOCOL_LV2:
      icon = "logo-lv2";
      break;
    case Z_PLUGIN_PROTOCOL_LADSPA:
      icon = "logo-ladspa";
      break;
    case Z_PLUGIN_PROTOCOL_AU:
      icon = "logo-au";
      break;
    case Z_PLUGIN_PROTOCOL_VST:
    case Z_PLUGIN_PROTOCOL_VST3:
      icon = "logo-vst";
      break;
    case Z_PLUGIN_PROTOCOL_SFZ:
    case Z_PLUGIN_PROTOCOL_SF2:
      icon = "file-music-line";
      break;
    default:
      icon = "plug";
      break;
    }
  return icon;
}

bool
plugin_protocol_is_supported (ZPluginProtocol protocol);

const char *
plugin_category_to_string (ZPluginCategory category);

PluginType
plugin_descriptor_get_carla_plugin_type_from_protocol (ZPluginProtocol protocol);

ZPluginProtocol
plugin_descriptor_get_protocol_from_carla_plugin_type (PluginType ptype);

ZPluginCategory
plugin_descriptor_get_category_from_carla_category_str (const char * category);

ZPluginCategory
plugin_descriptor_get_category_from_carla_category (PluginCategory carla_cat);

/**
 * Clones the plugin descriptor.
 */
NONNULL void
plugin_descriptor_copy (PluginDescriptor * dest, const PluginDescriptor * src);

/**
 * Clones the plugin descriptor.
 */
NONNULL PluginDescriptor *
plugin_descriptor_clone (const PluginDescriptor * src);

/**
 * Returns if the Plugin is an instrument or not.
 */
NONNULL bool
plugin_descriptor_is_instrument (const PluginDescriptor * const descr);

/**
 * Returns if the Plugin is an effect or not.
 */
NONNULL bool
plugin_descriptor_is_effect (const PluginDescriptor * const descr);

/**
 * Returns if the Plugin is a modulator or not.
 */
NONNULL int
plugin_descriptor_is_modulator (const PluginDescriptor * const descr);

/**
 * Returns if the Plugin is a midi modifier or not.
 */
NONNULL int
plugin_descriptor_is_midi_modifier (const PluginDescriptor * const descr);

/**
 * Returns the ZPluginCategory matching the given
 * string.
 */
NONNULL ZPluginCategory
plugin_descriptor_string_to_category (const char * str);

char *
plugin_descriptor_category_to_string (ZPluginCategory category);

/**
 * Gets an appropriate icon name for the given
 * descriptor.
 */
const char *
plugin_descriptor_get_icon_name (const PluginDescriptor * const self);

/**
 * Returns if the given plugin identifier can be
 * dropped in a slot of the given type.
 */
bool
plugin_descriptor_is_valid_for_slot_type (
  const PluginDescriptor * self,
  int                      slot_type,
  int                      track_type);

/**
 * Returns whether the two descriptors describe
 * the same plugin, ignoring irrelevant fields.
 */
NONNULL bool
plugin_descriptor_is_same_plugin (
  const PluginDescriptor * a,
  const PluginDescriptor * b);

/**
 * Returns if the Plugin has a supported custom
 * UI.
 */
NONNULL bool
plugin_descriptor_has_custom_ui (const PluginDescriptor * self);

/**
 * Returns the minimum bridge mode required for this
 * plugin.
 */
NONNULL CarlaBridgeMode
plugin_descriptor_get_min_bridge_mode (const PluginDescriptor * self);

/**
 * Returns whether the plugin is known to work, so it should
 * be whitelisted.
 *
 * Non-whitelisted plugins will run in full bridge mode. This
 * is to prevent crashes when Zrythm is not at fault.
 *
 * These must all be free-software plugins so that they can
 * be debugged if issues arise.
 */
NONNULL bool
plugin_descriptor_is_whitelisted (const PluginDescriptor * self);

NONNULL GMenuModel *
plugin_descriptor_generate_context_menu (const PluginDescriptor * self);

NONNULL void
plugin_descriptor_free (PluginDescriptor * self);

NONNULL void
plugin_descriptor_free_closure (void * data, GClosure * closure);

/**
 * @}
 */

#endif
