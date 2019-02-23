/*
 * Copyright (C) 2018-2019 Alexandros Theodotou
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

#include "audio/port.h"
#include "audio/position.h"
#include "plugins/plugin_manager.h"
#include "plugins/lv2/control.h"
#include "plugins/lv2/lv2_evbuf.h"
#include "plugins/lv2/symap.h"
#include "plugins/lv2/worker.h"
#include "plugins/lv2/zix/ring.h"
#include "plugins/lv2/suil.h"
#include "utils/sem.h"
#include "plugins/lv2/zix/thread.h"
#include "plugins/lv2/lv2_external_ui.h"

#include <lilv/lilv.h>
#include <serd/serd.h>

#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include <lv2/lv2plug.in/ns/ext/atom/forge.h>
#include <lv2/lv2plug.in/ns/ext/data-access/data-access.h>
#include <lv2/lv2plug.in/ns/ext/log/log.h>
#include <lv2/lv2plug.in/ns/ext/midi/midi.h>
#include <lv2/lv2plug.in/ns/ext/resize-port/resize-port.h>
#include <lv2/lv2plug.in/ns/ext/state/state.h>
#include <lv2/lv2plug.in/ns/ext/urid/urid.h>
#include <lv2/lv2plug.in/ns/ext/uri-map/uri-map.h>
#include <lv2/lv2plug.in/ns/ext/worker/worker.h>

#include "lv2/lv2plug.in/ns/ext/port-props/port-props.h"
#include "lv2/lv2plug.in/ns/ext/presets/presets.h"
#include "lv2/lv2plug.in/ns/ext/time/time.h"
#include "lv2/lv2plug.in/ns/extensions/ui/ui.h"
#include "lv2/lv2plug.in/ns/extensions/units/units.h"
#include "lv2/lv2plug.in/ns/ext/patch/patch.h"
#include "lv2/lv2plug.in/ns/ext/port-groups/port-groups.h"
#include "lv2/lv2plug.in/ns/ext/buf-size/buf-size.h"
#include "lv2/lv2plug.in/ns/ext/options/options.h"

#include <sratom/sratom.h>

#ifdef __clang__
#    define REALTIME __attribute__((annotate("realtime")))
#else
#    define REALTIME
#endif

typedef struct Lv2Plugin Lv2Plugin;
typedef struct _GtkWidget GtkWidget;
typedef struct _GtkCheckMenuItem GtkCheckMenuItem;

typedef struct {
	LilvNode* atom_AtomPort;
	LilvNode* atom_Chunk;
	LilvNode* atom_Float;
	LilvNode* atom_Path;
	LilvNode* atom_Sequence;
	LilvNode* ev_EventPort;
	LilvNode* lv2_AudioPort;
	LilvNode* lv2_CVPort;
	LilvNode* lv2_ControlPort;
	LilvNode* lv2_InputPort;
	LilvNode* lv2_OutputPort;
	LilvNode* lv2_connectionOptional;
	LilvNode* lv2_control;
	LilvNode* lv2_default;
	LilvNode* lv2_enumeration;
	LilvNode* lv2_integer;
	LilvNode* lv2_maximum;
	LilvNode* lv2_minimum;
	LilvNode* lv2_name;
	LilvNode* lv2_reportsLatency;
	LilvNode* lv2_sampleRate;
	LilvNode* lv2_symbol;
	LilvNode* lv2_toggled;
	LilvNode* midi_MidiEvent;
	LilvNode* pg_group;
	LilvNode* pprops_logarithmic;
	LilvNode* pprops_notOnGUI;
	LilvNode* pprops_rangeSteps;
	LilvNode* pset_Preset;
	LilvNode* pset_bank;
	LilvNode* rdfs_comment;
	LilvNode* rdfs_label;
	LilvNode* rdfs_range;
	LilvNode* rsz_minimumSize;
	LilvNode* work_interface;
	LilvNode* work_schedule;
	LilvNode* ui_externallv;
	LilvNode* ui_externalkx;
	LilvNode* end;  ///< NULL terminator for easy freeing of entire structure
} LV2_Nodes;

/* FIXME these should go to manager */
typedef struct {
	LV2_URID atom_Float;
	LV2_URID atom_Int;
	LV2_URID atom_Object;
	LV2_URID atom_Path;
	LV2_URID atom_String;
	LV2_URID atom_eventTransfer;
	LV2_URID bufsz_maxBlockLength;
	LV2_URID bufsz_minBlockLength;
	LV2_URID bufsz_sequenceSize;
	LV2_URID log_Error;
	LV2_URID log_Trace;
	LV2_URID log_Warning;
	LV2_URID midi_MidiEvent;
	LV2_URID param_sampleRate;
	LV2_URID patch_Get;
	LV2_URID patch_Put;
	LV2_URID patch_Set;
	LV2_URID patch_body;
	LV2_URID patch_property;
	LV2_URID patch_value;
	LV2_URID time_Position;
	LV2_URID time_bar;
	LV2_URID time_barBeat;
	LV2_URID time_beatUnit;
	LV2_URID time_beatsPerBar;
	LV2_URID time_beatsPerMinute;
	LV2_URID time_frame;
	LV2_URID time_speed;
	LV2_URID ui_updateRate;
} LV2_URIDs;

