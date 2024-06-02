// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
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

#include <cstdlib>

#include "gui/widgets/greeter.h"
#include "gui/widgets/main_window.h"
#include "plugins/cached_plugin_descriptors.h"
#include "plugins/carla_discovery.h"
#include "plugins/collections.h"
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

#include "gtk_wrapper.h"

/*#include <ctype.h>*/

/**
 * If category not already set in the categories, add it.
 */
static void
add_category_and_author (PluginManager * self, char * category, char * author)
{
  g_return_if_fail (category);
  if (!string_is_ascii (category))
    {
      g_warning ("Ignoring non-ASCII plugin category name...");
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
          self->plugin_authors[self->num_plugin_authors++] = g_strdup (author);
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

  /* if equal ignoring case, use opposite of strcmp() result to get lower before
   * upper */
  /* aka: return strcmp(b, a); */
  return -strcmp (pa->name, pb->name);
}

static void
add_expanded_paths (GStrvBuilder * builder, char ** paths_from_settings)
{
  const char * cur_path = NULL;
  for (int i = 0; (cur_path = paths_from_settings[i]) != NULL; i++)
    {
      char * expanded_cur_path = string_expand_env_vars (cur_path);
      /* split because the env might contain multiple paths */
      char ** expanded_paths =
        g_strsplit (expanded_cur_path, G_SEARCHPATH_SEPARATOR_S, 0);
      g_strv_builder_addv (builder, (const char **) expanded_paths);
      g_strfreev (expanded_paths);
      g_free (expanded_cur_path);
    }
}

static char **
plugin_manager_get_lv2_paths (const PluginManager * self)
{
  GStrvBuilder * builder = g_strv_builder_new ();

  if (ZRYTHM_TESTING)
    {
      /* add test plugins if testing */
      const char * tests_builddir = g_getenv ("G_TEST_BUILDDIR");
      const char * root_builddir = g_getenv ("G_TEST_BUILD_ROOT_DIR");
      g_return_val_if_fail (tests_builddir, NULL);
      g_return_val_if_fail (root_builddir, NULL);

      char * test_lv2_plugins =
        g_build_filename (tests_builddir, "lv2plugins", NULL);
      char * test_root_plugins =
        g_build_filename (root_builddir, "data", "plugins", NULL);
      g_strv_builder_add_many (
        builder, test_lv2_plugins, test_root_plugins, NULL);
      g_free (test_lv2_plugins);
      g_free (test_root_plugins);

      const char * paths_from_settings[] = {
        "${LV2_PATH}", "/usr/lib/lv2", NULL
      };
      add_expanded_paths (builder, (char **) paths_from_settings);

      char ** paths = g_strv_builder_end (builder);
      string_print_strv ("LV2 paths", paths);

      return paths;
    }

  char ** paths_from_settings =
    g_settings_get_strv (S_P_PLUGINS_PATHS, "lv2-search-paths");
  if (paths_from_settings[0] == NULL)
    {
      /* no paths given - use default */
#ifdef _WIN32
      g_strv_builder_add_many (
        builder, "C:\\Program Files\\Common Files\\LV2", NULL);
#elif defined(__APPLE__)
      g_strv_builder_add (builder, "/Library/Audio/Plug-ins/LV2");
#elif defined(FLATPAK_BUILD)
      g_strv_builder_add_many (
        builder, "/app/lib/lv2", "/app/extensions/Plugins/lv2", NULL);
#else /* non-flatpak UNIX */
      {
        char * home_lv2 = g_build_filename (g_get_home_dir (), ".lv2", NULL);
        g_strv_builder_add (builder, home_lv2);
        g_free (home_lv2);
      }
      g_strv_builder_add_many (
        builder, "/usr/lib/lv2", "/usr/local/lib/lv2", NULL);
#  if defined(INSTALLER_VER)
      g_strv_builder_add_many (
        builder, "/usr/lib64/lv2", "/usr/local/lib64/lv2", NULL);
#  else  /* else if unix and not installer ver */
      if (!string_is_equal (LIBDIR_NAME, "lib"))
        {
          g_strv_builder_add_many (
            builder, "/usr/" LIBDIR_NAME "/lv2",
            "/usr/local/" LIBDIR_NAME "/lv2", NULL);
        }
#  endif /* endif non-flatpak UNIX */
#endif   /* endif per-platform code */
    }
  else
    {
      /* use paths given */
      add_expanded_paths (builder, paths_from_settings);
    }

  /* add special paths */
  auto * dir_mgr = ZrythmDirectoryManager::getInstance ();
  char * builtin_plugins_path = dir_mgr->get_dir (SYSTEM_BUNDLED_PLUGINSDIR);
  char * special_plugins_path =
    dir_mgr->get_dir (SYSTEM_SPECIAL_LV2_PLUGINS_DIR);
  g_strv_builder_add_many (
    builder, builtin_plugins_path, special_plugins_path, NULL);
  g_free_and_null (builtin_plugins_path);
  g_free_and_null (special_plugins_path);

  char ** paths = g_strv_builder_end (builder);
  string_print_strv ("LV2 paths", paths);

  return paths;
}

PluginManager *
plugin_manager_new (void)
{
  PluginManager * self = object_new (PluginManager);

  self->plugin_descriptors =
    g_ptr_array_new_full (100, (GDestroyNotify) plugin_descriptor_free);

  /* init vst/dssi/ladspa */
  self->cached_plugin_descriptors = cached_plugin_descriptors_read_or_new ();

  /* fetch/create collections */
  self->collections = plugin_collections_read_or_new ();

  self->carla_discovery = z_carla_discovery_new (self);

  return self;
}

#ifdef HAVE_CARLA
static char **
plugin_manager_get_vst2_paths (const PluginManager * self)
{
  GStrvBuilder * builder = g_strv_builder_new ();

  if (ZRYTHM_TESTING)
    {
      const char * paths_from_settings[] = { "${VST_PATH}", NULL };
      add_expanded_paths (builder, (char **) paths_from_settings);

      char ** paths = g_strv_builder_end (builder);
      string_print_strv ("VST2 paths", paths);

      return paths;
    }

  char ** paths_from_settings =
    g_settings_get_strv (S_P_PLUGINS_PATHS, "vst2-search-paths");
  if (paths_from_settings[0] == NULL)
    {
      /* no paths given - use default */
#  ifdef _WIN32
      g_strv_builder_add_many (
        builder, "C:\\Program Files\\Common Files\\VST2",
        "C:\\Program Files\\VSTPlugins",
        "C:\\Program Files\\Steinberg\\VSTPlugins",
        "C:\\Program Files\\Common Files\\VST2",
        "C:\\Program Files\\Common Files\\Steinberg\\VST2", NULL);
#  elif defined(__APPLE__)
      g_strv_builder_add (builder, "/Library/Audio/Plug-ins/VST");
#  elif defined(FLATPAK_BUILD)
      g_strv_builder_add (builder, "/app/extensions/Plugins/vst");
#  else /* non-flatpak UNIX */
      {
        char * home_vst = g_build_filename (g_get_home_dir (), ".vst", NULL);
        g_strv_builder_add (builder, home_vst);
        g_free (home_vst);
      }
      g_strv_builder_add_many (
        builder, "/usr/lib/vst", "/usr/local/lib/vst", NULL);
#    if defined(INSTALLER_VER)
      g_strv_builder_add_many (
        builder, "/usr/lib64/vst", "/usr/local/lib64/vst", NULL);
#    else  /* else if unix and not installer ver */
      if (!string_is_equal (LIBDIR_NAME, "lib"))
        {
          g_strv_builder_add_many (
            builder, "/usr/" LIBDIR_NAME "/vst",
            "/usr/local/" LIBDIR_NAME "/vst", NULL);
        }
#    endif /* endif non-flatpak UNIX */
#  endif   /* endif per-platform code */
    }
  else
    {
      /* use paths given */
      add_expanded_paths (builder, paths_from_settings);
    }

  char ** paths = g_strv_builder_end (builder);
  string_print_strv ("VST2 paths", paths);

  return paths;
}

static char **
plugin_manager_get_vst3_paths (const PluginManager * self)
{
  GStrvBuilder * builder = g_strv_builder_new ();

  if (ZRYTHM_TESTING)
    {
      const char * paths_from_settings[] = { "${VST3_PATH}", NULL };
      add_expanded_paths (builder, (char **) paths_from_settings);

      char ** paths = g_strv_builder_end (builder);
      string_print_strv ("VST3 paths", paths);

      return paths;
    }

  char ** paths_from_settings =
    g_settings_get_strv (S_P_PLUGINS_PATHS, "vst3-search-paths");
  if (paths_from_settings[0] == NULL)
    {
      /* no paths given - use default */
#  ifdef _WIN32
      g_strv_builder_add_many (
        builder, "C:\\Program Files\\Common Files\\VST3",
        "C:\\Program Files\\Common Files\\VST3", NULL);
#  elif defined(__APPLE__)
      g_strv_builder_add (builder, "/Library/Audio/Plug-ins/VST3");
#  elif defined(FLATPAK_BUILD)
      g_strv_builder_add (builder, "/app/extensions/Plugins/vst3");
#  else /* non-flatpak UNIX */
      {
        char * home_vst3 = g_build_filename (g_get_home_dir (), ".vst3", NULL);
        g_strv_builder_add (builder, home_vst3);
        g_free (home_vst3);
      }
      g_strv_builder_add_many (
        builder, "/usr/lib/vst3", "/usr/local/lib/vst3", NULL);
#    if defined(INSTALLER_VER)
      g_strv_builder_add_many (
        builder, "/usr/lib64/vst3", "/usr/local/lib64/vst3", NULL);
#    else  /* else if unix and not installer ver */
      if (!string_is_equal (LIBDIR_NAME, "lib"))
        {
          g_strv_builder_add_many (
            builder, "/usr/" LIBDIR_NAME "/vst3",
            "/usr/local/" LIBDIR_NAME "/vst3", NULL);
        }
#    endif /* endif non-flatpak UNIX */
#  endif   /* endif per-platform code */
    }
  else
    {
      /* use paths given */
      add_expanded_paths (builder, paths_from_settings);
    }

  char ** paths = g_strv_builder_end (builder);
  string_print_strv ("VST3 paths", paths);

  return paths;
}

/**
 * Gets the SFZ or SF2 paths.
 */
static char **
plugin_manager_get_sf_paths (const PluginManager * self, bool sf2)
{
  char ** paths = NULL;
  if (ZRYTHM_TESTING)
    {
      paths =
        g_strsplit (G_SEARCHPATH_SEPARATOR_S, G_SEARCHPATH_SEPARATOR_S, -1);
    }
  else
    {
      paths = g_settings_get_strv (
        S_P_PLUGINS_PATHS, sf2 ? "sf2-search-paths" : "sfz-search-paths");
      g_return_val_if_fail (paths, NULL);
    }

  return paths;
}

static char **
plugin_manager_get_dssi_paths (const PluginManager * self)
{
  GStrvBuilder * builder = g_strv_builder_new ();

  if (ZRYTHM_TESTING)
    {
      const char * paths_from_settings[] = { "${DSSI_PATH}", NULL };
      add_expanded_paths (builder, (char **) paths_from_settings);

      char ** paths = g_strv_builder_end (builder);
      string_print_strv ("DSSI paths", paths);

      return paths;
    }

  char ** paths_from_settings =
    g_settings_get_strv (S_P_PLUGINS_PATHS, "dssi-search-paths");
  if (paths_from_settings[0] == NULL)
    {
      /* no paths given - use default */
#  if defined(FLATPAK_BUILD)
      g_strv_builder_add (builder, "/app/extensions/Plugins/dssi");
#  else /* non-flatpak UNIX */
      {
        char * home_dssi = g_build_filename (g_get_home_dir (), ".dssi", NULL);
        g_strv_builder_add (builder, home_dssi);
        g_free (home_dssi);
      }
      g_strv_builder_add_many (
        builder, "/usr/lib/dssi", "/usr/local/lib/dssi", NULL);
#    if defined(INSTALLER_VER)
      g_strv_builder_add_many (
        builder, "/usr/lib64/dssi", "/usr/local/lib64/dssi", NULL);
#    else  /* else if unix and not installer ver */
      if (!string_is_equal (LIBDIR_NAME, "lib"))
        {
          g_strv_builder_add_many (
            builder, "/usr/" LIBDIR_NAME "/dssi",
            "/usr/local/" LIBDIR_NAME "/dssi", NULL);
        }
#    endif /* endif non-flatpak UNIX */
#  endif   /* endif per-platform code */
    }
  else
    {
      /* use paths given */
      add_expanded_paths (builder, paths_from_settings);
    }

  char ** paths = g_strv_builder_end (builder);
  string_print_strv ("DSSI paths", paths);

  return paths;
}

static char **
plugin_manager_get_ladspa_paths (const PluginManager * self)
{
  GStrvBuilder * builder = g_strv_builder_new ();

  if (ZRYTHM_TESTING)
    {
      const char * paths_from_settings[] = { "${LADSPA_PATH}", NULL };
      add_expanded_paths (builder, (char **) paths_from_settings);

      char ** paths = g_strv_builder_end (builder);
      string_print_strv ("LADSPA paths", paths);

      return paths;
    }

  char ** paths_from_settings =
    g_settings_get_strv (S_P_PLUGINS_PATHS, "ladspa-search-paths");
  if (paths_from_settings[0] == NULL)
    {
      /* no paths given - use default */
#  if defined(FLATPAK_BUILD)
      g_strv_builder_add (builder, "/app/extensions/Plugins/ladspa");
#  else /* non-flatpak UNIX */
      g_strv_builder_add_many (
        builder, "/usr/lib/ladspa", "/usr/local/lib/ladspa", NULL);
#    if defined(INSTALLER_VER)
      g_strv_builder_add_many (
        builder, "/usr/lib64/ladspa", "/usr/local/lib64/ladspa", NULL);
#    else  /* else if unix and not installer ver */
      if (!string_is_equal (LIBDIR_NAME, "lib"))
        {
          g_strv_builder_add_many (
            builder, "/usr/" LIBDIR_NAME "/ladspa",
            "/usr/local/" LIBDIR_NAME "/ladspa", NULL);
        }
#    endif /* endif non-flatpak UNIX */
#  endif   /* endif per-platform code */
    }
  else
    {
      /* use paths given */
      add_expanded_paths (builder, paths_from_settings);
    }

  char ** paths = g_strv_builder_end (builder);
  string_print_strv ("LADSPA paths", paths);

  return paths;
}

/**
 * For the official paths see:
 * https://github.com/free-audio/clap/blob/b902efa94e7a069dc1576617ebd74d2310746bd4/include/clap/entry.h#L14
 */
static char **
plugin_manager_get_clap_paths (const PluginManager * self)
{
  GStrvBuilder * builder = g_strv_builder_new ();

#  ifndef CARLA_HAVE_CLAP_SUPPORT
  char ** empty_paths = g_strv_builder_end (builder);
  return empty_paths;
#  endif

  if (ZRYTHM_TESTING)
    {
      const char * paths_from_settings[] = { "${CLAP_PATH}", NULL };
      add_expanded_paths (builder, (char **) paths_from_settings);

      char ** paths = g_strv_builder_end (builder);
      string_print_strv ("CLAP paths", paths);

      return paths;
    }

  char ** paths_from_settings =
    g_settings_get_strv (S_P_PLUGINS_PATHS, "clap-search-paths");
  if (paths_from_settings[0] == NULL)
    {
      /* no paths given - use default */
#  ifdef _WIN32
      g_strv_builder_add_many (
        builder, "C:\\Program Files\\Common Files\\CLAP",
        "C:\\Program Files (x86)\\Common Files\\CLAP", NULL);
#  elif defined(__APPLE__)
      g_strv_builder_add (builder, "/Library/Audio/Plug-ins/CLAP");
#  elif defined(FLATPAK_BUILD)
      g_strv_builder_add (builder, "/app/extensions/Plugins/clap");
#  else /* non-flatpak UNIX */
      {
        char * home_clap = g_build_filename (g_get_home_dir (), ".clap", NULL);
        g_strv_builder_add (builder, home_clap);
        g_free (home_clap);
      }
      g_strv_builder_add_many (
        builder, "/usr/lib/clap", "/usr/local/lib/clap", NULL);
#    if defined(INSTALLER_VER)
      g_strv_builder_add_many (
        builder, "/usr/lib64/clap", "/usr/local/lib64/clap", NULL);
#    else  /* else if unix and not installer ver */
      if (!string_is_equal (LIBDIR_NAME, "lib"))
        {
          g_strv_builder_add_many (
            builder, "/usr/" LIBDIR_NAME "/clap",
            "/usr/local/" LIBDIR_NAME "/clap", NULL);
        }
#    endif /* endif non-flatpak UNIX */
#  endif   /* endif per-platform code */
    }
  else
    {
      /* use paths given */
      add_expanded_paths (builder, paths_from_settings);
    }

  char ** paths = g_strv_builder_end (builder);
  string_print_strv ("CLAP paths", paths);

  return paths;
}

static char **
plugin_manager_get_jsfx_paths (const PluginManager * self)
{
  GStrvBuilder * builder = g_strv_builder_new ();

  if (ZRYTHM_TESTING)
    {
      const char * paths_from_settings[] = { "${JSFX_PATH}", NULL };
      add_expanded_paths (builder, (char **) paths_from_settings);

      char ** paths = g_strv_builder_end (builder);
      string_print_strv ("JSFX paths", paths);

      return paths;
    }

  char ** paths_from_settings =
    g_settings_get_strv (S_P_PLUGINS_PATHS, "jsfx-search-paths");
  if (paths_from_settings[0] == NULL)
    {
      /* no paths given - use default */
    }
  else
    {
      /* use paths given */
      add_expanded_paths (builder, paths_from_settings);
    }

  char ** paths = g_strv_builder_end (builder);
  string_print_strv ("JSFX paths", paths);

  return paths;
}

static char **
plugin_manager_get_au_paths (const PluginManager * self)
{
  GStrvBuilder * builder = g_strv_builder_new ();

  g_strv_builder_add (builder, "/Library/Audio/Plug-ins/Components");
  char * user_components = g_build_filename (
    g_get_home_dir (), "Library", "Audio", "Plug-ins", "Components", NULL);
  g_strv_builder_add (builder, user_components);
  g_free (user_components);

  char ** paths = g_strv_builder_end (builder);
  string_print_strv ("AU paths", paths);

  return paths;
}
#endif /* HAVE_CARLA */

/**
 * Returns if the plugin manager supports the given
 * plugin protocol.
 */
bool
plugin_manager_supports_protocol (PluginManager * self, ZPluginProtocol protocol)
{
  switch (protocol)
    {
    case ZPluginProtocol::Z_PLUGIN_PROTOCOL_DUMMY:
    case ZPluginProtocol::Z_PLUGIN_PROTOCOL_LV2:
    case ZPluginProtocol::Z_PLUGIN_PROTOCOL_LADSPA:
    case ZPluginProtocol::Z_PLUGIN_PROTOCOL_VST:
    case ZPluginProtocol::Z_PLUGIN_PROTOCOL_VST3:
    case ZPluginProtocol::Z_PLUGIN_PROTOCOL_SFZ:
    case ZPluginProtocol::Z_PLUGIN_PROTOCOL_JSFX:
    case ZPluginProtocol::Z_PLUGIN_PROTOCOL_CLAP:
#ifdef HAVE_CARLA
      return true;
#else
      return false;
#endif
    case ZPluginProtocol::Z_PLUGIN_PROTOCOL_DSSI:
    case ZPluginProtocol::Z_PLUGIN_PROTOCOL_AU:
    case ZPluginProtocol::Z_PLUGIN_PROTOCOL_SF2:
      {
#ifdef HAVE_CARLA
        const char * const * carla_features = carla_get_supported_features ();
        const char *         feature;
        int                  i = 0;
        while ((feature = carla_features[i++]))
          {
#  define CHECK_FEATURE(str, format) \
    if ( \
      string_is_equal (feature, str) \
      && protocol == ZPluginProtocol::Z_PLUGIN_PROTOCOL_##format) \
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

char **
plugin_manager_get_paths_for_protocol (
  const PluginManager * self,
  const ZPluginProtocol protocol)
{
  char ** paths = NULL;
  switch (protocol)
    {
    case ZPluginProtocol::Z_PLUGIN_PROTOCOL_VST:
      paths = plugin_manager_get_vst2_paths (self);
      break;
    case ZPluginProtocol::Z_PLUGIN_PROTOCOL_VST3:
      paths = plugin_manager_get_vst3_paths (self);
      break;
    case ZPluginProtocol::Z_PLUGIN_PROTOCOL_DSSI:
      paths = plugin_manager_get_dssi_paths (self);
      break;
    case ZPluginProtocol::Z_PLUGIN_PROTOCOL_LADSPA:
      paths = plugin_manager_get_ladspa_paths (self);
      break;
    case ZPluginProtocol::Z_PLUGIN_PROTOCOL_SFZ:
      paths = plugin_manager_get_sf_paths (self, false);
      break;
    case ZPluginProtocol::Z_PLUGIN_PROTOCOL_SF2:
      paths = plugin_manager_get_sf_paths (self, true);
      break;
    case ZPluginProtocol::Z_PLUGIN_PROTOCOL_CLAP:
      paths = plugin_manager_get_clap_paths (self);
      break;
    case ZPluginProtocol::Z_PLUGIN_PROTOCOL_JSFX:
      paths = plugin_manager_get_jsfx_paths (self);
      break;
    case ZPluginProtocol::Z_PLUGIN_PROTOCOL_LV2:
      paths = plugin_manager_get_lv2_paths (self);
      break;
    case ZPluginProtocol::Z_PLUGIN_PROTOCOL_AU:
      paths = plugin_manager_get_au_paths (self);
      break;
    default:
      break;
    }
  g_return_val_if_fail (paths, NULL);

  return paths;
}

char *
plugin_manager_get_paths_for_protocol_separated (
  const PluginManager * self,
  const ZPluginProtocol protocol)
{
  char ** paths = plugin_manager_get_paths_for_protocol (self, protocol);
  if (paths)
    {
      char * paths_separated = g_strjoinv (G_SEARCHPATH_SEPARATOR_S, paths);
      g_strfreev (paths);
      return paths_separated;
    }
  else
    {
      return NULL;
    }
}

char *
plugin_manager_find_plugin_from_rel_path (
  const PluginManager * self,
  const ZPluginProtocol protocol,
  const char *          rel_path)
{
  char ** paths = plugin_manager_get_paths_for_protocol (self, protocol);
  if (!paths)
    {
      g_warning ("no paths for %s", rel_path);
      return g_strdup ("");
    }

  int    path_idx = 0;
  char * path;
  while ((path = paths[path_idx++]) != NULL)
    {
      char * full_path = g_build_filename (path, rel_path, NULL);
      if (g_file_test (full_path, G_FILE_TEST_EXISTS))
        {
          g_strfreev (paths);
          return full_path;
        }
    }
  g_strfreev (paths);

  g_warning ("no paths for %s", rel_path);
  return g_strdup ("");
}

void
plugin_manager_add_descriptor (PluginManager * self, PluginDescriptor * descr)
{
  g_return_if_fail (descr->protocol > ZPluginProtocol::Z_PLUGIN_PROTOCOL_DUMMY);
  g_ptr_array_add (self->plugin_descriptors, descr);
  add_category_and_author (self, descr->category_str, descr->author);
}

static gboolean
call_carla_discovery_idle (PluginManager * self)
{
  bool done = z_carla_discovery_idle (self->carla_discovery);
  if (done)
    {
      /* sort alphabetically */
      g_ptr_array_sort (self->plugin_descriptors, sort_plugin_func);
      qsort (
        self->plugin_categories, (size_t) self->num_plugin_categories,
        sizeof (char *), sort_alphabetical_func);
      qsort (
        self->plugin_authors, (size_t) self->num_plugin_authors,
        sizeof (char *), sort_alphabetical_func);

      cached_plugin_descriptors_serialize_to_file (
        self->cached_plugin_descriptors);
      if (self->scan_done_cb)
        {
          self->scan_done_cb (self->scan_done_cb_data);
        }

      return G_SOURCE_REMOVE;
    }

  return G_SOURCE_CONTINUE;
}

void
plugin_manager_begin_scan (
  PluginManager * self,
  const double    max_progress,
  double *        progress,
  GenericCallback cb,
  void *          user_data)
{
  if (getenv ("ZRYTHM_SKIP_PLUGIN_SCAN"))
    {
      if (cb)
        {
          cb (user_data);
        }
      return;
    }

  const double       start_progress = progress ? *progress : 0;
  const unsigned int num_plugin_types =
    (ENUM_VALUE_TO_INT (ZPluginProtocol::Z_PLUGIN_PROTOCOL_JSFX)
     - ENUM_VALUE_TO_INT (ZPluginProtocol::Z_PLUGIN_PROTOCOL_LV2))
    + 1;

  for (
    size_t i = ENUM_VALUE_TO_INT (ZPluginProtocol::Z_PLUGIN_PROTOCOL_LV2);
    i <= ENUM_VALUE_TO_INT (ZPluginProtocol::Z_PLUGIN_PROTOCOL_JSFX); i++)
    {
      ZPluginProtocol cur = ENUM_INT_TO_VALUE (ZPluginProtocol, i);
      if (!plugin_protocol_is_supported (cur))
        continue;

      z_carla_discovery_start (
        self->carla_discovery, CarlaBackend::BINARY_NATIVE, cur);
      /* also scan 32-bit on windows */
#ifdef _WIN32
      z_carla_discovery_start (
        self->carla_discovery, CarlaBackend::BINARY_WIN32, cur);
#endif
      if (progress)
        {
          *progress =
            start_progress
            + ((double) ((i - ENUM_VALUE_TO_INT (ZPluginProtocol::Z_PLUGIN_PROTOCOL_LV2))
                         + 1)
               / num_plugin_types)
                * (max_progress - start_progress);
          g_return_if_fail (zrythm_app->greeter);
          greeter_widget_set_progress_and_status (
            zrythm_app->greeter, _ ("Scanning Plugins"), NULL, *progress);
        }
    }

  self->scan_done_cb = cb;
  self->scan_done_cb_data = user_data;

  g_idle_add ((GSourceFunc) call_carla_discovery_idle, self);
}

PluginDescriptor *
plugin_manager_find_plugin_from_uri (PluginManager * self, const char * uri)
{
  for (size_t i = 0; i < self->plugin_descriptors->len; i++)
    {
      PluginDescriptor * descr =
        (PluginDescriptor *) g_ptr_array_index (self->plugin_descriptors, i);
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
        (PluginDescriptor *) g_ptr_array_index (self->plugin_descriptors, i);
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
        (PluginDescriptor *) g_ptr_array_index (self->plugin_descriptors, i);
      if (plugin_descriptor_is_instrument (descr))
        {
          return descr;
        }
    }
  return NULL;
}

void
plugin_manager_set_currently_scanning_plugin (
  PluginManager * self,
  const char *    filename,
  const char *    sha1)
{
  if (ZRYTHM_HAVE_UI)
    {
      greeter_widget_set_currently_scanned_plugin (
        zrythm_app->greeter, filename);
    }
}

void
plugin_manager_clear_plugins (PluginManager * self)
{
  g_ptr_array_remove_range (
    self->plugin_descriptors, 0, self->plugin_descriptors->len);

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

  g_ptr_array_unref (self->plugin_descriptors);

  object_free_w_func_and_null (
    cached_plugin_descriptors_free, self->cached_plugin_descriptors);
  object_free_w_func_and_null (plugin_collections_free, self->collections);

  object_free_w_func_and_null (z_carla_discovery_free, self->carla_discovery);

  object_zero_and_free (self);

  g_debug ("%s: done", __func__);
}
