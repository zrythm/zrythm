// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * ---
 *
 * Copyright (C) 2008-2012 Paul Davis
 * Copyright (C) David Robillard
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * ---
 */

#include "zrythm-config.h"

#include <stdlib.h>

#include "gui/widgets/main_window.h"
#include "plugins/cached_plugin_descriptors.h"
#include "plugins/carla/carla_discovery.h"
#include "plugins/collections.h"
#include "plugins/lv2_plugin.h"
#include "plugins/plugin.h"
#include "plugins/plugin_manager.h"
#include "settings/settings.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/io.h"
#include "utils/mem.h"
#include "utils/objects.h"
#include "utils/sort.h"
#include "utils/string.h"
#include "utils/ui.h"
#include "utils/windows.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include <ctype.h>
#include <lv2/buf-size/buf-size.h>
#include <lv2/event/event.h>
#include <lv2/midi/midi.h>
#include <lv2/options/options.h>
#include <lv2/parameters/parameters.h>
#include <lv2/patch/patch.h>
#include <lv2/port-groups/port-groups.h>
#include <lv2/port-props/port-props.h>
#include <lv2/presets/presets.h>
#include <lv2/resize-port/resize-port.h>
#include <lv2/time/time.h>
#include <lv2/units/units.h>

/**
 * If category not already set in the categories, add it.
 */
static void
add_category_and_author (
  PluginManager * self,
  char *          category,
  char *          author)
{
  g_return_if_fail (category);
  if (!string_is_ascii (category))
    {
      g_warning (
        "Ignoring non-ASCII plugin category "
        "name...");
    }
  bool ignore_category = false;
  for (int i = 0; i < self->num_plugin_categories; i++)
    {
      char * cat = self->plugin_categories[i];
      if (!strcmp (cat, category))
        {
          ignore_category = true;
          break;
        }
    }
  if (!ignore_category)
    {
      g_message ("%s: %s", __func__, category);
      self->plugin_categories[self->num_plugin_categories++] =
        g_strdup (category);
    }

  if (author)
    {
      bool ignore_author = false;
      for (int i = 0; i < self->num_plugin_authors; i++)
        {
          char * cat = self->plugin_authors[i];
          if (!strcmp (cat, author))
            {
              ignore_author = true;
              break;
            }
        }

      if (!ignore_author)
        {
          g_message ("%s: %s", __func__, author);
          self->plugin_authors[self->num_plugin_authors++] =
            g_strdup (author);
        }
    }
}

