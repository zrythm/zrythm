/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU General Affero Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * A project contains everything that should be
 * serialized.
 */

#include "config.h"

#include <time.h>
#include <sys/stat.h>

#include "zrythm.h"
#include "project.h"
#include "audio/automation_point.h"
#include "audio/automation_track.h"
#include "audio/channel.h"
#include "audio/chord_track.h"
#include "audio/engine.h"
#include "audio/marker_track.h"
#include "audio/midi_note.h"
#include "audio/mixer.h"
#include "audio/track.h"
#include "audio/tracklist.h"
#include "audio/transport.h"
#include "gui/widgets/header_notebook.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/region.h"
#include "gui/widgets/splash.h"
#include "gui/widgets/timeline_ruler.h"
#include "gui/widgets/track.h"
#include "plugins/lv2_plugin.h"
#include "utils/arrays.h"
#include "utils/datetime.h"
#include "utils/general.h"
#include "utils/flags.h"
#include "utils/io.h"
#include "utils/smf.h"
#include "utils/string.h"
#include "utils/ui.h"

#include <gtk/gtk.h>

#include <glib/gi18n.h>

/**
 * Tears down the project before loading another one.
 */
static void
tear_down (Project * self)
{
  g_message ("tearing down the project...");

  PROJECT->loaded = 0;

  if (self->title)
    g_free (self->title);

  engine_tear_down (AUDIO_ENGINE);

  track_free (P_CHORD_TRACK);

  free (self);
}

/**
 * Frees the current x if any and sets a copy of
 * the given string.
 */
#define DEFINE_SET_STR(x) \
static void \
set_##x ( \
  Project *    self, \
  const char * x) \
{ \
  if (self->x) \
    g_free (self->x); \
  self->x = \
    g_strdup (x); \
}

DEFINE_SET_STR (dir);
DEFINE_SET_STR (title);

#undef DEFINE_SET_STR

/**
 * Returns the filepath of a backup (directory),
 * if any,
 * if it has a newer timestamp than the actual
 * file being loaded.
 *
 * @param dir The non-backup dir.
 */
static char *
get_newer_backup (
  Project * self)
{
  GDir *dir;
  GError *error = NULL;
  const gchar *filename;
  struct stat stat_res;
  struct tm *orig_tm, *nowtm;
  time_t t1;
  time_t t2;

  char * filepath =
    project_get_project_file_path (self, 0);
  if (stat (filepath, &stat_res)==0)
    {
      orig_tm = localtime (&stat_res.st_mtime);
      t1 = mktime (orig_tm);
    }
  else
    {
      g_warning (
        "Failed to get last modified for %s",
        filepath);
      return NULL;
    }
  g_free (filepath);

  char * result = NULL;
  char * backups_dir =
    project_get_backups_dir (self);
  dir = g_dir_open (backups_dir, 0, &error);
  if (!dir)
    {
      return NULL;
    }
  while ((filename = g_dir_read_name(dir)))
    {
      char * full_path =
        g_build_filename (
          backups_dir,
          filename,
          PROJECT_FILE,
          NULL);
      g_message ("%s", full_path);

      if (stat (full_path, &stat_res)==0)
        {
          nowtm = localtime (&stat_res.st_mtime);
          t2 = mktime (nowtm);
          /* if backup is after original project */
          if (difftime (t2, t1) > 0)
            {
              if (!result)
                g_free (result);
              result =
                g_build_filename (
                  backups_dir,
                  filename,
                  NULL);
              t1 = t2;
            }
        }
      else
        {
          g_warning (
            "Failed to get last modified for %s",
            full_path);
          g_dir_close (dir);
          g_free (backups_dir);
          return NULL;
        }
      g_free (full_path);
    }
  g_free (backups_dir);
  g_dir_close (dir);

  return result;
}

static void
set_datetime_str (
  Project * self)
{
  if (self->datetime_str)
    g_free (self->datetime_str);
  self->datetime_str =
    datetime_get_current_as_string ();
}

