/*
 * Copyright (C) 2018-2020 Alexandros Theodotou <alex at zrythm dot org>
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

#include "zrythm-config.h"

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
#include "audio/router.h"
#include "audio/tempo_track.h"
#include "audio/track.h"
#include "audio/tracklist.h"
#include "audio/transport.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/backend/tracklist_selections.h"
#include "gui/widgets/create_project_dialog.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/region.h"
#include "gui/widgets/splash.h"
#include "gui/widgets/timeline_ruler.h"
#include "gui/widgets/track.h"
#include "plugins/carla_native_plugin.h"
#include "plugins/lv2_plugin.h"
#include "plugins/lv2/lv2_state.h"
#include "settings/settings.h"
#include "utils/arrays.h"
#include "utils/datetime.h"
#include "utils/general.h"
#include "utils/file.h"
#include "utils/flags.h"
#include "utils/io.h"
#include "utils/objects.h"
#include "utils/string.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include <zstd.h>

/**
 * Compresses/decompress a project from a file/data
 * to a file/data.
 *
 * @param compress True to compress, false to
 *   decompress.
 * @param[out] dest Pointer to a location to allocate
 *   memory.
 * @param[out] dest_size Pointer to a location to
 *   store the size of the allocated memory.
 * @param src Input buffer or filepath.
 * @param src_size Input buffer size, if not
 *   filepath.
 *
 * @return Error message if error, otherwise NULL.
 */
char *
_project_compress (
  bool                   compress,
  char **                _dest,
  size_t *               _dest_size,
  ProjectCompressionFlag dest_type,
  const char *           _src,
  const size_t           _src_size,
  ProjectCompressionFlag src_type)
{
  g_message (
    "using zstd v%d.%d.%d",
    ZSTD_VERSION_MAJOR,
    ZSTD_VERSION_MINOR,
    ZSTD_VERSION_RELEASE);

  char * src = NULL;
  size_t src_size = 0;
  switch (src_type)
    {
    case PROJECT_COMPRESS_DATA:
      src = (char *) _src;
      src_size = _src_size;
      break;
    case PROJECT_COMPRESS_FILE:
      {
        GError *err = NULL;
        g_file_get_contents (
          _src, &src, &src_size, &err);
        if (err)
          {
            char err_msg[800];
            strcpy (err_msg, err->message);
            g_error_free (err);
            return
              g_strdup_printf (
                _("Failed to open file: %s"),
                err_msg);
          }
      }
      break;
    }

  char * dest = NULL;
  size_t dest_size = 0;
  if (compress)
    {
      g_message ("compressing project...");
      size_t compress_bound =
        ZSTD_compressBound (src_size);
      dest = malloc (compress_bound);
      dest_size =
        ZSTD_compress (
          dest, compress_bound,
          src, src_size, 1);
      if (ZSTD_isError (dest_size))
        {
          free (dest);
          return
            g_strdup_printf (
              "Failed to compress project file: %s",
              ZSTD_getErrorName (dest_size));
        }
    }
  else /* decompress */
    {
#if (ZSTD_VERSION_MAJOR == 1 && \
  ZSTD_VERSION_MINOR < 3)
      unsigned long long const frame_content_size =
        ZSTD_getDecompressedSize (src, src_size);
      if (frame_content_size == 0)
#else
      unsigned long long const frame_content_size =
        ZSTD_getFrameContentSize (src, src_size);
      if (frame_content_size ==
            ZSTD_CONTENTSIZE_ERROR)
#endif
        {
          return
            g_strdup (
              _("Project not compressed by zstd!"));
        }
      dest =
        malloc ((size_t) frame_content_size);
      dest_size =
        ZSTD_decompress (
          dest, frame_content_size,
          src, src_size);
      if (ZSTD_isError (dest_size))
        {
          free (dest);
          return
            g_strdup_printf (
              _("Failed to decompress project "
              "file: %s"),
              ZSTD_getErrorName (dest_size));
        }
      if (dest_size != frame_content_size)
        {
          free (dest);
          return
            g_strdup_printf (
              _("uncompressed_size != "
              "frame_content_size impossible "
              "because zstd will check this "
              "condition!"));
        }
    }

  g_message (
    "%s : %u bytes -> %u bytes",
    compress ? "Compression" : "Decompression",
    (unsigned) src_size,
    (unsigned) dest_size);

  switch (src_type)
    {
    case PROJECT_COMPRESS_DATA:
      *_dest = dest;
      *_dest_size = dest_size;
      break;
    case PROJECT_COMPRESS_FILE:
      {
        GError *err = NULL;
        g_file_set_contents (
          *_dest, dest,
          (gssize) dest_size, &err);
        if (err)
          {
            char err_msg[800];
            strcpy (err_msg, err->message);
            g_error_free (err);
            return
              g_strdup_printf (
                _("Failed to write file: %s"),
                err_msg);
          }
      }
      break;
    }

  return NULL;
}