static int
sort_plugin_func (const void * a, const void * b)
{
  PluginDescriptor * pa = *(PluginDescriptor * const *) a;
  PluginDescriptor * pb = *(PluginDescriptor * const *) b;
  g_return_val_if_fail (pa->name && pb->name, -1);
  int r = strcasecmp (pa->name, pb->name);
  if (r)
    return r;

  /* if equal ignoring case, use opposite of strcmp()
   * result to get lower before upper */
  /* aka: return strcmp(b, a); */
  return -strcmp (pa->name, pb->name);
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
 * Returns a cached LilvNode for the given URI.
 *
 * If a node doesn't exist for the given URI, a
 * node is created and cached.
 */
const LilvNode *
plugin_manager_get_node (PluginManager * self, const char * uri)
{
  for (int i = 0; i < self->num_nodes; i++)
    {
      LilvNode * node = self->nodes[i];
      g_return_val_if_fail (lilv_node_is_uri (node), NULL);
      const char * node_uri = lilv_node_as_uri (node);
      if (string_is_equal (node_uri, uri))
        {
          return node;
        }
    }

  array_double_size_if_full (
    self->nodes, self->num_nodes, self->nodes_size,
    LilvNode *);
  LilvNode * new_node = lilv_new_uri (LILV_WORLD, uri);
  array_append (self->nodes, self->num_nodes, new_node);

  return new_node;
}

static void
create_and_load_lilv_word (PluginManager * self)
{
  g_message ("Creating Lilv World...");
  LilvWorld * world = lilv_world_new ();
  self->lilv_world = world;

  /* load all installed plugins on system */
  self->lv2_path = NULL;
  LilvNode * lv2_path = NULL;
  char *     env_lv2_path = getenv ("LV2_PATH");
  char *     builtin_plugins_path =
    zrythm_get_dir (ZRYTHM_DIR_SYSTEM_BUNDLED_PLUGINSDIR);
  char * special_plugins_path = zrythm_get_dir (
    ZRYTHM_DIR_SYSTEM_SPECIAL_LV2_PLUGINS_DIR);
  char * extra_zrythm_plugin_paths = g_strdup_printf (
    "%s" G_SEARCHPATH_SEPARATOR_S "%s", builtin_plugins_path,
    special_plugins_path);
  g_free_and_null (builtin_plugins_path);
  g_free_and_null (special_plugins_path);
  if (env_lv2_path && (strlen (env_lv2_path) > 0))
    {
      self->lv2_path = g_strdup_printf (
        "%s" G_SEARCHPATH_SEPARATOR_S "%s", env_lv2_path,
        extra_zrythm_plugin_paths);
    }
  /* else if no LV2_PATH passed */
  else
    {
#ifdef FLATPAK_BUILD
      self->lv2_path = g_strdup_printf (
        "%s/.lv2:/app/lib/lv2:"
        "/app/extensions/Plugins/lv2:%s",
        g_get_home_dir (), extra_zrythm_plugin_paths);
#elif defined(_WOE32)
      char * appdata_path = windows_get_special_path (
        WINDOWS_SPECIAL_PATH_APPDATA);
      char * common_program_files_path =
        windows_get_special_path (
          WINDOWS_SPECIAL_PATH_COMMON_PROGRAM_FILES);
      self->lv2_path = g_strdup_printf (
        "%s" G_SEARCHPATH_SEPARATOR_S
        "%s" G_SEARCHPATH_SEPARATOR_S "%s",
        appdata_path, common_program_files_path,
        extra_zrythm_plugin_paths);
      g_free_and_null (appdata_path);
      g_free_and_null (common_program_files_path);
#elif defined(__APPLE__)
      self->lv2_path = g_strdup_printf (
        "%s/Library/Audio/Plug-Ins/LV2:"
        "/Library/Audio/Plug-Ins/LV2:"
        "/usr/local/lib/lv2:/usr/lib/lv2:%s",
        g_get_home_dir (), extra_zrythm_plugin_paths);
#elif defined(INSTALLER_VER) // GNU or similar
      self->lv2_path = g_strdup_printf (
        "%s/.lv2:"
        "/usr/local/lib64/lv2:"
        "/usr/local/lib/lv2:"
        "/usr/lib64/lv2:"
        "/usr/lib/lv2:%s",
        g_get_home_dir (), extra_zrythm_plugin_paths);
#else                        // GNU or similar
      if (string_is_equal (LIBDIR_NAME, "lib"))
        {
          self->lv2_path = g_strdup_printf (
            "%s/.lv2:/usr/local/lib/lv2:"
            "/usr/lib/lv2:%s",
            g_get_home_dir (), extra_zrythm_plugin_paths);
        }
      else
        {
          self->lv2_path = g_strdup_printf (
            "%s/.lv2:/usr/local/" LIBDIR_NAME
            "/lv2:/usr/" LIBDIR_NAME
            "/lv2:"
            /* some distros report the wrong
               * LIBDIR_NAME so hardcode these */
            "/usr/local/lib/lv2:/usr/lib/lv2:%s",
            g_get_home_dir (), extra_zrythm_plugin_paths);
        }
#endif                       // end per-OS logic
    }

  g_message ("LV2 path: %s", self->lv2_path);

  g_return_if_fail (self->lv2_path);

  /* add test plugins if testing */
  if (ZRYTHM_TESTING)
    {
      const char * tests_builddir =
        g_getenv ("G_TEST_BUILDDIR");
      const char * root_builddir =
        g_getenv ("G_TEST_BUILD_ROOT_DIR");
      g_return_if_fail (tests_builddir);
      g_return_if_fail (root_builddir);

      char * before_path = self->lv2_path;
      self->lv2_path = g_strdup_printf (
        "%s:%s/lv2plugins:%s/data/plugins", before_path,
        tests_builddir, root_builddir);
      g_free (before_path);
    }

  lv2_path = lilv_new_string (world, self->lv2_path);

  g_message ("%s: LV2 path: %s", __func__, self->lv2_path);
  lilv_world_set_option (
    world, LILV_OPTION_LV2_PATH, lv2_path);

  if (ZRYTHM_TESTING)
    {
      g_message (
        "%s: loading specifications and plugin "
        "classes...",
        __func__);
      lilv_world_load_specifications (world);
      lilv_world_load_plugin_classes (world);
    }
  else
    {
      g_message ("%s: loading all...", __func__);
      lilv_world_load_all (world);
    }
}

static void
init_symap (PluginManager * self)
{
  /* symap URIDs */
#define SYMAP_MAP(target, uri) \
  self->urids.target = symap_map (self->symap, uri);

  SYMAP_MAP (atom_Float, LV2_ATOM__Float);
  SYMAP_MAP (atom_Int, LV2_ATOM__Int);
  SYMAP_MAP (atom_Object, LV2_ATOM__Object);
  SYMAP_MAP (atom_Path, LV2_ATOM__Path);
  SYMAP_MAP (atom_String, LV2_ATOM__String);
  SYMAP_MAP (atom_eventTransfer, LV2_ATOM__eventTransfer);
  SYMAP_MAP (
    bufsz_maxBlockLength, LV2_BUF_SIZE__maxBlockLength);
  SYMAP_MAP (
    bufsz_minBlockLength, LV2_BUF_SIZE__minBlockLength);
  SYMAP_MAP (
    bufsz_nominalBlockLength,
    LV2_BUF_SIZE__nominalBlockLength);
  SYMAP_MAP (bufsz_sequenceSize, LV2_BUF_SIZE__sequenceSize);
  SYMAP_MAP (log_Error, LV2_LOG__Error);
  SYMAP_MAP (log_Trace, LV2_LOG__Trace);
  SYMAP_MAP (log_Warning, LV2_LOG__Warning);
  SYMAP_MAP (midi_MidiEvent, LV2_MIDI__MidiEvent);
  SYMAP_MAP (param_sampleRate, LV2_PARAMETERS__sampleRate);
  SYMAP_MAP (patch_Get, LV2_PATCH__Get);
  SYMAP_MAP (patch_Put, LV2_PATCH__Put);
  SYMAP_MAP (patch_Set, LV2_PATCH__Set);
  SYMAP_MAP (patch_body, LV2_PATCH__body);
  SYMAP_MAP (patch_property, LV2_PATCH__property);
  SYMAP_MAP (patch_value, LV2_PATCH__value);
  SYMAP_MAP (time_Position, LV2_TIME__Position);
  SYMAP_MAP (time_bar, LV2_TIME__bar);
  SYMAP_MAP (time_barBeat, LV2_TIME__barBeat);
  SYMAP_MAP (time_beatUnit, LV2_TIME__beatUnit);
  SYMAP_MAP (time_beatsPerBar, LV2_TIME__beatsPerBar);
  SYMAP_MAP (time_beatsPerMinute, LV2_TIME__beatsPerMinute);
  SYMAP_MAP (time_frame, LV2_TIME__frame);
  SYMAP_MAP (time_speed, LV2_TIME__speed);
  SYMAP_MAP (ui_updateRate, LV2_UI__updateRate);
#ifdef HAVE_LV2_1_18
  SYMAP_MAP (ui_scaleFactor, LV2_UI__scaleFactor);
#endif

  SYMAP_MAP (z_hostInfo_name, Z_LV2_HOST_INFO__name);
  SYMAP_MAP (z_hostInfo_version, Z_LV2_HOST_INFO__version);
#undef SYMAP_MAP
}

static void
load_bundled_lv2_plugins (PluginManager * self)
{
#ifndef _WOE32
  GError *     err;
  const char * path = CONFIGURE_ZRYTHM_LIBDIR "/lv2";
  if (g_file_test (
        path, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR))
    {
      GDir * bundle_lv2_dir = g_dir_open (path, 0, &err);
      if (bundle_lv2_dir)
        {
          const char * dir;
          char *       str;
          while ((dir = g_dir_read_name (bundle_lv2_dir)))
            {
              str = g_strdup_printf (
                "file://%s%s%s%smanifest.ttl", path,
                G_DIR_SEPARATOR_S, dir, G_DIR_SEPARATOR_S);
              LilvNode * uri =
                lilv_new_uri (self->lilv_world, str);
              lilv_world_load_bundle (self->lilv_world, uri);
              g_message ("Loaded bundled plugin at %s", str);
              g_free (str);
              lilv_node_free (uri);
            }
          g_dir_close (bundle_lv2_dir);
        }
      else
        {
          char * msg = g_strdup_printf (
            "%s%s", _ ("Error loading LV2 bundle dir: "),
            err->message);
          if (ZRYTHM_HAVE_UI)
            {
              ui_show_error_message (NULL, msg);
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

  self->plugin_descriptors = g_ptr_array_new_full (
    100, (GDestroyNotify) plugin_descriptor_free);

  self->symap = symap_new ();
  zix_sem_init (&self->symap_lock, 1);

  self->nodes_size = 1;
  self->nodes =
    malloc (self->nodes_size * sizeof (LilvNode *));

  /* init lv2 */
  create_and_load_lilv_word (self);
  init_symap (self);
  load_bundled_lv2_plugins (self);

  /* init vst/dssi/ladspa */
  self->cached_plugin_descriptors =
    cached_plugin_descriptors_new ();

  /* fetch/create collections */
  self->collections = plugin_collections_new ();

  return self;
}

#ifdef HAVE_CARLA
static char **
get_vst_paths (PluginManager * self)
{
  g_message ("%s: getting paths...", __func__);

#  ifdef _WOE32
  char ** paths;
  if (ZRYTHM_TESTING)
    {
      GStrvBuilder * builder = g_strv_builder_new ();
      paths = g_strv_builder_end (builder);
    }
  else
    {
      paths = g_settings_get_strv (
        S_P_PLUGINS_PATHS, "vst-search-paths-windows");
    }
  g_return_val_if_fail (paths, NULL);
#  elif defined(__APPLE__)
  char ** paths = g_strsplit (
    "/Library/Audio/Plug-ins/VST" G_SEARCHPATH_SEPARATOR_S,
    G_SEARCHPATH_SEPARATOR_S, -1);
#  else
  char * vst_path = g_strdup (getenv ("VST_PATH"));
  if (!vst_path || (strlen (vst_path) == 0))
    {
#    ifdef FLATPAK_BUILD
      vst_path = g_strdup ("/app/extensions/Plugins/vst");
#    elif defined(INSTALLER_VER)
      vst_path = g_strdup_printf (
        "%s/.vst:%s/vst:"
        "/usr/lib/vst:"
        "/usr/lib64/vst:"
        "/usr/local/lib/vst:"
        "/usr/local/lib64/vst",
        g_get_home_dir (), g_get_home_dir ());
#    else  /* else if not installer ver */
      if (string_is_equal (LIBDIR_NAME, "lib"))
        {
          vst_path = g_strdup_printf (
            "%s/.vst:%s/vst:"
            "/usr/" LIBDIR_NAME
            "/vst:"
            "/usr/local/" LIBDIR_NAME "/vst",
            g_get_home_dir (), g_get_home_dir ());
        }
      else
        {
          vst_path = g_strdup_printf (
            "%s/.vst:%s/vst:"
            "/usr/lib/vst:"
            "/usr/" LIBDIR_NAME
            "/vst:"
            "/usr/local/lib/vst:"
            "/usr/local/" LIBDIR_NAME "/vst",
            g_get_home_dir (), g_get_home_dir ());
        }
#    endif /* FLATPAK_BUILD */

      g_message ("Using standard VST paths: %s", vst_path);
    }
  else
    {
      g_message (
        "using %s from the environment (VST_PATH)", vst_path);
    }
  g_return_val_if_fail (vst_path, NULL);
  char ** paths =
    g_strsplit (vst_path, G_SEARCHPATH_SEPARATOR_S, 0);
  g_free (vst_path);
#  endif   // __APPLE__

  g_message ("%s: done", __func__);

  return paths;
}

static char **
get_vst3_paths (PluginManager * self)
{
#  ifdef _WOE32
  return g_strsplit (
    "C:\\Program Files\\Common Files\\VST3" G_SEARCHPATH_SEPARATOR_S
    "C:\\Program Files (x86)\\Common Files\\VST3",
    G_SEARCHPATH_SEPARATOR_S, 0);
#  elif defined(__APPLE__)
  return g_strsplit (
    "/Library/Audio/Plug-ins/VST3" G_SEARCHPATH_SEPARATOR_S,
    G_SEARCHPATH_SEPARATOR_S, -1);
#  else
  char * vst_path = g_strdup (getenv ("VST3_PATH"));
  if (!vst_path || (strlen (vst_path) == 0))
    {
#    ifdef FLATPAK_BUILD
      vst_path = g_strdup ("/app/extensions/Plugins/vst3");
#    elif defined(INSTALLER_VER)
      vst_path = g_strdup_printf (
        "%s/.vst3:"
        "/usr/lib/vst3:"
        "/usr/lib64/vst3:"
        "/usr/local/lib/vst3:"
        "/usr/local/lib64/vst3",
        g_get_home_dir ());
#    else /* else if not installer ver */
      if (string_is_equal (LIBDIR_NAME, "lib"))
        {
          vst_path = g_strdup_printf (
            "%s/.vst3:"
            "/usr/lib/vst3:"
            "/usr/local/lib/vst3",
            g_get_home_dir ());
        }
      else
        {
          vst_path = g_strdup_printf (
            "%s/.vst3:"
            "/usr/lib/vst3:"
            "/usr/" LIBDIR_NAME
            "/vst3:"
            "/usr/local/lib/vst3:"
            "/usr/local/" LIBDIR_NAME "/vst3",
            g_get_home_dir ());
        }
#    endif

      g_message ("Using standard VST3 paths: %s", vst_path);
    }
  else
    {
      g_message (
        "using %s from the environment (VST3_PATH)", vst_path);
    }
  g_return_val_if_fail (vst_path, NULL);
  char ** paths =
    g_strsplit (vst_path, G_SEARCHPATH_SEPARATOR_S, 0);
  g_free (vst_path);
  return paths;
#  endif // __APPLE__
}

static int
get_vst3_count (PluginManager * self)
{
  char ** paths = get_vst3_paths (self);
  g_return_val_if_fail (paths, 0);
  int    path_idx = 0;
  char * path;
  int    count = 0;
  while ((path = paths[path_idx++]) != NULL)
    {
      if (!g_file_test (path, G_FILE_TEST_EXISTS))
        continue;

      char ** vst_plugins = io_get_files_in_dir_ending_in (
        path, 1, ".vst3", false);
      if (!vst_plugins)
        continue;

      char * plugin_path;
      int    plugin_idx = 0;
      while ((plugin_path = vst_plugins[plugin_idx++]) != NULL)
        {
          count++;
        }
      g_strfreev (vst_plugins);
    }
  g_strfreev (paths);

  return count;
}

static int
get_vst_count (PluginManager * self)
{
  char ** paths = get_vst_paths (self);
  g_return_val_if_fail (paths, 0);
  int    path_idx = 0;
  char * path;
  int    count = 0;
  while ((path = paths[path_idx++]) != NULL)
    {
      if (!g_file_test (path, G_FILE_TEST_EXISTS))
        continue;

      char ** vst_plugins = io_get_files_in_dir_ending_in (
        path, 1, LIB_SUFFIX, false);
      if (!vst_plugins)
        continue;

      char * plugin_path;
      int    plugin_idx = 0;
      while ((plugin_path = vst_plugins[plugin_idx++]) != NULL)
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
get_sf_paths (PluginManager * self, bool sf2)
{
  char ** paths = NULL;
  if (ZRYTHM_TESTING)
    {
      paths = g_strsplit (
        G_SEARCHPATH_SEPARATOR_S, G_SEARCHPATH_SEPARATOR_S,
        -1);
    }
  else
    {
      paths = g_settings_get_strv (
        S_P_PLUGINS_PATHS,
        sf2 ? "sf2-search-paths" : "sfz-search-paths");
      g_return_val_if_fail (paths, NULL);
    }

  return paths;
}

static int
get_sf_count (PluginManager * self, ZPluginProtocol prot)
{
  char ** paths =
    get_sf_paths (self, prot == Z_PLUGIN_PROTOCOL_SF2);
  g_return_val_if_fail (paths, 0);
  int    path_idx = 0;
  char * path;
  int    count = 0;
  while ((path = paths[path_idx++]) != NULL)
    {
      if (!g_file_test (path, G_FILE_TEST_EXISTS))
        continue;

      char ** sf_instruments = io_get_files_in_dir_ending_in (
        path, 1,
        (prot == Z_PLUGIN_PROTOCOL_SFZ) ? ".sfz" : ".sf2",
        false);
      if (!sf_instruments)
        continue;

      char * plugin_path;
      int    plugin_idx = 0;
      while (
        (plugin_path = sf_instruments[plugin_idx++]) != NULL)
        {
          count++;
        }
      g_strfreev (sf_instruments);
    }
  g_strfreev (paths);

  return count;
}

static char **
get_dssi_paths (PluginManager * self)
{
  g_debug ("%s: getting paths...", __func__);

  char * dssi_path = g_strdup (getenv ("DSSI_PATH"));
  if (!dssi_path || (strlen (dssi_path) == 0))
    {
#  ifdef FLATPAK_BUILD
      dssi_path = g_strdup ("/app/extensions/Plugins/dssi");
#  elif defined(INSTALLER_VER)
      dssi_path = g_strdup (
        "/usr/lib/dssi:"
        "/usr/lib64/dssi:"
        "/usr/local/lib/dssi:"
        "/usr/local/lib64/dssi");
#  else  /* else if not installer ver */
      if (string_is_equal (LIBDIR_NAME, "lib"))
        {
          dssi_path = g_strdup (
            "/usr/" LIBDIR_NAME
            "/dssi:"
            "/usr/local/" LIBDIR_NAME "/dssi");
        }
      else
        {
          dssi_path = g_strdup (
            "/usr/lib/dssi:"
            "/usr/" LIBDIR_NAME
            "/dssi:"
            "/usr/local/lib/dssi:"
            "/usr/local/" LIBDIR_NAME "/dssi");
        }
#  endif /* flatpak build */

      g_message ("Using standard DSSI paths: %s", dssi_path);
    }
  else
    {
      g_message (
        "using %s from the environment (DSSI_PATH)",
        dssi_path);
    }
  char ** paths =
    g_strsplit (dssi_path, G_SEARCHPATH_SEPARATOR_S, 0);
  g_free (dssi_path);

  g_debug ("%s: done", __func__);

  return paths;
}

static char **
get_ladspa_paths (PluginManager * self)
{
  g_debug ("%s: getting paths...", __func__);

  char * ladspa_path = g_strdup (getenv ("LADSPA_PATH"));
  if (!ladspa_path || (strlen (ladspa_path) == 0))
    {
#  ifdef FLATPAK_BUILD
      ladspa_path =
        g_strdup ("/app/extensions/Plugins/ladspa");
#  elif defined(INSTALLER_VER)
      ladspa_path = g_strdup (
        "/usr/lib/ladspa:"
        "/usr/lib64/ladspa:"
        "/usr/local/lib/ladspa:"
        "/usr/local/lib64/ladspa");
#  else  /* else if not installer ver */
      if (string_is_equal (LIBDIR_NAME, "lib"))
        {
          ladspa_path = g_strdup (
            "/usr/" LIBDIR_NAME
            "/ladspa:"
            "/usr/local/" LIBDIR_NAME "/ladspa");
        }
      else
        {
          ladspa_path = g_strdup (
            "/usr/lib/ladspa:"
            "/usr/" LIBDIR_NAME
            "/ladspa:"
            "/usr/local/lib/ladspa:"
            "/usr/local/" LIBDIR_NAME "/ladspa");
        }
#  endif /* flatpak build */

      g_message (
        "Using standard LADSPA paths: %s", ladspa_path);
    }
  else
    {
      g_message (
        "using %s from the environment (LADSPA)", ladspa_path);
    }
  char ** paths =
    g_strsplit (ladspa_path, G_SEARCHPATH_SEPARATOR_S, 0);
  g_free (ladspa_path);

  g_debug ("%s: done", __func__);

  return paths;
}

/**
 * For the official paths see:
 * https://github.com/free-audio/clap/blob/b902efa94e7a069dc1576617ebd74d2310746bd4/include/clap/entry.h#L14
 */
static char **
get_clap_paths (PluginManager * self)
{
  g_message ("%s: getting paths...", __func__);

#  ifdef _WOE32
  char ** paths = g_strsplit (
    "C:\\Program Files\\Common Files\\CLAP" G_SEARCHPATH_SEPARATOR_S
    "C:\\Program Files (x86)\\Common Files\\CLAP",
    G_SEARCHPATH_SEPARATOR_S, 0);
#  elif defined(__APPLE__)
  char ** paths = g_strsplit (
    "/Library/Audio/Plug-ins/CLAP" G_SEARCHPATH_SEPARATOR_S,
    G_SEARCHPATH_SEPARATOR_S, -1);
#  else
  char * clap_path = g_strdup (getenv ("CLAP_PATH"));
  if (!clap_path || (strlen (clap_path) == 0))
    {
#    ifdef FLATPAK_BUILD
      clap_path = g_strdup ("/app/extensions/Plugins/clap");
#    elif defined(INSTALLER_VER)
      clap_path = g_strdup_printf (
        "%s/.clap:%s/.local/lib/clap:"
        "/usr/lib/clap:"
        "/usr/lib64/clap:"
        "/usr/local/lib/clap:"
        "/usr/local/lib64/clap",
        g_get_home_dir (), g_get_home_dir ());
#    else /* else if not flatpak && not installer ver */
      if (string_is_equal (LIBDIR_NAME, "lib"))
        {
          clap_path = g_strdup_printf (
            "%s/.clap:%s/.local/lib/clap:"
            "/usr/" LIBDIR_NAME
            "/clap:"
            "/usr/local/" LIBDIR_NAME "/clap",
            g_get_home_dir (), g_get_home_dir ());
        }
      else
        {
          clap_path = g_strdup_printf (
            "%s/.clap:%s/.local/lib/clap:"
            "/usr/lib/clap:"
            "/usr/" LIBDIR_NAME
            "/clap:"
            "/usr/local/lib/clap:"
            "/usr/local/" LIBDIR_NAME "/clap",
            g_get_home_dir (), g_get_home_dir ());
        }
#    endif

      g_message ("Using standard CLAP paths: %s", clap_path);
    }
  else
    {
      g_message (
        "using %s from the environment (CLAP_PATH)",
        clap_path);
    }
  g_return_val_if_fail (clap_path, NULL);
  char ** paths =
    g_strsplit (clap_path, G_SEARCHPATH_SEPARATOR_S, 0);
  g_free (clap_path);
#  endif // __APPLE__

  g_message ("%s: done", __func__);

  return paths;
}

static int
get_clap_count (PluginManager * self)
{
  char ** paths = get_clap_paths (self);
  g_return_val_if_fail (paths, 0);
  int    path_idx = 0;
  char * path;
  int    count = 0;
  while ((path = paths[path_idx++]) != NULL)
    {
      if (!g_file_test (path, G_FILE_TEST_EXISTS))
        continue;

      char ** clap_plugins = io_get_files_in_dir_ending_in (
        path, 1, ".clap", false);
      if (!clap_plugins)
        continue;

      char * plugin_path;
      int    plugin_idx = 0;
      while (
        (plugin_path = clap_plugins[plugin_idx++]) != NULL)
        {
          count++;
        }
      g_strfreev (clap_plugins);
    }
  g_strfreev (paths);

  return count;
}

static char **
get_jsfx_paths (PluginManager * self)
{
  g_message ("%s: getting paths...", __func__);

  char ** env_paths = NULL;

  char * jsfx_env_path = g_strdup (getenv ("JSFX_PATH"));
  if (jsfx_env_path)
    {
      if (strlen (jsfx_env_path) > 0)
        {
          g_message (
            "Prepending %s from the environment (JSFX_PATH)",
            jsfx_env_path);
          env_paths = g_strsplit (
            jsfx_env_path, G_SEARCHPATH_SEPARATOR_S, 0);
        }
      g_free (jsfx_env_path);
    }

  /* append paths from preferences */
  char ** pref_paths;
  if (ZRYTHM_TESTING)
    {
      GStrvBuilder * builder = g_strv_builder_new ();
      pref_paths = g_strv_builder_end (builder);
    }
  else
    {
      pref_paths = g_settings_get_strv (
        S_P_PLUGINS_PATHS, "jsfx-search-paths");
    }
  g_return_val_if_fail (pref_paths, NULL);

  GStrvBuilder * builder = g_strv_builder_new ();
  if (env_paths)
    {
      g_strv_builder_addv (builder, (const char **) env_paths);
      g_strfreev (env_paths);
      env_paths = NULL;
    }
  /* FIXME skip duplicates, otherwise plugins show twice */
  g_strv_builder_addv (builder, (const char **) pref_paths);
  g_strfreev (pref_paths);

  char ** paths = g_strv_builder_end (builder);

  g_message ("%s: done", __func__);

  return paths;
}

static int
get_jsfx_count (PluginManager * self)
{
  char ** paths = get_jsfx_paths (self);
  g_return_val_if_fail (paths, 0);
  int    path_idx = 0;
  char * path;
  int    count = 0;
  while ((path = paths[path_idx++]) != NULL)
    {
      if (!g_file_test (path, G_FILE_TEST_EXISTS))
        continue;

      char ** jsfx_plugins = io_get_files_in_dir_ending_in (
        path, 1, ".jsfx", false);
      if (!jsfx_plugins)
        continue;

      char * plugin_path;
      int    plugin_idx = 0;
      while (
        (plugin_path = jsfx_plugins[plugin_idx++]) != NULL)
        {
          count++;
        }
      g_strfreev (jsfx_plugins);
    }
  g_strfreev (paths);

  return count;
}
#endif /* HAVE_CARLA */

/**
 * Returns if the plugin manager supports the given
 * plugin protocol.
 */
bool
plugin_manager_supports_protocol (
  PluginManager * self,
  ZPluginProtocol protocol)
{
  switch (protocol)
    {
    case Z_PLUGIN_PROTOCOL_DUMMY:
    case Z_PLUGIN_PROTOCOL_LV2:
      return true;
    case Z_PLUGIN_PROTOCOL_LADSPA:
    case Z_PLUGIN_PROTOCOL_VST:
    case Z_PLUGIN_PROTOCOL_VST3:
    case Z_PLUGIN_PROTOCOL_SFZ:
    case Z_PLUGIN_PROTOCOL_JSFX:
    case Z_PLUGIN_PROTOCOL_CLAP:
#ifdef HAVE_CARLA
      return true;
#else
      return false;
#endif
    case Z_PLUGIN_PROTOCOL_DSSI:
    case Z_PLUGIN_PROTOCOL_AU:
    case Z_PLUGIN_PROTOCOL_SF2:
      {
#ifdef HAVE_CARLA
        const char * const * carla_features =
          carla_get_supported_features ();
        const char * feature;
        int          i = 0;
        while ((feature = carla_features[i++]))
          {
#  define CHECK_FEATURE(str, format) \
    if ( \
      string_is_equal (feature, str) \
      && protocol == Z_PLUGIN_PROTOCOL_##format) \
    return true

            CHECK_FEATURE ("sf2", SF2);
            CHECK_FEATURE ("osc", DSSI);
            CHECK_FEATURE ("vst3", VST3);
            CHECK_FEATURE ("au", AU);
#  undef CHECK_FEATURE
          }
#endif /* HAVE_CARLA */
        return false;
      }
    }
  return false;
}

#ifdef HAVE_CARLA
/**
 * Used for plugin protocols that are scanned from paths.
 */
static void
scan_carla_descriptors_from_paths (
  PluginManager * self,
  ZPluginProtocol protocol,
  unsigned int *  count,
  const double    size,
  double *        progress,
  const double    start_progress,
  const double    max_progress)
{

  const char * protocol_str =
    plugin_protocol_to_str (protocol);
  if (!plugin_manager_supports_protocol (self, protocol))
    {
      g_warning (
        "Plugin protocol %s not supported in this "
        "build",
        protocol_str);
      return;
    }
  g_message ("Scanning %s plugins...", protocol_str);

  /* get paths and suffix */
  char **      paths = NULL;
  const char * suffix = NULL;
  switch (protocol)
    {
    case Z_PLUGIN_PROTOCOL_VST:
      paths = get_vst_paths (self);
#  ifdef __APPLE__
      suffix = ".vst";
#  else
      suffix = LIB_SUFFIX;
#  endif
      break;
    case Z_PLUGIN_PROTOCOL_VST3:
      paths = get_vst3_paths (self);
      suffix = ".vst3";
      break;
    case Z_PLUGIN_PROTOCOL_DSSI:
      paths = get_dssi_paths (self);
      suffix = LIB_SUFFIX;
      break;
    case Z_PLUGIN_PROTOCOL_LADSPA:
      paths = get_ladspa_paths (self);
      suffix = LIB_SUFFIX;
      break;
    case Z_PLUGIN_PROTOCOL_SFZ:
      paths = get_sf_paths (self, false);
      suffix = ".sfz";
      break;
    case Z_PLUGIN_PROTOCOL_SF2:
      paths = get_sf_paths (self, true);
      suffix = ".sf2";
      break;
    case Z_PLUGIN_PROTOCOL_CLAP:
      paths = get_clap_paths (self);
      suffix = ".clap";
      break;
    case Z_PLUGIN_PROTOCOL_JSFX:
      paths = get_jsfx_paths (self);
      suffix = ".jsfx";
      break;
    default:
      break;
    }
  g_return_if_fail (paths && suffix);

  int    path_idx = 0;
  char * path;
  while ((path = paths[path_idx++]) != NULL)
    {
      if (!g_file_test (path, G_FILE_TEST_EXISTS))
        continue;

      g_message (
        "scanning for %s plugins in %s", protocol_str, path);

      char ** plugins = io_get_files_in_dir_ending_in (
        path, 1, suffix, false);
      if (!plugins)
        continue;

      char * plugin_path;
      int    plugin_idx = 0;
      while ((plugin_path = plugins[plugin_idx++]) != NULL)
        {
          PluginDescriptor ** descriptors =
            cached_plugin_descriptors_get (
              self->cached_plugin_descriptors, plugin_path);

          /* if any cached descriptors are found */
          if (descriptors)
            {
              /* clone and add them to the list
               * of descriptors */
              PluginDescriptor * descriptor = NULL;
              int                i = 0;
              while ((descriptor = descriptors[i++]))
                {
                  bool added = false;
                  if (!g_ptr_array_find_with_equal_func (
                        self->plugin_descriptors, descriptor,
                        (GEqualFunc)
                          plugin_descriptor_is_same_plugin,
                        NULL))
                    {
                      PluginDescriptor * clone =
                        plugin_descriptor_clone (descriptor);
                      g_ptr_array_add (
                        self->plugin_descriptors, clone);
                      add_category_and_author (
                        self, clone->category_str,
                        clone->author);
                      added = true;
                    }
                  g_debug (
                    "Found cached %s %s%s", protocol_str,
                    descriptor->name,
                    added ? "" : " (skipped)");
                }
            }
          /* if no cached descriptors found */
          else
            {
              g_debug (
                "No cached descriptors found for "
                "%s",
                plugin_path);
              if (cached_plugin_descriptors_is_blacklisted (
                    self->cached_plugin_descriptors,
                    plugin_path))
                {
                  g_message (
                    "Ignoring blacklisted %s "
                    "plugin: %s",
                    protocol_str, plugin_path);
                }
              else
                {
                  if (
                    protocol == Z_PLUGIN_PROTOCOL_SFZ
                    || protocol == Z_PLUGIN_PROTOCOL_SF2)
                    {
                      descriptors =
                        object_new_n (2, PluginDescriptor *);
                      descriptors[0] =
                        plugin_descriptor_new ();
                      PluginDescriptor * descr =
                        descriptors[0];
                      descr->path = g_strdup (plugin_path);
                      GFile * file =
                        g_file_new_for_path (descr->path);
                      descr->ghash = g_file_hash (file);
                      g_object_unref (file);
                      descr->category = PC_INSTRUMENT;
                      descr->category_str =
                        plugin_descriptor_category_to_string (
                          descr->category);
                      descr->name =
                        io_path_get_basename_without_ext (
                          plugin_path);
                      char * parent_path =
                        io_path_get_parent_dir (plugin_path);
                      if (!parent_path)
                        {
                          g_warning (
                            "Failed to get parent dir of "
                            "%s",
                            plugin_path);
                          plugin_descriptor_free (descr);
                          descriptors[0] = NULL;
                          continue;
                        }
                      descr->author =
                        g_path_get_basename (parent_path);
                      g_free (parent_path);
                      descr->num_audio_outs = 2;
                      descr->num_midi_ins = 1;
                      descr->arch = ARCH_64;
                      descr->protocol = protocol;
                    }
                  /* else if not SFZ or SF2 */
                  else
                    {
                      descriptors =
                        z_carla_discovery_create_descriptors_from_file (
                          plugin_path, ARCH_64, protocol);

                      /* try 32-bit if above failed */
                      if (!descriptors)
                        {
                          g_debug (
                            "no descriptors for %s, "
                            "trying 32bit...",
                            plugin_path);
                          descriptors =
                            z_carla_discovery_create_descriptors_from_file (
                              plugin_path, ARCH_32, protocol);
                        }
                    }

                  g_debug (
                    "descriptors for %s: %p", plugin_path,
                    descriptors);

                  if (descriptors)
                    {
                      PluginDescriptor * descriptor = NULL;
                      int                i = 0;
                      while ((descriptor = descriptors[i++]))
                        {
                          g_ptr_array_add (
                            self->plugin_descriptors,
                            descriptor);
                          add_category_and_author (
                            self, descriptor->category_str,
                            descriptor->author);
                          g_message (
                            "Caching %s %s", protocol_str,
                            descriptor->name);

                          PluginDescriptor * clone =
                            plugin_descriptor_clone (
                              descriptor);
                          cached_plugin_descriptors_add (
                            self->cached_plugin_descriptors,
                            clone, F_NO_SERIALIZE);
                        }
                      g_debug (
                        "%d descriptors cached for "
                        "%s",
                        i - 1, plugin_path);
                    }
                  else
                    {
                      g_message (
                        "Blacklisting %s %s", protocol_str,
                        plugin_path);
                      cached_plugin_descriptors_blacklist (
                        self->cached_plugin_descriptors,
                        plugin_path, 0);
                    }
                }
            }
          (*count)++;

          if (progress)
            {
              *progress =
                start_progress
                + ((double) *count / size)
                    * (max_progress - start_progress);
              char prog_str[800];
              if (descriptors)
                {
                  sprintf (
                    prog_str, _ ("Scanned %s plugin: %s"),
                    protocol_str, descriptors[0]->name);

                  free (descriptors);
                  descriptors = NULL;
                }
              else
                {
                  sprintf (
                    prog_str,
                    /* TRANSLATORS: first argument
                     * is plugin protocol, 2nd
                     * argument is path */
                    _ ("Skipped %1$s plugin at "
                       "%2$s"),
                    protocol_str, plugin_path);
                }
              zrythm_app_set_progress_status (
                zrythm_app, prog_str, *progress);
            }
        }
      if (plugin_idx > 0 && !ZRYTHM_TESTING)
        {
          cached_plugin_descriptors_serialize_to_file (
            self->cached_plugin_descriptors);
        }
      g_strfreev (plugins);
    }
  g_strfreev (paths);
}
#endif

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

  double start_progress = progress ? *progress : 0;

  /* load all plugins with lilv */
  LilvWorld *         world = self->lilv_world;
  const LilvPlugins * lilv_plugins =
    lilv_world_get_all_plugins (world);
  self->lilv_plugins = lilv_plugins;

  if (getenv ("ZRYTHM_SKIP_PLUGIN_SCAN"))
    return;

  double size = (double) lilv_plugins_size (lilv_plugins);
#ifdef HAVE_CARLA
  size += (double) get_vst_count (self);
  size += (double) get_vst3_count (self);
  size += (double) get_sf_count (self, Z_PLUGIN_PROTOCOL_SFZ);
  size += (double) get_sf_count (self, Z_PLUGIN_PROTOCOL_SF2);
  size += (double) get_clap_count (self);
  size += (double) get_jsfx_count (self);
#  ifdef __APPLE__
  size += carla_get_cached_plugin_count (PLUGIN_AU, NULL);
#  endif
#endif

  /* scan LV2 */
  g_message ("%s: Scanning LV2 plugins...", __func__);
  unsigned int count = 0;
  LILV_FOREACH (plugins, i, lilv_plugins)
    {
      const LilvPlugin * p =
        lilv_plugins_get (lilv_plugins, i);

      PluginDescriptor * descriptor =
        lv2_plugin_create_descriptor_from_lilv (p);

      if (descriptor)
        {
          /* add descriptor to list */
          g_ptr_array_add (
            self->plugin_descriptors, descriptor);
          add_category_and_author (
            self, descriptor->category_str,
            descriptor->author);

          /* update descriptor in cached */
          const PluginDescriptor * found_descr =
            cached_plugin_descriptors_find (
              self->cached_plugin_descriptors, descriptor,
              F_CHECK_VALID, F_CHECK_BLACKLISTED);
          if (found_descr)
            {
              if (
                found_descr->num_audio_ins
                  != descriptor->num_audio_ins
                || found_descr->num_audio_outs
                     != descriptor->num_audio_outs
                || found_descr->num_midi_ins
                     != descriptor->num_midi_ins
                || found_descr->num_midi_outs
                     != descriptor->num_midi_outs
                || found_descr->num_cv_ins != descriptor->num_cv_ins
                || found_descr->num_cv_outs
                     != descriptor->num_cv_outs)
                cached_plugin_descriptors_replace (
                  self->cached_plugin_descriptors, descriptor,
                  F_SERIALIZE);
            }
          else
            {
              cached_plugin_descriptors_add (
                self->cached_plugin_descriptors, descriptor,
                F_NO_SERIALIZE);
            }
        }

      count++;

      if (progress)
        {
          *progress =
            start_progress
            + ((double) count / size)
                * (max_progress - start_progress);
          char prog_str[800];
          if (descriptor)
            {
              sprintf (
                prog_str, "%s: %s", _ ("Scanned LV2 plugin"),
                descriptor->name);
            }
          else
            {
              const LilvNode * lv2_uri =
                lilv_plugin_get_uri (p);
              const char * uri_str =
                lilv_node_as_string (lv2_uri);
              sprintf (
                prog_str, _ ("Skipped LV2 plugin at %s"),
                uri_str);
            }
          zrythm_app_set_progress_status (
            zrythm_app, prog_str, *progress);
        }
    }
  g_message ("%s: Scanned %d LV2 plugins", __func__, count);

  cached_plugin_descriptors_serialize_to_file (
    self->cached_plugin_descriptors);

#ifdef HAVE_CARLA

#  if !defined(_WOE32) && !defined(__APPLE__)
  /* scan ladspa */
  scan_carla_descriptors_from_paths (
    self, Z_PLUGIN_PROTOCOL_LADSPA, &count, size, progress,
    start_progress, max_progress);

  /* scan dssi */
  scan_carla_descriptors_from_paths (
    self, Z_PLUGIN_PROTOCOL_DSSI, &count, size, progress,
    start_progress, max_progress);
#  endif /* not apple/woe32 */

  /* scan vst */
  scan_carla_descriptors_from_paths (
    self, Z_PLUGIN_PROTOCOL_VST, &count, size, progress,
    start_progress, max_progress);

  /* scan vst3 */
  scan_carla_descriptors_from_paths (
    self, Z_PLUGIN_PROTOCOL_VST3, &count, size, progress,
    start_progress, max_progress);

  /* scan sfz */
  scan_carla_descriptors_from_paths (
    self, Z_PLUGIN_PROTOCOL_SFZ, &count, size, progress,
    start_progress, max_progress);

  /* scan sf2 */
  scan_carla_descriptors_from_paths (
    self, Z_PLUGIN_PROTOCOL_SF2, &count, size, progress,
    start_progress, max_progress);

  /* scan clap */
  scan_carla_descriptors_from_paths (
    self, Z_PLUGIN_PROTOCOL_CLAP, &count, size, progress,
    start_progress, max_progress);

  /* scan jsfx */
  scan_carla_descriptors_from_paths (
    self, Z_PLUGIN_PROTOCOL_JSFX, &count, size, progress,
    start_progress, max_progress);

#  ifdef __APPLE__
  /* scan AU plugins */
  g_message ("Scanning AU plugins...");
  unsigned int au_count =
    carla_get_cached_plugin_count (PLUGIN_AU, NULL);
  char * all_plugins =
    z_carla_discovery_run (ARCH_64, "au", ":all");
  g_message ("all plugins %s", all_plugins);
  g_message ("%u plugins found", au_count);
  if (all_plugins)
    {
      for (unsigned int i = 0; i < au_count; i++)
        {
          PluginDescriptor * descriptor =
            z_carla_discovery_create_au_descriptor_from_string (
              all_plugins, (int) i);

          if (descriptor)
            {
              g_ptr_array_add (
                self->plugin_descriptors, descriptor);
              add_category_and_author (
                self, descriptor->category_str,
                descriptor->author);
            }

          count++;

          if (progress)
            {
              *progress =
                start_progress
                + ((double) count / size)
                    * (max_progress - start_progress);
              char prog_str[800];
              if (descriptor)
                {
                  sprintf (
                    prog_str, "%s: %s",
                    _ ("Scanned AU plugin"), descriptor->name);
                }
              else
                {
                  sprintf (
                    prog_str, _ ("Skipped AU plugin at %u"),
                    i);
                }
              zrythm_app_set_progress_status (
                zrythm_app, prog_str, *progress);
            }
        }
    }
  else
    {
      g_warning ("failed to get AU plugins");
    }
#  endif // __APPLE__
#endif   // HAVE_CARLA

  /* sort alphabetically */
  g_ptr_array_sort (
    self->plugin_descriptors, sort_plugin_func);
  qsort (
    self->plugin_categories,
    (size_t) self->num_plugin_categories, sizeof (char *),
    sort_alphabetical_func);
  qsort (
    self->plugin_authors, (size_t) self->num_plugin_authors,
    sizeof (char *), sort_alphabetical_func);

  g_message (
    "%s: %d Plugins scanned.", __func__,
    self->plugin_descriptors->len);

  /*print_plugins ();*/
}

/**
 * Returns the PluginDescriptor instance for the
 * given URI.
 *
 * This instance is held by the plugin manager and
 * must not be free'd.
 */
PluginDescriptor *
plugin_manager_find_plugin_from_uri (
  PluginManager * self,
  const char *    uri)
{
  for (size_t i = 0; i < self->plugin_descriptors->len; i++)
    {
      PluginDescriptor * descr =
        g_ptr_array_index (self->plugin_descriptors, i);
      if (string_is_equal (uri, descr->uri))
        {
          return descr;
        }
    }

  return NULL;
}

/**
 * Finds and returns the PluginDescriptor instance
 * matching the given descriptor.
 *
 * This instance is held by the plugin manager and
 * must not be free'd.
 */
PluginDescriptor *
plugin_manager_find_from_descriptor (
  PluginManager *          self,
  const PluginDescriptor * src_descr)
{
  g_return_val_if_fail (self && src_descr, NULL);

  for (size_t i = 0; i < self->plugin_descriptors->len; i++)
    {
      PluginDescriptor * descr =
        g_ptr_array_index (self->plugin_descriptors, i);
      if (plugin_descriptor_is_same_plugin (src_descr, descr))
        {
          return descr;
        }
    }

  g_message ("descriptor for %s not found", src_descr->name);
  return NULL;
}

/**
 * Returns an instrument plugin, if any.
 */
PluginDescriptor *
plugin_manager_pick_instrument (PluginManager * self)
{
  for (size_t i = 0; i < self->plugin_descriptors->len; i++)
    {
      PluginDescriptor * descr =
        g_ptr_array_index (self->plugin_descriptors, i);
      if (plugin_descriptor_is_instrument (descr))
        {
          return descr;
        }
    }
  return NULL;
}

void
plugin_manager_clear_plugins (PluginManager * self)
{
  g_ptr_array_remove_range (
    self->plugin_descriptors, 0,
    self->plugin_descriptors->len);

  for (int i = 0; i < self->num_plugin_categories; i++)
    {
      g_free_and_null (self->plugin_categories[i]);
    }
  self->num_plugin_categories = 0;
}

void
plugin_manager_free (PluginManager * self)
{
  g_debug ("%s: Freeing...", __func__);

  symap_free (self->symap);
  zix_sem_destroy (&self->symap_lock);

  for (int i = 0; i < self->num_nodes; i++)
    {
      LilvNode * node = self->nodes[i];
#if 0
      g_debug (
        "freeing lilv node: %s", lilv_node_as_string (node));
#endif
      lilv_node_free (node);
    }

  object_free_w_func_and_null (
    lilv_world_free, self->lilv_world);

  g_ptr_array_unref (self->plugin_descriptors);

  object_free_w_func_and_null (
    cached_plugin_descriptors_free,
    self->cached_plugin_descriptors);
  object_free_w_func_and_null (
    plugin_collections_free, self->collections);

  object_zero_and_free (self);

  g_debug ("%s: done", __func__);
}
