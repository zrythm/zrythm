// SPDX-FileCopyrightText: © 2018-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#ifndef _WIN32
#  include <sys/mman.h>
#endif

#include "dsp/recording_manager.h"
#include "dsp/router.h"
#include "gui/backend/event_manager.h"
#include "gui/backend/file_manager.h"
#include "gui/widgets/main_window.h"
#include "plugins/plugin_manager.h"
#include "project.h"
#include "settings/chord_preset_pack_manager.h"
#include "settings/settings.h"
#include "utils/cairo.h"
#include "utils/curl.h"
#include "utils/env.h"
#include "utils/error.h"
#include "utils/io.h"
#include "utils/objects.h"
#include "utils/pcg_rand.h"
#include "utils/string.h"
#include "utils/symap.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

#include "Wrapper.h"
#include "gtk_wrapper.h"
#include <gdk-pixbuf/gdk-pixbuf.h>

typedef enum
{
  Z_ZRYTHM_ERROR_CANNOT_GET_LATEST_RELEASE,
} ZZrythmError;

#define Z_ZRYTHM_ERROR z_zrythm_error_quark ()
GQuark
z_zrythm_error_quark (void);
G_DEFINE_QUARK (z - zrythm - error - quark, z_zrythm_error)

Zrythm::Zrythm (const char * exe_path, bool have_ui, bool optimized_dsp)
{
  this->exe_path_ = g_strdup (exe_path);

  this->version = get_version (false);
  this->have_ui_ = have_ui;
  this->use_optimized_dsp = optimized_dsp;
  this->debug = env_get_int ("ZRYTHM_DEBUG", 0);

  /* these don't depend on anything external for sure */
  this->rand = pcg_rand_new ();
  this->symap = symap_new ();
  this->error_domain_symap = symap_new ();
  recent_projects_ = std::make_unique<StringArray> ();

  this->dir_mgr = std::make_unique<ZrythmDirectoryManager> ();
  this->settings = std::make_unique<Settings> ();
}

void
Zrythm::init (void)
{
  settings->init ();
  this->recording_manager = recording_manager_new ();
  this->plugin_manager = plugin_manager_new ();
  this->file_manager = file_manager_new ();
  this->chord_preset_pack_manager =
    chord_preset_pack_manager_new (have_ui_ && !ZRYTHM_TESTING);
  this->cairo_caches = z_cairo_caches_new ();

  if (have_ui_)
    {
      this->event_manager = event_manager_new ();
    }
}

void
Zrythm::add_to_recent_projects (const char * _filepath)
{
  recent_projects_->insert (0, _filepath);

  /* if we are at max projects, remove the last one */
  if (recent_projects_->size () > MAX_RECENT_PROJECTS)
    {
      recent_projects_->remove (MAX_RECENT_PROJECTS);
    }

  char ** tmp = recent_projects_->getNullTerminated ();
  g_settings_set_strv (S_GENERAL, "recent-projects", (const char * const *) tmp);
  g_strfreev (tmp);
}

void
Zrythm::remove_recent_project (char * filepath)
{
  recent_projects_->removeString (filepath);

  char ** tmp = recent_projects_->getNullTerminated ();
  g_settings_set_strv (S_GENERAL, "recent-projects", (const char * const *) tmp);
  g_strfreev (tmp);
}

/**
 * Returns the version string.
 *
 * Must be g_free()'d.
 *
 * @param with_v Include a starting "v".
 */
