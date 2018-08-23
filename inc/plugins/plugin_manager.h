/*
 * plugins/plugin_manager.h - Manages plugins
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

#ifndef __PLUGINS_PLUGIN_MANAGER_H__
#define __PLUGINS_PLUGIN_MANAGER_H__

#include <lilv/lilv.h>

#define PLUGIN_MANAGER zrythm_system->plugin_manager
#define LV2_SETTINGS PLUGIN_MANAGER->lv2_settings
#define LILV_WORLD LV2_SETTINGS.lilv_world

typedef struct Plugin Plugin;

typedef struct
{
	char*    name;              ///< Client name
	int      name_exact;        ///< Exit if name is taken
	char*    uuid;              ///< Session UUID
	char**   controls;          ///< Control values
	uint32_t buffer_size;       ///< Plugin <= >UI communication buffer size
	double   update_rate;       ///< UI update rate in Hz
	int      dump;              ///< Dump communication iff true
	int      trace;             ///< Print trace log iff true
	int      generic_ui;        ///< Use generic UI iff true
	int      show_hidden;       ///< Show controls for notOnGUI ports
	int      no_menu;           ///< Hide menu iff true
	int      show_ui;           ///< Show non-embedded UI
	int      print_controls;    ///< Print control changes to stdout
	int      non_interactive;   ///< Do not listen for commands on stdin
} LV2_Defaults;

typedef struct LV2_Settings
{
  LilvWorld             * lilv_world;
  const LilvPlugins           * lilv_plugins;
  LV2_Defaults           opts;         ///< general opts
	LilvNode * atom_AtomPort;
	LilvNode * atom_Chunk;
	LilvNode * atom_Sequence;
	LilvNode * atom_bufferType;
	LilvNode * atom_supports;
	LilvNode * atom_eventTransfer;
	LilvNode * ev_EventPort;
	LilvNode * ext_logarithmic;
	LilvNode * ext_notOnGUI;
	LilvNode * ext_expensive;
	LilvNode * ext_causesArtifacts;
	LilvNode * ext_notAutomatic;
	LilvNode * ext_rangeSteps;
	LilvNode * groups_group;
	LilvNode * groups_element;
	LilvNode * lv2_AudioPort;
	LilvNode * lv2_ControlPort;
	LilvNode * lv2_InputPort;
	LilvNode * lv2_OutputPort;
	LilvNode * lv2_inPlaceBroken;
	LilvNode * lv2_isSideChain;
	LilvNode * lv2_index;
	LilvNode * lv2_integer;
	LilvNode * lv2_default;
	LilvNode * lv2_minimum;
	LilvNode * lv2_maximum;
	LilvNode * lv2_reportsLatency;
	LilvNode * lv2_sampleRate;
	LilvNode * lv2_toggled;
	LilvNode * lv2_designation;
	LilvNode * lv2_enumeration;
	LilvNode * lv2_freewheeling;
	LilvNode * midi_MidiEvent;
	LilvNode * rdfs_comment;
	LilvNode * rdfs_label;
	LilvNode * rdfs_range;
	LilvNode * rsz_minimumSize;
	LilvNode * time_Position;
	LilvNode * ui_GtkUI;
	LilvNode * ui_external;
	LilvNode * ui_externalkx;
	LilvNode * units_unit;
	LilvNode * units_render;
	LilvNode * units_hz;
	LilvNode * units_midiNote;
	LilvNode * units_db;
	LilvNode * patch_writable;
	LilvNode * patch_Message;
#ifdef LV2_EXTENDED
	LilvNode * lv2_noSampleAccurateCtrl; // deprecated 2016-09-18
	LilvNode * auto_can_write_automatation;
	LilvNode * auto_automation_control;
	LilvNode * auto_automation_controlled;
	LilvNode * auto_automation_controller;
	LilvNode * inline_display_in_gui;
#endif
#ifdef HAVE_LV2_1_2_0
	LilvNode * bufz_powerOf2BlockLength;
	LilvNode * bufz_fixedBlockLength;
	LilvNode * bufz_nominalBlockLength;
	LilvNode * bufz_coarseBlockLength;
#endif
} LV2_Settings;

typedef struct Plugin_Manager
{
  Plugin         * plugins[400];
  int            num_plugins;
  LV2_Settings   lv2_settings;
} Plugin_Manager;

/**
 * Initializes plugin manager and scans for plugins
 */
void
plugin_manager_init ();

#endif
