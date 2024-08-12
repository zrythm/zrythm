// SPDX-FileCopyrightText: © 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include <inttypes.h>
#include <sys/stat.h>

#include "dsp/router.h"
#include "dsp/tempo_track.h"
#include "dsp/tracklist.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/greeter.h"
#include "gui/widgets/main_notebook.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/timeline_panel.h"
#include "project.h"
#include "project/project_init_flow_manager.h"
#include "schemas/project.h"
#include "utils/datetime.h"
#include "utils/error.h"
#include "utils/flags.h"
#include "utils/io.h"
#include "utils/objects.h"
#include "utils/string.h"
#include "utils/ui.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include "libadwaita_wrapper.h"
#include <glibmm.h>
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

void
ProjectInitFlowManager::recreate_main_window ()
{
  g_message ("recreating main window...");
  MAIN_WINDOW = main_window_widget_new (zrythm_app.get ());
  g_warn_if_fail (
    MAIN_WINDOW->center_dock->main_notebook->timeline_panel->tracklist);
}

MainWindowWidget *
ProjectInitFlowManager::hide_prev_main_window ()
{
  EVENT_MANAGER->stop_events ();

  MainWindowWidget * mww = MAIN_WINDOW;
  MAIN_WINDOW = NULL;
  if (GTK_IS_WIDGET (mww))
    {
      g_message ("hiding previous main window...");
      gtk_widget_set_visible (GTK_WIDGET (mww), false);
    }

  return mww;
}

void
ProjectInitFlowManager::destroy_prev_main_window (MainWindowWidget * mww)
{
  if (GTK_IS_WIDGET (mww))
    {
      g_message ("destroying previous main window...");
      main_window_widget_tear_down (mww);
      /*g_object_unref (mww);*/
    }
}

void
ProjectInitFlowManager::setup_main_window (Project &project)
{
  /* mimic behavior when starting the app */
  if (ZRYTHM_HAVE_UI)
    {
      g_message ("setting up main window...");
      EVENT_MANAGER->start_events ();
      main_window_widget_setup (MAIN_WINDOW);

      EVENTS_PUSH (EventType::ET_PROJECT_LOADED, &project);
    }
}

#ifdef HAVE_CYAML
/**
 * Upgrades the given project YAML's schema if needed.
 *
 * @return True if the schema was upgraded.
 */
void
ProjectInitFlowManager::upgrade_schema (char ** yaml, int src_ver)
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
            g_error_free (err);
            throw ZrythmException (_ ("Failed to deserialize v1 project file"));
          }

        /* only dropping undo history, so just re-serialize
         * into YAML */
        g_free (*yaml);
        err = NULL;
        *yaml = yaml_serialize (self, &project_schema_v1, &err);
        if (!*yaml)
          {
            g_error_free (err);
            throw ZrythmException (_ ("Failed to serialize v1 project file"));
          }
        cyaml_config_t cyaml_config;
        yaml_get_cyaml_config (&cyaml_config);

        /* free memory allocated by libcyaml */
        cyaml_free (&cyaml_config, &project_schema_v1, self, 0);

        /* call again for next iteration (may throw) */
        upgrade_schema (yaml, 3);
        return;
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
            g_error_free (err);
            throw ZrythmException (
              _ ("Failed to deserialize v1/2/3 project file"));
          }

        /* create the new project and serialize it */
        Project_v5   _new_prj;
        Project_v5 * new_prj = &_new_prj;
        memset (new_prj, 0, sizeof (Project_v5));
        new_prj->schema_version = 4;
        new_prj->title = old_prj->title;
        new_prj->datetime_str =
          g_strdup (datetime_get_current_as_string ().c_str ());
        new_prj->version = Zrythm::get_version (false);

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
            g_error_free (err);
            throw ZrythmException (_ ("Failed to serialize v3 project file"));
          }
        cyaml_config_t cyaml_config;
        yaml_get_cyaml_config (&cyaml_config);

        /* free memory allocated by libcyaml */
        cyaml_free (&cyaml_config, &project_schema_v1, old_prj, 0);

        /* free memory allocated now */
        g_free_and_null (new_prj->datetime_str);
        g_free_and_null (new_prj->version);
        return;
      }
      break;
    case 4:
      /* v4 note:
       * AutomationSelections had schema_version instead of base as the first
       * member. This was an issue and was fixed, but projects before this
       * change have wrong values. We are lucky they are all primitive types.
       * This can be fixed by simply dropping the undo history and the
       * selections. */
      {
        /* deserialize into the current version of the struct */
        GError *     err = NULL;
        Project_v5 * old_prj =
          (Project_v5 *) yaml_deserialize (*yaml, &project_schema_v5, &err);
        if (!old_prj)
          {
            g_error_free (err);
            throw ZrythmException (_ ("Failed to deserialize v4 project file"));
          }

        /* create the new project and serialize it */
        Project_v5   _new_prj;
        Project_v5 * new_prj = &_new_prj;
        memset (new_prj, 0, sizeof (Project_v5));
        *new_prj = *old_prj;
        new_prj->schema_version = 5;
        new_prj->title = old_prj->title;
        new_prj->datetime_str =
          g_strdup (datetime_get_current_as_string ().c_str ());
        new_prj->version = Zrythm::get_version (false);

        /* re-serialize */
        g_free (*yaml);
        err = NULL;
        *yaml = yaml_serialize (new_prj, &project_schema_v5, &err);
        if (!*yaml)
          {
            g_error_free (err);
            throw ZrythmException (_ ("Failed to serialize v4 project file"));
          }
        cyaml_config_t cyaml_config;
        yaml_get_cyaml_config (&cyaml_config);

        /* free memory allocated by libcyaml */
        cyaml_free (&cyaml_config, &project_schema_v5, old_prj, 0);

        /* free memory allocated now */
        g_free_and_null (new_prj->datetime_str);
        g_free_and_null (new_prj->version);
        return;
      }
      break;
    default:
      return;
    }
}