char *
Zrythm::get_version (bool with_v)
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
Zrythm::get_version_with_capabilities (char * buf, bool include_system_info)
{
  char * ver = get_version (0);

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
      char * system_nfo = get_system_info ();
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
Zrythm::get_system_info (void)
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
  g_string_append_printf (gstr, "Carla version: %s\n", Z_CARLA_VERSION_STRING);
#endif
  g_string_append_printf (
    gstr, "GTK version: %u.%u.%u\n", gtk_get_major_version (),
    gtk_get_minor_version (), gtk_get_micro_version ());
  g_string_append_printf (
    gstr, "libadwaita version: %u.%u.%u\n", adw_get_major_version (),
    adw_get_minor_version (), adw_get_micro_version ());
  g_string_append_printf (
    gstr, "libpanel version: %u.%u.%u\n", panel_get_major_version (),
    panel_get_minor_version (), panel_get_micro_version ());

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
Zrythm::is_release (bool official)
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
Zrythm::fetch_latest_release_ver_finish (GAsyncResult * result, GError ** error)
{
  return z_curl_get_page_contents_finish (result, error);
}

/**
 * @param callback A GAsyncReadyCallback to call when the
 *   request is satisfied.
 * @param callback_data Data to pass to @p callback.
 */
void
Zrythm::fetch_latest_release_ver_async (
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
Zrythm::is_latest_release (const char * remote_latest_release)
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
Zrythm::get_prefix (void)
{
#if defined(_WIN32) && defined(INSTALLER_VER)
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

char *
ZrythmDirectoryManager::get_user_dir (bool force_default)
{
  if (ZRYTHM_TESTING)
    {
      if (!this->testing_dir)
        {
          this->testing_dir = g_dir_make_tmp ("zrythm_test_dir_XXXXXX", NULL);
        }
      g_debug ("returning user dir: %s", this->testing_dir);
      return g_strdup (this->testing_dir);
    }

  char * dir = NULL;
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

char *
ZrythmDirectoryManager::get_default_user_dir (void)
{
  return get_user_dir (true);
}

char *
ZrythmDirectoryManager::get_dir (ZrythmDirType type)
{
  char * res = NULL;

  /* handle system dirs */
  if (type < USER_TOP)
    {
      char * prefix = Zrythm::get_prefix ();

      switch (type)
        {
        case SYSTEM_PREFIX:
          res = g_strdup (prefix);
          break;
        case SYSTEM_BINDIR:
          res = g_build_filename (prefix, "bin", NULL);
          break;
        case SYSTEM_PARENT_DATADIR:
          res = g_build_filename (prefix, "share", NULL);
          break;
        case SYSTEM_PARENT_LIBDIR:
          res = g_build_filename (prefix, LIBDIR_NAME, NULL);
          break;
        case SYSTEM_ZRYTHM_LIBDIR:
          {
            char * parent_path = get_dir (SYSTEM_PARENT_LIBDIR);
            res = g_build_filename (parent_path, "zrythm", NULL);
          }
          break;
        case SYSTEM_BUNDLED_PLUGINSDIR:
          {
            char * parent_path = get_dir (SYSTEM_ZRYTHM_LIBDIR);
            res = g_build_filename (parent_path, "lv2", NULL);
          }
          break;
        case SYSTEM_LOCALEDIR:
          res = g_build_filename (prefix, "share", "locale", NULL);
          break;
        case SYSTEM_SOURCEVIEW_LANGUAGE_SPECS_DIR:
          res = g_build_filename (
            prefix, "share", "gtksourceview-5", "language-specs", NULL);
          break;
        case SYSTEM_BUNDLED_SOURCEVIEW_LANGUAGE_SPECS_DIR:
          res = g_build_filename (
            prefix, "share", "zrythm", "gtksourceview-5", "language-specs",
            NULL);
          break;
        case SYSTEM_ZRYTHM_DATADIR:
          res = g_build_filename (prefix, "share", "zrythm", NULL);
          break;
        case SYSTEM_SAMPLESDIR:
          res = g_build_filename (prefix, "share", "zrythm", "samples", NULL);
          break;
        case SYSTEM_SCRIPTSDIR:
          res = g_build_filename (prefix, "share", "zrythm", "scripts", NULL);
          break;
        case SYSTEM_THEMESDIR:
          res = g_build_filename (prefix, "share", "zrythm", "themes", NULL);
          break;
        case SYSTEM_THEMES_CSS_DIR:
          {
            char * parent_path = get_dir (SYSTEM_THEMESDIR);
            res = g_build_filename (parent_path, "css", NULL);
          }
          break;
        case SYSTEM_THEMES_ICONS_DIR:
          {
            char * parent_path = get_dir (SYSTEM_THEMESDIR);
            res = g_build_filename (parent_path, "icons", NULL);
          }
          break;
        case SYSTEM_SPECIAL_LV2_PLUGINS_DIR:
          res = g_build_filename (prefix, "share", "zrythm", "lv2", NULL);
          break;
        case SYSTEM_FONTSDIR:
          res = g_build_filename (prefix, "share", "fonts", "zrythm", NULL);
          break;
        case SYSTEM_TEMPLATES:
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
      char * user_dir = get_user_dir (false);

      switch (type)
        {
        case USER_TOP:
          res = g_strdup (user_dir);
          break;
        case USER_PROJECTS:
          res = g_build_filename (user_dir, ZRYTHM_PROJECTS_DIR, NULL);
          break;
        case USER_TEMPLATES:
          res = g_build_filename (user_dir, "templates", NULL);
          break;
        case USER_LOG:
          res = g_build_filename (user_dir, "log", NULL);
          break;
        case USER_SCRIPTS:
          res = g_build_filename (user_dir, "scripts", NULL);
          break;
        case USER_THEMES:
          res = g_build_filename (user_dir, "themes", NULL);
          break;
        case USER_THEMES_CSS:
          res = g_build_filename (user_dir, "themes", "css", NULL);
          break;
        case USER_THEMES_ICONS:
          res = g_build_filename (user_dir, "themes", "icons", NULL);
          break;
        case USER_PROFILING:
          res = g_build_filename (user_dir, "profiling", NULL);
          break;
        case USER_GDB:
          res = g_build_filename (user_dir, "gdb", NULL);
          break;
        case USER_BACKTRACE:
          res = g_build_filename (user_dir, "backtraces", NULL);
          break;
        default:
          break;
        }

      g_free (user_dir);
    }

  return res;
}

bool
Zrythm::init_user_dirs_and_files (GError ** error)
{
  g_message ("initing dirs and files");
  char *   dir;
  bool     success;
  GError * err = NULL;

#define MK_USER_DIR(x) \
  dir = dir_mgr->get_dir (USER_##x); \
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

void
Zrythm::init_templates ()
{
  g_message ("Initializing templates...");

  GStrvBuilder * builder = g_strv_builder_new ();
  {
    char *  user_templates_dir = dir_mgr->get_dir (USER_TEMPLATES);
    char ** user_templates = io_get_files_in_dir (user_templates_dir, true);
    g_free (user_templates_dir);
    g_return_if_fail (user_templates);
    g_strv_builder_addv (builder, (const char **) user_templates);
    g_strfreev (user_templates);
  }
  if (!ZRYTHM_TESTING)
    {
      char *  system_templates_dir = dir_mgr->get_dir (SYSTEM_TEMPLATES);
      char ** system_templates =
        io_get_files_in_dir (system_templates_dir, true);
      g_free (system_templates_dir);
      g_return_if_fail (system_templates);
      g_strv_builder_addv (builder, (const char **) system_templates);
      g_strfreev (system_templates);
    }
  this->templates = g_strv_builder_end (builder);
  g_return_if_fail (this->templates);

  for (int i = 0; this->templates[i] != NULL; i++)
    {
      const char * tmpl = this->templates[i];
      g_message ("Template found: %s", tmpl);
      if (string_contains_substr (tmpl, "demo_zsong01"))
        {
          this->demo_template = g_strdup (tmpl);
        }
    }

  g_message ("done");
}

Zrythm::~Zrythm ()
{
  g_message ("%s: deleting Zrythm instance...", __func__);

  object_free_w_func_and_null (project_free, this->project);

  this->have_ui_ = false;

  object_free_w_func_and_null (z_cairo_caches_free, this->cairo_caches);
  object_free_w_func_and_null (recording_manager_free, this->recording_manager);
  object_free_w_func_and_null (plugin_manager_free, this->plugin_manager);
  object_free_w_func_and_null (event_manager_free, this->event_manager);
  object_free_w_func_and_null (file_manager_free, this->file_manager);
  object_free_w_func_and_null (
    chord_preset_pack_manager_free, this->chord_preset_pack_manager);

  object_free_w_func_and_null (symap_free, this->symap);
  object_free_w_func_and_null (symap_free, this->error_domain_symap);

  object_free_w_func_and_null (g_strfreev, this->templates);

  g_message ("%s: done", __func__);
}
