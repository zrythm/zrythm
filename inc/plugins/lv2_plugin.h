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
/*
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

#ifndef __PLUGINS_LV2_PLUGIN_H__
#define __PLUGINS_LV2_PLUGIN_H__

/** \file
 * LV2 Plugin API. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_ISATTY
#    include <unistd.h>
#endif

#include "audio/position.h"
#include "audio/port.h"
#include "plugins/lv2/control.h"
#include "plugins/lv2/lv2_evbuf.h"
#include "plugins/lv2/worker.h"
#include "zix/ring.h"
#include "plugins/lv2/suil.h"
#include "zix/sem.h"
#include "zix/thread.h"
#include "plugins/lv2/lv2_external_ui.h"

#include <lilv/lilv.h>
#include <serd/serd.h>

//#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
//#include <lv2/lv2plug.in/ns/ext/atom/forge.h>
#include <lv2/lv2plug.in/ns/ext/data-access/data-access.h>
#include <lv2/lv2plug.in/ns/ext/log/log.h>
#include <lv2/lv2plug.in/ns/ext/options/options.h>
#include <lv2/lv2plug.in/ns/ext/state/state.h>
//#include <lv2/lv2plug.in/ns/ext/urid/urid.h>
//#include <lv2/lv2plug.in/ns/ext/uri-map/uri-map.h>
//#include <lv2/lv2plug.in/ns/ext/worker/worker.h>

//#include "lv2/lv2plug.in/ns/extensions/ui/ui.h"

#include <sratom/sratom.h>

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

/* FIXME these should go to manager */

typedef struct Lv2Port
{
  /** Pointer back to parent port. */
  PortIdentifier  port_id;
  Port *          port;     ///< cache
	const LilvPort* lilv_port;  ///< LV2 port
  void*           sys_port;   ///< For audio/MIDI ports, otherwise NULL
	LV2_Evbuf*      evbuf;      ///< For MIDI ports, otherwise NULL
	void*           widget;     ///< Control widget, if applicable
	size_t          buf_size;   ///< Custom buffer size, or 0
	uint32_t        index;      ///< Port index
	float           control;    ///< For control ports, otherwise 0.0f
	bool            old_api;    ///< True for event, false for atom
} Lv2Port;