/**
 * Tears down the project.
 */
void
project_free (Project * self)
{
  g_message ("%s: tearing down...", __func__);

  PROJECT->loaded = false;

  g_free_and_null (self->title);

  if (self->audio_engine &&
      self->audio_engine->activated)
    {
      engine_activate (self->audio_engine, false);
    }
  object_free_w_func_and_null (
    tracklist_free, self->tracklist);
  object_free_w_func_and_null (
    engine_free, self->audio_engine);
  object_free_w_func_and_null (
    midi_mappings_free, self->midi_mappings);
  object_free_w_func_and_null (
    undo_manager_free, self->undo_manager);
  object_free_w_func_and_null (
    clip_editor_free, self->clip_editor);

  object_zero_and_free (self);

  g_message ("%s: done", __func__);
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
    project_get_path (
      self, PROJECT_PATH_PROJECT_FILE, false);
  if (stat (filepath, &stat_res) == 0)
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
    project_get_path (
      self, PROJECT_PATH_BACKUPS, false);
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
    project_get_path (
      self, PROJECT_PATH_BACKUPS, false);

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
      file_exists (
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
  char * str = g_path_get_basename (_dir);
  set_title (self, str);
  g_free (str);
}

/**
 * Checks that everything is okay with the project.
 */
void
project_sanity_check (Project * self)
{
  g_message ("%s: checking...", __func__);

  int max_size = 20;
  Port ** ports =
    calloc ((size_t) max_size, sizeof (Port *));
  int num_ports = 0;
  Port * port;
  port_get_all (
    &ports, &max_size, true, &num_ports);
  for (int i = 0; i < num_ports; i++)
    {
      port = ports[i];
      port_update_identifier (port, NULL);
    }
  free (ports);

  /* TODO add arranger_object_get_all and check
   * positions (arranger_object_sanity_check) */

  g_message ("%s: done", __func__);
}

static MainWindowWidget *
hide_prev_main_window (void)
{
  event_manager_stop_events (EVENT_MANAGER);

  MainWindowWidget * mww = MAIN_WINDOW;
  MAIN_WINDOW = NULL;
  if (GTK_IS_WIDGET (mww))
    {
      g_message (
        "hiding previous main window...");
      gtk_widget_set_visible (
        GTK_WIDGET (mww), false);
    }

  return mww;
}

static void
destroy_prev_main_window (
  MainWindowWidget * mww)
{
  if (GTK_IS_WIDGET (mww))
    {
      g_message (
        "destroying previous main window...");
      main_window_widget_tear_down (mww);
      gtk_widget_destroy (GTK_WIDGET (mww));
    }
}

static void
setup_main_window (
  Project * self)
{
  /* mimic behavior when starting the app */
  if (ZRYTHM_HAVE_UI)
    {
      g_message ("setting up main window...");
      event_manager_start_events (EVENT_MANAGER);
      main_window_widget_setup (MAIN_WINDOW);

      EVENTS_PUSH (
        ET_PROJECT_LOADED, self);
    }
}

static void
recreate_main_window (void)
{
  g_message ("recreating main window...");
  MAIN_WINDOW =
    main_window_widget_new (zrythm_app);
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
  g_message ("creating default project...");

  MainWindowWidget * mww = NULL;
  if (ZRYTHM_HAVE_UI)
    {
      g_message ("hiding prev window...");
      mww = hide_prev_main_window ();
    }

  if (self)
    {
      project_free (self);
    }
  self = project_new (ZRYTHM);

  self->tracklist = tracklist_new (self);

  /* initialize selections */
  project_init_selections (self);

  self->audio_engine = engine_new (self);
  self->undo_manager = undo_manager_new ();

  /* init midi mappings */
  self->midi_mappings = midi_mappings_new ();

  self->title =
    g_path_get_basename (
      ZRYTHM->create_project_path);

  /* init pinned tracks */

  /* chord */
  g_message ("adding chord track...");
  Track * track = chord_track_new (0);
  tracklist_append_track (
    TRACKLIST, track, F_NO_PUBLISH_EVENTS,
    F_NO_RECALC_GRAPH);
  track->pinned = 1;
  self->tracklist->chord_track = track;

  /* tempo */
  g_message ("adding tempo track...");
  track = tempo_track_default (1);
  tracklist_append_track (
    TRACKLIST, track, F_NO_PUBLISH_EVENTS,
    F_NO_RECALC_GRAPH);
  track->pinned = 1;
  self->tracklist->tempo_track = track;

  /* marker */
  g_message ("adding marker track...");
  track = marker_track_default (0);
  tracklist_append_track (
    TRACKLIST, track, F_NO_PUBLISH_EVENTS,
    F_NO_RECALC_GRAPH);
  track->pinned = 1;
  self->tracklist->marker_track = track;

  /* add master channel to mixer and tracklist */
  g_message ("adding master track...");
  track =
    track_new (
      TRACK_TYPE_MASTER, 2, _("Master"),
      F_WITHOUT_LANE);
  tracklist_append_track (
    TRACKLIST, track, F_NO_PUBLISH_EVENTS,
    F_NO_RECALC_GRAPH);
  self->tracklist->master_track = track;
  g_warn_if_fail (track->processor->stereo_in->l->is_project);
  tracklist_selections_add_track (
    self->tracklist_selections, track, 0);
  self->last_selection = SELECTION_TYPE_TRACK;

  engine_setup (self->audio_engine);

  tracklist_expose_ports_to_backend (
    self->tracklist);

  /* init ports */
  int max_size = 20;
  Port ** ports =
    calloc ((size_t) max_size, sizeof (Port *));
  int num_ports = 0;
  Port * port;
  port_get_all (
    &ports, &max_size, true, &num_ports);
  g_message ("Initializing loaded Ports...");
  for (int i = 0; i < num_ports; i++)
    {
      port = ports[i];
      port_set_is_project (port, true);
    }

  engine_update_frames_per_tick (
    AUDIO_ENGINE, TRANSPORT->beats_per_bar,
    tempo_track_get_current_bpm (P_TEMPO_TRACK),
    AUDIO_ENGINE->sample_rate);

  /* create untitled project */
  create_and_set_dir_and_title (
    self, ZRYTHM->create_project_path);

  if (ZRYTHM_HAVE_UI)
    {
      g_message ("recreating main window...");
      recreate_main_window ();

      if (mww)
        {
          g_message ("destroying prev window...");
          destroy_prev_main_window (mww);
        }
    }

  self->loaded = true;

  snap_grid_init (
    &self->snap_grid_timeline,
    NOTE_LENGTH_1_1);
  quantize_options_init (
    &self->quantize_opts_timeline,
    NOTE_LENGTH_1_1);
  snap_grid_init (
    &self->snap_grid_midi,
    NOTE_LENGTH_1_8);
  quantize_options_init (
    &self->quantize_opts_editor,
    NOTE_LENGTH_1_8);
  clip_editor_init (self->clip_editor);
  snap_grid_update_snap_points (
    &self->snap_grid_timeline);
  snap_grid_update_snap_points (
    &self->snap_grid_midi);
  quantize_options_update_quantize_points (
    &self->quantize_opts_timeline);
  quantize_options_update_quantize_points (
    &self->quantize_opts_editor);

  region_link_group_manager_init (
    &self->region_link_group_manager);

  g_message ("setting up main window...");
  setup_main_window (self);

  /* recalculate the routing graph */
  router_recalc_graph (ROUTER, F_NOT_SOFT);

  engine_set_run (self->audio_engine, true);

  g_message ("done");
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

  if (!PROJECT)
    {
      /* create a temporary project struct */
      PROJECT = project_new (ZRYTHM);
    }

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
              NULL,
              GTK_DIALOG_MODAL |
                GTK_DIALOG_DESTROY_WITH_PARENT,
              GTK_MESSAGE_INFO,
              GTK_BUTTONS_YES_NO,
              _("Newer backup found:\n  %s.\n"
                "Use the newer backup?"),
              PROJECT->backup_dir);
          gtk_window_set_title (
            GTK_WINDOW (dialog),
            _("Backup found"));
          gtk_window_set_position (
            GTK_WINDOW (dialog),
            GTK_WIN_POS_CENTER_ALWAYS);
          gtk_window_set_icon_name (
            GTK_WINDOW (dialog), "zrythm");
          if (MAIN_WINDOW)
            {
              gtk_widget_set_visible (
                GTK_WIDGET (MAIN_WINDOW), 0);
            }
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
          if (MAIN_WINDOW)
            {
              gtk_widget_set_visible (
                GTK_WIDGET (MAIN_WINDOW), 1);
            }
        }
    }

  bool use_backup = PROJECT->backup_dir != NULL;
  PROJECT->loading_from_backup = use_backup;

  /* get file contents */
  char * compressed_pj;
  gsize compressed_pj_size;
  GError *err = NULL;
  char * project_file_path_alloc =
    project_get_path (
      PROJECT, PROJECT_PATH_PROJECT_FILE,
      use_backup);
  char project_file_path[1600];
  strcpy (
    project_file_path, project_file_path_alloc);
  g_message (
    "%s: loading project file %s",
    __func__, project_file_path);
  g_file_get_contents (
    project_file_path, &compressed_pj,
    &compressed_pj_size, &err);
  if (err != NULL)
    {
      /* Report error to user, and free error */
      char str[800];
      sprintf (
        str, _("Unable to read file: %s"),
        err->message);
      ui_show_error_message (MAIN_WINDOW, str);
      g_error_free (err);
      RETURN_ERROR
    }

  /* decompress */
  g_message (
    "%s: decompressing project...", __func__);
  char * yaml = NULL;
  size_t yaml_size;
  char * error_msg =
    project_decompress (
      &yaml, &yaml_size,
      PROJECT_DECOMPRESS_DATA,
      compressed_pj, compressed_pj_size,
      PROJECT_DECOMPRESS_DATA);
  g_free (compressed_pj);
  if (error_msg)
    {
      g_warning (
        "Failed to decompress project file: %s",
        error_msg);
      ui_show_error_message (
        MAIN_WINDOW, error_msg);
      g_free (error_msg);
      return -1;
    }

  /* make string null-terminated */
  yaml =
    realloc (
      yaml,
      yaml_size + sizeof (char));
  yaml[yaml_size] = '\0';

  Project * self = project_deserialize (yaml);
  free (yaml);
  if (!self)
    {
      g_critical ("Failed to load project");
      return -1;
    }
  self->backup_dir =
    g_strdup (PROJECT->backup_dir);

  char * version = zrythm_get_version (0);
  if (!string_is_equal (
        self->version, version, 1))
    {
      char * str =
        g_strdup_printf (
          _("This project was created with a "
            "different version of %s (%s). "
            "It may not work correctly."),
          PROGRAM_NAME, self->version);
      ui_show_message_full (
        GTK_WINDOW (MAIN_WINDOW),
        GTK_MESSAGE_WARNING, str);
      g_free (str);
    }
  g_free (version);

  if (self == NULL)
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

  MainWindowWidget * mww = NULL;
  if (ZRYTHM_HAVE_UI)
    {
      g_message ("hiding prev window...");
      mww = hide_prev_main_window ();
    }

  if (PROJECT)
    {
      g_message (
        "%s: freeing previous project...",
        __func__);
      object_free_w_func_and_null (
        project_free, PROJECT);
    }

  g_message (
    "%s: initing loaded structures", __func__);
  PROJECT = self;

  /* re-update paths for the newly loaded project */
  set_dir (self, dir);

  /* set the tempo track */
  Tracklist * tracklist = self->tracklist;
  for (int i = 0; i < tracklist->num_tracks; i++)
    {
      Track * track = tracklist->tracks[i];

      if (track->type == TRACK_TYPE_TEMPO)
        {
          tracklist->tempo_track = track;
          break;
        }
    }

  char * filepath_noext =
    g_path_get_basename (dir);
  g_free (dir);

  self->title = filepath_noext;

  engine_init_loaded (self->audio_engine);
  undo_manager_init_loaded (self->undo_manager);

  clip_editor_init_loaded (CLIP_EDITOR);
  tracklist_init_loaded (self->tracklist);

  engine_update_frames_per_tick (
    AUDIO_ENGINE, TRANSPORT->beats_per_bar,
    tempo_track_get_current_bpm (P_TEMPO_TRACK),
    AUDIO_ENGINE->sample_rate);

  /* init ports */
  int max_size = 20;
  Port ** ports =
    calloc ((size_t) max_size, sizeof (Port *));
  int num_ports = 0;
  port_get_all (
    &ports, &max_size, true, &num_ports);
  g_message ("Initializing loaded Ports...");
  for (int i = 0; i < num_ports; i++)
    {
      Port * port = ports[i];
      port_init_loaded (port, true);
    }

  midi_mappings_init_loaded (
    self->midi_mappings);

  arranger_selections_init_loaded (
    (ArrangerSelections *)
    &self->timeline_selections, true);
  arranger_selections_init_loaded (
    (ArrangerSelections *)
    &self->midi_arranger_selections, true);
  arranger_selections_init_loaded (
    (ArrangerSelections *)
    &self->chord_selections, true);
  arranger_selections_init_loaded (
    (ArrangerSelections *)
    &self->automation_selections, true);

  tracklist_selections_init_loaded (
    self->tracklist_selections);

  snap_grid_update_snap_points (
    &self->snap_grid_timeline);
  snap_grid_update_snap_points (
    &self->snap_grid_midi);
  quantize_options_update_quantize_points (
    &self->quantize_opts_timeline);
  quantize_options_update_quantize_points (
    &self->quantize_opts_editor);

  region_link_group_manager_init_loaded (
    REGION_LINK_GROUP_MANAGER);

  if (ZRYTHM_HAVE_UI)
    {
      g_message ("recreating main window...");
      recreate_main_window ();

      if (mww)
        {
          g_message ("destroying prev window...");
          destroy_prev_main_window (mww);
        }
    }

  /* sanity check */
  project_sanity_check (self);

  engine_setup (self->audio_engine);

  for (int i = 0; i < num_ports; i++)
    {
      Port * port = ports[i];
      if (port_is_exposed_to_backend (port))
        {
          port_set_expose_to_backend (port, true);
        }
    }
  object_zero_and_free (ports);

  self->loaded = true;
  self->loading_from_backup = false;

  g_message ("project loaded");

  /* recalculate the routing graph */
  router_recalc_graph (ROUTER, F_NOT_SOFT);

  g_message ("setting up main window...");
  setup_main_window (self);

  engine_set_run (self->audio_engine, true);

  return 0;
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
  g_message (
    "%s: filename: %s, is template: %d",
    __func__, filename, is_template);

  if (filename)
    {
      int ret = load (filename, is_template);
      if (ret)
        {
          ui_show_error_message (
            NULL,
            _("Failed to load project. Will create "
            "a new one instead."));

          CreateProjectDialogWidget * dialog =
            create_project_dialog_widget_new ();

          ret =
            gtk_dialog_run (GTK_DIALOG (dialog));
          gtk_widget_destroy (GTK_WIDGET (dialog));

          if (ret != GTK_RESPONSE_OK)
            return -1;

          g_message (
            "%s: creating project %s",
            __func__, ZRYTHM->create_project_path);
          create_default (PROJECT);
        }
    }
  else
    {
      create_default (PROJECT);
    }

  engine_activate (AUDIO_ENGINE, true);

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
    {
      project_save (
        PROJECT, PROJECT->dir, 0, 0, F_NO_ASYNC);
    }

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
      gint64 cur_time = g_get_monotonic_time ();
      gint64 microsec_to_autosave =
        (gint64)
        g_settings_get_uint (
          S_P_PROJECTS_GENERAL,
          "autosave-interval") * 60 *
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
            PROJECT, PROJECT->dir, 1, 1,
            F_ASYNC);
          PROJECT->last_autosave_time = cur_time;
        }
    }

  return G_SOURCE_CONTINUE;
}