void
ProjectInitFlowManager::upgrade_to_json (char ** txt)
{
  GError *     err = NULL;
  Project_v5 * old_prj =
    (Project_v5 *) yaml_deserialize (*txt, &project_schema_v5, &err);
  if (!old_prj)
    {
      g_error_free (err);
      throw ZrythmException (_ ("Failed to deserialize v5 project file"));
    }

  *txt = project_v5_serialize_to_json_str (old_prj, &err);
  if (*txt == nullptr)
    {
      g_error_free (err);
      throw ZrythmException (
        _ ("Failed to convert v5 YAML project file to JSON"));
    }
}
#endif

void
ProjectInitFlowManager::create_default (
  std::unique_ptr<Project> &prj,
  std::string_view          prj_dir,
  bool                      headless,
  bool                      with_engine)
{
  g_message ("creating default project...");

  bool have_ui = !headless && ZRYTHM_HAVE_UI;

  MainWindowWidget * mww = NULL;
  if (have_ui)
    {
      g_message ("hiding prev window...");
      mww = hide_prev_main_window ();
    }

  prj.reset (new Project (Glib::path_get_basename (prj_dir.data ())));

  prj->add_default_tracks ();

  /* pre-setup engine */
  auto engine = prj->audio_engine_.get ();
  if (with_engine)
    {
      engine->pre_setup ();
    }

  engine->setup ();

  if (with_engine)
    {
      prj->tracklist_->expose_ports_to_backend ();
    }

  auto beats_per_bar = prj->tracklist_->tempo_track_->get_beats_per_bar ();
  engine->update_frames_per_tick (
    beats_per_bar, P_TEMPO_TRACK->get_current_bpm (), engine->sample_rate_,
    true, true, false);

  /* set directory/title and create standard dirs */
  prj->dir_ = prj_dir;
  prj->title_ = Glib::path_get_basename (prj_dir.data ());
  prj->make_project_dirs (false);

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

  prj->loaded_ = true;

  prj->clip_editor_.init ();

  prj->quantize_opts_timeline_->update_quantize_points ();
  prj->quantize_opts_editor_->update_quantize_points ();

  if (have_ui)
    {
      g_message ("setting up main window...");
      g_warn_if_fail (
        MAIN_WINDOW->center_dock->main_notebook->timeline_panel->tracklist);
      setup_main_window (*prj);
    }

  if (with_engine)
    {
      /* recalculate the routing graph */
      ROUTER->recalc_graph (false);

      prj->audio_engine_->run_.store (true);
    }

  z_debug ("done creating default project");
}

void
ProjectInitFlowManager::save_and_activate_after_successful_load_or_create ()
{
  try
    {
      /* if creating a new project (either from a template or default project),
       * save the newly created project */
      if (is_template_ || filename_.empty ())
        {
          PROJECT->save (PROJECT->dir_, false, false, false);
        }
    }
  catch (const ZrythmException &e)
    {
      call_last_callback_fail (e.what ());
      return;
    }

  PROJECT->activate ();
  call_last_callback_success ();
}

