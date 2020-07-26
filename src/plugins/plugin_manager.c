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
 * Copyright (C) 2008-2012 Paul Davis
 * Copyright (C) David Robillard
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "zrythm-config.h"

#include <stdlib.h>
#include <ctype.h>

#include "gui/widgets/main_window.h"
#include "plugins/cached_vst_descriptors.h"
#include "plugins/carla/carla_discovery.h"
#include "plugins/plugin.h"
#include "plugins/plugin_manager.h"
#include "plugins/lv2_plugin.h"
#include "settings/settings.h"
#include "utils/arrays.h"
#include "utils/io.h"
#include "utils/objects.h"
#include "utils/string.h"
#include "utils/ui.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include <lv2/event/event.h>
#include <lv2/options/options.h>
#include <lv2/parameters/parameters.h>
#include <lv2/buf-size/buf-size.h>
#include <lv2/patch/patch.h>
#include <lv2/port-groups/port-groups.h>
#include <lv2/midi/midi.h>
#include <lv2/port-props/port-props.h>
#include <lv2/presets/presets.h>
#include <lv2/resize-port/resize-port.h>
#include <lv2/time/time.h>
#include <lv2/units/units.h>

/**
 * If category not already set in the categories, add it.
 */
static void
add_category (
  PluginManager * self,
  char *          category)
{
  if (!string_is_ascii (category))
    {
      g_warning (
        "Ignoring non-ASCII plugin category "
        "name...");
    }
  for (int i = 0;
       i < self->num_plugin_categories; i++)
    {
      char * cat = self->plugin_categories[i];
      if (!strcmp (cat, category))
        {
          return;
        }
    }
  g_message ("%s: %s", __func__, category);
  self->plugin_categories[
    self->num_plugin_categories++] =
      g_strdup (category);
}

static int
sort_category_func (
  const void *a, const void *b)
{
  char * pa =
    *(char * const *) a;
  char * pb =
    *(char * const *) b;
  int r = strcasecmp(pa, pb);
  if (r)
    return r;

  /* if equal ignoring case, use opposite of strcmp() result to get
   * lower before upper */
  return -strcmp(pa, pb); /* aka: return strcmp(b, a); */
}

static int sort_plugin_func (
  const void *a, const void *b)
{
  PluginDescriptor * pa =
    *(PluginDescriptor * const *) a;
  PluginDescriptor * pb =
    *(PluginDescriptor * const *) b;
  int r = strcasecmp(pa->name, pb->name);
  if (r)
    return r;

  /* if equal ignoring case, use opposite of strcmp()
   * result to get lower before upper */
  /* aka: return strcmp(b, a); */
  return -strcmp(pa->name, pb->name);
}

/*static void*/
/*print_plugins ()*/
/*{*/
  /*for (int i = 0; i < PLUGIN_MANAGER->num_plugins; i++)*/
    /*{*/
      /*PluginDescriptor * descr =*/
        /*PLUGIN_MANAGER->plugin_descriptors[i];*/

      /*g_message ("[%d] %s (%s - %s)",*/
                 /*i,*/
                 /*descr->name,*/
                 /*descr->uri,*/
                 /*descr->category_str);*/
    /*}*/
/*}*/

