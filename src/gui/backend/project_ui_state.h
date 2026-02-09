// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "actions/arranger_object_creator.h"
#include "actions/arranger_object_selection_operator.h"
#include "actions/file_importer.h"
#include "actions/plugin_importer.h"
#include "actions/track_creator.h"
#include "gui/backend/arranger_tool.h"
#include "gui/backend/backend/clip_editor.h"
#include "gui/dsp/quantize_options.h"
#include "structure/arrangement/timeline.h"
#include "structure/project/project.h"
#include "undo/undo_stack.h"

#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::gui
{
/**
 * @brief UI-related state of a project.
 *
 * This is a layer on top of Project that provides UI-related functionality.
 */
class ProjectUiState : public QObject
{
  Q_OBJECT
  Q_PROPERTY (
    QString title READ getTitle WRITE setTitle NOTIFY titleChanged FINAL)
  Q_PROPERTY (
    zrythm::structure::arrangement::Timeline * timeline READ timeline CONSTANT
      FINAL)
  Q_PROPERTY (zrythm::gui::backend::ArrangerTool * tool READ tool CONSTANT FINAL)
  Q_PROPERTY (ClipEditor * clipEditor READ clipEditor CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::structure::project::Project * project READ project CONSTANT FINAL)
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
    QString projectDirectory READ projectDirectory WRITE setProjectDirectory
      NOTIFY projectDirectoryChanged FINAL)
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  using QuantizeOptions = old_dsp::QuantizeOptions;

  ProjectUiState (
    utils::QObjectUniquePtr<structure::project::Project> &&project);

  // =========================================================
  // QML interface
  // =========================================================

  QString                                  getTitle () const;
  void                                     setTitle (const QString &title);
  structure::project::Project *            project () const;
  gui::backend::ArrangerTool *             tool () const;
  ClipEditor *                             clipEditor () const;
  structure::arrangement::Timeline *       timeline () const;
  zrythm::actions::ArrangerObjectCreator * arrangerObjectCreator () const;
  zrythm::actions::TrackCreator *          trackCreator () const;
  actions::FileImporter *                  fileImporter () const;
  actions::PluginImporter *                pluginImporter () const;
  undo::UndoStack *                        undoStack () const;

  QString projectDirectory () const;
  void    setProjectDirectory (const QString &directory);

  Q_INVOKABLE actions::ArrangerObjectSelectionOperator *
              createArrangerObjectSelectionOperator (
                QItemSelectionModel * selectionModel) const;

  Q_SIGNAL void titleChanged (const QString &title);
  Q_SIGNAL void projectDirectoryChanged (const QString &directory);

  // =========================================================

  /**
   * Returns the filepath of a backup (directory), if any, if it has a newer
   * timestamp than main project file.
   *
   * Returns nullopt if there were errors or no backup was found.
   */
  std::optional<fs::path> get_newer_backup ();

private:
  static constexpr auto kClipEditorKey = "clipEditor"sv;
  static constexpr auto kTimelineKey = "timeline"sv;
  static constexpr auto kQuantizeOptsTimelineKey = "quantizeOptsTimeline"sv;
  static constexpr auto kQuantizeOptsEditorKey = "quantizeOptsEditor"sv;
  static constexpr auto kUndoStackKey = "undoStack"sv;

private:
  /** Project title. */
  utils::Utf8String title_;

  /** Project directory. */
  fs::path project_directory_;

  utils::QObjectUniquePtr<structure::project::Project> project_;

  /**
   * @brief Currently selected arranger tool.
   */
  utils::QObjectUniquePtr<gui::backend::ArrangerTool> tool_;

  /** Backend for the widget. */
  utils::QObjectUniquePtr<ClipEditor> clip_editor_;

  /** Timeline editor backend. */
  utils::QObjectUniquePtr<structure::arrangement::Timeline> timeline_;

  /** Quantize info for the piano roll. */
  std::unique_ptr<QuantizeOptions> quantize_opts_editor_;

  /** Quantize info for the timeline. */
  std::unique_ptr<QuantizeOptions> quantize_opts_timeline_;

  utils::QObjectUniquePtr<undo::UndoStack> undo_stack_;

  utils::QObjectUniquePtr<actions::ArrangerObjectCreator>
                                                   arranger_object_creator_;
  utils::QObjectUniquePtr<actions::TrackCreator>   track_creator_;
  utils::QObjectUniquePtr<actions::PluginImporter> plugin_importer_;
  utils::QObjectUniquePtr<actions::FileImporter>   file_importer_;
};
}
