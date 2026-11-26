// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "actions/arranger_object_creator.h"
#include "actions/arranger_object_selection_operator.h"
#include "actions/clip_operator.h"
#include "actions/file_importer.h"
#include "actions/parameter_operator.h"
#include "actions/plugin_importer.h"
#include "actions/plugin_operator.h"
#include "actions/port_connection_operator.h"
#include "actions/track_collection_operator.h"
#include "actions/track_creator.h"
#include "actions/uuid_property_operator.h"
#include "controllers/clipboard.h"
#include "controllers/recording_coordinator.h"
#include "controllers/recording_materializer.h"
#include "controllers/transport_controller.h"
#include "frontends/gui/backend/editor_arranger_objects_model.h"
#include "frontends/gui/backend/editor_function_registry.h"
#include "frontends/gui/backend/modular_project_model.h"
#include "frontends/gui/backend/timeline_arranger_objects_model.h"
#include "frontends/gui/qquick/generic_plugin_ui_controller.h"
#include "frontends/gui/qquick/qfuture_qml_wrapper.h"
#include "frontends/mcp/mcp_service.h"
#include "structure/project/project.h"
#include "structure/project/project_ui_state.h"
#include "undo/undo_stack.h"
#include "utils/composite_object_registry.h"

#include <QtQmlIntegration/qqmlintegration.h>

#include "operations/operation_dispatcher.h"

namespace zrythm::frontends::gui::old_dsp
{
class QuantizeOptions;
}

namespace zrythm::operations
{
class OperationEnvironment;
class OperationRegistry;
}

namespace zrythm::frontends::gui
{

/**
 * @brief Composition root for a project session.
 *
 * Owns and coordinates all the components needed for an active project:
 * - Core Project data
 * - Serializable UI state (ProjectUiState)
 * - Undo/redo history
 * - Action handlers for user operations
 *
 * QML accesses UI components (tool, clipEditor, timeline, snapGrids) via the
 * uiState property.
 */
class ProjectSession : public QObject
{
  Q_OBJECT
  Q_PROPERTY (QString title READ title WRITE setTitle NOTIFY titleChanged FINAL)
  Q_PROPERTY (
    zrythm::structure::project::Project * project READ project CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::structure::project::ProjectUiState * uiState READ uiState CONSTANT
      FINAL)
  Q_PROPERTY (zrythm::undo::UndoStack * undoStack READ undoStack CONSTANT FINAL)
  Q_PROPERTY (
    bool hasUnsavedChanges READ hasUnsavedChanges NOTIFY
      hasUnsavedChangesChanged FINAL)
  Q_PROPERTY (
    zrythm::operations::OperationDispatcher * operationDispatcher READ
      operationDispatcher CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::frontends::gui::TimelineArrangerObjectsModel *
      timelineArrangerObjects READ timelineArrangerObjects CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::frontends::gui::EditorArrangerObjectsModel * editorArrangerObjects
      READ editorArrangerObjects CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::frontends::gui::EditorFunctionRegistry * editorFunctions READ
      editorFunctions CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::actions::ArrangerObjectCreator * arrangerObjectCreator READ
      arrangerObjectCreator CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::actions::ClipOperator * clipOperator READ clipOperator CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::actions::ArrangerObjectSelectionOperator * arrangerObjectSelectionOperator
      READ arrangerObjectSelectionOperator CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::actions::TrackCreator * trackCreator READ trackCreator CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::actions::TrackCollectionOperator * trackCollectionOperator READ
      trackCollectionOperator CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::actions::PluginImporter * pluginImporter READ pluginImporter
      CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::actions::PluginOperator * pluginOperator READ pluginOperator
      CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::actions::ProcessorParameterOperator * processorParameterOperator
      READ processorParameterOperator CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::actions::PortConnectionOperator * portConnectionOperator READ
      portConnectionOperator CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::frontends::gui::qquick::GenericPluginUiController *
      genericPluginUiController READ genericPluginUiController CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::frontends::gui::ModularProjectModel * modularProjectModel READ
      modularProjectModel CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::actions::FileImporter * fileImporter READ fileImporter CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::actions::UuidPropertyOperator * uuidPropertyOperator READ
      uuidPropertyOperator CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::controllers::TransportController * transportController READ
      transportController CONSTANT FINAL)
  Q_PROPERTY (
    QString projectDirectory READ projectDirectory WRITE setProjectDirectory
      NOTIFY projectDirectoryChanged FINAL)
  Q_PROPERTY (
    zrythm::controllers::Clipboard * clipboard READ clipboard CONSTANT FINAL)
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  ProjectSession (
    utils::AppSettings                                    &app_settings,
    controllers::Clipboard                                &clipboard,
    utils::IObjectRegistry                                &app_registry,
    utils::QObjectUniquePtr<structure::project::Project> &&project);

