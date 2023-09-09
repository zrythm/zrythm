// SPDX-FileCopyrightText: © 2018-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#ifndef _WOE32
#  include <sys/mman.h>
#endif
#include <stdlib.h>

#include "actions/actions.h"
#include "actions/undo_manager.h"
#include "dsp/engine.h"
#include "dsp/quantize_options.h"
#include "dsp/recording_manager.h"
#include "dsp/router.h"
#include "dsp/track.h"
#include "dsp/tracklist.h"
#include "gui/backend/event_manager.h"
#include "gui/backend/file_manager.h"
#include "gui/backend/piano_roll.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/splash.h"
#include "plugins/plugin_manager.h"
#include "project.h"
#include "settings/chord_preset_pack_manager.h"
#include "settings/settings.h"
#include "utils/arrays.h"
#include "utils/cairo.h"
#include "utils/curl.h"
#include "utils/env.h"
#include "utils/error.h"
#include "utils/gtk.h"
#include "utils/io.h"
#include "utils/localization.h"
#include "utils/log.h"
#include "utils/object_pool.h"
#include "utils/objects.h"
#include "utils/pcg_rand.h"
#include "utils/string.h"
#include "utils/symap.h"
#include "utils/ui.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "Wrapper.h"
#include <gdk-pixbuf/gdk-pixbuf.h>

typedef enum
{
  Z_ZRYTHM_ERROR_CANNOT_GET_LATEST_RELEASE,
} ZZrythmError;

#define Z_ZRYTHM_ERROR z_zrythm_error_quark ()
GQuark
z_zrythm_error_quark (void);
G_DEFINE_QUARK (z - zrythm - error - quark, z_zrythm_error)

/** This is declared extern in zrythm.h. */
Zrythm * zrythm = NULL;

/**
 * FIXME move somewhere else.
 */
void
zrythm_add_to_recent_projects (Zrythm * self, const char * _filepath)
{
  /* if we are at max
   * projects */
  if (ZRYTHM->num_recent_projects == MAX_RECENT_PROJECTS)
    {
      /* free the last one and delete it */
      g_free (ZRYTHM->recent_projects[MAX_RECENT_PROJECTS - 1]);
      array_delete (
        ZRYTHM->recent_projects, ZRYTHM->num_recent_projects,
        ZRYTHM->recent_projects[ZRYTHM->num_recent_projects - 1]);
    }

  char * filepath = g_strdup (_filepath);

  array_insert (
    ZRYTHM->recent_projects, ZRYTHM->num_recent_projects, 0, filepath);

  /* set last element to NULL because the call
   * takes a NULL terminated array */
  ZRYTHM->recent_projects[ZRYTHM->num_recent_projects] = NULL;

  g_settings_set_strv (
    S_GENERAL, "recent-projects",
    (const char * const *) ZRYTHM->recent_projects);
}

