// SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <sys/stat.h>

#include "dsp/chord_track.h"
#include "dsp/marker.h"
#include "dsp/marker_track.h"
#include "dsp/modulator_track.h"
#include "dsp/router.h"
#include "dsp/tempo_track.h"
#include "dsp/tracklist.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/dialogs/create_project_dialog.h"
#include "gui/widgets/main_notebook.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/timeline_panel.h"
#include "io/serialization/project.h"
#include "project.h"
#include "schemas/project.h"
#include "utils/datetime.h"
#include "utils/debug.h"
#include "utils/dialogs.h"
#include "utils/error.h"
#include "utils/flags.h"
#include "utils/io.h"
#include "utils/objects.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <adwaita.h>

#include "project/project_init_flow_manager.h"
#include <time.h>
#include <yyjson.h>

typedef enum
{
  Z_PROJECT_INIT_FLOW_MANAGER_ERROR_FAILED,
} ZProjectInitFlowManagerError;

#define Z_PROJECT_INIT_FLOW_MANAGER_ERROR \
  z_project_init_flow_manager_error_quark ()
GQuark
z_project_init_flow_manager_error_quark (void);
G_DEFINE_QUARK (z - project - init - flow - manager - error - quark, z_project_init_flow_manager_error)

/**
 * This struct handles the initialization flow when creating a new project
 * or loading a project.
 *
 * It uses callbacks to notify an interested party when it finishes each step.
 */
typedef struct ProjectInitFlowManager
{
  char * filename;
  bool   is_template;

  /** Same as Project.dir. */
  char * dir;

  /** Callback/user data pairs. */
  GPtrArray * callback_stack;
  GPtrArray * user_data_stack;
} ProjectInitFlowManager;

static ProjectInitFlowManager *
project_init_flow_manager_new (void)
{
  ProjectInitFlowManager * flow_mgr = object_new (ProjectInitFlowManager);
  flow_mgr->callback_stack = g_ptr_array_new ();
  flow_mgr->user_data_stack = g_ptr_array_new ();
  return flow_mgr;
}

static void
project_init_flow_manager_append_callback (
  ProjectInitFlowManager * flow_mgr,
  ProjectInitDoneCallback  cb,
  void *                   user_data)
{
  g_ptr_array_add (flow_mgr->callback_stack, cb);
  g_ptr_array_add (flow_mgr->user_data_stack, user_data);
}

static void
_project_init_flow_manager_call_last_callback (
  ProjectInitFlowManager * self,
  bool                     success,
  GError *                 error)
{
  g_return_if_fail (self->callback_stack->len > 0);
  ProjectInitDoneCallback cb = g_ptr_array_steal_index (
    self->callback_stack, self->callback_stack->len - 1);
  void * user_data = g_ptr_array_steal_index (
    self->user_data_stack, self->user_data_stack->len - 1);
  cb (success, error, user_data);
}

static void
project_init_flow_manager_call_last_callback_fail (
  ProjectInitFlowManager * self,
  GError *                 error)
{
  _project_init_flow_manager_call_last_callback (self, false, error);
}

static void
project_init_flow_manager_call_last_callback_success (
  ProjectInitFlowManager * self)
{
  _project_init_flow_manager_call_last_callback (self, true, NULL);
}

static void
recreate_main_window (void)
{
  g_message ("recreating main window...");
  MAIN_WINDOW = main_window_widget_new (zrythm_app);
  g_warn_if_fail (
    MAIN_WINDOW->center_dock->main_notebook->timeline_panel->tracklist);
}

static MainWindowWidget *
hide_prev_main_window (void)
{
  event_manager_stop_events (EVENT_MANAGER);

  MainWindowWidget * mww = MAIN_WINDOW;
  MAIN_WINDOW = NULL;
  if (GTK_IS_WIDGET (mww))
    {
      g_message ("hiding previous main window...");
      gtk_widget_set_visible (GTK_WIDGET (mww), false);
    }

  return mww;
}

static void
destroy_prev_main_window (MainWindowWidget * mww)
{
  if (GTK_IS_WIDGET (mww))
    {
      g_message ("destroying previous main window...");
      main_window_widget_tear_down (mww);
      /*g_object_unref (mww);*/
    }
}

static void
setup_main_window (Project * self)
{
  /* mimic behavior when starting the app */
  if (ZRYTHM_HAVE_UI)
    {
      g_message ("setting up main window...");
      event_manager_start_events (EVENT_MANAGER);
      main_window_widget_setup (MAIN_WINDOW);

      EVENTS_PUSH (ET_PROJECT_LOADED, self);
    }
}

/**
 * Returns the filepath of a backup (directory),
 * if any,
 * if it has a newer timestamp than the actual
 * file being loaded.
 *
 * @param dir The non-backup dir.
 */
