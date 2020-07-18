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
#include "utils/env.h"
#include "utils/gtk.h"
#include "utils/localization.h"
#include "utils/log.h"
#include "utils/object_pool.h"
#include "utils/object_utils.h"
#include "utils/objects.h"
#include "utils/io.h"
#include "utils/symap.h"
#include "utils/ui.h"
#include "zrythm.h"

#include "Wrapper.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

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
  for (int i = 0; i < ZRYTHM->num_recent_projects;
       i++)
    {
      if (!strcmp (filepath,
                   ZRYTHM->recent_projects[i]))
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
  int with_v)
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
 * Returns the veresion and the capabilities.
 */
void
zrythm_get_version_with_capabilities (
  char * str)
{
  /* get compiled capabilities */
  char caps[1000] = "";
#ifdef HAVE_CARLA
  strcat (caps, "+carla ");
#endif
#ifdef HAVE_FFMPEG
  strcat (caps, "+ffmpeg ");
#endif
#ifdef HAVE_JACK
  strcat (caps, "+jack ");
#endif
#ifdef MANUAL_PATH
  strcat (caps, "+manual ");
#endif
#ifdef HAVE_RTMIDI
  strcat (caps, "+rtmidi ");
#endif
#ifdef HAVE_RTAUDIO
  strcat (caps, "+rtaudio ");
#endif
#ifdef HAVE_SDL
  strcat (caps, "+sdl2 ");
#endif
  if (strlen (caps) > 0)
    {
      caps[strlen (caps) - 1] = '\0';
    }

  char * ver = zrythm_get_version (0);

  sprintf (
    str,
    "%s %s%s (%s)\n"
    "  built with %s %s\n"
#ifdef HAVE_CARLA
    "    +carla\n"
#endif
#ifdef HAVE_FFMPEG
    "    +ffmpeg\n"
#endif
#ifdef HAVE_JACK
    "    +jack\n"
#endif
#ifdef MANUAL_PATH
    "    +manual\n"
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
    "",
    PROGRAM_NAME,
#ifdef TRIAL_VER
    /* TRANSLATORS: please keep the space at the
     * end */
    _("(trial) "),
#else
    "",
#endif
    ver, BUILD_TYPE,
    COMPILER, COMPILER_VERSION);

  g_free (ver);
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
#ifdef WINDOWS_RELEASE
  return
    io_get_registry_string_val ("InstallPath");
#elif defined(MAC_RELEASE)
  char bundle_path[PATH_MAX];
  int ret = io_get_bundle_path (bundle_path);
  g_return_val_if_fail (ret == 0, NULL);
  return io_path_get_parent_dir (bundle_path);
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
      if (ZRYTHM->testing_dir)
        return ZRYTHM->testing_dir;
      else
        {
          ZRYTHM->testing_dir =
            g_dir_make_tmp (
              "zrythm_test_dir_XXXXXX", NULL);
          return ZRYTHM->testing_dir;
        }
    }

  GSettings * settings =
    g_settings_new (
      GSETTINGS_ZRYTHM_PREFIX
      ".preferences.general.paths");
  g_return_val_if_fail (settings, NULL);

  char * dir =
    g_settings_get_string (
      settings, "zrythm-dir");
  if (force_default || strlen (dir) == 0)
    {
      g_free (dir);
      dir =
        g_build_filename (
          g_get_user_data_dir (), "zrythm", NULL);
    }

  g_object_unref (settings);

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
        case ZRYTHM_DIR_SYSTEM_LOCALEDIR:
          res =
            g_build_filename (
              prefix, "share", "locale", NULL);
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
        case ZRYTHM_DIR_SYSTEM_THEMESDIR:
          res =
            g_build_filename (
              prefix, "share", "zrythm",
              "themes", NULL);
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
              user_dir, "projects", NULL);
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
        case ZRYTHM_DIR_USER_THEMES:
          res =
            g_build_filename (
              user_dir, "themes", NULL);
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
        default:
          break;
        }

      g_free (user_dir);
    }

  return res;
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

  self->have_ui = false;

  object_free_w_func_and_null (
    z_cairo_caches_free, self->cairo_caches);
  object_free_w_func_and_null (
    project_free, self->project);
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
    log_free, self->log);

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
  bool have_ui,
  bool testing)
{
  g_message (
    "%s: allocating Zrythm instance...",
    __func__);

  Zrythm * self = object_new (Zrythm);
  ZRYTHM = self;

  self->have_ui = have_ui;
  self->testing = testing;
  self->settings = settings_new ();
  self->object_utils = object_utils_new ();
  self->recording_manager =
    recording_manager_new ();
  self->plugin_manager = plugin_manager_new ();
  self->symap = symap_new ();
  self->file_manager = file_manager_new ();
  self->log = log_new ();
  self->cairo_caches = z_cairo_caches_new ();

  if (have_ui)
    {
      self->event_manager =
        event_manager_new ();
    }

  return self;
}