static void
cache_lv2_uris (
  PluginManager * self)
{
  g_message ("%s: Caching...", __func__);

  /* Cache URIs */
  Lv2Nodes * nodes = &self->lv2_nodes;
#define ADD_LV2_NODE(key,val) \
  nodes->key = \
    lilv_new_uri (self->lv2_nodes.lilv_world, val)

  /* in alphabetical order */
  ADD_LV2_NODE (atom_AtomPort, LV2_ATOM__AtomPort);
  ADD_LV2_NODE (
    atom_bufferType, LV2_ATOM__bufferType);
  ADD_LV2_NODE (atom_Chunk, LV2_ATOM__Chunk);
  ADD_LV2_NODE (
    atom_eventTransfer, LV2_ATOM__eventTransfer);
  ADD_LV2_NODE (atom_Float, LV2_ATOM__Float);
  ADD_LV2_NODE (atom_Path, LV2_ATOM__Path);
  ADD_LV2_NODE (
    atom_Sequence, LV2_ATOM__Sequence);
  ADD_LV2_NODE (
    atom_supports, LV2_ATOM__supports);
  ADD_LV2_NODE (
    bufz_coarseBlockLength,
    "http://lv2plug.in/ns/ext/buf-size#coarseBlockLength");
  ADD_LV2_NODE (
    bufz_fixedBlockLength,
    LV2_BUF_SIZE__fixedBlockLength);
  ADD_LV2_NODE (
    bufz_powerOf2BlockLength,
    LV2_BUF_SIZE__powerOf2BlockLength);
  ADD_LV2_NODE (
    bufz_nominalBlockLength,
    "http://lv2plug.in/ns/ext/buf-size#nominalBlockLength");
  ADD_LV2_NODE (
    core_AudioPort, LV2_CORE__AudioPort);
  ADD_LV2_NODE (
    core_connectionOptional,
    LV2_CORE__connectionOptional);
  ADD_LV2_NODE (
    core_control, LV2_CORE__control);
  ADD_LV2_NODE (
    core_ControlPort, LV2_CORE__ControlPort);
  ADD_LV2_NODE (
    core_CVPort, LV2_CORE__CVPort);
  ADD_LV2_NODE (
    core_default, LV2_CORE__default);
  ADD_LV2_NODE (
    core_designation, LV2_CORE__designation);
  ADD_LV2_NODE (
    core_enumeration, LV2_CORE__enumeration);
  ADD_LV2_NODE (
    core_freeWheeling, LV2_CORE__freeWheeling);
  ADD_LV2_NODE (
    core_index, LV2_CORE__index);
  ADD_LV2_NODE (
    core_inPlaceBroken, LV2_CORE__inPlaceBroken);
  ADD_LV2_NODE (
    core_InputPort, LV2_CORE__InputPort);
  ADD_LV2_NODE (
    core_integer, LV2_CORE__integer);
  ADD_LV2_NODE (
    core_isSideChain,
    LV2_CORE_PREFIX "isSideChain");
  ADD_LV2_NODE (
    core_maximum, LV2_CORE__maximum);
  ADD_LV2_NODE (
    core_minimum, LV2_CORE__minimum);
  ADD_LV2_NODE (
    core_name, LV2_CORE__name);
  ADD_LV2_NODE (
    core_OutputPort, LV2_CORE__OutputPort);
  ADD_LV2_NODE (
    core_reportsLatency, LV2_CORE__reportsLatency);
  ADD_LV2_NODE (
    core_requiredFeature,
    LV2_CORE__requiredFeature);
  ADD_LV2_NODE (
    core_sampleRate, LV2_CORE__sampleRate);
  ADD_LV2_NODE (
    core_symbol, LV2_CORE__symbol);
  ADD_LV2_NODE (
    core_toggled, LV2_CORE__toggled);
  ADD_LV2_NODE (
    data_access,
    "http://lv2plug.in/ns/ext/data-access");
  ADD_LV2_NODE (
    instance_access,
    "http://lv2plug.in/ns/ext/instance-access");
  ADD_LV2_NODE (
    ev_EventPort,
    LV2_EVENT__EventPort);
  ADD_LV2_NODE (
    patch_Message,
    LV2_PATCH__Message);
  ADD_LV2_NODE (
    patch_readable,
    LV2_PATCH__readable);
  ADD_LV2_NODE (
    patch_writable,
    LV2_PATCH__writable);
  ADD_LV2_NODE (
    midi_MidiEvent,
    LV2_MIDI__MidiEvent);
  ADD_LV2_NODE (
    pg_element,
    LV2_PORT_GROUPS__element);
  ADD_LV2_NODE (
    pg_group,
    LV2_PORT_GROUPS__group);
  ADD_LV2_NODE (
    pprops_causesArtifacts,
    LV2_PORT_PROPS__causesArtifacts);
  ADD_LV2_NODE (
    pprops_expensive,
    LV2_PORT_PROPS__expensive);
  ADD_LV2_NODE (
    pprops_logarithmic,
    LV2_PORT_PROPS__logarithmic);
  ADD_LV2_NODE (
    pprops_notAutomatic,
    LV2_PORT_PROPS__notAutomatic);
  ADD_LV2_NODE (
    pprops_notOnGUI,
    LV2_PORT_PROPS__notOnGUI);
  ADD_LV2_NODE (
    pprops_rangeSteps,
    LV2_PORT_PROPS__rangeSteps);
  ADD_LV2_NODE (
    pprops_trigger, LV2_PORT_PROPS__trigger);
  ADD_LV2_NODE (
    pset_bank, LV2_PRESETS__bank);
  ADD_LV2_NODE (
    pset_Bank, LV2_PRESETS__Bank);
  ADD_LV2_NODE (
    pset_Preset, LV2_PRESETS__Preset);
  ADD_LV2_NODE (
    rdfs_comment, LILV_NS_RDFS "comment");
  ADD_LV2_NODE (
    rdfs_label, LILV_NS_RDFS "label");
  ADD_LV2_NODE (
    rdfs_range, LILV_NS_RDFS "range");
  ADD_LV2_NODE (
    rsz_minimumSize,
    LV2_RESIZE_PORT__minimumSize);
  ADD_LV2_NODE (
    state_threadSafeRestore,
    LV2_STATE__threadSafeRestore);
  ADD_LV2_NODE (
    time_Position, LV2_TIME__Position);
  ADD_LV2_NODE (
    ui_external,
    "http://lv2plug.in/ns/extensions/ui#external");
  ADD_LV2_NODE (
    ui_externalkx,
    "http://kxstudio.sf.net/ns/lv2ext/"
    "external-ui#Widget");
  ADD_LV2_NODE (ui_Gtk3UI, LV2_UI__Gtk3UI);
  ADD_LV2_NODE (ui_GtkUI, LV2_UI__GtkUI);
  ADD_LV2_NODE (ui_Qt4UI, LV2_UI__Qt4UI);
  ADD_LV2_NODE (ui_Qt5UI, LV2_UI__Qt5UI);
  ADD_LV2_NODE (units_db, LV2_UNITS__db);
  ADD_LV2_NODE (units_degree, LV2_UNITS__degree);
  ADD_LV2_NODE (units_hz, LV2_UNITS__hz);
  ADD_LV2_NODE (units_midiNote, LV2_UNITS__midiNote);
  ADD_LV2_NODE (units_mhz, LV2_UNITS__mhz);
  ADD_LV2_NODE (units_ms, LV2_UNITS__ms);
  ADD_LV2_NODE (units_render, LV2_UNITS__render);
  ADD_LV2_NODE (units_s, LV2_UNITS__s);
  ADD_LV2_NODE (units_unit, LV2_UNITS__unit);
  ADD_LV2_NODE (
    work_interface, LV2_WORKER__interface);
  ADD_LV2_NODE (
    work_schedule, LV2_WORKER__schedule);
#ifdef LV2_EXTENDED
  /*nodes->auto_can_write_automation = lilv_new_uri(world,LV2_AUTOMATE_URI__can_write);*/
  /*nodes->auto_automation_control     = lilv_new_uri(world,LV2_AUTOMATE_URI__control);*/
  /*nodes->auto_automation_controlled  = lilv_new_uri(world,LV2_AUTOMATE_URI__controlled);*/
  /*nodes->auto_automation_controller  = lilv_new_uri(world,LV2_AUTOMATE_URI__controller);*/
  /*nodes->inline_display_in_gui       = lilv_new_uri(world,LV2_INLINEDISPLAY__in_gui);*/
#endif

  ADD_LV2_NODE (
    zrythm_default_bank,
    "https://lv2.zrythm.org#default-bank");
  ADD_LV2_NODE (
    zrythm_default_preset,
    "https://lv2.zrythm.org#init-preset");

  nodes->end = NULL;
#undef ADD_LV2_NODE

  g_message ("%s: done", __func__);
}

