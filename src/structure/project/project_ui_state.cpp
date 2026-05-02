// SPDX-FileCopyrightText: © 2018-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/project/project_ui_state.h"
#include "utils/app_settings.h"

namespace zrythm::structure::project
{

ProjectUiState::ProjectUiState (
  Project            &project,
  utils::AppSettings &app_settings)
    : project_ (project), app_settings_ (app_settings),
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

dsp::AudioInputSelection *
ProjectUiState::audioInputSelectionForTrack (
  const structure::tracks::Track * track)
{
  if (track == nullptr)
    return nullptr;

  const auto uuid = track->get_uuid ();
  auto       it = audio_input_selections_.find (uuid);
  if (it != audio_input_selections_.end ())
    return it->second.get ();

  auto   sel = utils::make_qobject_unique<dsp::AudioInputSelection> (this);
  auto * raw = sel.get ();
  audio_input_selections_.emplace (uuid, std::move (sel));
  return raw;
}

void
to_json (nlohmann::json &j, const ProjectUiState &p)
{
  j = nlohmann::json{
    { ProjectUiState::kSnapGridTimelineKey, *p.snap_grid_timeline_ },
    { ProjectUiState::kSnapGridEditorKey,   *p.snap_grid_editor_   }
  };

  nlohmann::json selections_json = nlohmann::json::array ();
  for (const auto &[uuid, sel] : p.audio_input_selections_)
    {
      if (p.project_.get_track_registry ().contains (uuid))
        {
          selections_json.push_back (nlohmann::json::array ({ uuid, *sel }));
        }
    }
  if (!selections_json.empty ())
    {
      j[ProjectUiState::kAudioInputSelectionsKey] = selections_json;
    }
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
  if (j.contains (ProjectUiState::kAudioInputSelectionsKey))
    {
      p.audio_input_selections_.clear ();
      for (const auto &entry : j.at (ProjectUiState::kAudioInputSelectionsKey))
        {
          structure::tracks::Track::Uuid uuid;
          entry[0].get_to (uuid);
          auto sel = utils::make_qobject_unique<dsp::AudioInputSelection> (&p);
          from_json (entry[1], *sel);
          p.audio_input_selections_.emplace (uuid, std::move (sel));
        }
    }
}

} // namespace zrythm::structure::project
