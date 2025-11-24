// SPDX-FileCopyrightText: © 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include <ctime>

#include <inttypes.h>
#include <sys/stat.h>

#include "engine/session/graph_dispatcher.h"
#include "gui/backend/backend/cyaml_schemas/project.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/project/project_init_flow_manager.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/backend/project_manager.h"
#include "gui/backend/ui.h"
#include "structure/tracks/tempo_track.h"
#include "structure/tracks/tracklist.h"
#include "utils/datetime.h"
#include "utils/gtest_wrapper.h"
#include "utils/io.h"
#include "utils/utf8_string.h"

#include <yyjson.h>

using namespace zrythm;

ProjectInitFlowManager::~ProjectInitFlowManager ()
{
  if (open_backup_response_cb_id_)
    {
      // TODO
      // g_source_remove (open_backup_response_cb_id_);
      open_backup_response_cb_id_ = 0;
    }
}

void
ProjectInitFlowManager::append_callback (
  ProjectInitDoneCallback cb,
  void *                  user_data)
{
  callbacks_.emplace_back (cb, user_data);
}

void
ProjectInitFlowManager::call_last_callback (bool success, std::string error)
{
  auto cb = callbacks_.back ();
  callbacks_.pop_back ();
  cb.first (success, error, cb.second);
}

void
ProjectInitFlowManager::call_last_callback_fail (std::string error)
{
  call_last_callback (false, error);
}
void
ProjectInitFlowManager::call_last_callback_success ()
{
  call_last_callback (true, "");
}

#if 0
void
ProjectInitFlowManager::recreate_main_window ()
{
  z_info ("recreating main window...");
  MAIN_WINDOW = main_window_widget_new (zrythm_app.get ());
  z_warn_if_fail (
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
      z_info ("hiding previous main window...");
      gtk_widget_set_visible (GTK_WIDGET (mww), false);
    }

  return mww;
}

void
ProjectInitFlowManager::destroy_prev_main_window (MainWindowWidget * mww)
{
  if (GTK_IS_WIDGET (mww))
    {
      z_info ("destroying previous main window...");
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
      z_info ("setting up main window...");
      EVENT_MANAGER->start_events ();
      main_window_widget_setup (MAIN_WINDOW);

      /* EVENTS_PUSH (EventType::ET_PROJECT_LOADED, &project); */
    }
}
#endif