void
ProjectInitFlowManager::replace_main_window (MainWindowWidget * mww)
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

      z_return_if_fail (GTK_IS_WINDOW (MAIN_WINDOW));
    }
}

void
ProjectInitFlowManager::continue_load_from_file_after_open_backup_response ()
{
  bool use_backup = !PROJECT->backup_dir_.empty ();
  PROJECT->loading_from_backup_ = use_backup;

  char * text = nullptr;
  try
    {
      text = PROJECT->get_existing_uncompressed_text (use_backup);
    }
  catch (const ZrythmException &e)
    {
      call_last_callback_fail (e.what ());
      return;
    }

  struct yyjson_read_err json_read_err;
  yyjson_doc *           doc = yyjson_read_opts (
    text, strlen (text), YYJSON_READ_NOFLAG, nullptr, &json_read_err);
  bool json_read_success = doc != NULL;
  object_free_w_func_and_null (yyjson_doc_free, doc);
  [[maybe_unused]] bool upgraded = false;
  int  yaml_schema_ver = -1;
  if (!json_read_success)
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
              call_last_callback_fail (_ ("Invalid project: missing version"));
              return;
            }
          g_message ("project from text (version %s)...", prj_ver_str);
          g_free (prj_ver_str);

          schema_ver = string_get_regex_group_as_int (
            text, "---\nschema_version: (.*)\n", 1, -1);
          g_message ("detected schema version %d", schema_ver);
          if (schema_ver != 5)
            {
              try
                {
                  /* upgrade project */
#ifdef HAVE_CYAML
                  upgrade_schema (&text, schema_ver);
                  upgraded = true;
#else
                  upgraded = false;
#endif
                }
              catch (const ZrythmException &e)
                {
                  free (text);
                  call_last_callback_fail (
                    _ ("Failed to upgrade YAML project schema"));
                  return;
                }
            }

          try
            {
              /* upgrade latest yaml to json */
#ifdef HAVE_CYAML
              upgrade_to_json (&text);
              upgraded = true;
#else
              upgraded = false;
#endif
            }
          catch (const ZrythmException &e)
            {
              free (text);
              call_last_callback_fail (
                _ ("Failed to upgrade project schema to JSON"));
              return;
            }
        }
      else
        {
          std::string err_str = format_str (
            _ ("Failed to read JSON: [code: %" PRIu32 ", pos: %zu] %s"),
            json_read_err.code, json_read_err.pos, json_read_err.msg);
          free (text);
          call_last_callback_fail (err_str);
          return;
        }
    } /* endif !json_read_success */

  std::unique_ptr<Project> deserialized_project = std::make_unique<Project> ();
  try
    {
      gint64 time_before = g_get_monotonic_time ();
      deserialized_project->deserialize_from_json_string (text);
      gint64 time_after = g_get_monotonic_time ();
      g_message (
        "time to deserialize: %ldms", (long) (time_after - time_before) / 1000);
    }
  catch (const ZrythmException &e)
    {
      free (text);
      call_last_callback_fail (_ ("Failed to deserialize project YAML"));
      return;
    }
  free (text);
  deserialized_project->backup_dir_ = PROJECT->backup_dir_;

  /* check for FINISHED file */
  if (yaml_schema_ver > 3)
    {
      std::string finished_file_path =
        PROJECT->get_path (ProjectPath::FINISHED_FILE, use_backup);
      bool finished_file_exists =
        Glib::file_test (finished_file_path, Glib::FileTest::EXISTS);
      if (!finished_file_exists)
        {
          call_last_callback_fail (format_str (
            _ ("Could not load project: Corrupted project detected (missing FINISHED file at '%s')."),
            finished_file_path));
          return;
        }
    }

  g_message ("Project successfully deserialized.");

  /* if template, also copy the pool and plugin states */
  if (is_template_)
    {
      auto prev_pool_dir = Glib::build_filename (dir_, PROJECT_POOL_DIR);
      auto new_pool_dir =
        Glib::build_filename (gZrythm->create_project_path_, PROJECT_POOL_DIR);
      auto prev_plugins_dir = Glib::build_filename (dir_, PROJECT_PLUGINS_DIR);
      auto new_plugins_dir = Glib::build_filename (
        gZrythm->create_project_path_, PROJECT_PLUGINS_DIR);
      try
        {
          io_copy_dir (
            new_pool_dir, prev_pool_dir, F_NO_FOLLOW_SYMLINKS, F_RECURSIVE);
          io_copy_dir (
            new_plugins_dir, prev_plugins_dir, F_NO_FOLLOW_SYMLINKS,
            F_RECURSIVE);
        }
      catch (const ZrythmException &e)
        {
          call_last_callback_fail (e.what ());
          return;
        }
      dir_ = gZrythm->create_project_path_;
    }

  /* FIXME this is a hack, make sure none of this extra copying is needed */
  /* if backup, copy plugin states */
  if (use_backup)
    {
      auto prev_plugins_dir =
        Glib::build_filename (deserialized_project->dir_, PROJECT_PLUGINS_DIR);
      auto new_plugins_dir = Glib::build_filename (dir_, PROJECT_PLUGINS_DIR);
      try
        {
          io_copy_dir (
            new_plugins_dir, prev_plugins_dir, F_NO_FOLLOW_SYMLINKS,
            F_RECURSIVE);
        }
      catch (const ZrythmException &e)
        {
          call_last_callback_fail (e.what ());
          return;
        }
    }

  MainWindowWidget * mww = NULL;
  if (ZRYTHM_HAVE_UI)
    {
      g_message ("hiding prev window...");
      mww = hide_prev_main_window ();
    }

  PROJECT.reset (deserialized_project.release ());
  auto prj = PROJECT.get ();

  /* re-update paths for the newly loaded project */
  prj->dir_ = dir_;

  /* set the tempo track */
  for (auto track : prj->tracklist_->tracks_ | type_is<TempoTrack> ())
    {
      prj->tracklist_->tempo_track_ = track;
      break;
    }

  std::string filepath_noext = Glib::path_get_basename (dir_);

  prj->title_ = filepath_noext;

  prj->init_selections ();

  auto engine = prj->audio_engine_.get ();

  auto handle_err = [] (const ZrythmException &e) {
    GError *    err = g_error_new (0, 0, "%s", e.what ());
    AdwDialog * err_win = error_handle_prv (
      err, "%s", _ ("Failed to initialize the audio file pool"));
    g_signal_connect (
      err_win, "closed", G_CALLBACK (zrythm_exit_response_callback), nullptr);
  };

  try
    {
      prj->audio_engine_->init_loaded (prj);
    }
  catch (const ZrythmException &e)
    {
      handle_err (e);
      return;
    }
  engine->pre_setup ();

  /* re-load clips because sample rate can change during engine pre setup */
  try
    {
      engine->pool_->init_loaded ();
    }
  catch (const ZrythmException &e)
    {
      handle_err (e);
      return;
    }

  prj->clip_editor_.init_loaded ();

  auto tracklist = prj->tracklist_.get ();
  tracklist->init_loaded (*prj);

  int beats_per_bar = tracklist->tempo_track_->get_beats_per_bar ();
  engine->update_frames_per_tick (
    beats_per_bar, tracklist->tempo_track_->get_current_bpm (),
    engine->sample_rate_, true, true, false);

  /* undo manager must be loaded after updating engine frames per tick */
  if (prj->undo_manager_)
    {
      prj->undo_manager_->init_loaded ();
    }
  else
    {
      prj->undo_manager_ = std::make_unique<UndoManager> ();
    }

  prj->midi_mappings_->init_loaded ();

  /* note: when converting from older projects there may be no selections
   * (because it's too much work with little benefit to port the selections from
   * older projects) */

  auto init_or_create_arr_selections = [] (auto &selections) {
    using T =
      typename std::remove_reference_t<decltype (selections)>::element_type;
    if (selections)
      {
        selections->init_loaded (true, nullptr);
      }
    else
      {
        selections = std::make_unique<T> ();
      }
  };

  init_or_create_arr_selections (prj->audio_selections_);
  init_or_create_arr_selections (prj->chord_selections_);
  init_or_create_arr_selections (prj->automation_selections_);
  init_or_create_arr_selections (prj->timeline_selections_);
  init_or_create_arr_selections (prj->midi_selections_);

  prj->tracklist_selections_->init_loaded (*tracklist);

  prj->quantize_opts_timeline_->update_quantize_points ();
  prj->quantize_opts_editor_->update_quantize_points ();

  replace_main_window (mww);

  /* sanity check */
  z_warn_if_fail (prj->validate ());

  engine->setup ();

  /* init ports */
  std::vector<Port *> ports;
  prj->get_all_ports (ports);
  z_debug ("Initializing loaded Ports...");
  for (auto port : ports)
    {
      if (port->is_exposed_to_backend ())
        {
          port->set_expose_to_backend (true);
        }
    }

  prj->loaded_ = true;
  prj->loading_from_backup_ = false;

  z_debug ("project loaded");

  /* recalculate the routing graph */
  engine->router_->recalc_graph (false);

  z_debug ("setting up main window...");
  setup_main_window (*prj);

  engine->run_.store (true);

  if (
    prj->format_minor_ != prj->get_format_minor_version ()
    || yaml_schema_ver > 0)
    {
      ui_show_message_printf (
        _ ("Project Upgraded"),
        _ ("This project has been automatically upgraded "
           "to v1.%d. Saving this project will overwrite the "
           "old one. If you would like to keep both, please "
           "use 'Save As...'."),
        prj->get_format_minor_version ());
    }

  call_last_callback_success ();
}

