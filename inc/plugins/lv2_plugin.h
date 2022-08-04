// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * ---
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

  ---
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
#  include <unistd.h>
#endif

#include "audio/port.h"
#include "audio/position.h"
#include "plugins/lv2/ext/host_info.h"
#include "plugins/lv2/lv2_evbuf.h"
#include "plugins/lv2/lv2_external_ui.h"
#include "plugins/lv2/lv2_worker.h"

#include "zix/ring.h"
#include "zix/sem.h"
#include "zix/thread.h"
#include <lilv/lilv.h>
#include <lv2/data-access/data-access.h>
#include <lv2/log/log.h>
#include <lv2/options/options.h>
#include <lv2/state/state.h>
#include <lv2/urid/urid.h>
#include <sratom/sratom.h>
#include <suil/suil.h>

typedef struct Lv2Plugin         Lv2Plugin;
typedef struct _GtkWidget        GtkWidget;
typedef struct _GtkCheckMenuItem GtkCheckMenuItem;
typedef struct Port              Port;
typedef struct Plugin            Plugin;
typedef struct PluginDescriptor  PluginDescriptor;

/**
 * @addtogroup lv2
 *
 * @{
 */

#define LV2_PLUGIN_MAGIC 58173672
#define IS_LV2_PLUGIN(tr) \
  (tr && tr->magic == LV2_PLUGIN_MAGIC)

#define LV2_ZRYTHM__defaultBank \
  "https://lv2.zrythm.org#default-bank"
#define LV2_ZRYTHM__initPreset \
  "https://lv2.zrythm.org#init-preset"
#define LV2_KX__externalUi \
  "http://kxstudio.sf.net/ns/lv2ext/" \
  "external-ui#Widget"

/* currently missing from the spec */
#ifndef LV2_CORE__enabled
#  define LV2_CORE__enabled LV2_CORE_PREFIX "enabled"
#endif
#ifndef LV2_CORE__isSideChain
#  define LV2_CORE__isSideChain LV2_CORE_PREFIX "isSideChain"
#endif

#define LV2_PARAM_MAX_STR_LEN 1200

#define lv2_plugin_is_in_active_project(self) \
  (plugin_is_in_active_project ((self)->plugin))

/**
 * Used temporarily to transfer data.
 */
typedef struct Lv2Parameter
{
  /** URI URID. */
  LV2_URID urid;

  /** Value type URID (forge.Bool, forge.Int,
   * etc.). */
  LV2_URID value_type_urid;
  bool     readable;
  bool     writable;
  char     symbol[LV2_PARAM_MAX_STR_LEN];
  char     label[LV2_PARAM_MAX_STR_LEN];
  char     comment[LV2_PARAM_MAX_STR_LEN];

  /** Whether the ranges below are valid. */
  bool has_range;

  /** Value range. */
  float minf;
  float maxf;
  float deff;
} Lv2Parameter;

/**
 * Control change event, sent through ring buffers
 * for UI updates.
 */
typedef struct
{
  uint32_t index;
  uint32_t protocol;
  uint32_t size;
  uint8_t  body[];
} Lv2ControlChange;

/**
 * LV2 plugin.
 */