typedef struct Lv2Plugin
{
  LV2_Extension_Data_Feature ext_data;

  LV2_Feature map_feature;
  LV2_Feature unmap_feature;
  LV2_Feature make_path_feature;
  LV2_Feature sched_feature;
  LV2_Feature state_sched_feature;
  LV2_Feature safe_restore_feature;
  LV2_Feature log_feature;
  LV2_Feature options_feature;
  LV2_Feature def_state_feature;

  /** These features have no data */
  LV2_Feature buf_size_features[3];

  const LV2_Feature* features[11];

  const LV2_Feature* state_features[8];

  LV2_Options_Option options[6];

	LV2_Atom_Forge     forge;          ///< Atom forge
	Sratom*            sratom;         ///< Atom serialiser
	Sratom*            ui_sratom;      ///< Atom serialiser for UI thread
	ZixRing*           ui_events;      ///< Port events from UI
	ZixRing*           plugin_events;  ///< Port events from plugin
	void*              ui_event_buf;   ///< Buffer for reading UI port events
	LV2_Worker  worker;         ///< Worker thread implementation
	LV2_Worker  state_worker;   ///< Synchronous worker for state restore
	ZixSem             work_lock;      ///< Lock for plugin work() method
  ZixSem*            done;           ///< Exit semaphore
	char*              temp_dir;       ///< Temporary plugin state directory
	char*              save_dir;       ///< Plugin save directory
	const LilvPlugin*  lilv_plugin;         ///< Plugin class (RDF data)
  LilvState *        state;         ///< temporary storage
	LilvState*         preset;         ///< Current preset
	LilvUIs*           uis;            ///< All plugin UIs (RDF data)
	const LilvUI*      ui;             ///< Plugin UI (RDF data)
	const LilvNode*    ui_type;        ///< Plugin UI type (unwrapped)
	LilvInstance*      instance;       ///< Plugin instance (shared library)
	SuilHost*          ui_host;        ///< Plugin UI host support
	SuilInstance*      ui_instance;    ///< Plugin UI instance (shared library)
	void*              window;         ///< Window (if applicable) (GtkWindow)
  /** ID of the delete-event signal so that we can
   * deactivate before freeing the plugin. */
  gulong             delete_event_id;
	Lv2Port*          ports;          ///< Port array of size num_ports
	Lv2Controls        controls;       ///< Available plugin controls
	int                num_ports;      ///< Size of the two following arrays:
	uint32_t           plugin_latency; ///< Latency reported by plugin (if any)
	float              ui_update_hz;   ///< Frequency of UI updates
	uint32_t           event_delta_t;  ///< Frames since last update sent to UI
	uint32_t           midi_event_id;  ///< MIDI event class ID in event context
	bool               exit;           ///< True iff execution is finished
	bool               has_ui;         ///< True iff a control UI is present
	bool               request_update; ///< True iff a plugin update is needed
	bool               safe_restore;   ///< Plugin restore() is thread-safe
	int                control_in;     ///< Index of control input port
  ZixSem exit_sem;  /**< Exit semaphore */
	bool               externalui;     ///< True iff plugin has an external-ui
  LV2_External_UI_Widget* extuiptr;  ///< data structure used for external-ui
  GtkCheckMenuItem* active_preset_item;
  bool              updating;
	LV2_URID_Map       map;            ///< URI => Int map
	LV2_URID_Unmap     unmap;          ///< Int => URI map
	SerdEnv*           env;            ///< Environment for RDF printing

  /* transport related */
  int                rolling;
  Position           pos;
  int                bpm;

  Plugin             * plugin;           ///< base plugin instance (parent)

  char               * state_file; ///< for saving/loading state

  /** plugin feature data */
  LV2_State_Make_Path make_path;
  LV2_Worker_Schedule sched;
  LV2_Worker_Schedule ssched;
  LV2_Log_Log         llog;

} Lv2Plugin;

static const cyaml_schema_field_t
  lv2_port_fields_schema[] =
{
  CYAML_FIELD_MAPPING (
    "port_id", CYAML_FLAG_DEFAULT,
    Lv2Port, port_id, port_identifier_fields_schema),

	CYAML_FIELD_END
};

static const cyaml_schema_value_t
  lv2_port_schema =
{
	CYAML_VALUE_MAPPING (CYAML_FLAG_DEFAULT,
  Lv2Port, lv2_port_fields_schema),
};

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
lv2_plugin_init_loaded (Lv2Plugin * lv2_plgn);

char*
lv2_make_path(LV2_State_Make_Path_Handle handle,
               const char*                path);

void
lv2_save(Lv2Plugin* plugin, const char* dir);

typedef int (*PresetSink)(Lv2Plugin*           plugin,
                          const LilvNode* node,
                          const LilvNode* title,
                          void*           data);

int
lv2_load_presets(Lv2Plugin* plugin, PresetSink sink, void* data);

int
lv2_unload_presets(Lv2Plugin* plugin);

void
lv2_apply_state(Lv2Plugin* plugin, LilvState* state);

int
lv2_apply_preset(Lv2Plugin* plugin, const LilvNode* preset);

int
lv2_save_preset(Lv2Plugin*       plugin,
                 const char* dir,
                 const char* uri,
                 const char* label,
                 const char* filename);

int
lv2_delete_current_preset(Lv2Plugin* plugin);

static inline char*
lv2_strdup(const char* str)
{
	const size_t len  = strlen(str);
	char*        copy = (char*)malloc(len + 1);
	memcpy(copy, str, len + 1);
	return copy;
}