static char *
get_newer_backup (Project * self)
{
  GDir *        dir;
  GError *      error = NULL;
  const gchar * filename;
  struct stat   stat_res;
  struct tm *   orig_tm, *nowtm;
  time_t        t1;
  time_t        t2;

  char * filepath = project_get_path (self, PROJECT_PATH_PROJECT_FILE, false);
  g_return_val_if_fail (filepath, NULL);

  if (stat (filepath, &stat_res) == 0)
    {
      orig_tm = localtime (&stat_res.st_mtime);
      t1 = mktime (orig_tm);
    }
  else
    {
      g_warning ("Failed to get last modified for %s", filepath);
      return NULL;
    }
  g_free (filepath);

  char * result = NULL;
  char * backups_dir = project_get_path (self, PROJECT_PATH_BACKUPS, false);
  dir = g_dir_open (backups_dir, 0, &error);
  if (!dir)
    {
      return NULL;
    }
  while ((filename = g_dir_read_name (dir)))
    {
      char * full_path =
        g_build_filename (backups_dir, filename, PROJECT_FILE, NULL);
      g_message ("%s", full_path);

      if (stat (full_path, &stat_res) == 0)
        {
          nowtm = localtime (&stat_res.st_mtime);
          t2 = mktime (nowtm);
          /* if backup is after original project */
          if (difftime (t2, t1) > 0)
            {
              if (!result)
                g_free (result);
              result = g_build_filename (backups_dir, filename, NULL);
              t1 = t2;
            }
        }
      else
        {
          g_warning ("Failed to get last modified for %s", full_path);
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

/**
 * Upgrades the given project YAML's schema if needed.
 *
 * @return True if the schema was upgraded.
 */
COLD static bool
upgrade_schema (char ** yaml, int src_ver, GError ** error)
{
  g_message ("upgrading project schema from version %d...", src_ver);
  switch (src_ver)
    {
    case 1:
      {
        /* deserialize into the previous version of the struct */
        GError *     err = NULL;
        Project_v1 * self =
          (Project_v1 *) yaml_deserialize (*yaml, &project_schema_v1, &err);
        if (!self)
          {
            PROPAGATE_PREFIXED_ERROR_LITERAL (
              error, err, _ ("Failed to deserialize v1 project file"));
            return false;
          }

        /* only dropping undo history, so just re-serialize
       * into YAML */
        g_free (*yaml);
        err = NULL;
        *yaml = yaml_serialize (self, &project_schema_v1, &err);
        if (!*yaml)
          {
            PROPAGATE_PREFIXED_ERROR_LITERAL (
              error, err, _ ("Failed to serialize v1 project file"));
            return false;
          }
        cyaml_config_t cyaml_config;
        yaml_get_cyaml_config (&cyaml_config);

        /* free memory allocated by libcyaml */
        cyaml_free (&cyaml_config, &project_schema_v1, self, 0);

        /* call again for next iteration */
        err = NULL;
        bool upgraded = upgrade_schema (yaml, 3, &err);
        if (!upgraded)
          {
            PROPAGATE_PREFIXED_ERROR_LITERAL (
              error, err, "Failed to upgrade project schema to version 3");
            return false;
          }
        return upgraded;
      }
      break;
    case 2:
    case 3:
      {
        /* deserialize into the previous version of the struct */
        GError *     err = NULL;
        Project_v1 * old_prj =
          (Project_v1 *) yaml_deserialize (*yaml, &project_schema_v1, &err);
        if (!old_prj)
          {
            PROPAGATE_PREFIXED_ERROR_LITERAL (
              error, err, _ ("Failed to deserialize v2/3 project file"));
            return false;
          }

        /* create the new project and serialize it */
        Project_v5   _new_prj;
        Project_v5 * new_prj = &_new_prj;
        memset (new_prj, 0, sizeof (Project_v5));
        new_prj->schema_version = 4;
        new_prj->title = old_prj->title;
        new_prj->datetime_str = datetime_get_current_as_string ();
        new_prj->version = zrythm_get_version (false);

        /* upgrade */
        new_prj->tracklist = tracklist_upgrade_from_v1 (old_prj->tracklist);
        new_prj->audio_engine = engine_upgrade_from_v1 (old_prj->audio_engine);
        new_prj->tracklist_selections =
          tracklist_selections_upgrade_from_v1 (old_prj->tracklist_selections);

        new_prj->clip_editor = old_prj->clip_editor;
        new_prj->timeline = old_prj->timeline;
        new_prj->snap_grid_timeline = old_prj->snap_grid_timeline;
        new_prj->snap_grid_editor = old_prj->snap_grid_editor;
        new_prj->quantize_opts_timeline = old_prj->quantize_opts_timeline;
        new_prj->quantize_opts_editor = old_prj->quantize_opts_editor;
        new_prj->region_link_group_manager = old_prj->region_link_group_manager;
        new_prj->port_connections_manager = old_prj->port_connections_manager;
        new_prj->midi_mappings = old_prj->midi_mappings;
        new_prj->last_selection = old_prj->last_selection;

        /* re-serialize */
        g_free (*yaml);
        err = NULL;
        *yaml = yaml_serialize (new_prj, &project_schema_v5, &err);
        if (!*yaml)
          {
            PROPAGATE_PREFIXED_ERROR_LITERAL (
              error, err, _ ("Failed to serialize v3 project file"));
            return false;
          }
        cyaml_config_t cyaml_config;
        yaml_get_cyaml_config (&cyaml_config);

        /* free memory allocated by libcyaml */
        cyaml_free (&cyaml_config, &project_schema_v1, old_prj, 0);

        /* free memory allocated now */
        g_free_and_null (new_prj->datetime_str);
        g_free_and_null (new_prj->version);

        return true;
      }
      break;
    case 4:
      /* v4 note:
       * AutomationSelections had schema_version instead of
       * base as the first member. This was an issue and was
       * fixed, but projects before this change have wrong
       * values. We are lucky they are all primitive types.
       * This can be fixed by simply dropping the undo history
       * and the selections. */
      {
        /* deserialize into the current version of the struct */
        GError *     err = NULL;
        Project_v5 * old_prj =
          (Project_v5 *) yaml_deserialize (*yaml, &project_schema_v5, &err);
        if (!old_prj)
          {
            PROPAGATE_PREFIXED_ERROR_LITERAL (
              error, err, _ ("Failed to deserialize v4 project file"));
            return false;
          }

        /* create the new project and serialize it */
        Project_v5   _new_prj;
        Project_v5 * new_prj = &_new_prj;
        memset (new_prj, 0, sizeof (Project_v5));
        *new_prj = *old_prj;
        new_prj->schema_version = 5;
        new_prj->title = old_prj->title;
        new_prj->datetime_str = datetime_get_current_as_string ();
        new_prj->version = zrythm_get_version (false);

        /* re-serialize */
        g_free (*yaml);
        err = NULL;
        *yaml = yaml_serialize (new_prj, &project_schema_v5, &err);
        if (!*yaml)
          {
            PROPAGATE_PREFIXED_ERROR_LITERAL (
              error, err, _ ("Failed to serialize v4 project file"));
            return false;
          }
        cyaml_config_t cyaml_config;
        yaml_get_cyaml_config (&cyaml_config);

        /* free memory allocated by libcyaml */
        cyaml_free (&cyaml_config, &project_schema_v5, old_prj, 0);

        /* free memory allocated now */
        g_free_and_null (new_prj->datetime_str);
        g_free_and_null (new_prj->version);

        return true;
      }
      break;
    default:
      return true;
    }
}

static bool
upgrade_to_json (char ** txt, GError ** error)
{
  GError *     err = NULL;
  Project_v5 * old_prj =
    (Project_v5 *) yaml_deserialize (*txt, &project_schema_v5, &err);
  if (!old_prj)
    {
      PROPAGATE_PREFIXED_ERROR_LITERAL (
        error, err, _ ("Failed to deserialize v5 project file"));
      return false;
    }

  *txt = project_v5_serialize_to_json_str (old_prj, &err);
  if (*txt == NULL)
    {
      PROPAGATE_PREFIXED_ERROR_LITERAL (
        error, err, _ ("Failed to convert v5 YAML project file to JSON"));
      return false;
    }
  return true;
}

static void
project_activate (void)
{
  g_return_if_fail (PROJECT);

  g_message ("Activating main project %s (%p)...", PROJECT->title, PROJECT);

  PROJECT->last_saved_action = undo_manager_get_last_action (UNDO_MANAGER);

  engine_activate (AUDIO_ENGINE, true);

  /* pause engine */
  EngineState state;
  engine_wait_for_pause (AUDIO_ENGINE, &state, true, false);

  /* connect channel inputs to hardware and re-expose ports to
   * backend. has to be done after engine activation */
  Channel * ch;
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      ch = TRACKLIST->tracks[i]->channel;
      if (!ch)
        continue;

      channel_reconnect_ext_input_ports (ch);
    }
  tracklist_expose_ports_to_backend (TRACKLIST);

  /* reconnect graph */
  router_recalc_graph (ROUTER, F_NOT_SOFT);

  /* fix audio regions in case running under a new sample
   * rate */
  project_fix_audio_regions (PROJECT);

  /* resume engine */
  engine_resume (AUDIO_ENGINE, &state);

  g_message ("Project %s (%p) activated", PROJECT->title, PROJECT);

  return;
}

/**
 * Creates a default project.
 *
 * This is only used internally or for generating projects
 * from scripts.
 *
 * @param prj_dir The directory of the project to create,
 *   including its title.
 * @param headless Create the project assuming we are running
 *   without a UI.
 * @param start_engine Whether to also start the engine after
 *   creating the project.
 */
WARN_UNUSED_RESULT static Project *
create_default (
  Project *    self,
  const char * prj_dir,
  bool         headless,
  bool         with_engine,
  GError **    error)
{
  g_message ("creating default project...");

  bool have_ui = !headless && ZRYTHM_HAVE_UI;

  MainWindowWidget * mww = NULL;
  if (have_ui)
    {
      g_message ("hiding prev window...");
      mww = hide_prev_main_window ();
    }

  if (self)
    {
      project_free (self);
    }
  self = project_new (ZRYTHM);

  self->tracklist = tracklist_new (self, NULL);

  /* initialize selections */
  project_init_selections (self);

  self->audio_engine = engine_new (self);

  /* init undo manager */
  self->undo_manager = undo_manager_new ();

  self->title = g_path_get_basename (prj_dir);

  /* init pinned tracks */

  /* chord */
  g_message ("adding chord track...");
  Track * track = chord_track_new (TRACKLIST->num_tracks);
  tracklist_append_track (
    TRACKLIST, track, F_NO_PUBLISH_EVENTS, F_NO_RECALC_GRAPH);
  self->tracklist->chord_track = track;

  /* tempo */
  g_message ("adding tempo track...");
  track = tempo_track_default (TRACKLIST->num_tracks);
  tracklist_append_track (
    TRACKLIST, track, F_NO_PUBLISH_EVENTS, F_NO_RECALC_GRAPH);
  self->tracklist->tempo_track = track;
  int   beats_per_bar = tempo_track_get_beats_per_bar (track);
  int   beat_unit = tempo_track_get_beat_unit (track);
  bpm_t bpm = tempo_track_get_current_bpm (track);
  transport_update_caches (
    self->audio_engine->transport, beats_per_bar, beat_unit);
  engine_update_frames_per_tick (
    self->audio_engine, beats_per_bar, bpm, self->audio_engine->sample_rate,
    true, true, false);

  /* modulator */
  g_message ("adding modulator track...");
  track = modulator_track_default (TRACKLIST->num_tracks);
  tracklist_append_track (
    TRACKLIST, track, F_NO_PUBLISH_EVENTS, F_NO_RECALC_GRAPH);
  self->tracklist->modulator_track = track;

  /* marker */
  g_message ("adding marker track...");
  track = marker_track_default (TRACKLIST->num_tracks);
  tracklist_append_track (
    TRACKLIST, track, F_NO_PUBLISH_EVENTS, F_NO_RECALC_GRAPH);
  self->tracklist->marker_track = track;

  self->tracklist->pinned_tracks_cutoff = track->pos + 1;

  /* add master channel to mixer and tracklist */
  g_message ("adding master track...");
  track = track_new (
    TRACK_TYPE_MASTER, TRACKLIST->num_tracks, _ ("Master"), F_WITHOUT_LANE);
  tracklist_append_track (
    TRACKLIST, track, F_NO_PUBLISH_EVENTS, F_NO_RECALC_GRAPH);
  self->tracklist->master_track = track;
  tracklist_selections_add_track (self->tracklist_selections, track, 0);
  self->last_selection = SELECTION_TYPE_TRACKLIST;

  /* pre-setup engine */
  if (with_engine)
    {
      engine_pre_setup (self->audio_engine);
    }

  engine_setup (self->audio_engine);

  if (with_engine)
    {
      tracklist_expose_ports_to_backend (self->tracklist);
    }

  engine_update_frames_per_tick (
    AUDIO_ENGINE, beats_per_bar, tempo_track_get_current_bpm (P_TEMPO_TRACK),
    AUDIO_ENGINE->sample_rate, true, true, false);

  /* set directory/title and create standard dirs */
  g_free_and_null (self->dir);
  self->dir = g_strdup (prj_dir);
  g_free_and_null (self->title);
  self->title = g_path_get_basename (prj_dir);
  GError * err = NULL;
  bool     success = project_make_project_dirs (self, F_NOT_BACKUP, &err);
  if (!success)
    {
      PROPAGATE_PREFIXED_ERROR_LITERAL (
        error, err, "Failed to create project directories");
      return NULL;
    }

  if (have_ui)
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
    self->snap_grid_timeline, SNAP_GRID_TYPE_TIMELINE, NOTE_LENGTH_BAR, true);
  quantize_options_init (self->quantize_opts_timeline, NOTE_LENGTH_1_1);
  snap_grid_init (
    self->snap_grid_editor, SNAP_GRID_TYPE_EDITOR, NOTE_LENGTH_1_8, true);
  quantize_options_init (self->quantize_opts_editor, NOTE_LENGTH_1_8);
  clip_editor_init (self->clip_editor);
  timeline_init (self->timeline);
  quantize_options_update_quantize_points (self->quantize_opts_timeline);
  quantize_options_update_quantize_points (self->quantize_opts_editor);

  if (have_ui)
    {
      g_message ("setting up main window...");
      g_warn_if_fail (
        MAIN_WINDOW->center_dock->main_notebook->timeline_panel->tracklist);
      setup_main_window (self);
    }

  if (with_engine)
    {
      /* recalculate the routing graph */
      router_recalc_graph (ROUTER, F_NOT_SOFT);

      engine_set_run (self->audio_engine, true);
    }

  g_message ("done creating default project");

  return self;
}

static bool
save_newly_created_project_if_required (
  bool         is_template,
  const char * filename,
  GError **    error)
{
  /* if creating a new project (either from a template or default project),
   * save the newly created project */
  if (is_template || !filename)
    {
      GError * err = NULL;
      bool     success = project_save (
        PROJECT, PROJECT->dir, F_NOT_BACKUP, Z_F_NO_SHOW_NOTIFICATION,
        F_NO_ASYNC, &err);
      if (!success)
        {
          PROPAGATE_PREFIXED_ERROR (
            error, err, "%s", _ ("Failed to save project"));
          return false;
        }
    }

  return true;
}

/**
 * Called when a new project is created or one is loaded to save the project
 * and, if succeeded, activate it.
 */
static void
save_and_activate_after_successful_load_or_create (
  ProjectInitFlowManager * flow_mgr)
{
  GError * err = NULL;
  bool     success = save_newly_created_project_if_required (
    flow_mgr->is_template, flow_mgr->filename, &err);
  if (!success)
    {
      GError * error = NULL;
      PROPAGATE_PREFIXED_ERROR_LITERAL (
        &error, err, "Failed to save newly created project");
      project_init_flow_manager_call_last_callback_fail (flow_mgr, error);
      return;
    }

  project_activate ();
  project_init_flow_manager_call_last_callback_success (flow_mgr);
}

static void
replace_main_window (MainWindowWidget * mww)
{
  if (ZRYTHM_HAVE_UI)
    {
      g_message ("recreating main window...");
      recreate_main_window ();

      if (mww)
        {
          g_message ("destroying prev window...");
          destroy_prev_main_window (mww);
        }

      g_return_if_fail (GTK_IS_WINDOW (MAIN_WINDOW));
    }
}

static void
continue_load_from_file_after_open_backup_response (
  ProjectInitFlowManager * flow_mgr)
{
  bool use_backup = PROJECT->backup_dir != NULL;
  PROJECT->loading_from_backup = use_backup;

  GError * err = NULL;
  char *   text =
    project_get_existing_uncompressed_text (PROJECT, use_backup, &err);
  if (!text)
    {
      GError * error = NULL;
      PROPAGATE_PREFIXED_ERROR_LITERAL (
        &error, err, _ ("Failed to get existing text"));
      project_init_flow_manager_call_last_callback_fail (flow_mgr, error);
      return;
    }

  struct yyjson_read_err json_read_err;
  yyjson_doc *           doc = yyjson_read_opts (
    text, strlen (text), YYJSON_READ_NOFLAG, NULL, &json_read_err);
  bool upgraded = false;
  int  yaml_schema_ver = -1;
  if (!doc)
    {
      /* failed to read JSON - check if YAML */
      int schema_ver = string_get_regex_group_as_int (
        text, "---\nschema_version: (.*)\n", 1, -1);
      if (schema_ver > 0)
        {
          yaml_schema_ver = schema_ver;
          /* upgrade YAML and set doc */
          char * prj_ver_str =
            string_get_regex_group (text, "\nversion: (.*)\n", 1);
          if (!prj_ver_str)
            {
              GError * error = NULL;
              PROPAGATE_PREFIXED_ERROR_LITERAL (
                &error, err, _ ("Invalid project: missing version"));
              project_init_flow_manager_call_last_callback_fail (
                flow_mgr, error);
              return;
            }
          g_message ("project from text (version %s)...", prj_ver_str);
          g_free (prj_ver_str);

          schema_ver = string_get_regex_group_as_int (
            text, "---\nschema_version: (.*)\n", 1, -1);
          g_message ("detected schema version %d", schema_ver);
          if (schema_ver != 5)
            {
              /* upgrade project */
              upgraded = upgrade_schema (&text, schema_ver, &err);
              if (!upgraded)
                {
                  GError * error = NULL;
                  PROPAGATE_PREFIXED_ERROR_LITERAL (
                    &error, err, _ ("Failed to upgrade YAML project schema"));
                  free (text);
                  project_init_flow_manager_call_last_callback_fail (
                    flow_mgr, error);
                  return;
                }
            }

          /* upgrade latest yaml to json */
          upgraded = upgrade_to_json (&text, &err);
          if (!upgraded)
            {
              GError * error = NULL;
              PROPAGATE_PREFIXED_ERROR_LITERAL (
                &error, err, _ ("Failed to upgrade project schema to JSON"));
              free (text);
              project_init_flow_manager_call_last_callback_fail (
                flow_mgr, error);
              return;
            }
        }
      else
        {
          GError * error = NULL;
          g_set_error (
            &error, Z_PROJECT_INIT_FLOW_MANAGER_ERROR,
            Z_PROJECT_INIT_FLOW_MANAGER_ERROR_FAILED,
            _ ("Failed to read JSON: [code: %" PRIu32 ", pos: %zu] %s"),
            json_read_err.code, json_read_err.pos, json_read_err.msg);
          free (text);
          project_init_flow_manager_call_last_callback_fail (flow_mgr, error);
          return;
        }
    }

  gint64    time_before = g_get_monotonic_time ();
  Project * self = project_deserialize_from_json_str (text, &err);
  gint64    time_after = g_get_monotonic_time ();
  g_message (
    "time to deserialize: %ldms", (long) (time_after - time_before) / 1000);
  if (!self)
    {
      GError * error = NULL;
      PROPAGATE_PREFIXED_ERROR_LITERAL (
        &error, err, _ ("Failed to deserialize project YAML"));
      free (text);
      project_init_flow_manager_call_last_callback_fail (flow_mgr, error);
      return;
    }
  free (text);
  self->backup_dir = g_strdup (PROJECT->backup_dir);

  /* check for FINISHED file */
  if (yaml_schema_ver > 3)
    {
      char * finished_file_path =
        project_get_path (PROJECT, PROJECT_PATH_FINISHED_FILE, use_backup);
      bool finished_file_exists =
        g_file_test (finished_file_path, G_FILE_TEST_EXISTS);
      if (!finished_file_exists)
        {
          project_init_flow_manager_call_last_callback_fail (
            flow_mgr,
            g_error_new (
              Z_PROJECT_INIT_FLOW_MANAGER_ERROR,
              Z_PROJECT_INIT_FLOW_MANAGER_ERROR_FAILED,
              _ ("Could not load project: Corrupted project detected (missing FINISHED file at '%s')."),
              finished_file_path));
          return;
        }
      g_free (finished_file_path);
    }

  g_message ("Project successfully deserialized.");

  /* if template, also copy the pool and plugin states */
  if (flow_mgr->is_template)
    {
      char * prev_pool_dir =
        g_build_filename (flow_mgr->dir, PROJECT_POOL_DIR, NULL);
      char * new_pool_dir =
        g_build_filename (ZRYTHM->create_project_path, PROJECT_POOL_DIR, NULL);
      char * prev_plugins_dir =
        g_build_filename (flow_mgr->dir, PROJECT_PLUGINS_DIR, NULL);
      char * new_plugins_dir = g_build_filename (
        ZRYTHM->create_project_path, PROJECT_PLUGINS_DIR, NULL);
      bool success = io_copy_dir (
        new_pool_dir, prev_pool_dir, F_NO_FOLLOW_SYMLINKS, F_RECURSIVE, &err);
      if (!success)
        {
          GError * error = NULL;
          PROPAGATE_PREFIXED_ERROR_LITERAL (
            &error, err, "Failed to copy pool directory");
          project_init_flow_manager_call_last_callback_fail (flow_mgr, error);
          return;
        }
      success = io_copy_dir (
        new_plugins_dir, prev_plugins_dir, F_NO_FOLLOW_SYMLINKS, F_RECURSIVE,
        &err);
      if (!success)
        {
          GError * error = NULL;
          PROPAGATE_PREFIXED_ERROR_LITERAL (
            &error, err, "Failed to copy plugins directory");
          project_init_flow_manager_call_last_callback_fail (flow_mgr, error);
          return;
        }
      g_free (prev_pool_dir);
      g_free (new_pool_dir);
      g_free (prev_plugins_dir);
      g_free (new_plugins_dir);

      g_free (flow_mgr->dir);
      flow_mgr->dir = g_strdup (ZRYTHM->create_project_path);
    }

  /* FIXME this is a hack, make sure none of this extra copying is needed */
  /* if backup, copy plugin states */
  if (use_backup)
    {
      char * prev_plugins_dir =
        g_build_filename (self->backup_dir, PROJECT_PLUGINS_DIR, NULL);
      char * new_plugins_dir =
        g_build_filename (flow_mgr->dir, PROJECT_PLUGINS_DIR, NULL);
      bool success = io_copy_dir (
        new_plugins_dir, prev_plugins_dir, F_NO_FOLLOW_SYMLINKS, F_RECURSIVE,
        &err);
      if (!success)
        {
          GError * error = NULL;
          PROPAGATE_PREFIXED_ERROR_LITERAL (
            &error, err, "Failed to copy plugins directory");
          project_init_flow_manager_call_last_callback_fail (flow_mgr, error);
          return;
        }
      g_free (prev_plugins_dir);
      g_free (new_plugins_dir);
    }

  MainWindowWidget * mww = NULL;
  if (ZRYTHM_HAVE_UI)
    {
      g_message ("hiding prev window...");
      mww = hide_prev_main_window ();
    }

  if (PROJECT)
    {
      g_message ("%s: freeing previous project...", __func__);
      object_free_w_func_and_null (project_free, PROJECT);
    }

  g_message ("%s: initing loaded structures", __func__);
  PROJECT = self;

  /* re-update paths for the newly loaded project */
  g_free_and_null (self->dir);
  self->dir = g_strdup (flow_mgr->dir);

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

  char * filepath_noext = g_path_get_basename (flow_mgr->dir);

  self->title = filepath_noext;

  project_init_common (self);

  err = NULL;
  bool success = engine_init_loaded (self->audio_engine, self, &err);
  if (!success)
    {
on_failed_to_init_pool:
      GtkWindow * err_win = error_handle_prv (
        err, "%s", _ ("Failed to initialize the audio file pool"));
      g_signal_connect (
        err_win, "response", G_CALLBACK (zrythm_exit_response_callback), NULL);
      return;
    }
  engine_pre_setup (self->audio_engine);

  /* re-load clips because sample rate can change during engine pre setup */
  err = NULL;
  success = audio_pool_init_loaded (self->audio_engine->pool, &err);
  if (!success)
    {
      goto on_failed_to_init_pool;
    }

  clip_editor_init_loaded (self->clip_editor);
  timeline_init_loaded (self->timeline);
  tracklist_init_loaded (self->tracklist, self, NULL);

  int beats_per_bar = tempo_track_get_beats_per_bar (P_TEMPO_TRACK);
  engine_update_frames_per_tick (
    AUDIO_ENGINE, beats_per_bar, tempo_track_get_current_bpm (P_TEMPO_TRACK),
    AUDIO_ENGINE->sample_rate, true, true, false);

  /* undo manager must be loaded after updating engine
   * frames per tick */
  if (self->undo_manager)
    {
      undo_manager_init_loaded (self->undo_manager);
    }
  else
    {
      self->undo_manager = undo_manager_new ();
    }

  midi_mappings_init_loaded (self->midi_mappings);

  /* note: when converting from older projects there may be no
   * selections (because it's too much work with little
   * benefit to port the selections from older projects) */

#define INIT_OR_CREATE_ARR_SELECTIONS(prefix, c_type, type_name) \
  if (self->prefix##_selections) \
    { \
      arranger_selections_init_loaded ( \
        (ArrangerSelections *) self->prefix##_selections, true, NULL); \
    } \
  else \
    { \
      self->prefix##_selections = (c_type##Selections *) \
        arranger_selections_new (ARRANGER_SELECTIONS_TYPE_##type_name); \
    }

  INIT_OR_CREATE_ARR_SELECTIONS (audio, Audio, AUDIO);
  INIT_OR_CREATE_ARR_SELECTIONS (chord, Chord, CHORD);
  INIT_OR_CREATE_ARR_SELECTIONS (automation, Automation, AUTOMATION);
  INIT_OR_CREATE_ARR_SELECTIONS (timeline, Timeline, TIMELINE);
  INIT_OR_CREATE_ARR_SELECTIONS (midi_arranger, MidiArranger, MIDI);

  if (!self->mixer_selections)
    {
      self->mixer_selections = mixer_selections_new ();
      mixer_selections_init (self->mixer_selections);
    }

  tracklist_selections_init_loaded (self->tracklist_selections);

  quantize_options_update_quantize_points (self->quantize_opts_timeline);
  quantize_options_update_quantize_points (self->quantize_opts_editor);

  region_link_group_manager_init_loaded (self->region_link_group_manager);
  port_connections_manager_init_loaded (self->port_connections_manager);

  replace_main_window (mww);

  /* sanity check */
  project_validate (self);

  engine_setup (self->audio_engine);

  /* init ports */
  GPtrArray * ports = g_ptr_array_new ();
  port_get_all (ports);
  g_message ("Initializing loaded Ports...");
  for (size_t i = 0; i < ports->len; i++)
    {
      Port * port = g_ptr_array_index (ports, i);
      if (port_is_exposed_to_backend (port))
        {
          port_set_expose_to_backend (port, true);
        }
    }
  object_free_w_func_and_null (g_ptr_array_unref, ports);

  self->loaded = true;
  self->loading_from_backup = false;

  g_message ("project loaded");

  /* recalculate the routing graph */
  router_recalc_graph (ROUTER, F_NOT_SOFT);

  g_message ("setting up main window...");
  setup_main_window (self);

  engine_set_run (self->audio_engine, true);

  int format_minor = 6;
  if (format_minor != PROJECT_FORMAT_MINOR || yaml_schema_ver > 0)
    {
      ui_show_message_printf (
        _ ("Project Upgraded"),
        _ ("This project has been automatically upgraded "
           "to v1.%d. Saving this project will overwrite the "
           "old one. If you would like to keep both, please "
           "use 'Save As...'."),
        PROJECT_FORMAT_MINOR);
    }

  project_init_flow_manager_call_last_callback_success (flow_mgr);
}

static void
on_open_backup_response (
  AdwMessageDialog *       dialog,
  char *                   response,
  ProjectInitFlowManager * flow_mgr)
{
  if (string_is_equal (response, "ignore"))
    {
      g_free_and_null (PROJECT->backup_dir);
    }
  if (MAIN_WINDOW)
    {
      gtk_widget_set_visible (GTK_WIDGET (MAIN_WINDOW), true);
    }
  continue_load_from_file_after_open_backup_response (flow_mgr);
}

/**
 * @param filename The filename to open. This will be the template in the case
 * of template, or the actual project otherwise.
 * @param is_template Load the project as a template and create a new project
 * from it.
 *
 * @return Whether successful.
 */
static void
load_from_file (
  const char *             filename,
  const int                is_template,
  ProjectInitFlowManager * flow_mgr)
{
  g_return_if_fail (filename);
  flow_mgr->dir = io_get_dir (filename);

  if (!PROJECT)
    {
      /* create a temporary project struct */
      PROJECT = project_new (ZRYTHM);
    }

  g_free_and_null (PROJECT->dir);
  PROJECT->dir = g_strdup (flow_mgr->dir);

  /* if loading an actual project, check for backups */
  if (!is_template)
    {
      if (PROJECT->backup_dir)
        g_free (PROJECT->backup_dir);
      PROJECT->backup_dir = get_newer_backup (PROJECT);
      if (PROJECT->backup_dir)
        {
          g_message ("newer backup found %s", PROJECT->backup_dir);

          if (ZRYTHM_TESTING)
            {
              if (!ZRYTHM->open_newer_backup)
                {
                  g_free (PROJECT->backup_dir);
                  PROJECT->backup_dir = NULL;
                }
            }
          else
            {
              AdwMessageDialog * dialog = ADW_MESSAGE_DIALOG (
                adw_message_dialog_new (NULL, _ ("Open Backup?"), NULL));
              adw_message_dialog_format_body_markup (
                dialog, _ ("Newer backup found:\n  %s.\nUse the newer backup?"),
                PROJECT->backup_dir);
              adw_message_dialog_add_responses (
                dialog, "open-backup", _ ("Open Backup"), "ignore",
                _ ("Ignore"), NULL);
              gtk_window_present (GTK_WINDOW (dialog));
              if (MAIN_WINDOW)
                {
                  gtk_widget_set_visible (GTK_WIDGET (MAIN_WINDOW), false);
                }
              g_signal_connect (
                dialog, "response", G_CALLBACK (on_open_backup_response),
                flow_mgr);
              return;
            }
        }
    } /* endif !is_template */

  continue_load_from_file_after_open_backup_response (flow_mgr);
}

static void
on_create_project_response (
  GtkDialog *              dialog,
  gint                     response_id,
  ProjectInitFlowManager * flow_mgr)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      g_message (
        "%s: creating project %s", __func__, ZRYTHM->create_project_path);
      GError *  err = NULL;
      Project * created_prj = create_default (
        PROJECT, ZRYTHM->create_project_path, false, true, &err);
      if (!created_prj)
        {
          GError * error = NULL;
          PROPAGATE_PREFIXED_ERROR_LITERAL (
            &error, err, "Failed to create default project");
          project_init_flow_manager_call_last_callback_fail (flow_mgr, error);
        }

      save_and_activate_after_successful_load_or_create (flow_mgr);
    }
  else
    {
      project_init_flow_manager_call_last_callback_fail (
        flow_mgr,
        g_error_new_literal (
          Z_PROJECT_INIT_FLOW_MANAGER_ERROR,
          Z_PROJECT_INIT_FLOW_MANAGER_ERROR_FAILED, _ ("No project selected")));
    }
}

static void
load_from_file_ready_cb (bool success, GError * error, void * user_data)
{
  ProjectInitFlowManager * flow_mgr = (ProjectInitFlowManager *) user_data;
  if (!success)
    {
      HANDLE_ERROR_LITERAL (
        error,
        _ ("Failed to load project. Will create "
           "a new one instead."));

      CreateProjectDialogWidget * dialog = create_project_dialog_widget_new ();
      gtk_window_present (GTK_WINDOW (dialog));
      g_signal_connect (
        dialog, "response", G_CALLBACK (on_create_project_response), flow_mgr);
      return;
    } /* endif failed to load project */

  save_and_activate_after_successful_load_or_create (flow_mgr);
}

static void
cleanup_flow_mgr_cb (
  bool                     success,
  GError *                 error,
  ProjectInitFlowManager * flow_mgr)
{
  g_debug ("cleaning up project init flow manager...");
  z_return_if_fail_cmp (flow_mgr->callback_stack->len, ==, 1);
  z_return_if_fail_cmp (flow_mgr->user_data_stack->len, ==, 1);
  ProjectInitDoneCallback cb = g_ptr_array_steal_index (
    flow_mgr->callback_stack, flow_mgr->callback_stack->len - 1);
  void * user_data = g_ptr_array_steal_index (
    flow_mgr->user_data_stack, flow_mgr->user_data_stack->len - 1);
  cb (success, error, user_data);
  g_free_and_null (flow_mgr->filename);
  object_zero_and_free (flow_mgr);
  g_debug ("done cleaning up project init flow manager");
}

void
project_init_flow_manager_load_or_create_default_project (
  const char *            filename,
  const bool              is_template,
  ProjectInitDoneCallback cb,
  void *                  user_data)
{
  g_return_if_fail (cb);
  g_message (
    "%s: [STEP 0] filename: %s, is template: %d", __func__, filename,
    is_template);

  ProjectInitFlowManager * flow_mgr = project_init_flow_manager_new ();
  project_init_flow_manager_append_callback (flow_mgr, cb, user_data);
  /* clean up right before chaining up */
  project_init_flow_manager_append_callback (
    flow_mgr, (ProjectInitDoneCallback) cleanup_flow_mgr_cb, flow_mgr);
  flow_mgr->is_template = is_template;
  flow_mgr->filename = g_strdup (filename);

  if (filename)
    {
      project_init_flow_manager_append_callback (
        flow_mgr, load_from_file_ready_cb, flow_mgr);
      load_from_file (filename, is_template, flow_mgr);
      return;
    }
  /* else if no filename given */
  else
    {
      GError *  err = NULL;
      Project * created_prj = create_default (
        PROJECT, ZRYTHM->create_project_path, false, true, &err);
      if (!created_prj)
        {
          GError * error = NULL;
          PROPAGATE_PREFIXED_ERROR_LITERAL (
            &error, err, "Failed to create default project");
          project_init_flow_manager_call_last_callback_fail (flow_mgr, error);
          return;
        }

      save_and_activate_after_successful_load_or_create (flow_mgr);
    }
}