/**
 * Sets the next available backup dir to use for
 * saving a backup during this call.
 */
static void
set_and_create_next_available_backup_dir (
  Project * self)
{
  if (self->backup_dir)
    g_free (self->backup_dir);

  char * backups_dir =
    project_get_backups_dir (self);

  int i = 0;
  do
    {
      if (i > 0)
        {
          g_free (self->backup_dir);
          char * bak_title =
            g_strdup_printf (
              "%s.bak%d",
              self->title, i);
          self->backup_dir =
            g_build_filename (
              backups_dir,
              bak_title, NULL);
          g_free (bak_title);
        }
      else
        {
          char * bak_title =
            g_strdup_printf (
              "%s.bak",
              self->title);
          self->backup_dir =
            g_build_filename (
              backups_dir,
              bak_title, NULL);
          g_free (bak_title);
        }
      i++;
    } while (
      io_file_exists (
        self->backup_dir));
  g_free (backups_dir);

  io_mkdir (self->backup_dir);
}

/**
 * Sets the next available "Untitled Project" title
 * and directory.
 *
 * @param _dir The directory of the project to
 *   create, including its title.
 */
static void
create_and_set_dir_and_title (
  Project *    self,
  const char * _dir)
{
  set_dir (self, _dir);
  char * str =
    g_path_get_basename (_dir);
  set_title (self, str);
  g_free (str);
}

/**
 * Checks that everything is okay with the project.
 */
void
project_sanity_check (Project * self)
{
  /* TODO */
}

/**
 * Initializes the selections in the project.
 *
 * @note
 * Not meant to be used anywhere besides
 * tests and project.c
 */
void
project_init_selections (Project * self)
{
  arranger_selections_init (
    (ArrangerSelections *)
    &self->automation_selections,
    ARRANGER_SELECTIONS_TYPE_AUTOMATION);
  arranger_selections_init (
    (ArrangerSelections *)
    &self->chord_selections,
    ARRANGER_SELECTIONS_TYPE_CHORD);
  arranger_selections_init (
    (ArrangerSelections *)
    &self->timeline_selections,
    ARRANGER_SELECTIONS_TYPE_TIMELINE);
  arranger_selections_init (
    (ArrangerSelections *)
    &self->midi_arranger_selections,
    ARRANGER_SELECTIONS_TYPE_MIDI);
}

/**
 * Not to be used anywhere else. This is only made
 * public for testing.
 */