typedef struct Lv2Plugin
{
  LV2_Feature map_feature;
  LV2_Feature unmap_feature;
  LV2_Feature make_path_temp_feature;
  LV2_Feature sched_feature;
  LV2_Feature state_sched_feature;
  LV2_Feature safe_restore_feature;
  LV2_Feature log_feature;
  LV2_Feature options_feature;
  LV2_Feature def_state_feature;
  LV2_Feature hard_rt_capable_feature;
  LV2_Feature data_access_feature;
  LV2_Feature instance_access_feature;
  LV2_Feature bounded_block_length_feature;

  /**
   * Supported features passed when instantiating
   * plugins.
   */
  const LV2_Feature * features[13];

  /**
   * Features that are passed to state extension
   * calls, such as when saving the state.
   */
  const LV2_Feature * state_features[7];

  /**
   * Options to pass to plugin on instantiation.
   */
  LV2_Options_Option options[10];

  /**
   * Data access feature.
   *
   * An instance of this will be passed to the plugin
   * UI after instantiating the plugin, if the
   * plugin requires data access.
   *
   * @seealso https://lv2plug.in/ns/ext/data-access.
   */
  LV2_Extension_Data_Feature ext_data_feature;

  /** Plugin <=> UI communication buffer size. */
  uint32_t comm_buffer_size;

  /**
   * Options interface for setting plugin options
   * dynamically.
   *
   * @seealso http://lv2plug.in/ns/ext/options#interface.
   */
  const LV2_Options_Interface * options_iface;

  /** Atom forge (main/GTK thread). */
  LV2_Atom_Forge main_forge;
  /** Atom forge (DSP thread). */
  LV2_Atom_Forge dsp_forge;
  /** Atom serializer */
  Sratom * sratom;
  /** Atom serializer for UI thread. */
  Sratom * ui_sratom;
  /** Port events from UI to plugin. */
  ZixRing * ui_to_plugin_events;
  /** Port events from plugin to UI. */
  ZixRing * plugin_to_ui_events;
  /** Buffer for readding UI port events. */
  void * ui_event_buf;
  /** Worker thread implementation. */
  Lv2Worker worker;
  /** Synchronous worker for state restore. */
  Lv2Worker state_worker;
  /** Lock for plugin work() method. */
  ZixSem work_lock;
  /** Plugin class (RDF data). */
  const LilvPlugin * lilv_plugin;
  /** Current preset. */
  LilvState * preset;

  /** Plugin instance (shared library). */
  LilvInstance * instance;

  /** Plugin UI host support. */
  SuilHost * suil_host;
  /** Plugin UI instance (shared library). */
  SuilInstance * suil_instance;

  /**
   * Temporary plugin state directory (absolute
   * path).
   *
   * This is created at runtime and remembered.
   */
  char * temp_dir;

  /** Frames since last update sent to UI. */
  uint32_t event_delta_t;
  uint32_t
    midi_event_id; ///< MIDI event class ID in event context
  bool exit;       ///< True iff execution is finished

  /** Whether a plugin update is needed. */
  bool request_update;

  /** Whether plugin restore() is thread-safe. */
  bool safe_restore;

  /**
   * Whether the plugin has a default state that
   * must be loaded before running run() for the
   * first time.
   *
   * @note Set but not used - lilv handles this
   *   feature automatically.
   */
  bool has_default_state;

  /**
   * Index of control input port, or -1 if no port
   * with "control" designation found.
   *
   * @seealso http://lv2plug.in/ns/lv2core#control.
   */
  int control_in;

  /**
   * Index of enabled port, or -1 if no port with
   * "enabled" designation found.
   *
   * @seealso http://lv2plug.in/ns/lv2core#enabled.
   */
  int enabled_in;

  /** Exit semaphore. */
  ZixSem exit_sem;

  /** Whether the plugin has at least 1 atom port
   * that supports position. */
  bool want_position;

  /** Whether the plugin has an external UI. */
  bool has_external_ui;

  /** Data structure used for external UIs. */
  LV2_External_UI_Widget * external_ui_widget;

  bool updating;

  /** URI => Int map. */
  LV2_URID_Map map;

  /** Int => URI map. */
  LV2_URID_Unmap unmap;

  /** Environment for RDF printing. */
  SerdEnv * env;

  /** Transport was rolling or not last cycle. */
  int rolling;

  /** Global (start) frames the plugin was last
   * processed at. */
  unsigned_frame_t gframes;

  /** Last BPM known by the plugin. */
  float bpm;

  /** Base Plugin instance (parent). */
  Plugin * plugin;

  /** Used for external UIs. */
  LV2_External_UI_Host extui;

  /* ---- plugin feature data ---- */

  /** Make path feature data. */
  LV2_State_Make_Path make_path_temp;

  /** Plugin worker schedule. */
  LV2_Worker_Schedule sched;

  /** State worker schedule. */
  LV2_Worker_Schedule ssched;

  /** Log. */
  LV2_Log_Log llog;

  /* ---- end plugin feature data ---- */

  int magic;

} Lv2Plugin;

