// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <string_view>

#include "dsp/snap_grid.h"
#include "structure/arrangement/timeline.h"
#include "structure/project/arranger_tool.h"
#include "structure/project/clip_editor.h"
#include "structure/project/project.h"

#include <QtQmlIntegration/qqmlintegration.h>

#include <nlohmann/json_fwd.hpp>

namespace zrythm::structure::project
{

/**
 * @brief Serializable UI state for a project.
 *
 * Contains UI-related state that should be persisted between sessions:
 * - Currently selected arranger tool
 * - Clip editor state (selected region, etc.)
 * - Timeline state
 * - Snap/grid settings for timeline and editor
 *
 * This class does NOT own the Project; it receives a reference to access
 * necessary components (tempo map, registries) during construction.
 */
class ProjectUiState : public QObject
{
  Q_OBJECT
  Q_PROPERTY (
    zrythm::structure::arrangement::Timeline * timeline READ timeline CONSTANT
      FINAL)
  Q_PROPERTY (
    zrythm::structure::project::ArrangerTool * tool READ tool CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::structure::project::ClipEditor * clipEditor READ clipEditor CONSTANT
      FINAL)
  Q_PROPERTY (
    zrythm::dsp::SnapGrid * snapGridTimeline READ snapGridTimeline CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::dsp::SnapGrid * snapGridEditor READ snapGridEditor CONSTANT FINAL)
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  /**
   * @brief Constructs ProjectUiState with references from the given Project.
   *
   * @param project The Project to extract tempo map and registries from.
   * @param app_settings Application settings for snap grid defaults.
   */
  ProjectUiState (Project &project, utils::AppSettings &app_settings);

  structure::project::ArrangerTool * tool () const;
  structure::project::ClipEditor *   clipEditor () const;
  structure::arrangement::Timeline * timeline () const;
  dsp::SnapGrid *                    snapGridTimeline () const;
  dsp::SnapGrid *                    snapGridEditor () const;

private:
  static constexpr auto kSnapGridTimelineKey = "snapGridTimeline"sv;
  static constexpr auto kSnapGridEditorKey = "snapGridEditor"sv;
  friend void           to_json (nlohmann::json &j, const ProjectUiState &p);
  friend void           from_json (const nlohmann::json &j, ProjectUiState &p);

private:
  utils::AppSettings &app_settings_;

  /** Currently selected arranger tool. */
  utils::QObjectUniquePtr<structure::project::ArrangerTool> tool_;

  /** Clip editor state (selected region, etc.). */
  utils::QObjectUniquePtr<structure::project::ClipEditor> clip_editor_;

  /** Timeline editor state. */
  utils::QObjectUniquePtr<structure::arrangement::Timeline> timeline_;

  /** Snap/grid settings for the timeline. */
  utils::QObjectUniquePtr<dsp::SnapGrid> snap_grid_timeline_;

  /** Snap/grid settings for the editor (piano roll). */
  utils::QObjectUniquePtr<dsp::SnapGrid> snap_grid_editor_;
};

} // namespace zrythm::structure::project