  ~ProjectSession () override;

  QString                       title () const;
  void                          setTitle (const QString &title);
  QString                       projectDirectory () const;
  void                          setProjectDirectory (const QString &directory);
  structure::project::Project * project () const;
  structure::project::ProjectUiState * uiState () const;
  undo::UndoStack *                    undoStack () const;

  /**
   * @brief Whether the project changed since its last successful save.
   *
   * A change is an edit in the undo history. The flag clears when the
   * history is back at the state of the last save, and when a save
   * succeeds.
   */
  bool hasUnsavedChanges () const { return has_unsaved_changes_; }

  /** The session's operation dispatcher. */
  operations::OperationDispatcher *          operationDispatcher () const;
  TimelineArrangerObjectsModel *             timelineArrangerObjects () const;
  EditorArrangerObjectsModel *               editorArrangerObjects () const;
  EditorFunctionRegistry *                   editorFunctions () const;
  zrythm::actions::ArrangerObjectCreator *   arrangerObjectCreator () const;
  zrythm::actions::ClipOperator *            clipOperator () const;
  zrythm::actions::TrackCreator *            trackCreator () const;
  zrythm::actions::TrackCollectionOperator * trackCollectionOperator () const;
  actions::PluginImporter *                  pluginImporter () const;
  actions::PluginOperator *                  pluginOperator () const;
  actions::ProcessorParameterOperator * processorParameterOperator () const;
  actions::PortConnectionOperator *     portConnectionOperator () const;
  qquick::GenericPluginUiController *   genericPluginUiController () const;
  ModularProjectModel *                 modularProjectModel () const;
  actions::FileImporter *               fileImporter () const;
  actions::UuidPropertyOperator *       uuidPropertyOperator () const;
  controllers::TransportController *    transportController () const;
  controllers::RecordingCoordinator *   recordingCoordinator () const;

  /**
   * The services operation dispatch runs against for this session.
   */
  operations::OperationEnvironment * operation_environment () const;

  /**
   * The operations available in this session, with the builtin
   * operations registered.
   */
  const operations::OperationRegistry * operation_registry () const;

  /**
   * @brief Returns the application-wide object clipboard.
   */
  controllers::Clipboard * clipboard () const { return &clipboard_; }

  /**
   * @brief Returns the session's arranger object selection operator.
   *
   * Selection-based operations take the caller's selection model as an
   * argument, so the operator holds no per-view state.
   */
  zrythm::actions::ArrangerObjectSelectionOperator *
  arrangerObjectSelectionOperator () const;

  /**
   * @brief Saves the project to the current project directory.
   *
   * On success, the undo history position at the time of the call becomes
   * the saved state, so hasUnsavedChanges() is false until the next edit.
   *
   * @return A QFutureQmlWrapper that resolves to the saved project path on
   * success.
   * @pre projectDirectory must be set.
   */
  Q_INVOKABLE qquick::QFutureQmlWrapper * save ();

  /**
   * @brief Saves the project to a new directory created inside the given
   * parent directory and named after the project name.
   *
   * Updates projectDirectory and title on success, and makes the undo
   * history position at the time of the call the saved state. The call is
   * refused, resolving to an empty result, when the name is invalid, the
   * parent is not an absolute path, the target holds unrelated files, the
   * target cannot be inspected, or the target holds a project and
   * @p overwrite_existing_project is false.
   *
   * @param parent_directory Directory to create the project directory in.
   * @param name Project name, used as the new directory's name.
   * @param overwrite_existing_project Whether to replace an existing
   * project at the target. The replaced project's file is overwritten;
   * its other files stay on disk.
   * @return A QFutureQmlWrapper that resolves to the saved project path on
   * success.
   */
  Q_INVOKABLE qquick::QFutureQmlWrapper * saveAs (
    const QUrl    &parent_directory,
    const QString &name,
    bool           overwrite_existing_project);

  /**
   * @brief Finds a backup directory newer than the main project file.
   *
   * @return The backup path if one exists with a newer timestamp, nullopt
   * otherwise.
   */
  std::optional<std::filesystem::path> get_newer_backup ();

Q_SIGNALS:
  void titleChanged (const QString &title);
  void projectDirectoryChanged (const QString &directory);
  void hasUnsavedChangesChanged ();

private:
  /**
   * @brief Wires MIDI input selection channel changes to track processors.
   *
   * Iterates existing tracks and connects to TrackCollection::rowsInserted
   * to handle tracks added later.
   */
  void wire_midi_input_selections_to_tracks ();

