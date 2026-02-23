// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "actions/arranger_object_creator.h"
#include "actions/arranger_object_selection_operator.h"
#include "actions/file_importer.h"
#include "actions/plugin_importer.h"
#include "actions/track_creator.h"
#include "controllers/transport_controller.h"
#include "gui/dsp/quantize_options.h"
#include "gui/qquick/qfuture_qml_wrapper.h"
#include "structure/project/project.h"
#include "structure/project/project_ui_state.h"
#include "undo/undo_stack.h"

#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::gui
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
    zrythm::actions::ArrangerObjectCreator * arrangerObjectCreator READ
      arrangerObjectCreator CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::actions::TrackCreator * trackCreator READ trackCreator CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::actions::PluginImporter * pluginImporter READ pluginImporter
      CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::actions::FileImporter * fileImporter READ fileImporter CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::controllers::TransportController * transportController READ
      transportController CONSTANT FINAL)
  Q_PROPERTY (
    QString projectDirectory READ projectDirectory WRITE setProjectDirectory
      NOTIFY projectDirectoryChanged FINAL)
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  ProjectSession (
    utils::AppSettings                                    &app_settings,
    utils::QObjectUniquePtr<structure::project::Project> &&project);

  QString                       title () const;
  void                          setTitle (const QString &title);
  QString                       projectDirectory () const;
  void                          setProjectDirectory (const QString &directory);
  structure::project::Project * project () const;
  structure::project::ProjectUiState *     uiState () const;
  undo::UndoStack *                        undoStack () const;
  zrythm::actions::ArrangerObjectCreator * arrangerObjectCreator () const;
  zrythm::actions::TrackCreator *          trackCreator () const;
  actions::PluginImporter *                pluginImporter () const;
  actions::FileImporter *                  fileImporter () const;
  controllers::TransportController *       transportController () const;

  Q_INVOKABLE actions::ArrangerObjectSelectionOperator *
              createArrangerObjectSelectionOperator (
                QItemSelectionModel * selectionModel) const;

  /**
   * @brief Saves the project to the current project directory.
   *
   * @return A QFutureQmlWrapper that resolves to the saved project path on
   * success.
   * @pre projectDirectory must be set.
   */
  Q_INVOKABLE qquick::QFutureQmlWrapper * save ();

  /**
   * @brief Saves the project to a new directory.
   *
   * Updates projectDirectory and title on success.
   *
   * @param path The new directory path to save to.
   * @return A QFutureQmlWrapper that resolves to the saved project path on
   * success.
   */
  Q_INVOKABLE qquick::QFutureQmlWrapper * saveAs (const QString &path);

  /**
   * @brief Finds a backup directory newer than the main project file.
   *
   * @return The backup path if one exists with a newer timestamp, nullopt
   * otherwise.
   */
  std::optional<fs::path> get_newer_backup ();

Q_SIGNALS:
  void titleChanged (const QString &title);
  void projectDirectoryChanged (const QString &directory);

private:
  utils::AppSettings &app_settings_;

  // Project title and directory
  utils::Utf8String title_;
  fs::path          project_directory_;

  // Core project data
  utils::QObjectUniquePtr<structure::project::Project> project_;

  // Serializable UI state (tool, clipEditor, timeline, snapGrids)
  utils::QObjectUniquePtr<structure::project::ProjectUiState> ui_state_;

  // Undo/redo history
  utils::QObjectUniquePtr<undo::UndoStack> undo_stack_;

  // Quantize options for MIDI editing
  std::unique_ptr<old_dsp::QuantizeOptions> quantize_opts_editor_;
  std::unique_ptr<old_dsp::QuantizeOptions> quantize_opts_timeline_;

  // Action handlers for user operations
  utils::QObjectUniquePtr<actions::ArrangerObjectCreator>
                                                   arranger_object_creator_;
  utils::QObjectUniquePtr<actions::TrackCreator>   track_creator_;
  utils::QObjectUniquePtr<actions::PluginImporter> plugin_importer_;
  utils::QObjectUniquePtr<actions::FileImporter>   file_importer_;
  utils::QObjectUniquePtr<controllers::TransportController> transport_controller_;
};

} // namespace zrythm::gui
