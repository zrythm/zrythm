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

#include "config.h"

#include <stdlib.h>
#include <ctype.h>

#include "gui/widgets/main_window.h"
#include "plugins/cached_vst_descriptors.h"
#include "plugins/plugin.h"
#include "plugins/plugin_manager.h"
#include "plugins/lv2_plugin.h"
#include "plugins/vst_plugin.h"
#include "utils/arrays.h"
#include "utils/io.h"
#include "utils/string.h"
#include "utils/ui.h"
#include "zrythm.h"

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

/**
 * Initializes plugin manager.
 */
void
plugin_manager_init (PluginManager * self)
{
  g_message ("Initializing plugin manager...");
  self->num_plugins = 0;
  self->num_plugin_categories = 0;

  self->symap = symap_new();
  zix_sem_init(&self->symap_lock, 1);

  /* init lv2 settings */
  g_message ("Creating Lilv World...");
  LilvWorld * world = lilv_world_new ();
  self->lv2_nodes.lilv_world = world;

  /* load all installed plugins on system */
#ifndef _WOE32
  LilvNode * lv2_path = NULL;
  char * env_lv2_path = getenv ("LV2_PATH");
  if (env_lv2_path && (strlen (env_lv2_path) > 0))
    {
      lv2_path =
        lilv_new_string (world, env_lv2_path);
    }
  else
    {
      lv2_path =
        lilv_new_string (
          world,
          "~/.lv2:/usr/local/" LIBDIR_NAME
          "/lv2:/usr/" LIBDIR_NAME "/lv2");
    }
  g_return_if_fail (lv2_path);
  lilv_world_set_option (
    world, LILV_OPTION_LV2_PATH, lv2_path);
#endif
  lilv_world_load_all (world);

  /*load bundled plugins */
#ifndef _WOE32
  GError * err;
  const char * path =
    CONFIGURE_LIBDIR "/zrythm/lv2";
  if (g_file_test (path,
                   G_FILE_TEST_EXISTS |
                   G_FILE_TEST_IS_DIR))
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
                lilv_new_uri (world, str);
              lilv_world_load_bundle (
                world, uri);
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
          ui_show_error_message (
            MAIN_WINDOW ? MAIN_WINDOW : NULL,
            msg);
          g_free (msg);
        }
    }
#endif

  g_message ("Caching LV2 URIs...");

  /* Cache URIs */
  Lv2Nodes * nodes = &self->lv2_nodes;
#define ADD_LV2_NODE(key,val) \
  nodes->key = lilv_new_uri (world, val);

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
    core_sampleRate, LV2_CORE__sampleRate);
  ADD_LV2_NODE (
    core_symbol, LV2_CORE__symbol);
  ADD_LV2_NODE (
    core_toggled, LV2_CORE__toggled);
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
  ADD_LV2_NODE (units_db, LV2_UNITS__db);
  ADD_LV2_NODE (units_hz, LV2_UNITS__hz);
  ADD_LV2_NODE (units_midiNote, LV2_UNITS__midiNote);
  ADD_LV2_NODE (units_render, LV2_UNITS__render);
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
  nodes->end = NULL;
#undef ADD_LV2_NODE

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

  /* load cached VST plugins */
  self->cached_vst_descriptors =
    cached_vst_descriptors_new ();
}

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
  g_message ("scanning plugins...");

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

  /* scan LV2 */
  g_message ("Scanning LV2 plugins...");
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
            self->num_plugins++] =
              descriptor;
          add_category (
            self, descriptor->category_str);
        }

      count++;

      if (progress)
        *progress =
          start_progress +
          ((double) count / size) *
            (max_progress - start_progress);
    }
  g_message ("Scanned %d LV2 plugins", count);

  /* scan vst */
  g_message ("Scanning VST plugins...");
#ifdef _WOE32
  char ** paths =
    g_settings_get_strv (
      S_PREFERENCES, "vst-search-paths-windows");
  g_return_if_fail (paths);
#else
  char * vst_path =
    g_strdup (getenv ("VST_PATH"));
  if (!vst_path || (strlen (vst_path) == 0))
    vst_path =
      g_strdup (
        "~/.vst:~/vst:"
        "/usr/" LIBDIR_NAME "/vst:"
        "/usr/local/" LIBDIR_NAME "/vst");
  char ** paths =
    g_strsplit (vst_path, ":", 0);
  g_free (vst_path);
#endif
  int path_idx = 0;
  char * path;
  while ((path = paths[path_idx++]) != NULL)
    {
      if (!g_file_test (path, G_FILE_TEST_EXISTS))
        continue;

      g_message ("scanning for VSTs in %s", path);

      char ** vst_plugins =
#ifdef _WOE32
        io_get_files_in_dir_ending_in (
          path, 1, ".dll");
#else
        io_get_files_in_dir_ending_in (
          path, 1, ".so");
#endif
      if (!vst_plugins)
        continue;

      char * plugin;
      int plugin_idx = 0;
      while ((plugin = vst_plugins[plugin_idx++]) !=
               NULL)
        {
          PluginDescriptor * descriptor =
            cached_vst_descriptors_get (
              self->cached_vst_descriptors,
              plugin);

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
                  plugin))
                {
                  g_message (
                    "Ignoring blacklisted VST "
                    "plugin: %s", plugin);
                }
              else
                {
                  descriptor =
                    vst_plugin_create_descriptor_from_path (
                      plugin, 0);

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
                        descriptor, 1);
                    }
                  else
                    {
                      g_warning (
                        "Blacklisting VST %s",
                        plugin);
                      cached_vst_descriptors_blacklist (
                        self->cached_vst_descriptors,
                        plugin, 1);
                    }
                }
            }
        }
      g_strfreev (vst_plugins);
    }

  g_strfreev (paths);

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

  g_message ("%d Plugins scanned.",
             self->num_plugins);

  /*print_plugins ();*/
}

void
plugin_manager_free (
  PluginManager * self)
{
  symap_free (self->symap);
  zix_sem_destroy (&self->symap_lock);
}