typedef struct LV2_Port
{
  Port            * port;     ///< pointer back to parent port
	const LilvPort* lilv_port;  ///< LV2 port
  void*           sys_port;   ///< For audio/MIDI ports, otherwise NULL
	LV2_Evbuf*      evbuf;      ///< For MIDI ports, otherwise NULL
	void*           widget;     ///< Control widget, if applicable
	size_t          buf_size;   ///< Custom buffer size, or 0
	uint32_t        index;      ///< Port index
	float           control;    ///< For control ports, otherwise 0.0f
	bool            old_api;    ///< True for event, false for atom
} LV2_Port;

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

  LV2_Nodes          nodes;          ///< nodes
	LV2_Atom_Forge     forge;          ///< Atom forge
	Sratom*            sratom;         ///< Atom serialiser
	Sratom*            ui_sratom;      ///< Atom serialiser for UI thread
	Symap*             symap;          ///< URI map
	ZixSem             symap_lock;     ///< Lock for URI map
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
	LV2_Port*          ports;          ///< Port array of size num_ports
	Lv2Controls        controls;       ///< Available plugin controls
	uint32_t           num_ports;      ///< Size of the two following arrays:
	uint32_t           longest_sym;    ///< Longest port symbol (used for aligned console printing)
	uint32_t           plugin_latency; ///< Latency reported by plugin (if any)
	float              ui_update_hz;   ///< Frequency of UI updates
	uint32_t           event_delta_t;  ///< Frames since last update sent to UI
	uint32_t           midi_event_id;  ///< MIDI event class ID in event context
	bool               exit;           ///< True iff execution is finished
	bool               has_ui;         ///< True iff a control UI is present
	bool               request_update; ///< True iff a plugin update is needed
	bool               safe_restore;   ///< Plugin restore() is thread-safe
	uint32_t           control_in;     ///< Index of control input port
  ZixSem exit_sem;  /**< Exit semaphore */
	bool               externalui;     ///< True iff plugin has an external-ui
  LV2_External_UI_Widget* extuiptr;  ///< data structure used for external-ui
  GtkCheckMenuItem* active_preset_item;
  bool              updating;

  LV2_URIDs          urids;        ///< URIDs
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
  void *           host;
  void *           host_on_destroy_cb;
} Lv2Plugin;

int
lv2_printf(LV2_Log_Handle handle,
            LV2_URID       type,
            const char*    fmt, ...);

int
lv2_vprintf(LV2_Log_Handle handle,
             LV2_URID       type,
             const char*    fmt,
             va_list        ap);

static inline bool
lv2_ansi_start(FILE* stream, int color)
{
#ifdef HAVE_ISATTY
	if (isatty(fileno(stream))) {
		return fprintf(stream, "\033[0;%dm", color);
	}
#endif
	return 0;
}

static inline void
lv2_ansi_reset(FILE* stream)
{
#ifdef HAVE_ISATTY
	if (isatty(fileno(stream))) {
		fprintf(stream, "\033[0m");
		fflush(stream);
	}
#endif

int
lv2_printf(LV2_Log_Handle handle,
            LV2_URID       type,
            const char*    fmt, ...);

int
lv2_vprintf(LV2_Log_Handle handle,
             LV2_URID       type,
             const char*    fmt,
             va_list        ap);
}

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

LV2_Port*
lv2_port_by_symbol(Lv2Plugin* plugin, const char* sym);

void
lv2_ui_write(SuilController controller,
              uint32_t       port_index,
              uint32_t       buffer_size,
              uint32_t       protocol,
              const void*    buffer);

uint32_t
lv2_ui_port_index(SuilController controller, const char* symbol);

const char*
lv2_native_ui_type(Lv2Plugin* plugin);

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
lv2_ui_write(SuilController controller,
              uint32_t       port_index,
              uint32_t       buffer_size,
              uint32_t       protocol,
              const void*    buffer);

void
lv2_apply_ui_events(Lv2Plugin* plugin, uint32_t nframes);

uint32_t
lv2_ui_port_index(SuilController controller, const char* symbol);

void
lv2_ui_port_event(Lv2Plugin*       plugin,
                   uint32_t    port_index,
                   uint32_t    buffer_size,
                   uint32_t    protocol,
                   const void* buffer);

bool
lv2_send_to_ui(Lv2Plugin*       plugin,
                uint32_t    port_index,
                uint32_t    type,
                uint32_t    size,
                const void* body);

/* FIXME make separate header for UI stuff */
void
lv2_gtk_set_float_control(const Lv2ControlID* control, float value);

bool
lv2_run(Lv2Plugin* plugin, uint32_t nframes);

bool
lv2_update(Lv2Plugin* plugin);

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
lv2_save_state (Lv2Plugin * lv2_plugin, const char * dir);

/**
 * Frees memory
 */
void
lv2_free (Lv2Plugin * plugin);

#endif
