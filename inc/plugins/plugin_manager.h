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
 * Plugin manager.
 */

#ifndef __PLUGINS_PLUGIN_MANAGER_H__
#define __PLUGINS_PLUGIN_MANAGER_H__

#include "plugins/lv2/lv2_urid.h"
#include "utils/symap.h"

#include "zix/sem.h"

#include <lilv/lilv.h>

typedef struct CachedVstDescriptors
  CachedVstDescriptors;

/**
 * @addtogroup plugins
 *
 * @{
 */

#define PLUGIN_MANAGER (ZRYTHM->plugin_manager)
#define LV2_NODES (PLUGIN_MANAGER->lv2_nodes)
#define LILV_WORLD LV2_NODES.lilv_world
#define LV2_GENERATOR_PLUGIN "Generator"
#define LV2_CONSTANT_PLUGIN "Constant"
#define LV2_INSTRUMENT_PLUGIN "Instrument"
#define LV2_OSCILLATOR_PLUGIN "Oscillator"
#define PM_LILV_NODES (PLUGIN_MANAGER->lv2_nodes)
#define PM_URIDS (PLUGIN_MANAGER->urids)
#define PM_SYMAP (PLUGIN_MANAGER->symap)
#define PM_SYMAP_LOCK (PLUGIN_MANAGER->symap_lock)

/**
 * Cached LV2 nodes.
 */
typedef struct Lv2Nodes
{
  LilvWorld *         lilv_world;
  const LilvPlugins * lilv_plugins;
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
  LilvNode *          core_requiredFeature;
  LilvNode *          core_sampleRate;
  LilvNode *          core_symbol;
  LilvNode *          core_toggled;
  LilvNode *          data_access;
  LilvNode *          instance_access;
  LilvNode *          ev_EventPort;
  LilvNode *          patch_Message;
  LilvNode *          patch_readable;
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
  LilvNode *          pprops_trigger;
  LilvNode *          pset_bank;
  LilvNode *          pset_Bank;
  LilvNode *          pset_Preset;
  LilvNode *          rdfs_comment;
  LilvNode *          rdfs_label;
  LilvNode *          rdfs_range;
  LilvNode *          rsz_minimumSize;
  LilvNode *          state_threadSafeRestore;
  LilvNode *          time_Position;
  LilvNode *          ui_external;
  LilvNode *          ui_externalkx;
  LilvNode *          ui_Gtk3UI;
  LilvNode *          ui_GtkUI;
  LilvNode *          ui_Qt4UI;
  LilvNode *          ui_Qt5UI;
  LilvNode *          units_db;
  LilvNode *          units_degree;
  LilvNode *          units_hz;
  LilvNode *          units_midiNote;
  LilvNode *          units_mhz;
  LilvNode *          units_ms;
  LilvNode *          units_render;
  LilvNode *          units_s;
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

  /** Default bank to use for presets that don't
   * belong to a bank. */
  LilvNode *          zrythm_default_bank;
  /** Init preset. */
  LilvNode *          zrythm_default_preset;

  LilvNode* end;  ///< NULL terminator for easy freeing of entire structure
} Lv2Nodes;

typedef struct PluginDescriptor PluginDescriptor;

/**
 * The PluginManager is responsible for scanning
 * and keeping track of available Plugin's.
 */
typedef struct PluginManager
{
  PluginDescriptor *     plugin_descriptors[10000];
  char *                 plugin_categories[50];
  char *                 collections[50]; ///< TODO
  int                    num_plugin_categories;
  int                    num_plugins;
  Lv2Nodes               lv2_nodes;

  /** Cached VST descriptors */
  CachedVstDescriptors * cached_vst_descriptors;

  /** URI map for URID feature. */
  Symap*                 symap;
  /** Lock for URI map. */
  ZixSem                 symap_lock;

  /** URIDs. */
  Lv2URIDs               urids;

  char *                 lv2_path;

} PluginManager;

PluginManager *
plugin_manager_new (void);

/**
 * Scans for plugins, optionally updating the
 * progress.
 *
 * @param max_progress Maximum progress for this
 *   stage.
 * @param progress Pointer to a double (0.0-1.0) to
 *   update based on the current progress.
 */
void
plugin_manager_scan_plugins (
  PluginManager * self,
  const double    max_progress,
  double *        progress);

const PluginDescriptor *
plugin_manager_find_plugin_from_uri (
  PluginManager * self,
  const char *    uri);

void
plugin_manager_free (
  PluginManager * self);

/**
 * @}
 */

#endif