static void
create_default (Project * self)
{
  int loading_while_running = self->loaded;
  if (loading_while_running)
    {
      tear_down (self);
      PROJECT = calloc (1, sizeof (Project));
      self = PROJECT;

      MainWindowWidget * mww = MAIN_WINDOW;
      MAIN_WINDOW = NULL;
      ZRYTHM->event_queue = NULL;

      g_message ("destroying previous main "
                 "window...");
      gtk_widget_destroy (GTK_WIDGET (mww));
    }

  /* initialize selections */
  project_init_selections (self);

  engine_init (&self->audio_engine, 0);
  undo_manager_init (&self->undo_manager);

  /* init midi mappings */
  self->midi_mappings = midi_mappings_new ();

  self->title =
    g_path_get_basename (
      ZRYTHM->create_project_path);

  /* init pinned tracks */
  Track * track =
    chord_track_new ();
  tracklist_append_track (
    TRACKLIST, track, F_NO_PUBLISH_EVENTS,
    F_NO_RECALC_GRAPH);
  track->pinned = 1;
  TRACKLIST->chord_track = track;
  track =
    marker_track_default ();
  tracklist_append_track (
    TRACKLIST, track, F_NO_PUBLISH_EVENTS,
    F_NO_RECALC_GRAPH);
  track->pinned = 1;
  TRACKLIST->marker_track = track;

  /* add master channel to mixer and tracklist */
  track =
    track_new (
      TRACK_TYPE_MASTER, _("Master"),
      F_WITHOUT_LANE);
  tracklist_append_track (
    TRACKLIST, track, F_NO_PUBLISH_EVENTS,
    F_NO_RECALC_GRAPH);
  TRACKLIST->master_track = track;
  tracklist_selections_add_track (
    TRACKLIST_SELECTIONS, track);
  self->last_selection = SELECTION_TYPE_TRACK;

  /* create untitled project */
  create_and_set_dir_and_title (
    self, ZRYTHM->create_project_path);

  if (loading_while_running)
    {
      g_message ("recreating main window...");
      MAIN_WINDOW =
        main_window_widget_new (zrythm_app);
    }

  self->loaded = 1;

  snap_grid_init (
    &PROJECT->snap_grid_timeline,
    NOTE_LENGTH_1_1);
  quantize_options_init (
    &PROJECT->quantize_opts_timeline,
    NOTE_LENGTH_1_1);
  snap_grid_init (
    &PROJECT->snap_grid_midi,
    NOTE_LENGTH_1_8);
  quantize_options_init (
    &PROJECT->quantize_opts_editor,
    NOTE_LENGTH_1_8);
  clip_editor_init (&PROJECT->clip_editor);
  snap_grid_update_snap_points (
    &PROJECT->snap_grid_timeline);
  snap_grid_update_snap_points (
    &PROJECT->snap_grid_midi);
  quantize_options_update_quantize_points (
    &PROJECT->quantize_opts_timeline);
  quantize_options_update_quantize_points (
    &PROJECT->quantize_opts_editor);

  position_set_to_bar (
    &PROJECT->range_1, 1);
  position_set_to_bar (
    &PROJECT->range_2, 1);

  /* mimic behavior when starting the app */
  if (loading_while_running)
    {
      events_init (ZRYTHM);
      main_window_widget_refresh (MAIN_WINDOW);

      g_atomic_int_set (&AUDIO_ENGINE->run, 1);
    }

  header_notebook_widget_set_subtitle (
    MW_HEADER_NOTEBOOK,
    PROJECT->title);
}

/**
 * @param filename The filename to open. This will
 *   be the template in the case of template, or
 *   the actual project otherwise.
 * @param is_template Load the project as a
 *   template and create a new project from it.
 */