/**
 * Returns the requested project path as a newly
 * allocated string.
 *
 * @param backup Whether to get the path for the
 *   current backup instead of the main project.
 */
char *
project_get_path (
  Project *     self,
  ProjectPath   path,
  bool          backup)
{
  g_return_val_if_fail (self->dir, NULL);
  char * dir =
    backup ? self->backup_dir : self->dir;
  switch (path)
    {
    case PROJECT_PATH_BACKUPS:
      return
        g_build_filename (
          dir, PROJECT_BACKUPS_DIR, NULL);
    case PROJECT_PATH_EXPORTS:
      return
        g_build_filename (
          dir, PROJECT_EXPORTS_DIR, NULL);
    case PROJECT_PATH_PLUGINS:
      return
        g_build_filename (
          dir, PROJECT_PLUGINS_DIR, NULL);
    case PROJECT_PATH_PLUGIN_STATES:
      {
        char * plugins_dir =
          project_get_path (
            self, PROJECT_PATH_PLUGINS, backup);
        char * ret =
          g_build_filename (
            plugins_dir, PROJECT_PLUGIN_STATES_DIR,
            NULL);
        g_free (plugins_dir);
        return ret;
      }
      break;
    case PROJECT_PATH_PLUGIN_EXT_COPIES:
      {
        char * plugins_dir =
          project_get_path (
            self, PROJECT_PATH_PLUGINS, backup);
        char * ret =
          g_build_filename (
            plugins_dir,
            PROJECT_PLUGIN_EXT_COPIES_DIR,
            NULL);
        g_free (plugins_dir);
        return ret;
      }
      break;
    case PROJECT_PATH_PLUGIN_EXT_LINKS:
      {
        char * plugins_dir =
          project_get_path (
            self, PROJECT_PATH_PLUGINS, backup);
        char * ret =
          g_build_filename (
            plugins_dir,
            PROJECT_PLUGIN_EXT_LINKS_DIR,
            NULL);
        g_free (plugins_dir);
        return ret;
      }
      break;
    case PROJECT_PATH_POOL:
      return
        g_build_filename (
          self->dir, PROJECT_POOL_DIR, NULL);
    case PROJECT_PATH_PROJECT_FILE:
      if (backup)
        {
          return
            g_build_filename (
              self->backup_dir, PROJECT_FILE, NULL);
        }
      else
        {
          return
            g_build_filename (
              self->dir, PROJECT_FILE, NULL);
        }
      break;
    default:
      g_return_val_if_reached (NULL);
    }
  g_return_val_if_reached (NULL);
}