static void
create_and_load_lilv_word (
  PluginManager * self)
{
  g_message ("Creating Lilv World...");
  LilvWorld * world = lilv_world_new ();
  self->lv2_nodes.lilv_world = world;

  /* load all installed plugins on system */
#if !defined (_WOE32) && !defined (__APPLE__)
  LilvNode * lv2_path = NULL;
  char * env_lv2_path = getenv ("LV2_PATH");
  if (env_lv2_path && (strlen (env_lv2_path) > 0))
    {
      self->lv2_path = g_strdup (env_lv2_path);
      lv2_path =
        lilv_new_string (world, env_lv2_path);
    }
  else
    {
      if (string_is_equal (
            LIBDIR_NAME, "lib", 0))
        {
          self->lv2_path =
            g_strdup_printf (
              "%s/.lv2:/usr/local/lib/lv2:"
              "/usr/lib/lv2",
              g_get_home_dir ());
          lv2_path =
            lilv_new_string (
              world, self->lv2_path);
        }
      else
        {
          self->lv2_path =
            g_strdup_printf (
              "%s/.lv2:/usr/local/" LIBDIR_NAME
              "/lv2:/usr/" LIBDIR_NAME "/lv2:"
              /* some distros report the wrong
               * LIBDIR_NAME so hardcode these */
              "/usr/local/lib/lv2:/usr/lib/lv2",
              g_get_home_dir ());
          lv2_path =
            lilv_new_string (
              world, self->lv2_path);
        }
    }
  g_return_if_fail (lv2_path);
  g_message (
    "%s: LV2 path: %s", __func__, self->lv2_path);
  lilv_world_set_option (
    world, LILV_OPTION_LV2_PATH, lv2_path);
#endif
  if (ZRYTHM_TESTING)
    {
      g_message (
        "%s: loading specifications and plugin "
        "classes...",  __func__);
      lilv_world_load_specifications (world);
      lilv_world_load_plugin_classes (world);
    }
  else
    {
      g_message (
        "%s: loading all...", __func__);
      lilv_world_load_all (world);
    }
}

