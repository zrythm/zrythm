/*
 * Copyright (C) 2018-2022 Alexandros Theodotou <alex at zrythm dot org>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "zrythm-config.h"

#ifndef _WOE32
#include <sys/mman.h>
#endif
#include <stdlib.h>

#include "actions/actions.h"
#include "actions/undo_manager.h"
#include "audio/engine.h"
#include "audio/router.h"
#include "audio/quantize_options.h"
#include "audio/recording_manager.h"
#include "audio/track.h"
#include "audio/tracklist.h"
#include "gui/accel.h"
#include "gui/backend/event_manager.h"
#include "gui/backend/file_manager.h"
#include "gui/backend/piano_roll.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/splash.h"
#include "plugins/plugin_manager.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/arrays.h"
#include "utils/cairo.h"
#include "utils/curl.h"
#include "utils/env.h"
#include "utils/gtk.h"
#include "utils/localization.h"
#include "utils/log.h"
#include "utils/object_pool.h"
#include "utils/object_utils.h"
#include "utils/objects.h"
#include "utils/io.h"
#include "utils/pcg_rand.h"
#include "utils/string.h"
#include "utils/symap.h"
#include "utils/ui.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include "Wrapper.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

typedef enum
{
  Z_ZRYTHM_ERROR_CANNOT_GET_LATEST_RELEASE,
} ZZrythmError;

#define Z_ZRYTHM_ERROR \
  z_zrythm_error_quark ()
GQuark z_zrythm_error_quark (void);
G_DEFINE_QUARK (
  z-zrythm-error-quark, z_zrythm_error)

/** This is declared extern in zrythm.h. */
Zrythm * zrythm = NULL;

/**
 * FIXME move somewhere else.
 */
void
zrythm_add_to_recent_projects (
  Zrythm * self,
  const char * _filepath)
{
  /* if we are at max
   * projects */
  if (ZRYTHM->num_recent_projects ==
        MAX_RECENT_PROJECTS)
    {
      /* free the last one and delete it */
      g_free (ZRYTHM->recent_projects[
                MAX_RECENT_PROJECTS - 1]);
      array_delete (
        ZRYTHM->recent_projects,
        ZRYTHM->num_recent_projects,
        ZRYTHM->recent_projects[
          ZRYTHM->num_recent_projects - 1]);
    }

  char * filepath =
    g_strdup (_filepath);

  array_insert (
    ZRYTHM->recent_projects,
    ZRYTHM->num_recent_projects,
    0,
    filepath);

  /* set last element to NULL because the call
   * takes a NULL terminated array */
  ZRYTHM->recent_projects[
    ZRYTHM->num_recent_projects] = NULL;

  g_settings_set_strv (
    S_GENERAL, "recent-projects",
    (const char * const *) ZRYTHM->recent_projects);
}

void
zrythm_remove_recent_project (
  char * filepath)
{
  /* FIXME use GStrvBuilder */
  for (int i = 0; i < ZRYTHM->num_recent_projects;
       i++)
    {
      const char * recent_project =
        ZRYTHM->recent_projects[i];
      g_return_if_fail (recent_project);
      if (string_is_equal (filepath, recent_project))
        {
          array_delete (ZRYTHM->recent_projects,
                        ZRYTHM->num_recent_projects,
                        ZRYTHM->recent_projects[i]);

          ZRYTHM->recent_projects[
            ZRYTHM->num_recent_projects] = NULL;

          g_settings_set_strv (
            S_GENERAL, "recent-projects",
            (const char * const *)
              ZRYTHM->recent_projects);
        }

    }
}

/**
 * Returns the version string.
 *
 * Must be g_free()'d.
 *
 * @param with_v Include a starting "v".
 */
char *
zrythm_get_version (
  bool with_v)
{
  const char * ver = PACKAGE_VERSION;

  if (with_v)
    {
      if (ver[0] == 'v')
        return g_strdup (ver);
      else
        return
          g_strdup_printf ("v%s", ver);
    }
  else
    {
      if (ver[0] == 'v')
        return g_strdup (ver + 1);
      else
        return g_strdup (ver);
    }
}

/**
 * Returns the version and the capabilities.
 *
 * @param buf Buffer to write the string to.
 */
