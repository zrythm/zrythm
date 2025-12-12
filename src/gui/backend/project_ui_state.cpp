// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/project_ui_state.h"

#include <QQmlEngine>

namespace zrythm::gui
{
ProjectUiState::ProjectUiState (
  utils::QObjectUniquePtr<structure::project::Project> &&project)
    : project_ (std::move (project)),
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
      arranger_object_creator_ (
        utils::make_qobject_unique<zrythm::actions::ArrangerObjectCreator> (
          *undo_stack_,
          *project_->arrangerObjectFactory (),
          *project_->snapGridTimeline (),
          *project_->snapGridEditor (),
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
          this))
{
  project_->setParent (this);
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
} // namespace zrythm::gui