/**
 * Joins two strings.
 *
 * Used in the state when creating paths. Can
 * probably get rid of this and use glib.
 */
static inline char*
lv2_strjoin(const char* a, const char* b)
{
	const size_t a_len = strlen(a);
	const size_t b_len = strlen(b);
	char* const  out   = (char*)malloc(a_len + b_len + 1);

	memcpy(out,         a, a_len);
	memcpy(out + a_len, b, b_len);
	out[a_len + b_len] = '\0';

	return out;
}

Lv2Port*
lv2_port_by_symbol(Lv2Plugin* plugin, const char* sym);

void
lv2_ui_write(SuilController  controller,
              uint32_t      port_index,
              uint32_t      buffer_size,
              uint32_t       protocol,
              const void*    buffer);

uint32_t
lv2_ui_port_index(SuilController controller, const char* symbol);

bool
lv2_discover_ui(Lv2Plugin* plugin);

int
lv2_open_ui(Lv2Plugin* plugin);

void
lv2_init_ui(Lv2Plugin* plugin);

int
lv2_close_ui(Lv2Plugin* plugin);

void
lv2_ui_instantiate(Lv2Plugin*       plugin,
                    const char* native_ui_type,
                    void*       parent);

bool
lv2_ui_is_resizable(Lv2Plugin* plugin);

void
lv2_apply_ui_events(Lv2Plugin* plugin, uint32_t nframes);

uint32_t
lv2_ui_port_index(SuilController controller, const char* symbol);

bool
lv2_send_to_ui(Lv2Plugin*       plugin,
                uint32_t    port_index,
                uint32_t    type,
                uint32_t    size,
                const void* body);

bool
lv2_plugin_run(Lv2Plugin* plugin, uint32_t nframes);

int
lv2_plugin_update (gpointer data);

int
lv2_ui_resize(Lv2Plugin* plugin, int width, int height);


/**
   Allocate port buffers (only necessary for MIDI).
*/
void
lv2_allocate_port_buffers(Lv2Plugin* plugin);

/** Expose a port to the system (if applicable) and connect it to its buffer. */
void
lv2_backend_activate_port(Lv2Plugin* plugin, uint32_t port_index);

/**
 * Returns a newly allocated plugin descriptor for the given LilvPlugin
 * if it can be hosted, otherwise NULL.
 */
PluginDescriptor *
lv2_create_descriptor_from_lilv (const LilvPlugin * lp);

/**
 * Creates an LV2 plugin from given uri.
 *
 * Used when populating the plugin browser.
 */
Lv2Plugin *
lv2_create_from_uri (Plugin    * plugin,  ///< a newly created plugin, with its descriptor filled in
                     const char * uri ///< the uri
                     );

/**
 * Creates an LV2 plugin from a state file.
 */
Lv2Plugin *
lv2_create_from_state (Plugin    * plugin,  ///< a newly created plugin
                       const char * _path    ///< path for state to load
                       );

/**
 * Loads an LV2 plugin from its state file.
 *
 * Used when loading project files.
 */
Lv2Plugin *
lv2_load_from_state (Plugin    * plugin);  ///< a newly created plugin

/**
 * Instantiate the LV2 plugin.
 *
 * This is where the work is done.
 */
int
lv2_instantiate (Lv2Plugin      * plugin,   ///< plugin to instantiate
                 char            * preset_uri   ///< uri of preset to load
                );

/**
 * Creates a new LV2 plugin using the given Plugin instance.
 *
 * The given plugin instance must be a newly allocated one.
 */
Lv2Plugin *
lv2_new (Plugin *plugin ///< a newly allocated plugin instance
         );

/** TODO */
void
lv2_cleanup (Lv2Plugin *plugin);

/**
 * Processes the plugin for this cycle.
 */
void
lv2_plugin_process (Lv2Plugin * lv2_plugin);

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
 * Frees memory
 */
void
lv2_free (Lv2Plugin * plugin);

#endif