void
zrythm_get_version_with_capabilities (
  char * buf)
{
  char * ver = zrythm_get_version (0);

  sprintf (
    buf,
    "%s %s%s (%s)\n"
    "  built with %s %s for %s%s\n"
#ifdef HAVE_CARLA
    "    +carla\n"
#endif

#ifdef HAVE_JACK2
    "    +jack2\n"
#elif defined (HAVE_JACK)
    "    +jack1\n"
#endif

#ifdef MANUAL_PATH
    "    +manual\n"
#endif
#ifdef HAVE_PULSEAUDIO
    "    +pulse\n"
#endif
#ifdef HAVE_RTMIDI
    "    +rtmidi\n"
#endif
#ifdef HAVE_RTAUDIO
    "    +rtaudio\n"
#endif
#ifdef HAVE_SDL
    "    +sdl2\n"
#endif

#ifdef HAVE_GUILE
    "    +guile\n"
#endif

#ifdef HAVE_LSP_DSP
    "    +lsp-dsp-lib\n"
#endif

    "",
    PROGRAM_NAME,

#ifdef TRIAL_VER
    "(trial) ",
#else
    "",
#endif

    ver,

#if 0
    "optimization " OPTIMIZATION
#ifdef IS_DEBUG_BUILD
    " - debug"
#endif
#endif
    BUILD_TYPE
    ,

    COMPILER, COMPILER_VERSION, HOST_MACHINE_SYSTEM,

#ifdef APPIMAGE_BUILD
    " (appimage)"
#elif defined (INSTALLER_VER)
    " (installer)"
#else
    ""
#endif

    );

  g_free (ver);
}

/**
 * Returns whether the current Zrythm version is a
 * release version.
 *
 * @note This only does regex checking.
 */
bool
zrythm_is_release (
  bool official)
{
#ifndef INSTALLER_VER
  if (official)
    {
      return false;
    }
#endif

  return
    !string_contains_substr (PACKAGE_VERSION, "g");
}

/**
 * Returns the latest release version.
 */
char *
zrythm_fetch_latest_release_ver (void)
{
  static char * ver = NULL;
  static bool called = false;

  if (called && ver)
    {
      return g_strdup (ver);
    }

  char * page =
    z_curl_get_page_contents_default (
      "https://www.zrythm.org/releases/?C=M;O=D");
  if (!page)
    {
      g_warning ("failed to get page");
      return NULL;
    }

  ver =
    string_get_regex_group (
      page, "title=\"zrythm-(.+).tar.xz\"", 1);
  g_free (page);

  if (!ver)
    {
      g_warning ("failed to parse version");
      return NULL;
    }

  g_debug ("latest release: %s", ver);
  called = true;

  return g_strdup (ver);
}

/**
 * Returns whether this is the latest release.
 *
 * @p error will be set if an error occured and the
 * return value should be ignored.
 */
bool
zrythm_is_latest_release (
  GError ** error)
{
  g_return_val_if_fail (
    *error == NULL, false);

  char * latest_release =
    zrythm_fetch_latest_release_ver ();
  if (!latest_release)
    {
      g_set_error_literal (
        error, Z_ZRYTHM_ERROR,
        Z_ZRYTHM_ERROR_CANNOT_GET_LATEST_RELEASE,
        "Error getting latest release");
      return false;
    }

  bool ret = false;
  if (string_is_equal (
        latest_release, PACKAGE_VERSION))
    {
      ret = true;
    }
  g_free (latest_release);

  return ret;
}

/**
 * Returns the prefix or in the case of windows
 * the root dir (C/program files/zrythm) or in the
 * case of macos the bundle path.
 *
 * In all cases, "share" is expected to be found
 * in this dir.
 *
 * @return A newly allocated string.
 */
char *
zrythm_get_prefix (void)
{
#if defined (_WOE32) && defined (INSTALLER_VER)
  return
    io_get_registry_string_val ("InstallPath");
#elif defined (__APPLE__) && defined (INSTALLER_VER)
  char bundle_path[PATH_MAX];
  int ret = io_get_bundle_path (bundle_path);
  g_return_val_if_fail (ret == 0, NULL);
  return io_path_get_parent_dir (bundle_path);
#elif defined (APPIMAGE_BUILD)
  g_return_val_if_fail (
    zrythm_app->appimage_runtime_path, NULL);
  return
    g_build_filename (
      zrythm_app->appimage_runtime_path, "usr",
      NULL);
#else
  return g_strdup (PREFIX);
#endif
}

/**
 * Gets the zrythm directory, either from the
 * settings if non-empty, or the default
 * ($XDG_DATA_DIR/zrythm).
 *
 * @param force_default Ignore the settings and get
 *   the default dir.
 *
 * Must be free'd by caler.
 */