/**
 * Creates an empty project object.
 */
Project *
project_new (
  Zrythm * _zrythm)
{
  g_message ("%s: Creating...", __func__);

  Project * self = object_new (Project);

  if (_zrythm)
    {
      _zrythm->project = self;
    }

  self->clip_editor = clip_editor_new ();
  self->tracklist_selections =
    tracklist_selections_new (true);

  g_message ("%s: done", __func__);

  return self;
}

/**
 * Projet save data.
 */
typedef struct ProjectSaveData
{
  /** Project clone (with memcpy). */
  Project   project;

  /** Full path to save to. */
  char *    project_file_path;

  bool      is_backup;

  /** To be set to true when the thread finishes. */
  bool      finished;

  bool      show_notification;

  /** Whether an error occured during saving. */
  bool      has_error;
} ProjectSaveData;

static void
project_save_data_free (
  ProjectSaveData * self)
{
  if (self->project_file_path)
    {
      g_free_and_null (self->project_file_path);
    }

  object_zero_and_free (self);
}

/**
 * Thread that does the serialization and saving.
 */
static void *
serialize_project_thread (
  ProjectSaveData * data)
{
  char * compressed_yaml;
  char * error_msg;
  size_t compressed_size;

  /* generate yaml */
  g_message ("serializing project to yaml...");
  GError *err = NULL;
  gint64 time_before = g_get_monotonic_time ();
  char * yaml = project_serialize (&data->project);
  gint64 time_after = g_get_monotonic_time ();
  g_message (
    "time to serialize: %ldms",
    (long) (time_after - time_before) / 1000);
  if (!yaml)
    {
      g_critical ("Failed to serialize project");
      data->has_error = true;
      goto serialize_end;
    }

  /* compress */
  error_msg =
    project_compress (
      &compressed_yaml, &compressed_size,
      PROJECT_COMPRESS_DATA,
      yaml, strlen (yaml) * sizeof (char),
      PROJECT_COMPRESS_DATA);
  g_free (yaml);
  if (error_msg)
    {
      g_critical (
        "Failed to compress project file: %s",
        error_msg);
      ui_show_error_message (
        MAIN_WINDOW, error_msg);
      g_free (error_msg);
      data->has_error = true;
      goto serialize_end;
    }

  /* set file contents */
  g_message (
    "%s: saving project file...", __func__);
  g_file_set_contents (
    data->project_file_path, compressed_yaml,
    (gssize) compressed_size, &err);
  free (compressed_yaml);
  if (err != NULL)
    {
      g_critical (
        "%s: Unable to write project file: %s",
        __func__, err->message);
      g_error_free (err);
      data->has_error = true;
    }

  g_message (
    "%s: successfully saved project", __func__);

serialize_end:
  data->finished = true;
  return NULL;
}

