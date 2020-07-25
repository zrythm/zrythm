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
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
  Copyright 2007-2016 David Robillard <http://drobilla.net>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/**
 * \file
 *
 * LV2 Plugin API.
 */

#ifndef __PLUGINS_LV2_PLUGIN_H__
#define __PLUGINS_LV2_PLUGIN_H__

#include "zrythm-config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_ISATTY
#    include <unistd.h>
#endif

#include "audio/position.h"
#include "audio/port.h"
#include "plugins/lv2/lv2_control.h"
#include "plugins/lv2/lv2_evbuf.h"
#include "plugins/lv2/lv2_worker.h"
#include "plugins/lv2/lv2_port.h"
#include "zix/ring.h"
#include "zix/sem.h"
#include "zix/thread.h"
#include "plugins/lv2/lv2_external_ui.h"

#include <lilv/lilv.h>

#include <lv2/data-access/data-access.h>
#include <lv2/log/log.h>
#include <lv2/options/options.h>
#include <lv2/state/state.h>

#include <sratom/sratom.h>

#include <suil/suil.h>

#ifdef __clang__
#  define REALTIME __attribute__((annotate("realtime")))
#else
#  define REALTIME
#endif

typedef struct Lv2Plugin Lv2Plugin;
typedef struct _GtkWidget GtkWidget;
typedef struct _GtkCheckMenuItem GtkCheckMenuItem;
typedef struct Port Port;
typedef struct Plugin Plugin;
typedef struct PluginDescriptor PluginDescriptor;

/**
 * @addtogroup lv2
 *
 * @{
 */

#define LV2_PLUGIN_MAGIC 58173672
#define IS_LV2_PLUGIN(tr) \
  (tr && tr->magic == LV2_PLUGIN_MAGIC)

/**
 * LV2 plugin.
 */
typedef struct Lv2Plugin
{
  LV2_Extension_Data_Feature ext_data;

  LV2_Feature        map_feature;
  LV2_Feature        unmap_feature;
  LV2_Feature        make_path_feature_save;
  LV2_Feature        make_path_feature_temp;
  LV2_Feature        sched_feature;
  LV2_Feature        state_sched_feature;
  LV2_Feature        safe_restore_feature;
  LV2_Feature        log_feature;
  LV2_Feature        options_feature;
  LV2_Feature        def_state_feature;

  /** These features have no data */
  LV2_Feature        buf_size_features[3];

  const LV2_Feature* features[11];

  /** These are the features that are passed to
   * state extension calls, such as when saving
   * the state. */
  const LV2_Feature* state_features[8];

  LV2_Options_Option options[6];

  /** Plugin <=> UI communication buffer size. */
  uint32_t           comm_buffer_size;

  /** Atom forge. */
  LV2_Atom_Forge     forge;
  /** Atom serializer */
  Sratom*            sratom;
  /** Atom serializer for UI thread. */
  Sratom*            ui_sratom;
  /** Port events from UI to plugin. */
  ZixRing*           ui_to_plugin_events;
  /** Port events from plugin to UI. */
  ZixRing*           plugin_to_ui_events;
  /** Buffer for readding UI port events. */
  void*              ui_event_buf;
  /** Worker thread implementation. */
  LV2_Worker  worker;
  /** Synchronous worker for state restore. */
  LV2_Worker  state_worker;
  /** Lock for plugin work() method. */
  ZixSem             work_lock;
  /** Exit semaphore. */
  ZixSem*            done;
  /** Plugin class (RDF data). */
  const LilvPlugin*  lilv_plugin;
  /** Current preset. */
  LilvState*         preset;
  /** All plugin UIs (RDF data). */
  LilvUIs*           uis;
  /** Plugin UI (RDF data). */
  const LilvUI*      ui;
  /** The native UI type for this plugin. */
  const LilvNode*    ui_type;
  /** Plugin instance (shared library). */
  LilvInstance*      instance;
  /** Plugin UI host support. */
  SuilHost*          ui_host;
  /** Plugin UI instance (shared library). */
  SuilInstance*      ui_instance;

  /**
   * Temporary plugin state directory (absolute
   * path).
   *
   * This is created at runtime and remembered.
   */
  char *             temp_dir;

  /** Port array of size num_ports. */
  Lv2Port*          ports;
  int                num_ports;

  /** Available Lv2Plugin controls. */
  Lv2Controls        controls;

  /** Frames since last update sent to UI. */
  uint32_t           event_delta_t;
  uint32_t           midi_event_id;  ///< MIDI event class ID in event context
  bool               exit;           ///< True iff execution is finished

  /** Whether the plugin has its own UI. */
  bool               has_custom_ui;

  /** Whether a plugin update is needed. */
  bool               request_update;

  /** Whether plugin restore() is thread-safe. */
  bool               safe_restore;
  int                control_in;     ///< Index of control input port
  ZixSem exit_sem;  /**< Exit semaphore */

  /** Whether the plugin has at least 1 atom port
   * that supports position. */
  bool               want_position;

  /** Whether the plugin has an external UI. */
  bool               has_external_ui;

  /** Data structure used for external UIs. */
  LV2_External_UI_Widget* external_ui_widget;

  bool               updating;

  /** URI => Int map. */
  LV2_URID_Map       map;

  /** Int => URI map. */
  LV2_URID_Unmap     unmap;

  /** Environment for RDF printing. */
  SerdEnv*           env;

  /** Transport was rolling or not last cycle. */
  int                rolling;

  /** Global (start) frames the plugin was last
   * processed at. */
  long               gframes;

  /** Last BPM known by the plugin. */
  float              bpm;

  /** Base Plugin instance (parent). */
  Plugin *           plugin;

  /* ---- plugin feature data ---- */

  /** Make path feature data. */
  LV2_State_Make_Path make_path_save;
  LV2_State_Make_Path make_path_temp;

  LV2_Worker_Schedule sched;
  LV2_Worker_Schedule ssched;
  LV2_Log_Log         llog;

  int                 magic;

} Lv2Plugin;