static int
load (
  const char * filename,
  const int is_template)
{
  g_warn_if_fail (filename);
  char * dir = io_get_dir (filename);

  set_dir (PROJECT, dir);

  /* if loading an actual project, check for
   * backups */
  if (!is_template)
    {
      if (PROJECT->backup_dir)
        g_free (PROJECT->backup_dir);
      PROJECT->backup_dir =
        get_newer_backup (PROJECT);
      if (PROJECT->backup_dir)
        {
          g_message (
            "newer backup found %s",
            PROJECT->backup_dir);

          GtkWidget * dialog =
            gtk_message_dialog_new (
              GTK_WINDOW (MAIN_WINDOW),
              GTK_DIALOG_MODAL |
                GTK_DIALOG_DESTROY_WITH_PARENT,
              GTK_MESSAGE_INFO,
              GTK_BUTTONS_YES_NO,
              _("Newer backup found:\n  %s.\n"
                "Use the newer backup?"),
              PROJECT->backup_dir);
          gtk_widget_set_visible (
            GTK_WIDGET (MAIN_WINDOW), 0);
          int res =
            gtk_dialog_run (GTK_DIALOG (dialog));
          switch (res)
            {
            case GTK_RESPONSE_YES:
              break;
            case GTK_RESPONSE_NO:
              g_free (PROJECT->backup_dir);
              PROJECT->backup_dir = NULL;
              break;
            default:
              break;
            }
          gtk_widget_destroy (dialog);
          gtk_widget_set_visible (
            GTK_WIDGET (MAIN_WINDOW), 1);
        }
    }

  gchar * yaml;
  GError *err = NULL;

  char * project_file_path =
    project_get_project_file_path (
      PROJECT, PROJECT->backup_dir != NULL);
  g_message (
    "loading project file %s",
    project_file_path);
  g_file_get_contents (
    project_file_path,
    &yaml,
    NULL,
    &err);
  if (err != NULL)
    {
      // Report error to user, and free error
      char * str =
        g_strdup_printf (
          "Unable to read file: %s",
          err->message);
      ui_show_error_message (
        MAIN_WINDOW,
        str);
      g_free (str);
      g_error_free (err);
      RETURN_ERROR
    }
  g_free (project_file_path);

  Project * prj = project_deserialize (yaml);
  g_free (yaml);
  prj->backup_dir =
    g_strdup (PROJECT->backup_dir);

  char * version = zrythm_get_version (0);
  if (!string_is_equal (
        prj->version,
        version, 1))
    {
      char * str =
        g_strdup_printf (
          _("This project was created with a "
            "different version of Zrythm (%s). "
            "It may not work correctly."),
          prj->version);
      ui_show_message_full (
        GTK_WINDOW (MAIN_WINDOW),
        GTK_MESSAGE_WARNING, str);
      g_free (str);
    }
  g_free (version);

  if (prj == NULL)
    {
      ui_show_error_message (
        MAIN_WINDOW,
        _("Failed to load project. Please check the "
        "logs for more information."));
      RETURN_ERROR;
    }
  g_message ("Project successfully deserialized.");

  if (is_template)
    {
      g_free (dir);
      dir =
        g_strdup (ZRYTHM->create_project_path);
    }

  int loading_while_running = PROJECT->loaded;
  if (loading_while_running)
    {
      tear_down (PROJECT);
      PROJECT = prj;

      MainWindowWidget * mww = MAIN_WINDOW;
      MAIN_WINDOW = NULL;
      ZRYTHM->event_queue = NULL;

      g_message ("destroying previous main "
                 "window...");
      gtk_widget_destroy (GTK_WIDGET (mww));
    }

  g_message ("initing loaded structures");
  PROJECT = prj;

  /* re-update paths for the newly loaded project */
  set_dir (prj, dir);

  undo_manager_init (&PROJECT->undo_manager);
  engine_init (AUDIO_ENGINE, 1);

  char * filepath_noext = g_path_get_basename (dir);
  PROJECT->title = filepath_noext;

  g_free (dir);

  tracklist_init_loaded (&PROJECT->tracklist);
  clip_editor_init_loaded (CLIP_EDITOR);

  /* init ports */
  Port * ports[10000];
  int num_ports;
  Port * port;
  port_get_all (ports, &num_ports);
  g_message ("Initializing loaded Ports...");
  for (int i = 0; i < num_ports; i++)
    {
      port = ports[i];
      port_init_loaded (port);
    }
  midi_mappings_init_loaded (
    PROJECT->midi_mappings);

  arranger_selections_init_loaded (
    (ArrangerSelections *)
    &PROJECT->timeline_selections);
  arranger_selections_init_loaded (
    (ArrangerSelections *)
    &PROJECT->midi_arranger_selections);
  arranger_selections_init_loaded (
    (ArrangerSelections *)
    &PROJECT->chord_selections);
  arranger_selections_init_loaded (
    (ArrangerSelections *)
    &PROJECT->automation_selections);

  tracklist_selections_init_loaded (
    &PROJECT->tracklist_selections);

  snap_grid_update_snap_points (
    &PROJECT->snap_grid_timeline);
  snap_grid_update_snap_points (
    &PROJECT->snap_grid_midi);
  quantize_options_update_quantize_points (
    &PROJECT->quantize_opts_timeline);
  quantize_options_update_quantize_points (
    &PROJECT->quantize_opts_editor);

  if (loading_while_running)
    {
      g_message ("recreating main window...");
      MAIN_WINDOW =
        main_window_widget_new (zrythm_app);
    }

  /* sanity check */
  project_sanity_check (PROJECT);

  PROJECT->loaded = 1;
  g_message ("project loaded");

  /* mimic behavior when starting the app */
  if (loading_while_running)
    {
      events_init (ZRYTHM);
      main_window_widget_refresh (MAIN_WINDOW);

      g_atomic_int_set (&AUDIO_ENGINE->run, 1);
    }

  header_notebook_widget_set_subtitle (
    MW_HEADER_NOTEBOOK,
    PROJECT->title);

  RETURN_OK;
}