void
zrythm_remove_recent_project (char * filepath)
{
  /* FIXME use GStrvBuilder */
  for (int i = 0; i < ZRYTHM->num_recent_projects; i++)
    {
      const char * recent_project = ZRYTHM->recent_projects[i];
      g_return_if_fail (recent_project);
      if (string_is_equal (filepath, recent_project))
        {
          array_delete (
            ZRYTHM->recent_projects, ZRYTHM->num_recent_projects,
            ZRYTHM->recent_projects[i]);

          ZRYTHM->recent_projects[ZRYTHM->num_recent_projects] = NULL;

          g_settings_set_strv (
            S_GENERAL, "recent-projects",
            (const char * const *) ZRYTHM->recent_projects);
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
zrythm_get_version (bool with_v)
{
  const char * ver = PACKAGE_VERSION;

  if (with_v)
    {
      if (ver[0] == 'v')
        return g_strdup (ver);
      else
        return g_strdup_printf ("v%s", ver);
    }
  else
    {
      if (ver[0] == 'v')
        return g_strdup (&ver[1]);
      else
        return g_strdup (ver);
    }
}

/**
 * Returns the version and the capabilities.
 *
 * @param buf Buffer to write the string to.
 * @param include_system_info Whether to include
 *   additional system info (for bug reports).
 */
void
zrythm_get_version_with_capabilities (char * buf, bool include_system_info)
{
  char * ver = zrythm_get_version (0);

  GString * gstr = g_string_new (NULL);

  g_string_append_printf (
    gstr,
    "%s %s%s (%s)\n"
    "  built with %s %s for %s%s\n"
#ifdef HAVE_CARLA
    "    +carla\n"
#endif

#ifdef HAVE_JACK2
    "    +jack2\n"
#elif defined(HAVE_JACK)
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
#  ifdef IS_DEBUG_BUILD
    " - debug"
#  endif
#endif
    BUILD_TYPE,

    COMPILER, COMPILER_VERSION, HOST_MACHINE_SYSTEM,

#ifdef APPIMAGE_BUILD
    " (appimage)"
#elif defined(INSTALLER_VER)
    " (installer)"
#else
    ""
#endif

  );

  if (include_system_info)
    {
      g_string_append (gstr, "\n");
      char * system_nfo = zrythm_get_system_info ();
      g_string_append (gstr, system_nfo);
      g_free (system_nfo);
    }

  char * str = g_string_free (gstr, false);
  strcpy (buf, str);

  g_free (str);
  g_free (ver);
}

/**
 * Returns system info (mainly used for bug
 * reports).
 */
char *
zrythm_get_system_info (void)
{
  GString * gstr = g_string_new (NULL);

  char * content = NULL;
  bool   ret = g_file_get_contents ("/etc/os-release", &content, NULL, NULL);
  if (ret)
    {
      g_string_append_printf (gstr, "%s\n", content);
      g_free (content);
    }

  g_string_append_printf (
    gstr, "XDG_SESSION_TYPE=%s\n", g_getenv ("XDG_SESSION_TYPE"));
  g_string_append_printf (
    gstr, "XDG_CURRENT_DESKTOP=%s\n", g_getenv ("XDG_CURRENT_DESKTOP"));
  g_string_append_printf (
    gstr, "DESKTOP_SESSION=%s\n", g_getenv ("DESKTOP_SESSION"));

  g_string_append (gstr, "\n");

#ifdef HAVE_CARLA
  g_string_append_printf (gstr, "Carla version: %s\n", CARLA_VERSION_STRING);
#endif
  g_string_append_printf (
    gstr, "GTK version: %u.%u.%u\n", gtk_get_major_version (),
    gtk_get_minor_version (), gtk_get_micro_version ());

  char * str = g_string_free (gstr, false);

  return str;
}

/**
 * Returns whether the current Zrythm version is a
 * release version.
 *
 * @note This only does regex checking.
 */
bool
zrythm_is_release (bool official)
{
#ifndef INSTALLER_VER
  if (official)
    {
      return false;
    }
#endif

  return !string_contains_substr (PACKAGE_VERSION, "g");
}

char *
zrythm_fetch_latest_release_ver_finish (GAsyncResult * result, GError ** error)
{
  return z_curl_get_page_contents_finish (result, error);
}

/**
 * @param callback A GAsyncReadyCallback to call when the
 *   request is satisfied.
 * @param callback_data Data to pass to @p callback.
 */
void
zrythm_fetch_latest_release_ver_async (
  GAsyncReadyCallback callback,
  gpointer            callback_data)
{
  z_curl_get_page_contents_async (
    "https://www.zrythm.org/zrythm-version.txt", 8, callback, callback_data);
}

/**
 * Returns whether the given release string is the latest
 * release.
 */
bool
zrythm_is_latest_release (const char * remote_latest_release)
{
  return string_is_equal (remote_latest_release, PACKAGE_VERSION);
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
#if defined(_WOE32) && defined(INSTALLER_VER)
  return io_get_registry_string_val ("InstallPath");
#elif defined(__APPLE__) && defined(INSTALLER_VER)
  char bundle_path[PATH_MAX];
  int  ret = io_get_bundle_path (bundle_path);
  g_return_val_if_fail (ret == 0, NULL);
  return io_path_get_parent_dir (bundle_path);
#elif defined(APPIMAGE_BUILD)
  g_return_val_if_fail (zrythm_app->appimage_runtime_path, NULL);
  return g_build_filename (zrythm_app->appimage_runtime_path, "usr", NULL);
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
 * Must be free'd by caller.
 */
char *
zrythm_get_user_dir (bool force_default)
{
  if (ZRYTHM_TESTING)
    {
      if (!ZRYTHM->testing_dir)
        {
          ZRYTHM->testing_dir = g_dir_make_tmp ("zrythm_test_dir_XXXXXX", NULL);
        }
      g_debug ("returning user dir: %s", ZRYTHM->testing_dir);
      return g_strdup (ZRYTHM->testing_dir);
    }

  char * dir = NULL;
  if (SETTINGS && S_P_GENERAL_PATHS)
    {
      dir = g_settings_get_string (S_P_GENERAL_PATHS, "zrythm-dir");
    }
  else
    {
      GSettings * settings =
        g_settings_new (GSETTINGS_ZRYTHM_PREFIX ".preferences.general.paths");
      g_return_val_if_fail (settings, NULL);

      dir = g_settings_get_string (settings, "zrythm-dir");

      g_object_unref (settings);
    }

  if (force_default || strlen (dir) == 0)
    {
      g_free (dir);
      dir = g_build_filename (g_get_user_data_dir (), "zrythm", NULL);
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
zrythm_get_dir (ZrythmDirType type)
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
          res = g_build_filename (prefix, "bin", NULL);
          break;
        case ZRYTHM_DIR_SYSTEM_PARENT_DATADIR:
          res = g_build_filename (prefix, "share", NULL);
          break;
        case ZRYTHM_DIR_SYSTEM_PARENT_LIBDIR:
          res = g_build_filename (prefix, LIBDIR_NAME, NULL);
          break;
        case ZRYTHM_DIR_SYSTEM_ZRYTHM_LIBDIR:
          {
            char * parent_path =
              zrythm_get_dir (ZRYTHM_DIR_SYSTEM_PARENT_LIBDIR);
            res = g_build_filename (parent_path, "zrythm", NULL);
          }
          break;
        case ZRYTHM_DIR_SYSTEM_BUNDLED_PLUGINSDIR:
          {
            char * parent_path =
              zrythm_get_dir (ZRYTHM_DIR_SYSTEM_ZRYTHM_LIBDIR);
            res = g_build_filename (parent_path, "lv2", NULL);
          }
          break;
        case ZRYTHM_DIR_SYSTEM_LOCALEDIR:
          res = g_build_filename (prefix, "share", "locale", NULL);
          break;
        case ZRYTHM_DIR_SYSTEM_SOURCEVIEW_LANGUAGE_SPECS_DIR:
          res = g_build_filename (
            prefix, "share", "gtksourceview-5", "language-specs", NULL);
          break;
        case ZRYTHM_DIR_SYSTEM_BUNDLED_SOURCEVIEW_LANGUAGE_SPECS_DIR:
          res = g_build_filename (
            prefix, "share", "zrythm", "gtksourceview-5", "language-specs",
            NULL);
          break;
        case ZRYTHM_DIR_SYSTEM_ZRYTHM_DATADIR:
          res = g_build_filename (prefix, "share", "zrythm", NULL);
          break;
        case ZRYTHM_DIR_SYSTEM_SAMPLESDIR:
          res = g_build_filename (prefix, "share", "zrythm", "samples", NULL);
          break;
        case ZRYTHM_DIR_SYSTEM_SCRIPTSDIR:
          res = g_build_filename (prefix, "share", "zrythm", "scripts", NULL);
          break;
        case ZRYTHM_DIR_SYSTEM_THEMESDIR:
          res = g_build_filename (prefix, "share", "zrythm", "themes", NULL);
          break;
        case ZRYTHM_DIR_SYSTEM_THEMES_CSS_DIR:
          {
            char * parent_path = zrythm_get_dir (ZRYTHM_DIR_SYSTEM_THEMESDIR);
            res = g_build_filename (parent_path, "css", NULL);
          }
          break;
        case ZRYTHM_DIR_SYSTEM_THEMES_ICONS_DIR:
          {
            char * parent_path = zrythm_get_dir (ZRYTHM_DIR_SYSTEM_THEMESDIR);
            res = g_build_filename (parent_path, "icons", NULL);
          }
          break;
        case ZRYTHM_DIR_SYSTEM_SPECIAL_LV2_PLUGINS_DIR:
          res = g_build_filename (prefix, "share", "zrythm", "lv2", NULL);
          break;
        case ZRYTHM_DIR_SYSTEM_FONTSDIR:
          res = g_build_filename (prefix, "share", "fonts", "zrythm", NULL);
          break;
        case ZRYTHM_DIR_SYSTEM_TEMPLATES:
          res = g_build_filename (prefix, "share", "zrythm", "templates", NULL);
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
          res = g_build_filename (user_dir, ZRYTHM_PROJECTS_DIR, NULL);
          break;
        case ZRYTHM_DIR_USER_TEMPLATES:
          res = g_build_filename (user_dir, "templates", NULL);
          break;
        case ZRYTHM_DIR_USER_LOG:
          res = g_build_filename (user_dir, "log", NULL);
          break;
        case ZRYTHM_DIR_USER_SCRIPTS:
          res = g_build_filename (user_dir, "scripts", NULL);
          break;
        case ZRYTHM_DIR_USER_THEMES:
          res = g_build_filename (user_dir, "themes", NULL);
          break;
        case ZRYTHM_DIR_USER_THEMES_CSS:
          res = g_build_filename (user_dir, "themes", "css", NULL);
          break;
        case ZRYTHM_DIR_USER_THEMES_ICONS:
          res = g_build_filename (user_dir, "themes", "icons", NULL);
          break;
        case ZRYTHM_DIR_USER_PROFILING:
          res = g_build_filename (user_dir, "profiling", NULL);
          break;
        case ZRYTHM_DIR_USER_GDB:
          res = g_build_filename (user_dir, "gdb", NULL);
          break;
        case ZRYTHM_DIR_USER_BACKTRACE:
          res = g_build_filename (user_dir, "backtraces", NULL);
          break;
        default:
          break;
        }

      g_free (user_dir);
    }

  return res;
}

/**
 * Initializes/creates the default dirs/files in the user
 * directory.
 *
 * @return Whether successful.
 */
bool
zrythm_init_user_dirs_and_files (Zrythm * self, GError ** error)
{
  g_message ("initing dirs and files");
  char *   dir;
  bool     success;
  GError * err = NULL;

#define MK_USER_DIR(x) \
  dir = zrythm_get_dir (ZRYTHM_DIR_USER_##x); \
  g_return_val_if_fail (dir, false); \
  success = io_mkdir (dir, &err); \
  if (!success) \
    { \
      PROPAGATE_PREFIXED_ERROR ( \
        error, err, "Failed to create directory %s", dir); \
      return false; \
    } \
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

  return true;
}

/**
 * Initializes the array of project templates.
 */
void
zrythm_init_templates (Zrythm * self)
{
  g_message ("Initializing templates...");

  GStrvBuilder * builder = g_strv_builder_new ();
  {
    char *  user_templates_dir = zrythm_get_dir (ZRYTHM_DIR_USER_TEMPLATES);
    char ** user_templates = io_get_files_in_dir (user_templates_dir, true);
    g_free (user_templates_dir);
    g_return_if_fail (user_templates);
    g_strv_builder_addv (builder, (const char **) user_templates);
    g_strfreev (user_templates);
  }
  if (!ZRYTHM_TESTING)
    {
      char * system_templates_dir = zrythm_get_dir (ZRYTHM_DIR_SYSTEM_TEMPLATES);
      char ** system_templates =
        io_get_files_in_dir (system_templates_dir, true);
      g_free (system_templates_dir);
      g_return_if_fail (system_templates);
      g_strv_builder_addv (builder, (const char **) system_templates);
      g_strfreev (system_templates);
    }
  ZRYTHM->templates = g_strv_builder_end (builder);
  g_return_if_fail (ZRYTHM->templates);

  for (int i = 0; ZRYTHM->templates[i] != NULL; i++)
    {
      const char * template = ZRYTHM->templates[i];
      g_message ("Template found: %s", template);
      if (string_contains_substr (template, "demo_zsong01"))
        {
          ZRYTHM->demo_template = g_strdup (template);
        }
    }

  g_message ("done");
}

/**
 * Frees the instance and any unfreed members.
 */
void
zrythm_free (Zrythm * self)
{
  g_message ("%s: deleting Zrythm instance...", __func__);

  object_free_w_func_and_null (project_free, self->project);

  self->have_ui = false;

  object_free_w_func_and_null (z_cairo_caches_free, self->cairo_caches);
  object_free_w_func_and_null (recording_manager_free, self->recording_manager);
  object_free_w_func_and_null (plugin_manager_free, self->plugin_manager);
  object_free_w_func_and_null (event_manager_free, self->event_manager);
  object_free_w_func_and_null (file_manager_free, self->file_manager);
  object_free_w_func_and_null (
    chord_preset_pack_manager_free, self->chord_preset_pack_manager);

  object_free_w_func_and_null (symap_free, self->symap);
  object_free_w_func_and_null (symap_free, self->error_domain_symap);

  object_free_w_func_and_null (settings_free, self->settings);

  object_free_w_func_and_null (g_strfreev, self->templates);

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
zrythm_new (const char * exe_path, bool have_ui, bool testing, bool optimized_dsp)
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
  self->recording_manager = recording_manager_new ();
  self->plugin_manager = plugin_manager_new ();
  self->symap = symap_new ();
  self->error_domain_symap = symap_new ();
  self->file_manager = file_manager_new ();
  self->chord_preset_pack_manager =
    chord_preset_pack_manager_new (have_ui && !testing);
  self->cairo_caches = z_cairo_caches_new ();

  if (have_ui)
    {
      self->event_manager = event_manager_new ();
    }

  return self;
}