static const cyaml_schema_field_t
  lv2_plugin_fields_schema[] =
{
  CYAML_FIELD_SEQUENCE_COUNT (
    "ports", CYAML_FLAG_POINTER,
    Lv2Plugin, ports, num_ports,
    &lv2_port_schema, 0, CYAML_UNLIMITED),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
  lv2_plugin_schema =
{
  CYAML_VALUE_MAPPING (CYAML_FLAG_POINTER,
  Lv2Plugin, lv2_plugin_fields_schema),
};

void
lv2_plugin_init_loaded (
  Lv2Plugin * self,
  bool        project);

/**
 * Returns a newly allocated plugin descriptor for
 * the given LilvPlugin
 * if it can be hosted, otherwise NULL.
 */
PluginDescriptor *
lv2_plugin_create_descriptor_from_lilv (
  const LilvPlugin * lp);

/**
 * Creates an LV2 plugin from given uri.
 *
 * Used when populating the plugin browser.
 *
 * @param plugin A newly created Plugin with its
 *   descriptor filled in.
 * @param uri The URI.
 */
Lv2Plugin *
lv2_plugin_new_from_uri (
  Plugin    *  plugin,
  const char * uri);

static inline int
lv2_plugin_has_custom_ui (
  Lv2Plugin * self)
{
  return self->ui != NULL;
}

/**
 * Instantiate the plugin.
 *
 * All of the actual initialization is done here.
 * If this is a new plugin, preset_uri should be
 * empty. If the project is being loaded, preset
 * uri should be the state file path.
 *
 * @param self Plugin to instantiate.
 * @param use_state_file Whether to use the plugin's
 *   state file to instantiate the plugin.
 * @param preset_uri URI of preset to load.
 * @param state State to load, if loading from
 *   a state. This is used when cloning plugins
 *   for example. The state of the original plugin
 *   is passed here.
 *
 * @return 0 if OK, non-zero if error.
 */
int
lv2_plugin_instantiate (
  Lv2Plugin *  self,
  bool         project,
  bool         use_state_file,
  char *       preset_uri,
  LilvState *  state);

/**
 * Creates a new LV2 plugin using the given Plugin
 * instance.
 *
 * The given plugin instance must be a newly
 * allocated one.
 *
 * @param plugin A newly allocated Plugin instance.
 */
Lv2Plugin *
lv2_plugin_new (
  Plugin *plugin);

/**
 * Processes the plugin for this cycle.
 *
 * @param g_start_frames The global start frames.
 * @param nframes The number of frames to process.
 */
void
lv2_plugin_process (
  Lv2Plugin * lv2_plugin,
  const long  g_start_frames,
  const nframes_t   nframes);

/**
 * Returns the plugin's latency in samples.
 *
 * This will be 0 if the plugin does not report
 * latency.
 */
nframes_t
lv2_plugin_get_latency (
  Lv2Plugin * pl);

/**
 * In order of preference.
 */
typedef enum Lv2PluginPickUiFlag
{
  /** Plugin UI wrappable using Suil. */
  LV2_PLUGIN_UI_WRAPPABLE,

  /** External/KxExternal UI. */
  LV2_PLUGIN_UI_EXTERNAL,

  /** Gtk2. */
  LV2_PLUGIN_UI_FOR_BRIDGING,
} Lv2PluginPickUiFlag;

/**
 * Pick the most preferable UI.
 *
 * @param[out] ui (Output) UI of the specific
 *   plugin.
 * @param[out] ui_type UI type (eg, X11).
 *
 * @return Whether a UI was picked.
 */
bool
lv2_plugin_pick_ui (
  const LilvUIs *     uis,
  Lv2PluginPickUiFlag flag,
  const LilvUI **     out_ui,
  const LilvNode **   out_ui_type);

bool
lv2_plugin_ui_type_is_external (
  const LilvNode * ui_type);

Lv2Control*
lv2_plugin_get_control_by_symbol (
  Lv2Plugin * plugin,
  const char* sym);

/**
 * Function to get a port value.
 *
 * Used when saving the state.
 * This function MUST set size and type
 * appropriately.
 */
const void *
lv2_plugin_get_port_value (
  const char * port_sym,
  void       * user_data,
  uint32_t   * size,
  uint32_t   * type);

char *
lv2_plugin_get_abs_state_file_path (
  Lv2Plugin * self,
  bool        is_backup);

/**
 * Updates theh PortIdentifier's in the Lv2Plugin.
 */
void
lv2_plugin_update_port_identifiers (
  Lv2Plugin * self);

/**
 * Allocate port buffers (only necessary for MIDI).
 */
void
lv2_plugin_allocate_port_buffers (
  Lv2Plugin* plugin);

int
lv2_plugin_activate (
  Lv2Plugin * self,
  bool        activate);

/**
 * Populates the banks in the plugin instance.
 */
void
lv2_plugin_populate_banks (
  Lv2Plugin * self);

int
lv2_plugin_cleanup (
  Lv2Plugin * self);

/**
 * Frees the Lv2Plugin and all its components.
 */
void
lv2_plugin_free (
  Lv2Plugin * plugin);

/**
 * @}
 */

#endif
