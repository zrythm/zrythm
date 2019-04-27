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

#ifndef __PLUGINS_PLUGIN_MANAGER_H__
#define __PLUGINS_PLUGIN_MANAGER_H__

#include <lilv/lilv.h>

#define PLUGIN_MANAGER (&ZRYTHM->plugin_manager)
#define LV2_NODES PLUGIN_MANAGER->lv2_nodes
#define LILV_WORLD LV2_NODES.lilv_world
#define LV2_GENERATOR_PLUGIN "Generator"
#define LV2_CONSTANT_PLUGIN "Constant"
#define LV2_INSTRUMENT_PLUGIN "Instrument"
#define LV2_OSCILLATOR_PLUGIN "Oscillator"
#define IS_LV2_PLUGIN_CATEGORY(p, c) \
  (g_strcmp0 (((Plugin *)p)->descr->category, c) == 0)
#define PM_LILV_NODES (PLUGIN_MANAGER->lv2_nodes)


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

typedef struct Lv2Nodes
{
  LilvWorld *         lilv_world;
  const LilvPlugins * lilv_plugins;
  /** General opts. */
  LV2_Defaults        opts;
	LilvNode *          atom_AtomPort;
	LilvNode *          atom_bufferType;
	LilvNode *          atom_Chunk;
	LilvNode *          atom_eventTransfer;
	LilvNode *          atom_Float;
	LilvNode *          atom_Path;
	LilvNode *          atom_Sequence;
	LilvNode *          atom_supports;
	LilvNode *          bufz_coarseBlockLength;
	LilvNode *          bufz_fixedBlockLength;
	LilvNode *          bufz_powerOf2BlockLength;
	LilvNode *          bufz_nominalBlockLength;
	LilvNode *          core_AudioPort;
	LilvNode *          core_connectionOptional;
	LilvNode *          core_control;
	LilvNode *          core_ControlPort;
	LilvNode *          core_CVPort;
	LilvNode *          core_default;
	LilvNode *          core_designation;
	LilvNode *          core_enumeration;
	LilvNode *          core_freeWheeling;
	LilvNode *          core_index;
	LilvNode *          core_inPlaceBroken;
	LilvNode *          core_InputPort;
	LilvNode *          core_integer;
	LilvNode *          core_isSideChain;
	LilvNode *          core_minimum;
	LilvNode *          core_maximum;
	LilvNode *          core_name;
	LilvNode *          core_OutputPort;
	LilvNode *          core_reportsLatency;
	LilvNode *          core_sampleRate;
	LilvNode *          core_symbol;
	LilvNode *          core_toggled;
	LilvNode *          ev_EventPort;
	LilvNode *          patch_Message;
	LilvNode *          patch_writable;
	LilvNode *          midi_MidiEvent;
	LilvNode *          pg_element;
	LilvNode *          pg_group;
	LilvNode *          pprops_causesArtifacts;
	LilvNode *          pprops_expensive;
	LilvNode *          pprops_logarithmic;
	LilvNode *          pprops_notAutomatic;
	LilvNode *          pprops_notOnGUI;
	LilvNode *          pprops_rangeSteps;
	LilvNode *          pset_bank;
	LilvNode *          pset_Preset;
	LilvNode *          rdfs_comment;
	LilvNode *          rdfs_label;
	LilvNode *          rdfs_range;
	LilvNode *          rsz_minimumSize;
  LilvNode *          state_threadSafeRestore;
	LilvNode *          time_position;
	LilvNode *          ui_external;
	LilvNode *          ui_externalkx;
	LilvNode *          ui_Gtk3UI;
	LilvNode *          ui_GtkUI;
	LilvNode *          units_db;
	LilvNode *          units_hz;
	LilvNode *          units_midiNote;
	LilvNode *          units_render;
	LilvNode *          units_unit;
	LilvNode *          work_interface;
	LilvNode *          work_schedule;
#ifdef LV2_EXTENDED
	LilvNode *          auto_can_write_automatation;
	LilvNode *          auto_automation_control;
	LilvNode *          auto_automation_controlled;
	LilvNode *          auto_automation_controller;
	LilvNode *          inline_display_in_gui;
#endif
	LilvNode* end;  ///< NULL terminator for easy freeing of entire structure
} Lv2Nodes;

typedef struct PluginDescriptor PluginDescriptor;

typedef struct PluginManager
{
  PluginDescriptor *     plugin_descriptors[10000];
  char *                 plugin_categories[50];
  char *                 collections[50]; ///< TODO
  int                    num_plugin_categories;
  int                    num_plugins;
  Lv2Nodes               lv2_nodes;

} PluginManager;

/**
 * Initializes plugin manager.
 */
void
plugin_manager_init (PluginManager * self);

/**
 * Scans for plugins.
 */
void
plugin_manager_scan_plugins (PluginManager * self);

#endif