#if 0
static const cyaml_schema_field_t
  lv2_plugin_fields_schema[] =
{
  YAML_FIELD_DYN_PTR_ARRAY_VAR_COUNT (
    Lv2Plugin, ports, lv2_port_schema),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
  lv2_plugin_schema =
{
  YAML_VALUE_PTR (
    Lv2Plugin, lv2_plugin_fields_schema),
};
#endif

NONNULL
void
lv2_plugin_init_loaded (Lv2Plugin * self);

/**
 * Returns a newly allocated plugin descriptor for
 * the given LilvPlugin
 * if it can be hosted, otherwise NULL.
 */
NONNULL
PluginDescriptor *
lv2_plugin_create_descriptor_from_lilv (const LilvPlugin * lp);

/**
 * Creates an LV2 plugin from given uri.
 *
 * Used when populating the plugin browser.
 *
 * @param plugin A newly created Plugin with its
 *   descriptor filled in.
 * @param uri The URI.
 */
NONNULL_ARGS (1, 2)
Lv2Plugin *
lv2_plugin_new_from_uri (
  Plugin *     plugin,
  const char * uri,
  GError **    error);

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
NONNULL_ARGS (1)
int
lv2_plugin_instantiate (
  Lv2Plugin * self,
  bool        use_state_file,
  char *      preset_uri,
  LilvState * state,
  GError **   error);

/**
 * Creates a new LV2 plugin using the given Plugin
 * instance.
 *
 * The given plugin instance must be a newly
 * allocated one.
 *
 * @param plugin A newly allocated Plugin instance.
 */
NONNULL
Lv2Plugin *
lv2_plugin_new (Plugin * plugin);

/**
 * Processes the plugin for this cycle.
 */
NONNULL
void
lv2_plugin_process (
  Lv2Plugin *                         self,
  const EngineProcessTimeInfo * const time_nfo);

/**
 * Returns the plugin's latency in samples.
 *
 * This will be 0 if the plugin does not report
 * latency.
 */
NONNULL
nframes_t
lv2_plugin_get_latency (Lv2Plugin * pl);

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
 * Returns whether the plugin has a custom UI that
 * is deprecated (GtkUI, QtUI, etc.).
 *
 * @return If the plugin has a deprecated UI,
 *   returns the UI URI, otherwise NULL.
 */
NONNULL
char *
lv2_plugin_has_deprecated_ui (const char * uri);

/**
 * Returns whether the given UI uri is supported.
 */
NONNULL
bool
lv2_plugin_is_ui_supported (
  const char * pl_uri,
  const char * ui_uri);

/**
 * Returns the UI URIs that this plugin has.
 */
void
lv2_plugin_get_uis (
  const char * pl_uri,
  char **      uris,
  int *        num_uris);

/**
 * Pick the most preferable UI for the given flag.
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

NONNULL
char *
lv2_plugin_get_ui_class (
  const char * pl_uri,
  const char * ui_uri);

/**
 * Returns the bundle path of the UI as a URI.
 */
NONNULL
char *
lv2_plugin_get_ui_bundle_uri (
  const char * pl_uri,
  const char * ui_uri);

/**
 * Returns the binary path of the UI as a URI.
 */
NONNULL
char *
lv2_plugin_get_ui_binary_uri (
  const char * pl_uri,
  const char * ui_uri);

/**
 * Pick the most preferable UI.
 *
 * Calls lv2_plugin_pick_ui().
 *
 * @param[out] ui (Output) UI of the specific
 *   plugin.
 * @param[out] ui_type UI type (eg, X11).
 *
 * @return Whether a UI was picked.
 */
bool
lv2_plugin_pick_most_preferable_ui (
  const char * plugin_uri,
  char **      out_ui,
  char **      out_ui_type,
  bool         allow_bridged);

/* FIXME remove */
NONNULL
bool
lv2_plugin_ui_type_is_external (const LilvNode * ui_type);

NONNULL
bool
lv2_plugin_is_ui_external (
  const char * uri,
  const char * ui_uri,
  GError **    error);

/**
 * Ported from Lv2Control.
 */
void
lv2_plugin_set_control (
  Port *       port,
  uint32_t     size,
  LV2_URID     type,
  const void * body);

/**
 * Returns the property port matching the given
 * property URID.
 */
NONNULL
Port *
lv2_plugin_get_property_port (
  Lv2Plugin * self,
  LV2_URID    property);

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
  void *       user_data,
  uint32_t *   size,
  uint32_t *   type);

NONNULL
char *
lv2_plugin_get_library_path (Lv2Plugin * self);

NONNULL
char *
lv2_plugin_get_abs_state_file_path (
  Lv2Plugin * self,
  bool        is_backup);

/**
 * Allocate port buffers (only necessary for MIDI).
 */
NONNULL
void
lv2_plugin_allocate_port_buffers (Lv2Plugin * plugin);

NONNULL
int
lv2_plugin_activate (Lv2Plugin * self, bool activate);

/**
 * Returns whether the plugin can be cleaned up
 * (some plugins crash on cleanup).
 */
NONNULL
bool
lv2_plugin_can_cleanup (const char * uri);

/**
 * Populates the banks in the plugin instance.
 */

NONNULL
void
lv2_plugin_populate_banks (Lv2Plugin * self);

NONNULL
int
lv2_plugin_cleanup (Lv2Plugin * self);

/**
 * Frees the Lv2Plugin and all its components.
 */
NONNULL
void
lv2_plugin_free (Lv2Plugin * self);

/**
 * @}
 */

#endif