char *
zrythm_get_user_dir (
  bool  force_default)
{
  if (ZRYTHM_TESTING)
    {
      if (!ZRYTHM->testing_dir)
        {
          ZRYTHM->testing_dir =
            g_dir_make_tmp (
              "zrythm_test_dir_XXXXXX", NULL);
        }
      g_debug (
        "returning user dir: %s",
        ZRYTHM->testing_dir);
      return g_strdup (ZRYTHM->testing_dir);
    }

  char * dir = NULL;
  if (SETTINGS && S_P_GENERAL_PATHS)
    {
      dir =
        g_settings_get_string (
          S_P_GENERAL_PATHS, "zrythm-dir");
    }
  else
    {
      GSettings * settings =
        g_settings_new (
          GSETTINGS_ZRYTHM_PREFIX
          ".preferences.general.paths");
      g_return_val_if_fail (settings, NULL);

      dir =
        g_settings_get_string (
          settings, "zrythm-dir");

      g_object_unref (settings);
    }

  if (force_default || strlen (dir) == 0)
    {
      g_free (dir);
      dir =
        g_build_filename (
          g_get_user_data_dir (), "zrythm", NULL);
    }

  return dir;
}

/**
 * Returns the default user "zrythm" dir.
 *
 * This is used when resetting or when the
 * dir is not selected by the user yet.
 */
char *
zrythm_get_default_user_dir (void)
{
  return zrythm_get_user_dir (true);
}

/**
 * Returns a Zrythm directory specified by
 * \ref type.
 *
 * @return A newly allocated string.
 */
char *
zrythm_get_dir (
  ZrythmDirType type)
{
  char * res = NULL;

  /* handle system dirs */
  if (type < ZRYTHM_DIR_USER_TOP)
    {
      char * prefix = zrythm_get_prefix ();

      switch (type)
        {
        case ZRYTHM_DIR_SYSTEM_PREFIX:
          res = g_strdup (prefix);
          break;
        case ZRYTHM_DIR_SYSTEM_BINDIR:
          res =
            g_build_filename (
              prefix, "bin", NULL);
          break;
        case ZRYTHM_DIR_SYSTEM_PARENT_DATADIR:
          res =
            g_build_filename (
              prefix, "share", NULL);
          break;
        case ZRYTHM_DIR_SYSTEM_PARENT_LIBDIR:
          res =
            g_build_filename (
              prefix, LIBDIR_NAME, NULL);
          break;
        case ZRYTHM_DIR_SYSTEM_ZRYTHM_LIBDIR:
          {
            char * parent_path =
              zrythm_get_dir (
                ZRYTHM_DIR_SYSTEM_PARENT_LIBDIR);
            res =
              g_build_filename (
                parent_path, "zrythm", NULL);
          }
          break;
        case ZRYTHM_DIR_SYSTEM_LOCALEDIR:
          res =
            g_build_filename (
              prefix, "share", "locale", NULL);
          break;
        case ZRYTHM_DIR_SYSTEM_SOURCEVIEW_LANGUAGE_SPECS_DIR:
          res =
            g_build_filename (
              prefix, "share",
              "gtksourceview-5",
              "language-specs", NULL);
          break;
        case ZRYTHM_DIR_SYSTEM_BUNDLED_SOURCEVIEW_LANGUAGE_SPECS_DIR:
          res =
            g_build_filename (
              prefix, "share", "zrythm",
              "gtksourceview-5",
              "language-specs", NULL);
          break;
        case ZRYTHM_DIR_SYSTEM_ZRYTHM_DATADIR:
          res =
            g_build_filename (
              prefix, "share", "zrythm", NULL);
          break;
        case ZRYTHM_DIR_SYSTEM_SAMPLESDIR:
          res =
            g_build_filename (
              prefix, "share", "zrythm",
              "samples", NULL);
          break;
        case ZRYTHM_DIR_SYSTEM_SCRIPTSDIR:
          res =
            g_build_filename (
              prefix, "share", "zrythm",
              "scripts", NULL);
          break;
        case ZRYTHM_DIR_SYSTEM_THEMESDIR:
          res =
            g_build_filename (
              prefix, "share", "zrythm",
              "themes", NULL);
          break;
        case ZRYTHM_DIR_SYSTEM_THEMES_CSS_DIR:
          res =
            g_build_filename (
              prefix, "share", "zrythm",
              "themes", "css", NULL);
          break;
        case ZRYTHM_DIR_SYSTEM_LV2_PLUGINS_DIR:
          res =
            g_build_filename (
              prefix, "share", "zrythm",
              "lv2", NULL);
          break;
        case ZRYTHM_DIR_SYSTEM_FONTSDIR:
          res =
            g_build_filename (
              prefix, "share", "fonts",
              "zrythm", NULL);
          break;
        default:
          break;
        }

      g_free (prefix);
    }
  /* handle user dirs */
  else
    {
      char * user_dir = zrythm_get_user_dir (false);

      switch (type)
        {
        case ZRYTHM_DIR_USER_TOP:
          res = g_strdup (user_dir);
          break;
        case ZRYTHM_DIR_USER_PROJECTS:
          res =
            g_build_filename (
              user_dir, ZRYTHM_PROJECTS_DIR, NULL);
          break;
        case ZRYTHM_DIR_USER_TEMPLATES:
          res =
            g_build_filename (
              user_dir, "templates", NULL);
          break;
        case ZRYTHM_DIR_USER_LOG:
          res =
            g_build_filename (
              user_dir, "log", NULL);
          break;
        case ZRYTHM_DIR_USER_SCRIPTS:
          res =
            g_build_filename (
              user_dir, "scripts", NULL);
          break;
        case ZRYTHM_DIR_USER_THEMES:
          res =
            g_build_filename (
              user_dir, "themes", NULL);
          break;
        case ZRYTHM_DIR_USER_THEMES_CSS:
          res =
            g_build_filename (
              user_dir, "themes", "css", NULL);
          break;
        case ZRYTHM_DIR_USER_PROFILING:
          res =
            g_build_filename (
              user_dir, "profiling", NULL);
          break;
        case ZRYTHM_DIR_USER_GDB:
          res =
            g_build_filename (
              user_dir, "gdb", NULL);
          break;
        case ZRYTHM_DIR_USER_BACKTRACE:
          res =
            g_build_filename (
              user_dir, "backtraces", NULL);
          break;
        default:
          break;
        }

      g_free (user_dir);
    }

  return res;
}