/**
 * Idle func to check if the project has finished
 * saving and show a notification.
 */
static int
project_idle_saved_cb (
  ProjectSaveData * data)
{
  if (!data->finished)
    {
      return G_SOURCE_CONTINUE;
    }

  if (data->is_backup)
    {
      g_message (_("Backup saved."));
    }
  else
    {
      if (!ZRYTHM_TESTING)
        {
          zrythm_add_to_recent_projects (
            ZRYTHM, data->project_file_path);
        }
      if (data->show_notification)
        ui_show_notification (_("Project saved."));
    }

  if (ZRYTHM_HAVE_UI)
    {
      EVENTS_PUSH (ET_PROJECT_SAVED, PROJECT);
    }

  object_free_w_func_and_null (
    project_save_data_free, data);

  return G_SOURCE_REMOVE;
}

/**
 * Saves the project to a project file in the
 * given dir.
 *
 * @param is_backup 1 if this is a backup. Backups
 *   will be saved as <original filename>.bak<num>.
 * @param show_notification Show a notification
 *   in the UI that the project was saved.
 * @param async Save asynchronously in another
 *   thread.
 *
 * @return Non-zero if error.
 */
int
project_save (
  Project *    self,
  const char * _dir,
  const bool   is_backup,
  const bool   show_notification,
  const bool   async)
{
  int i, j;

  char * dir = g_strdup (_dir);

  /* set the dir and create it if it doesn't
   * exist */
  set_dir (self, dir);
  io_mkdir (PROJECT->dir);
  char * tmp;

  /* set the title */
  char * basename =
    g_path_get_basename (dir);
  set_title (self, basename);
  g_free (basename);
  g_free (dir);

  /* save current datetime */
  set_datetime_str (self);

  /* if backup, get next available backup dir */
  if (is_backup)
    set_and_create_next_available_backup_dir (self);

#define MK_PROJECT_DIR(_path) \
  tmp = \
    project_get_path ( \
      self, PROJECT_PATH_##_path, is_backup); \
  io_mkdir (tmp); \
  g_free_and_null (tmp)

  MK_PROJECT_DIR (EXPORTS);
  MK_PROJECT_DIR (POOL);
  MK_PROJECT_DIR (PLUGIN_STATES);
  MK_PROJECT_DIR (PLUGIN_EXT_COPIES);
  MK_PROJECT_DIR (PLUGIN_EXT_LINKS);

  /* write plugin states, prepare channels for
   * serialization, */
  Track * track;
  Channel * ch;
  Plugin * pl;
  for (i = 0; i < TRACKLIST->num_tracks; i++)
    {
      track = TRACKLIST->tracks[i];
      if (track->type == TRACK_TYPE_CHORD)
        continue;

      ch = track->channel;
      if (!ch)
        continue;

      for (j = 0; j < STRIP_SIZE * 2 + 1; j++)
        {
          if (j < STRIP_SIZE)
            pl = ch->midi_fx[j];
          else if (j == STRIP_SIZE)
            pl = ch->instrument;
          else
            pl = ch->inserts[j - (STRIP_SIZE + 1)];

          if (!pl)
            continue;

          /* save state */
#ifdef HAVE_CARLA
          if (pl->descr->open_with_carla)
            {
              carla_native_plugin_save_state (
                pl->carla, is_backup);
            }
          else
            {
#endif
              switch (pl->descr->protocol)
                {
                case PROT_LV2:
                  lv2_state_save_to_file (
                    pl->lv2, is_backup);
                  break;
                default:
                  g_warn_if_reached ();
                  break;
                }
#ifdef HAVE_CARLA
            }
#endif
        }
    }

  ProjectSaveData * data =
    object_new (ProjectSaveData);
  data->project_file_path =
    project_get_path (
      self, PROJECT_PATH_PROJECT_FILE, is_backup);
  data->show_notification = show_notification;
  data->is_backup = is_backup;
  memcpy (
    &data->project, PROJECT, sizeof (Project));
  if (async)
    {
      g_thread_new (
        "serialize_project_thread",
        (GThreadFunc)
        serialize_project_thread, data);
      g_idle_add (
        (GSourceFunc) project_idle_saved_cb,
        data);
    }
  else
    {
      /* call synchronously */
      serialize_project_thread (data);
      project_idle_saved_cb (data);
    }

  RETURN_OK;
}

SERIALIZE_SRC (Project, project)
DESERIALIZE_SRC (Project, project)
PRINT_YAML_SRC (Project, project)
