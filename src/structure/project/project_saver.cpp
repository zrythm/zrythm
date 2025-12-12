// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/project/project.h"
#include "structure/project/project_path_provider.h"
#include "structure/project/project_saver.h"
#include "utils/datetime.h"
#include "utils/io_utils.h"
#include "utils/views.h"

#include <zstd.h>

namespace zrythm::structure::project
{

inline auto
plugin_state_dir_projection (const plugins::PluginPtrVariant &pl_var)
{
  return std::visit (
    [] (auto &&pl) { return pl->get_state_directory (); }, pl_var);
}

ProjectSaver::ProjectSaver (const Project &project) : project_ (project) { }

void
ProjectSaver::make_project_dirs (const Project &project, bool is_backup)
{
  for (
    auto type :
    { gui::ProjectPathProvider::ProjectPath::BackupsDir,
      gui::ProjectPathProvider::ProjectPath::ExportsDir,
      gui::ProjectPathProvider::ProjectPath::ExportStemsDir,
      gui::ProjectPathProvider::ProjectPath::AudioFilePoolDir,
      gui::ProjectPathProvider::ProjectPath::PluginStates,
      gui::ProjectPathProvider::ProjectPath::PLUGIN_EXT_COPIES,
      gui::ProjectPathProvider::ProjectPath::PLUGIN_EXT_LINKS })
    {
      const auto dir =
        project.get_directory (is_backup)
        / gui::ProjectPathProvider::get_path (type);
      assert (!dir.empty ());
      try
        {
          utils::io::mkdir (dir);
        }
      catch (ZrythmException &e)
        {
          throw ZrythmException (
            fmt::format ("Failed to create directory {}", dir));
        }
    }
}

void
ProjectSaver::compress_or_decompress (
  bool              compress,
  char **           _dest,
  size_t *          _dest_size,
  CompressionFlag   dest_type,
  const QByteArray &src)
{
  z_info (
    "using zstd v{}.{}.{}", ZSTD_VERSION_MAJOR, ZSTD_VERSION_MINOR,
    ZSTD_VERSION_RELEASE);

  char * dest = nullptr;
  size_t dest_size = 0;
  if (compress)
    {
      z_info ("compressing project...");
      size_t compress_bound = ZSTD_compressBound (src.size ());
      dest = (char *) malloc (compress_bound);
      dest_size =
        ZSTD_compress (dest, compress_bound, src.constData (), src.size (), 1);
      if (ZSTD_isError (dest_size))
        {
          free (dest);
          throw ZrythmException (format_str (
            "Failed to compress project file: {}",
            ZSTD_getErrorName (dest_size)));
        }
    }
  else /* decompress */
    {
#if (ZSTD_VERSION_MAJOR == 1 && ZSTD_VERSION_MINOR < 3)
      unsigned long long const frame_content_size =
        ZSTD_getDecompressedSize (src.constData (), src.size ());
      if (frame_content_size == 0)
#else
      unsigned long long const frame_content_size =
        ZSTD_getFrameContentSize (src.constData (), src.size ());
      if (frame_content_size == ZSTD_CONTENTSIZE_ERROR)
#endif
        {
          throw ZrythmException ("Project not compressed by zstd");
        }
      dest = (char *) malloc ((size_t) frame_content_size);
      dest_size = ZSTD_decompress (
        dest, frame_content_size, src.constData (), src.size ());
      if (ZSTD_isError (dest_size))
        {
          free (dest);
          throw ZrythmException (format_str (
            "Failed to decompress project file: {}",
            ZSTD_getErrorName (dest_size)));
        }
      if (dest_size != frame_content_size)
        {
          free (dest);
          /* impossible because zstd will check this condition */
          throw ZrythmException ("uncompressed_size != frame_content_size");
        }
    }

  z_debug (
    "{} : {} bytes -> {} bytes", compress ? "Compression" : "Decompression",
    src.size (), dest_size);

  switch (dest_type)
    {
    case CompressionFlag::PROJECT_COMPRESS_DATA:
      *_dest = dest;
      *_dest_size = dest_size;
      break;
    case CompressionFlag::PROJECT_COMPRESS_FILE:
      {
        // setting the resulting data to the file at path `_dest`
        try
          {
            utils::io::set_file_contents (fs::path (*_dest), dest, dest_size);
          }
        catch (const ZrythmException &e)
          {
            throw ZrythmException (
              fmt::format ("Failed to write project file: {}", e.what ()));
          }
      }
      break;
    }
}

void
ProjectSaver::set_and_create_next_available_backup_dir ()
{
// TODO
#if 0
  auto backups_dir = get_path (ProjectPath::BACKUPS, false);

  int i = 0;
  do
    {
      if (i > 0)
        {
          std::string bak_title = fmt::format ("{}.bak{}", title_, i);
          backup_dir_ = backups_dir / bak_title;
        }
      else
        {
          std::string bak_title = fmt::format ("{}.bak", title_);
          backup_dir_ = backups_dir / bak_title;
        }
      i++;
    }
  while (
    backup_dir_.has_value () && utils::io::path_exists (backup_dir_.value ()));

  try
    {
      utils::io::mkdir (*backup_dir_);
    }
  catch (ZrythmException &e)
    {
      throw ZrythmException (format_qstr (
        QObject::tr ("Failed to create backup directory {}"), backup_dir_));
    }
#endif
}

std::string
ProjectSaver::get_existing_uncompressed_text (bool backup)
{
  /* get file contents */
  const auto project_file_path =
    project_.get_directory (backup)
    / gui::ProjectPathProvider::get_path (
      gui::ProjectPathProvider::ProjectPath::ProjectFile);
  z_debug ("getting text for project file {}", project_file_path);

  QByteArray compressed_pj{};
  try
    {
      compressed_pj = utils::io::read_file_contents (project_file_path);
    }
  catch (const ZrythmException &e)
    {
      throw ZrythmException (format_qstr (
        QObject::tr ("Unable to read file at {}: {}"), project_file_path,
        e.what ()));
    }

  /* decompress */
  z_info ("decompressing project...");
  char * text = nullptr;
  size_t text_size = 0;
  try
    {
      decompress (
        &text, &text_size, CompressionFlag::PROJECT_DECOMPRESS_DATA,
        compressed_pj);
    }
  catch (ZrythmException &e)
    {
      throw ZrythmException (format_qstr (
        QObject::tr ("Unable to decompress project file at {}"),
        project_file_path));
    }

  /* make string null-terminated */
  text = (char *) realloc (text, text_size + sizeof (char));
  text[text_size] = '\0';
  std::string ret (text);
  free (text);
  return ret;
}

int
ProjectSaver::autosave_cb (void * data)
{
#if 0
  if (
    !PROJECT || !PROJECT->loaded_ || PROJECT->dir_.empty ()
    || PROJECT->datetime_str_.empty () || !MAIN_WINDOW || !MAIN_WINDOW->setup)
    return G_SOURCE_CONTINUE;

  unsigned int autosave_interval_mins =
    g_settings_get_uint (S_P_PROJECTS_GENERAL, "autosave-interval");

  /* return if autosave disabled */
  if (autosave_interval_mins <= 0)
    return G_SOURCE_CONTINUE;

  auto &out_ports =
    PROJECT->tracklist_->master_track_->channel ()->stereo_out_;
  auto cur_time = SteadyClock::now ();
  /* subtract 4 seconds because the time this gets called is not exact (this is
   * an old comment and I don't remember why this is done) */
  auto autosave_interval =
    std::chrono::minutes (autosave_interval_mins) - std::chrono::seconds (4);

  /* skip if semaphore busy */
  SemaphoreRAII sem (PROJECT->save_sem_);
  if (!sem.is_acquired ())
    {
      z_debug ("can't acquire project lock - skipping autosave");
      return G_SOURCE_CONTINUE;
    }

  auto * last_action = PROJECT->undo_manager_->get_last_action ();

  /* skip if bad time to save or rolling */
  if (cur_time - PROJECT->last_successful_autosave_time_ < autosave_interval || TRANSPORT->is_rolling() || (TRANSPORT->play_state_ == Transport::PlayState::RollRequested && (TRANSPORT->preroll_frames_remaining_ > 0 || TRANSPORT->countin_frames_remaining_ > 0)))
    {
      return G_SOURCE_CONTINUE;
    }

  /* skip if sound is playing */
  if (
    out_ports->get_l ().get_peak () >= 0.0001f
    || out_ports->get_r ().get_peak () >= 0.0001f)
    {
      z_debug ("sound is playing, skipping autosave");
      return G_SOURCE_CONTINUE;
    }

  /* skip if currently performing action */
  if (arranger_widget_any_doing_action ())
    {
      z_debug ("in the middle of an action, skipping autosave");
      return G_SOURCE_CONTINUE;
    }

  if (PROJECT->last_action_in_last_successful_autosave_ == last_action)
    {
      z_debug ("last action is same as previous backup - skipping autosave");
      return G_SOURCE_CONTINUE;
    }

  /* skip if any modal window is open */
  if (ZRYTHM_HAVE_UI)
    {
      auto toplevels = gtk_window_get_toplevels ();
      auto num_toplevels = g_list_model_get_n_items (toplevels);
      for (guint i = 0; i < num_toplevels; i++)
        {
          auto window = GTK_WINDOW (g_list_model_get_item (toplevels, i));
          if (
            gtk_widget_get_visible (GTK_WIDGET (window))
            && (gtk_window_get_modal (window) || gtk_window_get_transient_for (window) == GTK_WINDOW (MAIN_WINDOW)))
            {
              z_debug ("modal/transient windows exist - skipping autosave");
              z_gtk_widget_print_hierarchy (GTK_WIDGET (window));
              return G_SOURCE_CONTINUE;
            }
        }
    }

  /* ok to save */
  try
    {
      PROJECT->save (PROJECT->dir_, true, true, true);
      PROJECT->last_successful_autosave_time_ = cur_time;
    }
  catch (const ZrythmException &e)
    {
      if (ZRYTHM_HAVE_UI)
        {
          e.handle (QObject::tr ("Failed to save the project"));
        }
      else
        {
          z_warning ("{}", e.what ());
        }
    }

  return G_SOURCE_CONTINUE;
#endif
  return 0;
}

ProjectSaver::SerializeProjectThread::SerializeProjectThread (SaveContext &ctx)
    : juce::Thread ("SerializeProject"), ctx_ (ctx)
{
  startThread ();
}

ProjectSaver::SerializeProjectThread::~SerializeProjectThread ()
{
  stopThread (-1);
}

void
ProjectSaver::SerializeProjectThread::run ()
{
  char * compressed_json{};
  size_t compressed_size{};

  /* generate json */
  z_debug ("serializing project to json...");
  QElapsedTimer timer;
  timer.start ();
  std::optional<nlohmann::json> json;
  try
    {
      json = *ctx_.main_project_;
    }
  catch (const ZrythmException &e)
    {
      e.handle ("Failed to serialize project");
      ctx_.has_error_ = true;
      goto serialize_end;
    }
  z_debug ("time to serialize: {}ms", timer.elapsed ());

  /* compress */
  try
    {
      const auto json_str = json->dump ();
      // warning: this byte array depends on json_str being alive while it's used
      QByteArray src_data = QByteArray::fromRawData (
        json_str.c_str (), static_cast<qsizetype> (json_str.length ()));
      compress (
        &compressed_json, &compressed_size,
        CompressionFlag::PROJECT_COMPRESS_DATA, src_data);
    }
  catch (const ZrythmException &ex)
    {
      ex.handle (QObject::tr ("Failed to compress project file"));
      ctx_.has_error_ = true;
      goto serialize_end;
    }

  /* set file contents */
  z_debug ("saving project file at {}...", ctx_.project_file_path_);
  try
    {
      utils::io::set_file_contents (
        ctx_.project_file_path_, compressed_json, compressed_size);
    }
  catch (const ZrythmException &e)
    {
      ctx_.has_error_ = true;
      z_error ("Unable to write project file: {}", e.what ());
    }
  free (compressed_json);

  z_debug ("successfully saved project");

serialize_end:
  // ctx_.main_project_->legacy_undo_manager_->action_sem_.release ();
  ctx_.finished_.store (true);
}

bool
ProjectSaver::idle_saved_callback (SaveContext * ctx)
{
  if (!ctx->finished_)
    {
      return true;
    }

  if (ctx->is_backup_)
    {
      z_debug ("Backup saved.");
// TODO
#if 0
      if (ZRYTHM_HAVE_UI)
        {

          // ui_show_notification (QObject::tr ("Backup saved."));
        }
#endif
    }
  else
    {
// TODO
#if 0
      if (ZRYTHM_HAVE_UI && !ZRYTHM_TESTING && !ZRYTHM_BENCHMARKING)
        {
          zrythm::gui::ProjectManager::get_instance ()->add_to_recent_projects (
            utils::Utf8String::from_path (ctx->project_file_path_).to_qstring ());
        }
      if (ctx->show_notification_)
        {
          // ui_show_notification (QObject::tr ("Project saved."));
        }
#endif
    }

#if 0
  if (ZRYTHM_HAVE_UI && PROJECT->loaded_ && MAIN_WINDOW)
    {
      /* EVENTS_PUSH (EventType::ET_PROJECT_SAVED, PROJECT.get ()); */
    }
#endif

  ctx->progress_info_.mark_completed (ProgressInfo::CompletionType::SUCCESS, {});

  return false;
}

void
ProjectSaver::cleanup_plugin_state_dirs (
  const Project &main_project,
  bool           is_backup)
{
  z_debug ("cleaning plugin state dirs{}...", is_backup ? " for backup" : "");

  std::vector<plugins::PluginPtrVariant> plugins;
  for (
    const auto &pl_var :
    main_project.get_plugin_registry ().get_hash_map () | std::views::values)
    {
      plugins.push_back (pl_var);
    }
  for (
    const auto &pl_var :
    project_.get_plugin_registry ().get_hash_map () | std::views::values)
    {
      plugins.push_back (pl_var);
    }

  for (const auto &[i, pl_var] : utils::views::enumerate (plugins))
    {
      z_debug ("plugin {}: {}", i, plugin_state_dir_projection (pl_var));
    }

  auto plugin_states_path =
    main_project.get_directory (false)
    / gui::ProjectPathProvider::get_path (
      zrythm::gui::ProjectPathProvider::ProjectPath::PluginStates);

  try
    {
      QDir srcdir (
        utils::Utf8String::from_path (plugin_states_path).to_qstring ());
      const auto entries =
        srcdir.entryInfoList (QDir::Files | QDir::NoDotAndDotDot);

      for (const auto &filename : entries)
        {
          const auto filename_str =
            utils::Utf8String::from_qstring (filename.fileName ()).to_path ();
          const auto full_path = plugin_states_path / filename_str;

          bool found = std::ranges::contains (
            plugins, filename_str, plugin_state_dir_projection);
          if (!found)
            {
              z_debug ("removing unused plugin state in {}", full_path);
              utils::io::rmdir (full_path, true);
            }
        }
    }
  catch (const ZrythmException &e)
    {
      z_critical ("Failed to open directory: {}", e.what ());
      return;
    }

  z_debug ("cleaned plugin state directories");
}

void
ProjectSaver::
  save (const bool is_backup, const bool show_notification, const bool async)
{
  const auto dir = project_.get_directory (is_backup);
  z_info (
    "Saving project at {}, is backup: {}, show notification: {}, async: {}",
    dir, is_backup, show_notification, async);

  /* pause engine */
  EngineState state{};
  bool        engine_paused = false;
  auto *      engine = project_.engine ();
  if (engine->activated ())
    {
      engine->wait_for_pause (state, false, true);
      engine_paused = true;
    }

  /* if async, lock the undo manager */
  if (async)
    {
      // legacy_undo_manager_->action_sem_.acquire ();
    }

  /* set the dir and create it if it doesn't exist */
  try
    {
      utils::io::mkdir (dir);
    }

  catch (const ZrythmException &e)
    {
      throw ZrythmException (

        fmt::format ("Failed to create project directory {}", dir));
    }

// TODO
#if 0
  /* set the title */
  title_ = utils::Utf8String::from_path (dir.filename ());

  /* save current datetime */
  datetime_str_ = utils::datetime::get_current_as_string ();

  /* set the project version  */
  version_ = Zrythm::get_version (false);
#endif

  /* if backup, get next available backup dir */
  if (is_backup)
    {
      try
        {
          set_and_create_next_available_backup_dir ();
        }

      catch (const ZrythmException &e)
        {
          throw ZrythmException (
            QObject::tr ("Failed to create backup directory"));
        }
    }

  try
    {
      make_project_dirs (project_, is_backup);
    }

  catch (const ZrythmException &e)
    {
      throw ZrythmException ("Failed to create project directories");
    }

// TODO
#if 0
  if (this == get_active_instance ())
    {
      /* write the pool */
      pool_->remove_unused (is_backup);
    }
#endif

  try
    {
      project_.pool_->write_to_disk (is_backup);
    }
  catch (const ZrythmException &e)
    {
      throw ZrythmException ("Failed to write audio pool to disk");
    }

  const auto project_file_path =
    dir
    / gui::ProjectPathProvider::get_path (
      gui::ProjectPathProvider::ProjectPath::ProjectFile);
  const auto temp_project_file_path =
    dir
    / (utils::Utf8String::from_path (
         gui::ProjectPathProvider::get_path (
           gui::ProjectPathProvider::ProjectPath::ProjectFile))
       + u8".tmp")
        .to_path ();

  auto ctx = std::make_unique<SaveContext> ();
  ctx->main_project_ = &project_;
  ctx->project_file_path_ = temp_project_file_path;
  ctx->show_notification_ = show_notification;
  ctx->is_backup_ = is_backup;
#if 0
  ctx->project_ = [&] () {
    if (ZRYTHM_IS_QT_THREAD)
      {
        return std::unique_ptr<Project> (clone (is_backup));
      }
    else
      {
        Project *  cloned_prj = nullptr;
        QThread *  currentThread = QThread::currentThread ();
        QEventLoop loop;
        QMetaObject::invokeMethod (
          QCoreApplication::instance (),
          [this, currentThread, is_backup, &cloned_prj, &loop] () {
            cloned_prj = clone (is_backup);

            // need to move the temporary cloned project to the outer scope's
            // thread, because it will be free'd on that thread too
            cloned_prj->moveToThread (currentThread);
            loop.quit ();
          },
          Qt::QueuedConnection);
        loop.exec ();
        return std::unique_ptr<Project> (cloned_prj);
      }
  }();
#endif

  if (is_backup)
    {
      /* copy plugin states */
      auto prj_pl_states_dir =
        project_.get_directory (false)
        / gui::ProjectPathProvider::get_path (
          zrythm::gui::ProjectPathProvider::ProjectPath::PluginsDir);
      auto prj_backup_pl_states_dir =
        project_.get_directory (true)
        / gui::ProjectPathProvider::get_path (
          zrythm::gui::ProjectPathProvider::ProjectPath::PluginsDir);
      try
        {
          utils::io::copy_dir (
            prj_backup_pl_states_dir, prj_pl_states_dir, false, true);
        }
      catch (const ZrythmException &e)
        {
          throw ZrythmException (QObject::tr ("Failed to copy plugin states"));
        }
    }

  if (!is_backup)
    {
      /* cleanup unused plugin states */
      cleanup_plugin_state_dirs (project_, is_backup);
    }

  /* TODO verify all plugin states exist */

  if (async)
    {
      SerializeProjectThread save_thread (*ctx);

      /* TODO: show progress dialog */
      if (/* ZRYTHM_HAVE_UI */ false)
        {
          auto timer = new QTimer ();
          QObject::connect (timer, &QTimer::timeout, &project_, [timer, &ctx] () {
            bool keep_calling = idle_saved_callback (ctx.get ());
            if (!keep_calling)
              {
                timer->stop ();
                timer->deleteLater ();
              }
          });
          timer->start (100);

          /* show progress while saving (TODO) */
        }
      else
        {
          while (!ctx->finished_.load ())
            {
              std::this_thread::sleep_for (std::chrono::milliseconds (1));
            }
          idle_saved_callback (ctx.get ());
        }
    }
  else /* else if no async */
    {
      /* call synchronously */
      SerializeProjectThread save_thread (*ctx);
      while (save_thread.isThreadRunning ())
        {
          std::this_thread::sleep_for (std::chrono::milliseconds (1));
        }
      idle_saved_callback (ctx.get ());
    }

  /* copy the actual project file to signal that we're finished */
  {
    utils::io::copy_file (project_file_path, temp_project_file_path);
    utils::io::remove (temp_project_file_path);
  }

// TODO
#if 0
  auto last_action = legacy_undo_manager_->get_last_action ();
  if (is_backup)
    {
      last_action_in_last_successful_autosave_ = last_action;
    }
  else
    {
      last_saved_action_ = last_action;
    }
#endif

  if (engine_paused)
    {
      engine->resume (state);
    }

  z_info (
    "Saved project at {}, is backup: {}, show notification: {}, async: {}", dir,
    is_backup, show_notification, async);
}

bool
ProjectSaver::has_unsaved_changes () const
{
  /* simply check if the last performed action matches the last action when the
   * project was last saved/loaded */
// TODO
#if 0
  auto last_performed_action = legacy_undo_manager_->get_last_action ();
  return last_performed_action != last_saved_action_;
#endif
  return true;
}
}