/**
 * Initializes/creates the default dirs/files in
 * the user directory.
 */
NONNULL
void
zrythm_init_user_dirs_and_files (
  Zrythm * self)
{
  g_message ("initing dirs and files");
  char * dir;

#define MK_USER_DIR(x) \
  dir =  zrythm_get_dir (ZRYTHM_DIR_USER_##x); \
  io_mkdir (dir); \
  g_free (dir)

  MK_USER_DIR (TOP);
  MK_USER_DIR (PROJECTS);
  MK_USER_DIR (TEMPLATES);
  MK_USER_DIR (LOG);
  MK_USER_DIR (THEMES);
  MK_USER_DIR (THEMES_CSS);
  MK_USER_DIR (PROFILING);
  MK_USER_DIR (GDB);

#undef MK_USER_DIR
}

/**
 * Initializes the array of project templates.
 */
void
zrythm_init_templates (
  Zrythm * self)
{
  g_message ("Initializing templates...");

  char * user_templates_dir =
    zrythm_get_dir (ZRYTHM_DIR_USER_TEMPLATES);
  ZRYTHM->templates =
    io_get_files_in_dir (user_templates_dir, true);
  g_return_if_fail (ZRYTHM->templates);
  g_free (user_templates_dir);

  g_message ("done");
}

/**
 * Frees the instance and any unfreed members.
 */
void
zrythm_free (
  Zrythm * self)
{
  g_message (
    "%s: deleting Zrythm instance...",
    __func__);

  object_free_w_func_and_null (
    project_free, self->project);

  self->have_ui = false;

  object_free_w_func_and_null (
    z_cairo_caches_free, self->cairo_caches);
  object_free_w_func_and_null (
    recording_manager_free,
    self->recording_manager);
  object_free_w_func_and_null (
    plugin_manager_free,
    self->plugin_manager);
  object_free_w_func_and_null (
    event_manager_free, self->event_manager);
  object_free_w_func_and_null (
    file_manager_free, self->file_manager);

  /* free object utils around last */
  object_free_w_func_and_null (
    object_utils_free, self->object_utils);

  object_free_w_func_and_null (
    symap_free, self->symap);
  object_free_w_func_and_null (
    symap_free, self->error_domain_symap);

  if (ZRYTHM == self)
    {
      ZRYTHM = NULL;
    }

  object_zero_and_free (self);

  g_message ("%s: done", __func__);
}

/**
 * Creates a new Zrythm instance.
 *
 * @param have_ui Whether Zrythm is instantiated
 *   with a UI (false if headless).
 * @param testing Whether this is a unit test.
 */
Zrythm *
zrythm_new (
  const char * exe_path,
  bool         have_ui,
  bool         testing,
  bool         optimized_dsp)
{
  Zrythm * self = object_new (Zrythm);
  ZRYTHM = self;

  self->rand = pcg_rand_new ();

  self->exe_path = g_strdup (exe_path);

  self->version = zrythm_get_version (false);
  self->have_ui = have_ui;
  self->testing = testing;
  self->use_optimized_dsp = optimized_dsp;
  self->settings = settings_new ();
  self->object_utils = object_utils_new ();
  self->recording_manager =
    recording_manager_new ();
  self->plugin_manager = plugin_manager_new ();
  self->symap = symap_new ();
  self->error_domain_symap = symap_new ();
  self->file_manager = file_manager_new ();
  self->cairo_caches = z_cairo_caches_new ();

  if (have_ui)
    {
      self->event_manager =
        event_manager_new ();
    }

  return self;
}
