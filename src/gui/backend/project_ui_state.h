// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "gui/backend/arranger_tool.h"
#include "gui/backend/backend/clip_editor.h"
#include "gui/backend/backend/project.h"
#include "structure/arrangement/timeline.h"

#include <QtQmlIntegration>

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
    zrythm::structure::arrangement::Timeline * timeline READ timeline CONSTANT
      FINAL)
  Q_PROPERTY (zrythm::gui::backend::ArrangerTool * tool READ tool CONSTANT FINAL)
  Q_PROPERTY (ClipEditor * clipEditor READ clipEditor CONSTANT FINAL)
  Q_PROPERTY (Project * project READ project CONSTANT FINAL)
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  ProjectUiState (utils::QObjectUniquePtr<Project> &&project);

  // =========================================================
  // QML interface
  // =========================================================

  Project *                          project () const;
  gui::backend::ArrangerTool *       tool () const;
  ClipEditor *                       clipEditor () const;
  structure::arrangement::Timeline * timeline () const;

  // =========================================================

private:
  static constexpr auto kClipEditorKey = "clipEditor"sv;
  static constexpr auto kTimelineKey = "timeline"sv;

private:
  utils::QObjectUniquePtr<Project> project_;

  /**
   * @brief Currently selected arranger tool.
   */
  utils::QObjectUniquePtr<gui::backend::ArrangerTool> tool_;

  /** Backend for the widget. */
  utils::QObjectUniquePtr<ClipEditor> clip_editor_;

  /** Timeline editor backend. */
  utils::QObjectUniquePtr<structure::arrangement::Timeline> timeline_;
};
}