static void
init_symap (
  PluginManager * self)
{
  /* symap URIDs */
#define SYMAP_MAP(target,uri) \
  self->urids.target = \
    symap_map (self->symap, uri);

  SYMAP_MAP (atom_Float, LV2_ATOM__Float);
  SYMAP_MAP (atom_Int, LV2_ATOM__Int);
  SYMAP_MAP (atom_Object, LV2_ATOM__Object);
  SYMAP_MAP (atom_Path, LV2_ATOM__Path);
  SYMAP_MAP (atom_String, LV2_ATOM__String);
  SYMAP_MAP (
    atom_eventTransfer, LV2_ATOM__eventTransfer);
  SYMAP_MAP (
    bufsz_maxBlockLength,
    LV2_BUF_SIZE__maxBlockLength);
  SYMAP_MAP (
    bufsz_minBlockLength,
    LV2_BUF_SIZE__minBlockLength);
  SYMAP_MAP (
    bufsz_sequenceSize,
    LV2_BUF_SIZE__sequenceSize);
  SYMAP_MAP ( log_Error, LV2_LOG__Error);
  SYMAP_MAP (log_Trace, LV2_LOG__Trace);
  SYMAP_MAP (log_Warning, LV2_LOG__Warning);
  SYMAP_MAP (midi_MidiEvent, LV2_MIDI__MidiEvent);
  SYMAP_MAP (
    param_sampleRate,
    LV2_PARAMETERS__sampleRate);
  SYMAP_MAP (patch_Get, LV2_PATCH__Get);
  SYMAP_MAP (patch_Put, LV2_PATCH__Put);
  SYMAP_MAP (patch_Set, LV2_PATCH__Set);
  SYMAP_MAP (patch_body, LV2_PATCH__body);
  SYMAP_MAP (patch_property, LV2_PATCH__property);
  SYMAP_MAP (patch_value, LV2_PATCH__value);
  SYMAP_MAP (time_Position, LV2_TIME__Position);
  SYMAP_MAP (time_barBeat, LV2_TIME__barBeat);
  SYMAP_MAP (time_beatUnit, LV2_TIME__beatUnit);
  SYMAP_MAP (
    time_beatsPerBar,
    LV2_TIME__beatsPerBar);
  SYMAP_MAP (
    time_beatsPerMinute,
    LV2_TIME__beatsPerMinute);
  SYMAP_MAP (time_frame, LV2_TIME__frame);
  SYMAP_MAP (time_speed, LV2_TIME__speed);
  SYMAP_MAP (ui_updateRate, LV2_UI__updateRate);
#undef SYMAP_MAP
}

static void
load_bundled_lv2_plugins (
  PluginManager * self)
{
#ifndef _WOE32
  GError * err;
  const char * path =
    CONFIGURE_LIBDIR "/zrythm/lv2";
  if (g_file_test (
        path,
        G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR))
    {
      GDir * bundle_lv2_dir =
        g_dir_open (path, 0, &err);
      if (bundle_lv2_dir)
        {
          const char * dir;
          char * str;
          while ((dir = g_dir_read_name (bundle_lv2_dir)))
            {
              str =
                g_strdup_printf (
                  "file://%s%s%s%smanifest.ttl",
                  path,
                  G_DIR_SEPARATOR_S,
                  dir,
                  G_DIR_SEPARATOR_S);
              LilvNode * uri =
                lilv_new_uri (
                  self->lv2_nodes.lilv_world, str);
              lilv_world_load_bundle (
                self->lv2_nodes.lilv_world, uri);
              g_message ("Loaded bundled plugin at %s",
                         str);
              g_free (str);
              lilv_node_free (uri);
            }
        }
      else
        {
          char * msg =
            g_strdup_printf ("%s%s",
            _("Error loading LV2 bundle dir: "),
            err->message);
          if (ZRYTHM_HAVE_UI)
            {
              ui_show_error_message (
                MAIN_WINDOW ? MAIN_WINDOW : NULL,
                msg);
            }
          g_free (msg);
        }
    }
#endif
}

