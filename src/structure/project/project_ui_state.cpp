// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/project/project_ui_state.h"

namespace zrythm::structure::project
{

ProjectUiState::ProjectUiState (
  Project            &project,
  utils::AppSettings &app_settings)
    : app_settings_ (app_settings),
      tool_ (new structure::project::ArrangerTool (this)),
      clip_editor_ (
        utils::make_qobject_unique<structure::project::ClipEditor> (
          project.get_arranger_object_registry (),
          [&project] (const auto &id) {
            return project.get_track_registry ().find_by_id_or_throw (id);
          },
          this)),
      timeline_ (
        utils::make_qobject_unique<structure::arrangement::Timeline> (this)),
      snap_grid_timeline_ (
        utils::make_qobject_unique<dsp::SnapGrid> (
          project.tempo_map (),
          utils::NoteLength::Bar,
          [this] {
            return app_settings_.timelineLastCreatedObjectLengthInTicks ();
          },
          this)),
      snap_grid_editor_ (
        utils::make_qobject_unique<dsp::SnapGrid> (
          project.tempo_map (),
          utils::NoteLength::Note_1_8,
          [this] {
            return app_settings_.editorLastCreatedObjectLengthInTicks ();
          },
          this))
{
}

structure::project::ArrangerTool *
ProjectUiState::tool () const
{
  return tool_.get ();
}

structure::project::ClipEditor *
ProjectUiState::clipEditor () const
{
  return clip_editor_.get ();
}

structure::arrangement::Timeline *
ProjectUiState::timeline () const
{
  return timeline_.get ();
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

void
to_json (nlohmann::json &j, const ProjectUiState &p)
{
  j = nlohmann::json{
    { ProjectUiState::kSnapGridTimelineKey, *p.snap_grid_timeline_ },
    { ProjectUiState::kSnapGridEditorKey,   *p.snap_grid_editor_   }
  };
}

void
from_json (const nlohmann::json &j, ProjectUiState &p)
{
  if (j.contains (ProjectUiState::kSnapGridTimelineKey))
    {
      from_json (
        j.at (ProjectUiState::kSnapGridTimelineKey), *p.snap_grid_timeline_);
    }
  if (j.contains (ProjectUiState::kSnapGridEditorKey))
    {
      from_json (
        j.at (ProjectUiState::kSnapGridEditorKey), *p.snap_grid_editor_);
    }
}

} // namespace zrythm::structure::project
