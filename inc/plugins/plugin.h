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

#include "audio/automatable.h"
#include "audio/port.h"
#include "plugins/lv2_plugin.h"
#include "plugins/vst_plugin.h"
#include "utils/types.h"

/* pulled in from X11 */
#undef Bool

/**
 * @addtogroup plugins
 *
 * @{
 */

#define DUMMY_PLUGIN "Dummy Plugin"

#define IS_PLUGIN_CATEGORY(p, c) \
  (g_strcmp0 (((Plugin *)p)->descr->category, c) == 0)
#define IS_PLUGIN_DESCR_CATEGORY(d,c) \
  (g_strcmp0 (d->category,c) == 0)

typedef struct Channel Channel;
typedef struct VstPlugin VstPlugin;

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
} PluginDescriptor;

/**
 * The base plugin
 * Inheriting plugins must have this as a child
 */
typedef struct Plugin
{
  /**
   * Pointer back to plugin in its original format.
   */
  Lv2Plugin *          lv2;

  /** VST plugin. */
  VstPlugin *          vst;

  /** Descriptor. */
  PluginDescriptor *   descr;

  /** Ports coming in as input, for seralization. */
  //PortIdentifier *    in_port_ids;

  /** Ports coming in as input. */
  Port **             in_ports;
  int                 num_in_ports;
  size_t              in_ports_size;

  /** Outgoing port identifiers for serialization. */
  //PortIdentifier *    out_port_ids;

  /** Outgoing ports. */
  Port **             out_ports;
  int                 num_out_ports;
  size_t              out_ports_size;

  /** Ports with unknown direction (not used). */
  //PortIdentifier *    unknown_port_ids;
  Port **             unknown_ports;
  int                 num_unknown_ports;
  size_t              unknown_ports_size;

  /** The Channel this plugin belongs to. */
  Track              * track;
  int                  track_pos;

  /**
   * A subset of the automation tracks in the
   * automation tracklist of the track this plugin
   * is in.
   *
   * These are not meant to be serialized and are
   * used when e.g. moving plugins.
   */
  AutomationTrack **  ats;
  int                 num_ats;
  size_t              ats_size;

  /**
   * The slot this plugin is at in its channel.
   */
  int                  slot;

  /** Enabled or not. */
  int                  enabled;

  /** Whether plugin UI is opened or not. */
  int                  visible;

  /** The latency in samples. */
  nframes_t            latency;

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

  /** The Plugin's window TODO move here from
   * Lv2Plugin and VstPlugin. */
  GtkWindow *          window;
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
    "category_str", CYAML_FLAG_POINTER,
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

  CYAML_FIELD_END
};

static const cyaml_schema_field_t
plugin_fields_schema[] =
{
  CYAML_FIELD_MAPPING_PTR (
    "descr", CYAML_FLAG_POINTER,
    Plugin, descr,
    descriptor_fields_schema),
  CYAML_FIELD_MAPPING_PTR (
    "lv2", CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    Plugin, lv2,
    lv2_plugin_fields_schema),
  CYAML_FIELD_MAPPING_PTR (
    "vst", CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    Plugin, vst,
    vst_plugin_fields_schema),
  CYAML_FIELD_SEQUENCE_COUNT (
    "in_ports", CYAML_FLAG_POINTER,
    Plugin, in_ports, num_in_ports,
    &port_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "out_ports", CYAML_FLAG_POINTER,
    Plugin, out_ports, num_out_ports,
    &port_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "unknown_ports", CYAML_FLAG_POINTER,
    Plugin, unknown_ports, num_unknown_ports,
    &port_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_INT (
    "enabled", CYAML_FLAG_DEFAULT,
    Plugin, enabled),
  CYAML_FIELD_INT (
    "visible", CYAML_FLAG_DEFAULT,
    Plugin, visible),
  CYAML_FIELD_INT (
    "track_pos", CYAML_FLAG_DEFAULT,
    Plugin, track_pos),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
plugin_schema =
{
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL |
    CYAML_FLAG_ALLOW_NULL_PTR,
    Plugin, plugin_fields_schema),
};

void
plugin_init_loaded (
  Plugin * self);

static inline const char *
plugin_protocol_to_str (
  PluginProtocol prot)
{
  for (size_t i = 0;
       i < G_N_ELEMENTS (plugin_protocol_strings);
       i++)
    {
      if (plugin_protocol_strings[i].val ==
            (int64_t) prot)
        {
          return
            plugin_protocol_strings[i].str;
        }
    }
  g_return_val_if_reached (NULL);
}

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
 * Adds an unknown port to the plugin's list.
 */
void
plugin_add_unknown_port (
  Plugin * pl,
  Port *   port);

/**
 * Creates/initializes a plugin and its internal
 * plugin (LV2, etc.)
 * using the given descriptor.
 */
Plugin *
plugin_new_from_descr (
  const PluginDescriptor * descr);

/**
 * Sets the UI refresh rate on the Plugin.
 */
void
plugin_set_ui_refresh_rate (
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
 * Clones the plugin descriptor.
 */
void
plugin_copy_descr (
  const PluginDescriptor * src,
  PluginDescriptor * dest);

/**
 * Clones the plugin descriptor.
 */
PluginDescriptor *
plugin_clone_descr (
  const PluginDescriptor * src);

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

/**
 * Moves the Plugin's automation from one Channel
 * to another.
 */
void
plugin_move_automation (
  Plugin *  pl,
  Channel * prev_ch,
  Channel * ch);

/**
 * Returns if the Plugin has a supported custom
 * UI.
 */
int
plugin_has_supported_custom_ui (
  Plugin * self);

/**
 * Updates the plugin's latency.
 */
void
plugin_update_latency (
  Plugin * pl);

/**
 * Generates automatables for the plugin.
 *
 *
 * Plugin must be instantiated already.
 */
void
plugin_generate_automation_tracks (
  Plugin * plugin);

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
plugin_open_ui (Plugin *plugin);

/**
 * Returns if Plugin exists in MixerSelections.
 */
int
plugin_is_selected (
  Plugin * pl);

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

#endif
