/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "config.h"

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
#include <serd/serd.h>

#include <lv2/lv2plug.in/ns/ext/data-access/data-access.h>
#include <lv2/lv2plug.in/ns/ext/log/log.h>
#include <lv2/lv2plug.in/ns/ext/options/options.h>
#include <lv2/lv2plug.in/ns/ext/state/state.h>

#include <sratom/sratom.h>

#include <suil/suil.h>

#ifdef __clang__
#    define REALTIME __attribute__((annotate("realtime")))
#else
#    define REALTIME
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

typedef struct Lv2Plugin
{
  LV2_Extension_Data_Feature ext_data;

  LV2_Feature        map_feature;
  LV2_Feature        unmap_feature;
  LV2_Feature        make_path_feature;
  LV2_Feature        sched_feature;
  LV2_Feature        state_sched_feature;
  LV2_Feature        safe_restore_feature;
  LV2_Feature        log_feature;
  LV2_Feature        options_feature;
  LV2_Feature        def_state_feature;

  /** These features have no data */
  LV2_Feature        buf_size_features[3];

  const LV2_Feature* features[11];

  const LV2_Feature* state_features[8];

  LV2_Options_Option options[6];

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
  /** Temporary plugin state directory. */
	char*              temp_dir;
  /** Plugin save directory. */
	char*              save_dir;
  /** Plugin class (RDF data). */
	const LilvPlugin*  lilv_plugin;
  /** Temporary storage. */
  LilvState *        state;
  /** Current preset. */
	LilvState*         preset;
  /** All plugin UIs (RDF data). */
	LilvUIs*           uis;
  /** Plugin UI (RDF data). */
	const LilvUI*      ui;
  /** Plugin UI type (unwrapped). */
	const LilvNode*    ui_type;
  /** Plugin instance (shared library). */
	LilvInstance*      instance;
  /** Plugin UI host support. */
	SuilHost*          ui_host;
  /** Plugin UI instance (shared library). */
	SuilInstance*      ui_instance;

  /**
   * Window (if applicable) (GtkWindow).
   *
   * This is used by both generic UIs and X11/etc
   * UIs.
   */
	void*              window;

  /** ID of the delete-event signal so that we can
   * deactivate before freeing the plugin. */
  gulong             delete_event_id;

  /** Port array of size num_ports. */
	Lv2Port*          ports;
	int                num_ports;

  /** Available Lv2Plugin controls. */
	Lv2Controls        controls;

  /** Latency reported by the Lv2Plugin, if any. */
	uint32_t           plugin_latency;

  /** Update frequencey of the UI, in Hz (times
   * per second). */
	float              ui_update_hz;

  /** Frames since last update sent to UI. */
	uint32_t           event_delta_t;
	uint32_t           midi_event_id;  ///< MIDI event class ID in event context
	bool               exit;           ///< True iff execution is finished

  /** Whether the plugin has its own UI. */
	bool               has_custom_ui;

  /** Whether a plugin update is needed. */
	bool               request_update;
	bool               safe_restore;   ///< Plugin restore() is thread-safe
	int                control_in;     ///< Index of control input port
  ZixSem exit_sem;  /**< Exit semaphore */
	bool               externalui;     ///< True iff plugin has an external-ui
  LV2_External_UI_Widget* extuiptr;  ///< data structure used for external-ui
  GtkCheckMenuItem* active_preset_item;
  bool              updating;

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

  /** For saving/loading the state. */
  char *             state_file;

  /** plugin feature data */
  LV2_State_Make_Path make_path;
  LV2_Worker_Schedule sched;
  LV2_Worker_Schedule ssched;
  LV2_Log_Log         llog;

} Lv2Plugin;

static const cyaml_schema_field_t
  lv2_plugin_fields_schema[] =
{
  CYAML_FIELD_SEQUENCE_COUNT (
    "ports", CYAML_FLAG_POINTER,
    Lv2Plugin, ports, num_ports,
    &lv2_port_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_STRING_PTR (
    "state_file", CYAML_FLAG_POINTER,
    Lv2Plugin, state_file,
    0, CYAML_UNLIMITED),

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
  Lv2Plugin * lv2_plgn);

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

/**
 * Instantiate the plugin.
 *
 * All of the actual initialization is done here.
 * If this is a new plugin, preset_uri should be
 * empty. If the project is being loaded, preset
 * uri should be the state file path.
 *
 * @param self Plugin to instantiate.
 * @param preset_uri URI of preset to load.
 */
int
lv2_plugin_instantiate (
  Lv2Plugin *  plugin,
  char * preset_uri);

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
 * Saves the current state in given dir.
 *
 * Used when saving the project.
 */
int
lv2_plugin_save_state_to_file (
  Lv2Plugin * lv2_plugin,
  const char * dir);

/**
 * Saves the current state to a string (returned).
 *
 * MUST be free'd by caller.
 */
int
lv2_plugin_save_state_to_str (
  Lv2Plugin * lv2_plugin);

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
