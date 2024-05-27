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

#include "utils/types.h"

#include <gio/gio.h>
#include <glib/gi18n.h>

#include <CarlaBackend.h>

TYPEDEF_STRUCT_UNDERSCORED (WrappedObjectWithChangeSignal);
enum class TrackType;
enum class ZPluginSlotType;

/**
 * @addtogroup plugins
 *
 * @{
 */

#define PLUGIN_DESCRIPTOR_SCHEMA_VERSION 1

/**
 * Plugin category.
 */
#ifdef __cplusplus
enum class ZPluginCategory
#else
typedef enum
#endif
{
  /** None specified. */
  Z_PLUGIN_CATEGORY_NONE,
  Z_PLUGIN_CATEGORY_DELAY,
  Z_PLUGIN_CATEGORY_REVERB,
  Z_PLUGIN_CATEGORY_DISTORTION,
  Z_PLUGIN_CATEGORY_WAVESHAPER,
  Z_PLUGIN_CATEGORY_DYNAMICS,
  Z_PLUGIN_CATEGORY_AMPLIFIER,
  Z_PLUGIN_CATEGORY_COMPRESSOR,
  Z_PLUGIN_CATEGORY_ENVELOPE,
  Z_PLUGIN_CATEGORY_EXPANDER,
  Z_PLUGIN_CATEGORY_GATE,
  Z_PLUGIN_CATEGORY_LIMITER,
  Z_PLUGIN_CATEGORY_FILTER,
  Z_PLUGIN_CATEGORY_ALLPASS_FILTER,
  Z_PLUGIN_CATEGORY_BANDPASS_FILTER,
  Z_PLUGIN_CATEGORY_COMB_FILTER,
  Z_PLUGIN_CATEGORY_EQ,
  Z_PLUGIN_CATEGORY_MULTI_EQ,
  Z_PLUGIN_CATEGORY_PARA_EQ,
  Z_PLUGIN_CATEGORY_HIGHPASS_FILTER,
  Z_PLUGIN_CATEGORY_LOWPASS_FILTER,
  Z_PLUGIN_CATEGORY_GENERATOR,
  Z_PLUGIN_CATEGORY_CONSTANT,
  Z_PLUGIN_CATEGORY_INSTRUMENT,
  Z_PLUGIN_CATEGORY_OSCILLATOR,
  Z_PLUGIN_CATEGORY_MIDI,
  Z_PLUGIN_CATEGORY_MODULATOR,
  Z_PLUGIN_CATEGORY_CHORUS,
  Z_PLUGIN_CATEGORY_FLANGER,
  Z_PLUGIN_CATEGORY_PHASER,
  Z_PLUGIN_CATEGORY_SIMULATOR,
  Z_PLUGIN_CATEGORY_SIMULATOR_REVERB,
  Z_PLUGIN_CATEGORY_SPATIAL,
  Z_PLUGIN_CATEGORY_SPECTRAL,
  Z_PLUGIN_CATEGORY_PITCH,
  Z_PLUGIN_CATEGORY_UTILITY,
  Z_PLUGIN_CATEGORY_ANALYZER,
  Z_PLUGIN_CATEGORY_CONVERTER,
  Z_PLUGIN_CATEGORY_FUNCTION,
  Z_PLUGIN_CATEGORY_MIXER,
#ifdef __cplusplus
};
#else
} ZPluginCategory;
#endif

/**
 * Plugin protocol.
 */
#ifdef __cplusplus
enum class ZPluginProtocol
#else
typedef enum
#endif
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
#ifdef __cplusplus
};
#else
} ZPluginProtocol;
#endif

/**
 * 32 or 64 bit.
 */
enum class ZPluginArchitecture
{
  Z_PLUGIN_ARCHITECTURE_32,
  Z_PLUGIN_ARCHITECTURE_64
};

/**
 * Carla bridge mode.
 */
enum class CarlaBridgeMode
{
  None,
  UI,
  Full,
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
  ZPluginArchitecture arch;
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
    case ZPluginProtocol::Z_PLUGIN_PROTOCOL_LV2:
      icon = "logo-lv2";
      break;
    case ZPluginProtocol::Z_PLUGIN_PROTOCOL_LADSPA:
      icon = "logo-ladspa";
      break;
    case ZPluginProtocol::Z_PLUGIN_PROTOCOL_AU:
      icon = "logo-au";
      break;
    case ZPluginProtocol::Z_PLUGIN_PROTOCOL_VST:
    case ZPluginProtocol::Z_PLUGIN_PROTOCOL_VST3:
      icon = "logo-vst";
      break;
    case ZPluginProtocol::Z_PLUGIN_PROTOCOL_SFZ:
    case ZPluginProtocol::Z_PLUGIN_PROTOCOL_SF2:
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

CarlaBackend::PluginType
plugin_descriptor_get_carla_plugin_type_from_protocol (ZPluginProtocol protocol);

ZPluginProtocol
plugin_descriptor_get_protocol_from_carla_plugin_type (
  CarlaBackend::PluginType ptype);

ZPluginCategory
plugin_descriptor_get_category_from_carla_category_str (const char * category);

ZPluginCategory
plugin_descriptor_get_category_from_carla_category (
  CarlaBackend::PluginCategory carla_cat);

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
 * Returns if the given plugin identifier can be dropped in a slot of the given
 * type.
 */
bool
plugin_descriptor_is_valid_for_slot_type (
  const PluginDescriptor * self,
  ZPluginSlotType          slot_type,
  TrackType                track_type);

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