PluginManager *
plugin_manager_new (void)
{
  PluginManager * self = object_new (PluginManager);

  self->symap = symap_new();
  zix_sem_init(&self->symap_lock, 1);

  /* init lv2 */
  create_and_load_lilv_word (self);
  cache_lv2_uris (self);
  init_symap (self);
  load_bundled_lv2_plugins (self);

  /* init vst */
  self->cached_vst_descriptors =
    cached_vst_descriptors_new ();

  return self;
}

#ifdef HAVE_CARLA
static char **
get_vst_paths (
  PluginManager * self)
{
  g_message ("%s: getting paths...", __func__);

#ifdef _WOE32
  char ** paths =
    g_settings_get_strv (
      S_P_PLUGINS_PATHS,
      "vst-search-paths-windows");
  g_return_val_if_fail (paths, NULL);
#elif defined (__APPLE__)
  char ** paths =
    g_strsplit (
      "/Library/Audio/Plug-ins/VST" PATH_SPLIT,
      PATH_SPLIT, -1);
#else
  char * vst_path =
    g_strdup (getenv ("VST_PATH"));
  if (!vst_path || (strlen (vst_path) == 0))
    {
      if (string_is_equal (
            LIBDIR_NAME, "lib", false))
        {
          vst_path =
            g_strdup_printf (
              "%s/.vst:%s/vst:"
              "/usr/" LIBDIR_NAME "/vst:"
              "/usr/local/" LIBDIR_NAME "/vst",
              g_get_home_dir (), g_get_home_dir ());
        }
      else
        {
          vst_path =
            g_strdup_printf (
              "%s/.vst:%s/vst:"
              "/usr/lib/vst:"
              "/usr/" LIBDIR_NAME "/vst:"
              "/usr/local/lib/vst:"
              "/usr/local/" LIBDIR_NAME "/vst",
              g_get_home_dir (), g_get_home_dir ());
        }

      g_message (
        "Using standard VST paths: %s", vst_path);
    }
  else
    {
      g_message (
        "using %s from the environment (VST_PATH)",
        vst_path);
    }
  char ** paths =
    g_strsplit (vst_path, PATH_SPLIT, 0);
  g_free (vst_path);
#endif // __APPLE__

  g_message ("%s: done", __func__);

  return paths;
}

#if defined (_WOE32) || defined (__APPLE__)
static char **
get_vst3_paths (
  PluginManager * self)
{
#ifdef _WOE32
  return
    g_strsplit (
      "C:\\Program Files\\Common Files\\VST3"
      PATH_SPLIT
      "C:\\Program Files (x86)\\Common Files\\VST3",
      PATH_SPLIT, 0);
#elif defined (__APPLE__)
  return
    g_strsplit (
      "/Library/Audio/Plug-ins/VST3" PATH_SPLIT,
      PATH_SPLIT, -1);
#endif // __APPLE__
}

static int
get_vst3_count (
  PluginManager * self)
{
  char ** paths = get_vst3_paths (self);
  int path_idx = 0;
  char * path;
  int count = 0;
  while ((path = paths[path_idx++]) != NULL)
    {
      if (!g_file_test (path, G_FILE_TEST_EXISTS))
        continue;

      char ** vst_plugins =
        io_get_files_in_dir_ending_in (
          path, 1, ".vst3");
      if (!vst_plugins)
        continue;

      char * plugin_path;
      int plugin_idx = 0;
      while ((plugin_path = vst_plugins[plugin_idx++]) !=
               NULL)
        {
          count++;
        }
      g_strfreev (vst_plugins);
    }
  g_strfreev (paths);

  return count;
}
#endif // _WOE32 || __APPLE__

static int
get_vst_count (
  PluginManager * self)
{
  char ** paths = get_vst_paths (self);
  int path_idx = 0;
  char * path;
  int count = 0;
  while ((path = paths[path_idx++]) != NULL)
    {
      if (!g_file_test (path, G_FILE_TEST_EXISTS))
        continue;

      char ** vst_plugins =
        io_get_files_in_dir_ending_in (
          path, 1, LIB_SUFFIX);
      if (!vst_plugins)
        continue;

      char * plugin_path;
      int plugin_idx = 0;
      while ((plugin_path = vst_plugins[plugin_idx++]) !=
               NULL)
        {
          count++;
        }
      g_strfreev (vst_plugins);
    }
  g_strfreev (paths);

  return count;
}

/**
 * Gets the SFZ or SF2 paths.
 */
