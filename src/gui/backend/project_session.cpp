// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "controllers/project_saver.h"
#include "gui/backend/project_session.h"
#include "structure/project/project_path_provider.h"
#include "utils/io_utils.h"

#include <QQmlEngine>

namespace zrythm::gui
{

ProjectSession::ProjectSession (
  utils::AppSettings                                    &app_settings,
  utils::QObjectUniquePtr<structure::project::Project> &&project)
    : app_settings_ (app_settings), project_ (std::move (project)),
      ui_state_ (
        utils::make_qobject_unique<
          structure::project::ProjectUiState> (*project_, app_settings_)),
      undo_stack_ (
        utils::make_qobject_unique<undo::UndoStack> (
          [this] (const auto &action, bool recalculate_graph) {
            project_->engine ()
              ->execute_function_with_paused_processing_synchronously (
                action, recalculate_graph);
          },
          this)),
      quantize_opts_editor_ (
        std::make_unique<old_dsp::QuantizeOptions> (utils::NoteLength::Note_1_8)),
      quantize_opts_timeline_ (
        std::make_unique<old_dsp::QuantizeOptions> (utils::NoteLength::Note_1_1)),
      arranger_object_creator_ (
        utils::make_qobject_unique<actions::ArrangerObjectCreator> (
          *undo_stack_,
          *project_->arrangerObjectFactory (),
          *ui_state_->snapGridTimeline (),
          *ui_state_->snapGridEditor (),
          this)),
      track_creator_ (
        utils::make_qobject_unique<actions::TrackCreator> (
          *undo_stack_,
          *project_->track_factory_,
          *project_->tracklist ()->collection (),
          *project_->tracklist ()->trackRouting (),
          *project_->tracklist ()->singletonTracks (),
          this)),
      plugin_importer_ (
        utils::make_qobject_unique<actions::PluginImporter> (
          *undo_stack_,
          *project_->plugin_factory_,
          *track_creator_,
          [] (plugins::PluginUuidReference plugin_ref) {
            z_debug ("Plugin instantiation completed");
            plugins::plugin_ptr_variant_to_base (plugin_ref.get_object ())
              ->setUiVisible (true);
          },
          this)),
      file_importer_ (
        utils::make_qobject_unique<actions::FileImporter> (
          *undo_stack_,
          *arranger_object_creator_,
          *track_creator_,
          this)),
      transport_controller_ (
        utils::make_qobject_unique<controllers::TransportController> (
          *project_->transport_,
          *ui_state_->snapGridTimeline (),
          this))
{
  project_->setParent (this);
}

QString
ProjectSession::title () const
{
  return title_.to_qstring ();
}

void
ProjectSession::setTitle (const QString &title)
{
  const auto std_str = utils::Utf8String::from_qstring (title);
  if (title_ == std_str)
    return;

  title_ = std_str;
  Q_EMIT titleChanged (title);
}

QString
ProjectSession::projectDirectory () const
{
  return utils::Utf8String::from_path (project_directory_).to_qstring ();
}

void
ProjectSession::setProjectDirectory (const QString &directory)
{
  auto path = utils::Utf8String::from_qstring (directory).to_path ();
  if (project_directory_ == path)
    return;

  project_directory_ = path;
  Q_EMIT projectDirectoryChanged (directory);
}

structure::project::Project *
ProjectSession::project () const
{
  return project_.get ();
}

structure::project::ProjectUiState *
ProjectSession::uiState () const
{
  return ui_state_.get ();
}

undo::UndoStack *
ProjectSession::undoStack () const
{
  return undo_stack_.get ();
}

zrythm::actions::ArrangerObjectCreator *
ProjectSession::arrangerObjectCreator () const
{
  return arranger_object_creator_.get ();
}

zrythm::actions::TrackCreator *
ProjectSession::trackCreator () const
{
  return track_creator_.get ();
}

actions::PluginImporter *
ProjectSession::pluginImporter () const
{
  return plugin_importer_.get ();
}

actions::FileImporter *
ProjectSession::fileImporter () const
{
  return file_importer_.get ();
}

controllers::TransportController *
ProjectSession::transportController () const
{
  return transport_controller_.get ();
}

actions::ArrangerObjectSelectionOperator *
ProjectSession::createArrangerObjectSelectionOperator (
  QItemSelectionModel * selectionModel) const
{
  auto * sel_operator = new actions::ArrangerObjectSelectionOperator (
    *undo_stack_, *selectionModel,
    [this] (structure::arrangement::ArrangerObjectPtrVariant obj_var) {
      return std::visit (
        [&] (const auto &obj)
          -> actions::ArrangerObjectSelectionOperator::ArrangerObjectOwnerPtrVariant {
          using ObjT = base_type<decltype (obj)>;
          if constexpr (structure::arrangement::LaneOwnedObject<ObjT>)
            {
              return static_cast<structure::arrangement::ArrangerObjectOwner<
                ObjT> *> (project_->tracklist ()->getTrackLaneForObject (obj));
            }
          else if constexpr (structure::arrangement::TimelineObject<ObjT>)
            {
              return dynamic_cast<
                structure::arrangement::ArrangerObjectOwner<ObjT> *> (
                project_->tracklist ()->getTrackForTimelineObject (obj));
            }
          else
            {
              return dynamic_cast<structure::arrangement::ArrangerObjectOwner<
                ObjT> *> (obj->parentObject ());
            }
        },
        obj_var);
    },
    *project_->arrangerObjectFactory ());

  QQmlEngine::setObjectOwnership (sel_operator, QQmlEngine::JavaScriptOwnership);

  return sel_operator;
}

std::optional<fs::path>
ProjectSession::get_newer_backup ()
{
  // TODO
  return std::nullopt;
  const auto filepath =
    project_directory_
    / structure::project::ProjectPathProvider::get_path (
      structure::project::ProjectPathProvider::ProjectPath::ProjectFile);

  std::filesystem::file_time_type original_time;
  if (std::filesystem::exists (filepath))
    {
      original_time = std::filesystem::last_write_time (filepath);
    }
  else
    {
      z_warning ("Failed to get last modified for {}", filepath);
      return std::nullopt;
    }

  fs::path   result;
  const auto backups_dir =
    project_directory_
    / structure::project::ProjectPathProvider::get_path (
      structure::project::ProjectPathProvider::ProjectPath::BackupsDir);
  try
    {
      for (const auto &entry : std::filesystem::directory_iterator (backups_dir))
        {
          auto full_path =
            entry.path ()
            / structure::project::ProjectPathProvider::get_path (
              structure::project::ProjectPathProvider::ProjectPath::ProjectFile);
          z_debug ("{}", full_path);

          if (std::filesystem::exists (full_path))
            {
              auto backup_time = std::filesystem::last_write_time (full_path);
              if (backup_time > original_time)
                {
                  result = entry.path ();
                  original_time = backup_time;
                }
            }
          else
            {
              z_warning ("Failed to get last modified for {}", full_path);
              return std::nullopt;
            }
        }
    }
  catch (const std::filesystem::filesystem_error &e)
    {
      z_warning ("Error accessing backup directory: {}", e.what ());
      return std::nullopt;
    }

  return result;
}

gui::qquick::QFutureQmlWrapper *
ProjectSession::save ()
{
  assert (!project_directory_.empty ());

  auto future = controllers::ProjectSaver::save (
    *project_, *ui_state_, *undo_stack_, Zrythm::get_app_version (),
    project_directory_, false);

  auto * wrapper = new gui::qquick::QFutureQmlWrapperT<QString> (future);
  QQmlEngine::setObjectOwnership (wrapper, QQmlEngine::JavaScriptOwnership);

  return wrapper;
}

gui::qquick::QFutureQmlWrapper *
ProjectSession::saveAs (const QString &path)
{
  auto new_path = utils::Utf8String::from_qstring (path).to_path ();

  auto future = controllers::ProjectSaver::save (
    *project_, *ui_state_, *undo_stack_, Zrythm::get_app_version (), new_path,
    false);

  auto * wrapper = new gui::qquick::QFutureQmlWrapperT<QString> (future);

  // Update project directory and title when save completes
  QObject::connect (
    wrapper, &gui::qquick::QFutureQmlWrapperT<QString>::finished, this,
    [this, new_path, future] () {
      if (future.resultCount () > 0)
        {
          auto saved_path = future.result ();
          if (!saved_path.isEmpty ())
            {
              setProjectDirectory (saved_path);
              setTitle (
                utils::Utf8String::from_path (
                  utils::io::path_get_basename (new_path))
                  .to_qstring ());
            }
        }
    });

  QQmlEngine::setObjectOwnership (wrapper, QQmlEngine::JavaScriptOwnership);

  return wrapper;
}

} // namespace zrythm::gui