void
ProjectInitFlowManager::create_default (
  std::unique_ptr<Project> &prj,
  std::string              &prj_dir,
  bool                      headless,
  bool                      with_engine)
{
  z_info ("creating default project...");

  bool have_ui = !headless && ZRYTHM_HAVE_UI;

#if 0
  MainWindowWidget * mww = NULL;
  if (have_ui)
    {
      z_info ("hiding prev window...");
      mww = hide_prev_main_window ();
    }
#endif

  auto prj_dir_basename = utils::io::path_get_basename (prj_dir);
  prj = std::make_unique<Project> (prj_dir_basename);

  prj->add_default_tracks ();

  /* pre-setup engine */
  auto engine = prj->audio_engine_.get ();
  z_return_if_fail (engine);
  if (with_engine)
    {
      engine->pre_setup_open_devices ();
    }

  engine->setup ();

  auto beats_per_bar = prj->tracklist_->tempo_track_->get_beats_per_bar ();
  engine->update_frames_per_tick (
    beats_per_bar, prj->tracklist_->tempo_track_->get_current_bpm (),
    engine->sample_rate_, true, true, false);

  /* set directory/title and create standard dirs */
  prj->dir_ = prj_dir;
  prj->title_ =
    utils::Utf8String::from_path (utils::io::path_get_basename (prj_dir));
  prj->make_project_dirs (false);

  if (have_ui)
    {
      z_info ("recreating main window...");
      recreate_main_window ();

#if 0
      if (mww)
        {
          z_info ("destroying prev window...");
          destroy_prev_main_window (mww);
        }
#endif
    }

  prj->loaded_ = true;

  prj->clip_editor_->init ();

  prj->quantize_opts_timeline_->update_quantize_points (*prj->transport_);
  prj->quantize_opts_editor_->update_quantize_points (*prj->transport_);

  if (have_ui)
    {
      z_info ("setting up main window...");
      // z_warn_if_fail (
      // MAIN_WINDOW->center_dock->main_notebook->timeline_panel->tracklist);
      // setup_main_window (*prj);
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

#if 0
void
ProjectInitFlowManager::replace_main_window (MainWindowWidget * mww)
{
  if (ZRYTHM_HAVE_UI)
    {
      z_info ("recreating main window...");
      recreate_main_window ();

      if (mww)
        {
          z_info ("destroying prev window...");
          destroy_prev_main_window (mww);
        }

      z_return_if_fail (GTK_IS_WINDOW (MAIN_WINDOW));
    }
}
#endif

void
ProjectInitFlowManager::continue_load_from_file_after_open_backup_response ()
{
  bool use_backup = !PROJECT->backup_dir_.empty ();
  PROJECT->loading_from_backup_ = use_backup;

  std::string text;
  try
    {
      text = PROJECT->get_existing_uncompressed_text (use_backup);
    }
  catch (const ZrythmException &e)
    {
      call_last_callback_fail (e.what ());
      return;
    }

  struct yyjson_read_err json_read_err = {};
  yyjson_doc *           doc = yyjson_read_opts (
    // NOLINTNEXTLINE
    const_cast<char *> (text.c_str ()), text.length (), YYJSON_READ_NOFLAG,
    nullptr, &json_read_err);
  bool json_read_success = doc != nullptr;
  object_free_w_func_and_null (yyjson_doc_free, doc);
  [[maybe_unused]] bool upgraded = false;
  int                   yaml_schema_ver = -1;
  if (!json_read_success)
    {
      /* failed to read JSON - check if YAML */
      int schema_ver = utils::string::get_regex_group_as_int (
        text, "---\nschema_version: (.*)\n", 1, -1);
      if (schema_ver > 0)
        {
          yaml_schema_ver = schema_ver;
          /* upgrade YAML and set doc */
          auto prj_ver_str =
            utils::string::get_regex_group (text, "\nversion: (.*)\n", 1);
          if (prj_ver_str.empty ())
            {
              call_last_callback_fail (
                utils::to_std_string (
                  QObject::tr ("Invalid project: missing version")));
              return;
            }
          z_info ("project from text (version {})...", prj_ver_str);

          schema_ver = utils::string::get_regex_group_as_int (
            text, "---\nschema_version: (.*)\n", 1, -1);
          z_info ("detected schema version {}", schema_ver);
          if (schema_ver != 5)
            {
              try
                {
                  /* upgrade project */
                  upgraded = false;
                }
              catch (const ZrythmException &e)
                {
                  call_last_callback_fail (
                    utils::to_std_string (
                      QObject::tr ("Failed to upgrade YAML project schema")));
                  return;
                }
            }

          try
            {
              /* upgrade latest yaml to json */
              upgraded = false;
            }
          catch (const ZrythmException &e)
            {
              call_last_callback_fail (
                utils::to_std_string (
                  QObject::tr ("Failed to upgrade project schema to JSON")));
              return;
            }
        }
      else
        {
          std::string err_str = format_str (
            "Failed to read JSON: [code: {}, pos: {}] {}", json_read_err.code,
            json_read_err.pos, json_read_err.msg);
          call_last_callback_fail (err_str);
          return;
        }
    } /* endif !json_read_success */

  std::unique_ptr<Project> deserialized_project = std::make_unique<Project> ();
  try
    {
      auto time_before = Zrythm::getInstance ()->get_monotonic_time_usecs ();
      deserialized_project->deserialize_from_json_string (text.c_str ());
      auto time_after = Zrythm::getInstance ()->get_monotonic_time_usecs ();
      z_info (
        "time to deserialize: {}ms", (long) (time_after - time_before) / 1000);
    }
  catch (const ZrythmException &e)
    {
      call_last_callback_fail (
        utils::to_std_string (
          QObject::tr ("Failed to deserialize project YAML")));
      return;
    }
  deserialized_project->backup_dir_ = PROJECT->backup_dir_;

  /* check for FINISHED file */
  if (yaml_schema_ver > 3)
    {
      auto finished_file_path =
        PROJECT->get_path (ProjectPath::FINISHED_FILE, use_backup);
      bool finished_file_exists = fs::exists (finished_file_path);
      if (!finished_file_exists)
        {
          call_last_callback_fail (format_str (
            utils::qstring_to_std_string (
              QObject::tr (
                "Could not load project: Corrupted project detected (missing FINISHED file at '{}').")),
            finished_file_path));
          return;
        }
    }

  z_info ("Project successfully deserialized.");

  /* if template, also copy the pool and plugin states */
  if (is_template_)
    {
      auto prev_pool_dir = fs::path (dir_) / PROJECT_POOL_DIR;
      auto new_pool_dir =
        fs::path (gZrythm->create_project_path_) / PROJECT_POOL_DIR;
      auto prev_plugins_dir = fs::path (dir_) / PROJECT_PLUGINS_DIR;
      auto new_plugins_dir =
        fs::path (gZrythm->create_project_path_) / PROJECT_PLUGINS_DIR;
      try
        {
          utils::io::copy_dir (new_pool_dir, prev_pool_dir, false, true);
          utils::io::copy_dir (new_plugins_dir, prev_plugins_dir, false, true);
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
      const auto &prev_prj_backup_dir = deserialized_project->backup_dir_;
      z_return_if_fail (!prev_prj_backup_dir.empty ());
      auto prev_plugins_dir = prev_prj_backup_dir / PROJECT_PLUGINS_DIR;
      auto new_plugins_dir = fs::path (dir_) / PROJECT_PLUGINS_DIR;
      try
        {
          utils::io::copy_dir (new_plugins_dir, prev_plugins_dir, false, true);
        }
      catch (const ZrythmException &e)
        {
          call_last_callback_fail (e.what ());
          return;
        }
    }

#if 0
  MainWindowWidget * mww = NULL;
  if (ZRYTHM_HAVE_UI)
    {
      z_info ("hiding prev window...");
      mww = hide_prev_main_window ();
    }
#endif

  zrythm::gui::ProjectManager::get_instance ()->setActiveProject (
    deserialized_project.release ());
  auto * prj = zrythm::gui::ProjectManager::get_instance ()->getActiveProject ();

  /* re-update paths for the newly loaded project */
  prj->dir_ = dir_;

  /* set the tempo track */
  auto get_tempo_track = [] (auto &prj) -> TempoTrack * {
    return (prj->tracklist_->get_track_span ()
            | std::views::filter (TrackSpan::type_projection<TempoTrack>)
            | std::views::transform (TrackSpan::type_transformation<TempoTrack>)
            | std::views::take (1))
      .front ();
  };
  prj->tracklist_->tempo_track_ = get_tempo_track (prj);

  std::string filepath_noext = utils::io::path_get_basename (dir_);

  prj->title_ = filepath_noext;

  auto engine = prj->audio_engine_.get ();

  auto handle_err = [] (const ZrythmException &e) {
#if 0
    GError *    err = g_error_new (0, 0, "%s", e.what ());
    AdwDialog * err_win = error_handle_prv (
      err, "%s", QObject::tr ("Failed to initialize the audio file pool"));
    g_signal_connect (
      err_win, "closed", G_CALLBACK (ZrythmApp::exit_response_callback),
      nullptr);
#endif
    z_warning ("error: {}", e.what ());
  };

  try
    {
      auto * tempo_track = prj->tracklist_->tempo_track_;
      if (!tempo_track)
        {
          tempo_track = get_tempo_track (prj);
        }
      if (!tempo_track)
        {
          throw ZrythmException ("Tempo track not found");
        }

      prj->transport_->init_loaded (prj, tempo_track);
      prj->audio_engine_->init_loaded (prj);
    }
  catch (const ZrythmException &e)
    {
      handle_err (e);
      return;
    }
  engine->pre_setup_open_devices ();

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

  prj->clip_editor_->init_loaded ();

  auto * tracklist = prj->tracklist_;
  tracklist->init_loaded (prj->get_port_registry (), *prj);

  int beats_per_bar = tracklist->tempo_track_->get_beats_per_bar ();
  engine->update_frames_per_tick (
    beats_per_bar, tracklist->tempo_track_->get_current_bpm (),
    engine->sample_rate_, true, true, false);

  /* undo manager must be loaded after updating engine frames per tick */
  if (prj->undo_manager_)
    {
      prj->undo_manager_->init_loaded (engine->sample_rate_);
    }
  else
    {
      prj->undo_manager_ = new gui::actions::UndoManager (prj);
    }

  prj->midi_mappings_->init_loaded ();

  /* note: when converting from older projects there may be no selections
   * (because it's too much work with little benefit to port the selections from
   * older projects) */

  prj->quantize_opts_timeline_->update_quantize_points (*prj->transport_);
  prj->quantize_opts_editor_->update_quantize_points (*prj->transport_);

  // replace_main_window (mww);

  engine->setup ();

  /* init ports */
  std::vector<dsp::Port *> ports;
  prj->get_all_ports (ports);
  z_debug ("Initializing loaded Ports...");
  for (auto port : ports)
    {
      if (port->is_exposed_to_backend ())
        {
          AUDIO_ENGINE->set_port_exposed_to_backend (port, true);
        }
    }

  prj->loaded_ = true;
  prj->loading_from_backup_ = false;

  z_debug ("project loaded");

  /* recalculate the routing graph */
  engine->graph_dispatcher_->recalc_graph (false);

#if 0
  z_debug ("setting up main window...");
  setup_main_window (*prj);
#endif

  engine->run_.store (true);

  if (
    prj->format_minor_ != prj->get_format_minor_version ()
    || yaml_schema_ver > 0)
    {
// TODO
#if 0
      ui_show_message_printf (
        QObject::tr ("Project Upgraded"),
        QObject::tr (
          "This project has been automatically upgraded "
          "to v{}.{}. Saving this project will overwrite the "
          "old one. If you would like to keep both, please "
          "use 'Save As...'."),
        prj->get_format_major_version (), prj->get_format_minor_version ());
#endif
    }

  call_last_callback_success ();
}

#if 0
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
  flow_mgr->open_backup_response_cb_id_ = 0;
}
#endif

void
ProjectInitFlowManager::load_from_file ()
{
  z_return_if_fail (!filename_.empty ());
  dir_ = utils::io::get_dir (filename_);

  if (!PROJECT)
    {
      /* create a temporary project struct */
      zrythm::gui::ProjectManager::get_instance ()->setActiveProject (
        new Project ());
    }

  PROJECT->dir_ = dir_;

  /* if loading an actual project, check for backups */
  if (!is_template_)
    {
      PROJECT->backup_dir_.clear ();
      PROJECT->backup_dir_ = PROJECT->get_newer_backup ();
      if (PROJECT->backup_dir_)
        {
          z_debug ("newer backup found {}", PROJECT->backup_dir_->string ());

          if (ZRYTHM_TESTING || ZRYTHM_BENCHMARKING)
            {
              if (!gZrythm->open_newer_backup_)
                {
                  PROJECT->backup_dir_->clear ();
                }
            }
          else
            {
#if 0
              AdwMessageDialog * dialog = ADW_MESSAGE_DIALOG (
                adw_message_dialog_new (nullptr, QObject::tr ("Open Backup?"), nullptr));
              adw_message_dialog_format_body_markup (
                dialog, QObject::tr ("Newer backup found:\n  %s.\nUse the newer backup?"),
                PROJECT->backup_dir_.c_str ());
              adw_message_dialog_add_responses (
                dialog, "open-backup", QObject::tr ("Open Backup"), "ignore",
                QObject::tr ("Ignore"), nullptr);
              gtk_window_present (GTK_WINDOW (dialog));
              if (MAIN_WINDOW)
                {
                  gtk_widget_set_visible (GTK_WIDGET (MAIN_WINDOW), false);
                }
              open_backup_response_cb_id_ = g_signal_connect (
                dialog, "response", G_CALLBACK (on_open_backup_response), this);
#endif
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
      if (ZRYTHM_HAVE_UI)
        {
#if 0
          GreeterWidget * greeter =
            greeter_widget_new (zrythm_app.get (), nullptr, false, false);
          gtk_window_present (GTK_WINDOW (greeter));
#endif
        }

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
  z_debug ("[STEP 0] filename: {}, is template: {}", filename, is_template);

  append_callback (cb, user_data);

  if (!filename.empty ())
    {
      append_callback (load_from_file_ready_cb, this);
      load_from_file ();
      return;
    }

  /* else if no filename given */
  try
    {
      z_return_if_fail (gZrythm);
      // FIXME/TODO - it expects a reference to the active project here
      auto prj = std::make_unique<Project> ();
      create_default (prj, gZrythm->create_project_path_, false, true);
    }
  catch (const ZrythmException &e)
    {
      call_last_callback_fail ("Failed to create default project");
      return;
    }

  save_and_activate_after_successful_load_or_create ();
}