  /** @brief Sets the saved state and updates hasUnsavedChanges(). */
  void set_saved_state (std::uint64_t state_id);

  /** @brief Recomputes hasUnsavedChanges() from the undo history. */
  void update_has_unsaved_changes ();

  /**
   * @brief Wires the chord track's MIDI expansion to the chord pad bank.
   */
  void wire_chord_track_to_pad_bank ();

  utils::AppSettings &app_settings_;

  // Application-wide object clipboard (shared across project sessions)
  controllers::Clipboard &clipboard_;

  // True while a Save As is running: overlapping calls are refused so
  // each save's identity restore only ever sees its own lineage.
  // UI-thread only (saveAs runs there and the finished handler is
  // queued back to this thread)
  bool save_as_in_flight_ = false;

  // Project title and directory
  utils::Utf8String     title_;
  std::filesystem::path project_directory_;

  // UndoStack::stateId() at the last successful save; 0 matches a history
  // with no applied entry
  std::uint64_t saved_state_id_{};
  bool          has_unsaved_changes_{ false };

  // Core project data
  utils::QObjectUniquePtr<structure::project::Project> project_;

  // Resolves the project registry and the application registry; the
  // session's operations dispatch against this composite
  std::unique_ptr<utils::CompositeObjectRegistry> composite_registry_;

  // Serializable UI state (tool, clipEditor, timeline, snapGrids)
  utils::QObjectUniquePtr<structure::project::ProjectUiState> ui_state_;

  // Undo/redo history
  utils::QObjectUniquePtr<undo::UndoStack> undo_stack_;

  // Unified arranger object models (timeline-wide and open-clip)
  utils::QObjectUniquePtr<TimelineArrangerObjectsModel>
    timeline_arranger_objects_;
  utils::QObjectUniquePtr<EditorArrangerObjectsModel> editor_arranger_objects_;
  utils::QObjectUniquePtr<EditorFunctionRegistry>     editor_functions_;

  // Quantize options for MIDI editing
  std::unique_ptr<old_dsp::QuantizeOptions> quantize_opts_editor_;
  std::unique_ptr<old_dsp::QuantizeOptions> quantize_opts_timeline_;

  // Action handlers for user operations
  utils::QObjectUniquePtr<actions::ArrangerObjectCreator>
                                                 arranger_object_creator_;
  utils::QObjectUniquePtr<actions::ClipOperator> clip_operator_;
  utils::QObjectUniquePtr<actions::ArrangerObjectSelectionOperator>
    arranger_object_selection_operator_;
  utils::QObjectUniquePtr<actions::TrackCreator> track_creator_;
  utils::QObjectUniquePtr<actions::TrackCollectionOperator>
    track_collection_operator_;
  utils::QObjectUniquePtr<qquick::GenericPluginUiController>
                                               generic_plugin_ui_controller_;
  utils::QObjectUniquePtr<ModularProjectModel> modular_project_model_;
  utils::QObjectUniquePtr<actions::PluginImporter> plugin_importer_;
  utils::QObjectUniquePtr<actions::PluginOperator> plugin_operator_;
  utils::QObjectUniquePtr<actions::ProcessorParameterOperator>
    processor_parameter_operator_;
  utils::QObjectUniquePtr<actions::PortConnectionOperator>
                                                 port_connection_operator_;
  utils::QObjectUniquePtr<actions::FileImporter> file_importer_;
  utils::QObjectUniquePtr<actions::UuidPropertyOperator> uuid_property_operator_;
  utils::QObjectUniquePtr<controllers::TransportController> transport_controller_;
  utils::QObjectUniquePtr<controllers::RecordingCoordinator>
    recording_coordinator_;
  utils::QObjectUniquePtr<controllers::RecordingMaterializer>
    recording_materializer_;

  // Operation dispatch for this session: the services operations run
  // against, and the available operations
  std::unique_ptr<operations::OperationEnvironment> operation_environment_;
  std::unique_ptr<operations::OperationRegistry>    operation_registry_;
  utils::QObjectUniquePtr<operations::OperationDispatcher> operation_dispatcher_;

  /** The MCP frontend exposing the operations over the operations API. */
  utils::QObjectUniquePtr<mcp::McpService> mcp_service_;
};

} // namespace zrythm::frontends::gui