static void
on_open_backup_response (
  AdwMessageDialog *       dialog,
  char *                   response,
  ProjectInitFlowManager * flow_mgr)
{
  if (string_is_equal (response, "ignore"))
    {
      PROJECT->backup_dir_.clear ();
    }
  if (MAIN_WINDOW)
    {
      gtk_widget_set_visible (GTK_WIDGET (MAIN_WINDOW), true);
    }
  flow_mgr->continue_load_from_file_after_open_backup_response ();
}

void
ProjectInitFlowManager::load_from_file ()
{
  z_return_if_fail (!filename_.empty ());
  dir_ = io_get_dir (filename_);

  if (!PROJECT)
    {
      /* create a temporary project struct */
      PROJECT = std::make_unique<Project> ();
    }

  PROJECT->dir_ = dir_;

  /* if loading an actual project, check for backups */
  if (!is_template_)
    {
      PROJECT->backup_dir_.clear ();
      PROJECT->backup_dir_ = PROJECT->get_newer_backup ();
      ;
      if (!PROJECT->backup_dir_.empty ())
        {
          z_debug ("newer backup found %s", PROJECT->backup_dir_);

          if (ZRYTHM_TESTING)
            {
              if (!gZrythm->open_newer_backup_)
                {
                  PROJECT->backup_dir_.clear ();
                }
            }
          else
            {
              AdwMessageDialog * dialog = ADW_MESSAGE_DIALOG (
                adw_message_dialog_new (nullptr, _ ("Open Backup?"), nullptr));
              adw_message_dialog_format_body_markup (
                dialog, _ ("Newer backup found:\n  %s.\nUse the newer backup?"),
                PROJECT->backup_dir_.c_str ());
              adw_message_dialog_add_responses (
                dialog, "open-backup", _ ("Open Backup"), "ignore",
                _ ("Ignore"), nullptr);
              gtk_window_present (GTK_WINDOW (dialog));
              if (MAIN_WINDOW)
                {
                  gtk_widget_set_visible (GTK_WIDGET (MAIN_WINDOW), false);
                }
              g_signal_connect (
                dialog, "response", G_CALLBACK (on_open_backup_response), this);
              return;
            }
        }
    } /* endif !is_template */

  continue_load_from_file_after_open_backup_response ();
}