/**
 * If project has a filename set, it loads that.
 * Otherwise it loads the default project.
 *
 * @param filename The filename to open. This will
 *   be the template in the case of template, or
 *   the actual project otherwise.
 * @param is_template Load the project as a
 *   template and create a new project from it.
 *
 * @return 0 if successful, non-zero otherwise.
 */
int
project_load (
  char *    filename,
  const int is_template)
{
  if (filename)
    load (filename, is_template);
  else
    create_default (PROJECT);

  engine_activate (AUDIO_ENGINE);

  /* connect channel inputs to hardware. has to
   * be done after engine activation */
  Channel *ch;
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      ch = TRACKLIST->tracks[i]->channel;
      if (!ch)
        continue;

      channel_reconnect_ext_input_ports (ch);
    }

  /* set the version */
  PROJECT->version =
    zrythm_get_version (0);

  if (is_template || !filename)
    project_save (PROJECT, PROJECT->dir, 0, 0);

  return 0;
}

/**
 * Autosave callback.
 *
 * This will keep getting called at regular short
 * intervals, and if enough time has passed and
 * it's okay to save it will autosave, otherwise it
 * will wait until the next interval and check
 * again.
 */
int
project_autosave_cb (
  void * data)
{
  if (PROJECT && PROJECT->loaded &&
      PROJECT->dir &&
      PROJECT->datetime_str)
    {
      gint cur_time = g_get_monotonic_time ();
      unsigned int microsec_to_autosave =
        g_settings_get_uint (
          S_PREFERENCES, "autosave-interval") * 60 *
          1000000 -
          /* subtract 4 seconds because the time
           * this gets called is not exact */
          4 * 1000000;

      /* bad time to save */
      if (cur_time - PROJECT->last_autosave_time <
            microsec_to_autosave ||
          TRANSPORT_IS_ROLLING)
        {
          return G_SOURCE_CONTINUE;
        }
      /* ok to save */
      else
        {
          project_save (
            PROJECT, PROJECT->dir, 1, 1);
          PROJECT->last_autosave_time = cur_time;
        }
    }

  return G_SOURCE_CONTINUE;
}

/**
 * Sets if the project has range and updates UI.
 */
void
project_set_has_range (int has_range)
{
  PROJECT->has_range = has_range;

  EVENTS_PUSH (ET_RANGE_SELECTION_CHANGED, NULL);
}

/**
 * Returns the backups dir for the given Project.
 */
char *
project_get_backups_dir (
  Project * self)
{
  g_warn_if_fail (self->dir);
  return
    g_build_filename (
      self->dir,
      PROJECT_BACKUPS_DIR,
      NULL);
}

/**
 * Returns the exports dir for the given Project.
 *
 * Must be g_free()'d.
 */
char *
project_get_exports_dir (
  Project * self)
{
  g_warn_if_fail (self->dir);
  return
    g_build_filename (
      self->dir,
      PROJECT_EXPORTS_DIR,
      NULL);
}

/**
 * Returns the states dir for the given Project.
 *
 * @param is_backup 1 to get the states dir of the
 *   current backup instead of the main project.
 */
char *
project_get_states_dir (
  Project * self,
  const int is_backup)
{
  g_warn_if_fail (self->dir);
  if (is_backup)
    return
      g_build_filename (
        self->backup_dir,
        PROJECT_STATES_DIR,
        NULL);
  else
    return
      g_build_filename (
        self->dir,
        PROJECT_STATES_DIR,
        NULL);
}

/**
 * Returns the audio dir for the given Project.
 */
char *
project_get_pool_dir (
  Project * self)
{
  g_warn_if_fail (self->dir);
  return
    g_build_filename (
      self->dir,
      PROJECT_POOL_DIR,
      NULL);
}