static char **
get_sf_paths (
  PluginManager * self,
  bool            sf2)
{
  char ** paths = NULL;
  if (ZRYTHM_TESTING)
    {
      paths = g_strsplit (PATH_SPLIT, PATH_SPLIT, -1);
    }
  else
    {
      paths =
        g_settings_get_strv (
          S_P_PLUGINS_PATHS,
          sf2 ?
            "sf2-search-paths" :
            "sfz-search-paths");
      g_return_val_if_fail (paths, NULL);
    }

  return paths;
}
#endif // HAVE_CARLA

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
  double *        progress)
{
  g_return_if_fail (self);

  g_message ("%s: Scanning...", __func__);

  double start_progress =
    progress ? *progress : 0;

  /* load all plugins with lilv */
  LilvWorld * world = LILV_WORLD;
  const LilvPlugins * lilv_plugins =
    lilv_world_get_all_plugins (world);
  LV2_NODES.lilv_plugins = lilv_plugins;

  if (getenv ("NO_SCAN_PLUGINS"))
    return;

  double size =
    (double) lilv_plugins_size (lilv_plugins);
#ifdef HAVE_CARLA
  size += (double) get_vst_count (self);
#  if defined (_WOE32) || defined (__APPLE__)
  size += (double) get_vst3_count (self);
#  endif
#  ifdef __APPLE__
  size +=
    carla_get_cached_plugin_count (PLUGIN_AU, NULL);
#  endif
#endif

  /* scan LV2 */
  g_message (
    "%s: Scanning LV2 plugins...", __func__);
  unsigned int count = 0;
  LILV_FOREACH (plugins, i, lilv_plugins)
    {
      const LilvPlugin* p =
        lilv_plugins_get (lilv_plugins, i);

      PluginDescriptor * descriptor =
        lv2_plugin_create_descriptor_from_lilv (p);

      if (descriptor)
        {
          self->plugin_descriptors[
            self->num_plugins++] = descriptor;
          add_category (
            self, descriptor->category_str);
        }

      count++;

      if (progress)
        {
          *progress =
            start_progress +
            ((double) count / size) *
              (max_progress - start_progress);
          char prog_str[800];
          if (descriptor)
            {
              sprintf (
                prog_str, "%s: %s",
                _("Scanned LV2 plugin"),
                descriptor->name);
            }
          else
            {
              const LilvNode*  lv2_uri =
                lilv_plugin_get_uri (p);
              const char * uri_str =
                lilv_node_as_string (lv2_uri);
              sprintf (
                prog_str,
                _("Skipped LV2 plugin at %s"),
                uri_str);
            }
          zrythm_app_set_progress_status (
            zrythm_app, prog_str, *progress);
        }
    }
  g_message (
    "%s: Scanned %d LV2 plugins", __func__, count);

#ifdef HAVE_CARLA
  /* scan vst */
  g_message ("Scanning VST plugins...");
  char ** paths = get_vst_paths (self);
  int path_idx = 0;
  char * path;
  while ((path = paths[path_idx++]) != NULL)
    {
      if (!g_file_test (path, G_FILE_TEST_EXISTS))
        continue;

      g_message ("scanning for VSTs in %s", path);

      char ** vst_plugins =
        io_get_files_in_dir_ending_in (
          path, 1,
#ifdef __APPLE__
          ".vst"
#else
          LIB_SUFFIX
#endif
          );
      if (!vst_plugins)
        continue;

      char * plugin_path;
      int plugin_idx = 0;
      while ((plugin_path = vst_plugins[plugin_idx++]) !=
               NULL)
        {
          PluginDescriptor * descriptor =
            cached_vst_descriptors_get (
              self->cached_vst_descriptors,
              plugin_path);

          if (descriptor)
            {
              g_message (
                "Found cached VST %s",
                descriptor->name);
              array_append (
                self->plugin_descriptors,
                self->num_plugins,
                descriptor);
              add_category (
                self, descriptor->category_str);
            }
          else
            {
              if (
                cached_vst_descriptors_is_blacklisted (
                  self->cached_vst_descriptors,
                  plugin_path))
                {
                  g_message (
                    "Ignoring blacklisted VST "
                    "plugin: %s", plugin_path);
                }
              else
                {
                  descriptor =
                    z_carla_discovery_create_vst_descriptor (
                      plugin_path, ARCH_64,
                      PROT_VST);

                  /* try 32-bit if above failed */
                  if (!descriptor)
                    descriptor =
                      z_carla_discovery_create_vst_descriptor (
                        plugin_path, ARCH_32,
                        PROT_VST);

                  if (descriptor)
                    {
                      array_append (
                        self->plugin_descriptors,
                        self->num_plugins,
                        descriptor);
                      add_category (
                        self, descriptor->category_str);
                      g_message (
                        "Caching VST %s",
                        descriptor->name);
                      cached_vst_descriptors_add (
                        self->cached_vst_descriptors,
                        descriptor, 0);
                    }
                  else
                    {
                      g_message (
                        "Blacklisting VST %s",
                        plugin_path);
                      cached_vst_descriptors_blacklist (
                        self->cached_vst_descriptors,
                        plugin_path, 0);
                    }
                }
            }
          count++;

          if (progress)
            {
              *progress =
                start_progress +
                ((double) count / size) *
                  (max_progress - start_progress);
              char prog_str[800];
              if (descriptor)
                {
                  sprintf (
                    prog_str, "%s: %s",
                    _("Scanned VST plugin"),
                    descriptor->name);
                }
              else
                {
                  sprintf (
                    prog_str,
                    _("Skipped VST plugin at %s"),
                    plugin_path);
                }
              zrythm_app_set_progress_status (
                zrythm_app, prog_str, *progress);
            }
        }
      if (plugin_idx > 0)
        {
          cached_vst_descriptors_serialize_to_file (
            self->cached_vst_descriptors);
        }
      g_strfreev (vst_plugins);
    }
  g_strfreev (paths);

#if defined (_WOE32) || defined (__APPLE__)
  /* scan vst3 */
  g_message ("Scanning VST3 plugins...");
  paths = get_vst3_paths (self);
  path_idx = 0;
  while ((path = paths[path_idx++]) != NULL)
    {
      g_message ("path %s", path);
      if (!g_file_test (path, G_FILE_TEST_EXISTS))
        continue;

      g_message ("scanning for VST3s in %s", path);

      char ** vst_plugins =
        io_get_files_in_dir_ending_in (
          path, 1, ".vst3");
      if (!vst_plugins)
        continue;

      char * plugin_path;
      int plugin_idx = 0;
      while ((plugin_path = vst_plugins[plugin_idx++]) !=
               NULL)
        {
          PluginDescriptor * descriptor =
            cached_vst_descriptors_get (
              self->cached_vst_descriptors,
              plugin_path);

          if (descriptor)
            {
              g_message (
                "Found cached VST %s",
                descriptor->name);
              array_append (
                self->plugin_descriptors,
                self->num_plugins,
                descriptor);
              add_category (
                self, descriptor->category_str);
            }
          else
            {
              if (
                cached_vst_descriptors_is_blacklisted (
                  self->cached_vst_descriptors,
                  plugin_path))
                {
                  g_message (
                    "Ignoring blacklisted VST "
                    "plugin: %s", plugin_path);
                }
              else
                {
                  if (!descriptor)
                    descriptor =
                      z_carla_discovery_create_vst_descriptor (
                      plugin_path,
                      string_contains_substr (
                        plugin_path,
                        "C:\\Program Files (x86)",
                        false) ? ARCH_32 : ARCH_64,
                      PROT_VST3);

                  if (descriptor)
                    {
                      array_append (
                        self->plugin_descriptors,
                        self->num_plugins,
                        descriptor);
                      add_category (
                        self, descriptor->category_str);
                      g_message (
                        "Caching VST %s",
                        descriptor->name);
                      cached_vst_descriptors_add (
                        self->cached_vst_descriptors,
                        descriptor, 0);
                    }
                  else
                    {
                      g_message (
                        "Blacklisting VST3 %s",
                        plugin_path);
                      cached_vst_descriptors_blacklist (
                        self->cached_vst_descriptors,
                        plugin_path, 0);
                    }
                }
            }
          count++;

          if (progress)
            {
              *progress =
                start_progress +
                ((double) count / size) *
                  (max_progress - start_progress);
              char prog_str[800];
              if (descriptor)
                {
                  sprintf (
                    prog_str, "%s: %s",
                    _("Scanned VST plugin"),
                    descriptor->name);
                }
              else
                {
                  sprintf (
                    prog_str,
                    _("Skipped VST plugin at %s"),
                    plugin_path);
                }
              zrythm_app_set_progress_status (
                zrythm_app, prog_str, *progress);
            }
        }
      if (plugin_idx > 0)
        {
          cached_vst_descriptors_serialize_to_file (
            self->cached_vst_descriptors);
        }
      g_strfreev (vst_plugins);
    }
  g_strfreev (paths);
#endif // _WOE32 || __APPLE__

#ifdef __APPLE__
  /* scan AU plugins */
  g_message ("Scanning AU plugins...");
  unsigned int au_count =
    carla_get_cached_plugin_count (PLUGIN_AU, NULL);
  char * all_plugins =
    z_carla_discovery_run (
      ARCH_64, "au", ":all");
  g_message ("all plugins %s", all_plugins);
  g_message ("%u plugins found", au_count);
  for (unsigned int i = 0; i < au_count; i++)
    {
      PluginDescriptor * descriptor =
        z_carla_discovery_create_au_descriptor_from_string (
          &all_plugins, i);

      if (descriptor)
        {
          self->plugin_descriptors[
            self->num_plugins++] =
              descriptor;
          add_category (
            self, descriptor->category_str);
        }

      count++;

      if (progress)
        {
          *progress =
            start_progress +
            ((double) count / size) *
              (max_progress - start_progress);
          char prog_str[800];
          if (descriptor)
            {
              sprintf (
                prog_str, "%s: %s",
                _("Scanned AU plugin"),
                descriptor->name);
            }
          else
            {
              sprintf (
                prog_str,
                _("Skipped AU plugin at %u"),
                i);
            }
          zrythm_app_set_progress_status (
            zrythm_app, prog_str, *progress);
        }
    }
#endif // __APPLE__
  for (int i = 0; i < 2; i++)
    {
      char type[12];
      strcpy (type, (i == 0) ? "SFZ" : "SF2");
      char suffix[12];
      strcpy (suffix, (i == 0) ? ".sfz" : ".sf2");
      /* scan sfz */
      g_message (
        "Scanning %s instruments...", type);
      paths = get_sf_paths (self, i);
      g_return_if_fail (paths);
      path_idx = 0;
      while ((path = paths[path_idx++]) != NULL)
        {
          if (!g_file_test (path, G_FILE_TEST_EXISTS))
            continue;

          g_message (
            "scanning for %ss in %s", type, path);

          char ** sf_instruments =
            io_get_files_in_dir_ending_in (
              path, 1, suffix);
          if (!sf_instruments)
            continue;

          char * ins_path;
          int ins_idx = 0;
          while ((ins_path =
                    sf_instruments[ins_idx++]) != NULL)
            {
              PluginDescriptor * descr =
                calloc (1, sizeof (PluginDescriptor));

              descr->path = g_strdup (ins_path);
              descr->category = PC_INSTRUMENT;
              descr->category_str =
                plugin_descriptor_category_to_string (
                  descr->category);
              descr->name =
                io_path_get_basename_without_ext (
                  ins_path);
              char * parent_path =
                io_path_get_parent_dir (ins_path);
              if (!parent_path)
                {
                  g_warning (
                    "Failed to get parent dir of "
                    "%s", ins_path);
                  plugin_descriptor_free (descr);
                  continue;
                }
              descr->author =
                g_path_get_basename (parent_path);
              g_free (parent_path);
              descr->num_audio_outs = 2;
              descr->num_midi_ins = 1;
              descr->arch = ARCH_64;
              descr->protocol =
                i == 0 ? PROT_SFZ : PROT_SF2;
              descr->open_with_carla = true;
              descr->bridge_mode =
                z_carla_discovery_get_bridge_mode (
                  descr);

              array_append (
                self->plugin_descriptors,
                self->num_plugins, descr);
              add_category (
                self, descr->category_str);

              if (progress)
                {
                  char prog_str[800];
                  sprintf (
                    prog_str,
                    "Scanned %s instrument: %s",
                    type,
                    descr->name);
                  zrythm_app_set_progress_status (
                    zrythm_app, prog_str,
                    *progress);
                }
            }
          g_strfreev (sf_instruments);
        }
      g_strfreev (paths);
    }
#endif // HAVE_CARLA

  /* sort alphabetically */
  qsort (self->plugin_descriptors,
         (size_t) self->num_plugins,
         sizeof (Plugin *),
         sort_plugin_func);
  qsort (self->plugin_categories,
         (size_t)
           self->num_plugin_categories,
         sizeof (char *),
         sort_category_func);

  g_message (
    "%s: %d Plugins scanned.",
    __func__, self->num_plugins);

  /*print_plugins ();*/
}

const PluginDescriptor *
plugin_manager_find_plugin_from_uri (
  PluginManager * self,
  const char *    uri)
{
  for (int i = 0; i < self->num_plugins; i++)
    {
      PluginDescriptor * descr =
        self->plugin_descriptors[i];
      if (string_is_equal (uri, descr->uri, false))
        {
          return descr;
        }
    }

  return NULL;
}

void
plugin_manager_free (
  PluginManager * self)
{
  g_message ("%s: Freeing...", __func__);

  symap_free (self->symap);
  zix_sem_destroy (&self->symap_lock);
  lilv_world_free (self->lv2_nodes.lilv_world);

  object_free_w_func_and_null (
    cached_vst_descriptors_free,
    self->cached_vst_descriptors);

  object_zero_and_free (self);

  g_message ("%s: done", __func__);
}