void
ProjectInitFlowManager::
  load_from_file_ready_cb (bool success, std::string error, void * user_data)
{
  ProjectInitFlowManager * flow_mgr = (ProjectInitFlowManager *) user_data;
  if (!success)
    {
      GreeterWidget * greeter =
        greeter_widget_new (zrythm_app.get (), nullptr, false, false);
      gtk_window_present (GTK_WINDOW (greeter));

      flow_mgr->call_last_callback_fail (error);
      return;
    } /* endif failed to load project */

  flow_mgr->save_and_activate_after_successful_load_or_create ();
}

ProjectInitFlowManager::ProjectInitFlowManager (
  std::string_view        filename,
  const bool              is_template,
  ProjectInitDoneCallback cb,
  void *                  user_data)
    : filename_ (filename), is_template_ (is_template)
{
  z_return_if_fail (cb);
  z_debug (
    "%s: [STEP 0] filename: %s, is template: %d", __func__, filename,
    is_template);

  append_callback (cb, user_data);

  if (!filename.empty ())
    {
      append_callback (load_from_file_ready_cb, this);
      load_from_file ();
      return;
    }
  /* else if no filename given */
  else
    {
      try
        {
          create_default (PROJECT, gZrythm->create_project_path_, false, true);
        }
      catch (const ZrythmException &e)
        {
          call_last_callback_fail ("Failed to create default project");
          return;
        }

      save_and_activate_after_successful_load_or_create ();
    }
}