/**
 * Returns the full project file (project.yml)
 * path.
 *
 * @param is_backup 1 to get the states dir of the
 *   current backup instead of the main project.
 */
char *
project_get_project_file_path (
  Project * self,
  const int is_backup)
{
  g_warn_if_fail (self->dir);
  if (is_backup)
    return
      g_build_filename (
        self->backup_dir,
        PROJECT_FILE,
        NULL);
  else
    return
      g_build_filename (
        self->dir,
        PROJECT_FILE,
        NULL);
}

/**
 * Saves the project to a project file in the
 * given dir.
 *
 * @param is_backup 1 if this is a backup. Backups
 *   will be saved as <original filename>.bak<num>.
 * @param show_notification Show a notification
 *   in the UI that the project was saved.
 */
int
project_save (
  Project *    self,
  const char * _dir,
  const int    is_backup,
  const int    show_notification)
{
  int i, j;

  char * dir = g_strdup (_dir);

  /* set the dir and create it if it doesn't
   * exist */
  set_dir (self, dir);
  io_mkdir (PROJECT->dir);
  char * exports_dir =
    project_get_exports_dir (PROJECT);
  io_mkdir (exports_dir);
  char * pool_dir =
    project_get_pool_dir (PROJECT);
  io_mkdir (pool_dir);
  g_free (exports_dir);
  g_free (pool_dir);

  /* set the title */
  char * basename =
    io_path_get_basename (dir);
  set_title (self, basename);
  g_free (basename);

  /* save current datetime */
  set_datetime_str (self);

  /* if backup, get next available backup dir */
  if (is_backup)
    set_and_create_next_available_backup_dir (self);

  /* write plugin states, prepare channels for
   * serialization, */
  Track * track;
  Channel * ch;
  Plugin * pl;
  char * states_dir =
    project_get_states_dir (self, is_backup);
  for (i = 0; i < TRACKLIST->num_tracks; i++)
    {
      track = TRACKLIST->tracks[i];
      if (track->type == TRACK_TYPE_CHORD)
        continue;

      ch = track->channel;
      if (!ch)
        continue;

      channel_prepare_for_serialization (ch);

      for (j = 0; j < STRIP_SIZE; j++)
        {
          pl = ch->plugins[j];

          if (!pl)
            continue;

          if (pl->descr->protocol == PROT_LV2)
            {
              char * tmp =
                g_strdup_printf (
                  "%s_%d",
                  ch->track->name,
                  j);
              char * state_dir_plugin =
                g_build_filename (
                  states_dir,
                  tmp,
                  NULL);
              g_free (tmp);

              Lv2Plugin * lv2_plugin =
                (Lv2Plugin *)
                pl->lv2;
              lv2_plugin_save_state_to_file (
                lv2_plugin,
                state_dir_plugin);
              g_free (state_dir_plugin);
            }
        }
    }
  g_free (states_dir);

  char * project_file_path =
    project_get_project_file_path (
      self, is_backup);
  char * yaml = project_serialize (PROJECT);
  g_return_val_if_fail (yaml, -1);
  GError *err = NULL;
  g_file_set_contents (
    project_file_path, yaml, -1, &err);
  g_free (yaml);
  if (err != NULL)
    {
      // Report error to user, and free error
      g_warning ("Unable to read file: %s",
                 err->message);
      g_error_free (err);
      RETURN_ERROR;
    }

  if (is_backup)
    {
      if (show_notification)
        ui_show_notification (_("Backup saved."));
    }
  else
    {
      zrythm_add_to_recent_projects (
        ZRYTHM, project_file_path);
      if (show_notification)
        ui_show_notification (_("Project saved."));
    }
  g_free (project_file_path);

  header_notebook_widget_set_subtitle (
    MW_HEADER_NOTEBOOK,
    PROJECT->title);

  g_free (dir);

  RETURN_OK;
}

SERIALIZE_SRC (Project, project)
DESERIALIZE_SRC (Project, project)
PRINT_YAML_SRC (Project, project)
