// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/project_ui_state.h"
#include "structure/project/project_path_provider.h"

#include <QQmlEngine>

namespace zrythm::gui
{
ProjectUiState::ProjectUiState (
  utils::AppSettings                                    &app_settings,
  utils::QObjectUniquePtr<structure::project::Project> &&project)
    : app_settings_ (app_settings), project_ (std::move (project)),
      tool_ (new gui::backend::ArrangerTool (this)),
      clip_editor_ (
        utils::make_qobject_unique<ClipEditor> (
          project_->get_arranger_object_registry (),
          [&] (const auto &id) {
            return project_->get_track_registry ().find_by_id_or_throw (id);
          },
          this)),
      timeline_ (
        utils::make_qobject_unique<structure::arrangement::Timeline> (this)),
      quantize_opts_editor_ (
        std::make_unique<QuantizeOptions> (zrythm::utils::NoteLength::Note_1_8)),
      quantize_opts_timeline_ (
        std::make_unique<QuantizeOptions> (zrythm::utils::NoteLength::Note_1_1)),
      undo_stack_ (
        utils::make_qobject_unique<undo::UndoStack> (
          [&] (const auto &action, bool recalculate_graph) {
            project_->engine ()
              ->execute_function_with_paused_processing_synchronously (
                action, recalculate_graph);
          },
          this)),
      snap_grid_timeline_ (
        utils::make_qobject_unique<dsp::SnapGrid> (
          project_->tempo_map (),
          utils::NoteLength::Bar,
          [this] {
            return app_settings_.timelineLastCreatedObjectLengthInTicks ();
          },
          this)),
      snap_grid_editor_ (
        utils::make_qobject_unique<dsp::SnapGrid> (
          project_->tempo_map (),
          utils::NoteLength::Note_1_8,
          [this] {
            return app_settings_.editorLastCreatedObjectLengthInTicks ();
          },
          this)),
      arranger_object_creator_ (
        utils::make_qobject_unique<zrythm::actions::ArrangerObjectCreator> (
          *undo_stack_,
          *project_->arrangerObjectFactory (),
          *snap_grid_timeline_,
          *snap_grid_editor_,
          this)),
      track_creator_ (
        utils::make_qobject_unique<zrythm::actions::TrackCreator> (
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
            // Show UI by default when importing plugins
            z_debug ("Plugin instantiation completed");
            zrythm::plugins::plugin_ptr_variant_to_base (
              plugin_ref.get_object ())
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
          *snap_grid_timeline_,
          this))
{
  project_->setParent (this);
}

QString
ProjectUiState::getTitle () const
{
  return title_;
}

void
ProjectUiState::setTitle (const QString &title)
{
  const auto std_str = utils::Utf8String::from_qstring (title);
  if (title_ == std_str)
    return;

  title_ = std_str;
  Q_EMIT titleChanged (title);
}

QString
ProjectUiState::projectDirectory () const
{
  return utils::Utf8String::from_path (project_directory_).to_qstring ();
}

void
ProjectUiState::setProjectDirectory (const QString &directory)
{
  auto path = utils::Utf8String::from_qstring (directory).to_path ();
  if (project_directory_ == path)
    return;

  project_directory_ = path;
  Q_EMIT projectDirectoryChanged (directory);
}

actions::ArrangerObjectSelectionOperator *
ProjectUiState::createArrangerObjectSelectionOperator (
  QItemSelectionModel * selectionModel) const
{
  // FIXME: this method should be someplace else - Project has too many concerns
  // currently
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

  // Transfer ownership to QML JavaScript engine for proper cleanup
  QQmlEngine::setObjectOwnership (sel_operator, QQmlEngine::JavaScriptOwnership);

  return sel_operator;
}

structure::project::Project *
ProjectUiState::project () const
{
  return project_.get ();
}

structure::arrangement::Timeline *
ProjectUiState::timeline () const
{
  return timeline_.get ();
}

gui::backend::ArrangerTool *
ProjectUiState::tool () const
{
  return tool_.get ();
}

ClipEditor *
ProjectUiState::clipEditor () const
{
  return clip_editor_.get ();
}

actions::ArrangerObjectCreator *
ProjectUiState::arrangerObjectCreator () const
{
  return arranger_object_creator_.get ();
}

actions::TrackCreator *
ProjectUiState::trackCreator () const
{
  return track_creator_.get ();
}

actions::PluginImporter *
ProjectUiState::pluginImporter () const
{
  return plugin_importer_.get ();
}

actions::FileImporter *
ProjectUiState::fileImporter () const
{
  return file_importer_.get ();
}

undo::UndoStack *
ProjectUiState::undoStack () const
{
  return undo_stack_.get ();
}

dsp::SnapGrid *
ProjectUiState::snapGridTimeline () const
{
  return snap_grid_timeline_.get ();
}

dsp::SnapGrid *
ProjectUiState::snapGridEditor () const
{
  return snap_grid_editor_.get ();
}

controllers::TransportController *
ProjectUiState::transportController () const
{
  return transport_controller_.get ();
}

std::optional<fs::path>
ProjectUiState::get_newer_backup ()
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

} // namespace zrythm::gui
