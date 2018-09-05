/*
 * plugins/plugin.h - a base plugin for plugins (lv2/vst/ladspa/etc.)
 *                         to inherit from
 *
 * Copyright (C) 2018 Alexandros Theodotou
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

#include "audio/port.h"

#include <jack/jack.h>

/* TODO set reasonable values */
#define MAX_IN_PORTS 260
#define MAX_OUT_PORTS 260
#define MAX_UNKNOWN_PORTS 80

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

enum PluginProtocol
{
 PROT_LV2,
 PROT_DSSI,
 PROT_LADSPA,
 PROT_VST,
 PROT_VST3
};

enum PluginArchitecture
{
  ARCH_32,
  ARCH_64
};

/***
 * A descriptor to be implemented by all plugins
 * This will be used throughout the UI
 */
typedef struct Plugin_Descriptor
{
  char                 * author;
  char                 * name;
  char                 * website;
  //int                  category;           ///< one of the above
  char                 * category;           ///< one of the above
  uint8_t              num_audio_ins;             ///< # of input ports
  uint8_t              num_midi_ins;             ///< # of input ports
  uint8_t              num_audio_outs;            ///< # of output ports
  uint8_t              num_midi_outs;            ///< # of output ports
  uint8_t              num_ctrl_ins;            ///< # of input ctrls
  uint8_t              num_ctrl_outs;            ///< # of output ctrls
  int                  arch;               ///< architecture 32/64bit
  int                  protocol;           ///< VST/LV2/DSSI/LADSPA...
  char                 * path;
  char                 * uri;            ///< for LV2 plugins
} Plugin_Descriptor;

/**
 * The base plugin
 * Inheriting plugins must have this as a child
 */
typedef struct Plugin
{
  void                 * original_plugin;     ///< pointer to original plugin inheriting this base plugin
  Plugin_Descriptor    * descr;                 ///< descriptor
  Port                 * in_ports[MAX_IN_PORTS];           ///< ports coming in as input
  int                  num_in_ports;    ///< counter
  Port                 * out_ports[MAX_OUT_PORTS];           ///< ports going out as output
  int                  num_out_ports;    ///< counter
  Port                 * unknown_ports[MAX_UNKNOWN_PORTS];           ///< ports with unknown direction
  int                  num_unknown_ports;    ///< counter
  Channel              * channel;        ///< pointer to channel it belongs to
  int                  processed;  ///< processed or not yet
} Plugin;

/**
 * Creates a empty plugin.
 *
 * To be filled in by the caller.
 */
Plugin *
plugin_new ();

/**
 * Creates a dummy plugin.
 *
 * Used when filling up channel slots. A dummy plugin has 2 output ports
 * and 2 input ports and a MIDI input.
 */
Plugin *
plugin_new_dummy (Channel * channel    ///< channel it belongs to
            );

/**
 * Instantiates the plugin (e.g. when adding to a channel)
 */
int
plugin_instantiate (Plugin * plugin);

/**
 * Process plugin
 */
void
plugin_process (Plugin * plugin, nframes_t nframes);

/**
 * Returns whether given plugin is a dummy plugin or not.
 */
int
plugin_is_dummy (Plugin *plugin);

/**
 * Frees given plugin, breaks all its port connections, and frees its ports
 * and other internal pointers
 */
void
plugin_free (Plugin *plugin);

#endif

