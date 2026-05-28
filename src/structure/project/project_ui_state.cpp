// SPDX-FileCopyrightText: © 2018-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/project/project_ui_state.h"
#include "utils/app_settings.h"
#include "utils/logger.h"
#include "utils/registry_utils.h"

namespace zrythm::structure::project
{

ProjectUiState::ProjectUiState (
  Project            &project,
  utils::AppSettings &app_settings)
    : project_ (project), app_settings_ (app_settings),
      tool_ (new structure::project::ArrangerTool (this)),
      clip_editor_ (
        utils::make_qobject_unique<
          structure::project::ClipEditor> (project.get_registry (), this)),
      timeline_ (
        utils::make_qobject_unique<
          structure::arrangement::Timeline> (project.get_registry (), this)),
      snap_grid_timeline_ (
        utils::make_qobject_unique<dsp::SnapGrid> (
          project.tempo_map (),
          dsp::notes::NoteLength::Bar,
          [this] {
            return app_settings_.timelineLastCreatedObjectLengthInTicks ();
          },
          this)),
      snap_grid_editor_ (
        utils::make_qobject_unique<dsp::SnapGrid> (
          project.tempo_map (),
          dsp::notes::NoteLength::Note_1_8,
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
ProjectUiState::get_or_create_audio_input_selection (
  const structure::tracks::Track::Uuid &uuid)
{
  auto it = audio_input_selections_.find (uuid);
  if (it != audio_input_selections_.end ())
    return it->second.get ();

  auto   sel = utils::make_qobject_unique<dsp::AudioInputSelection> (this);
  auto * raw = sel.get ();

  auto emit_changed = [this] { Q_EMIT audioInputSelectionChanged (); };
  QObject::connect (
    raw, &dsp::AudioInputSelection::deviceNameChanged, this, emit_changed);
  QObject::connect (
    raw, &dsp::AudioInputSelection::firstChannelChanged, this, emit_changed);
  QObject::connect (
    raw, &dsp::AudioInputSelection::stereoChanged, this, emit_changed);

  audio_input_selections_.emplace (uuid, std::move (sel));
  return raw;
}

dsp::AudioInputSelection *
ProjectUiState::find_audio_input_selection (
  const structure::tracks::Track::Uuid &uuid) const
{
  auto it = audio_input_selections_.find (uuid);
  if (it != audio_input_selections_.end ())
    return it->second.get ();
  return nullptr;
}

dsp::AudioInputSelection *
ProjectUiState::audioInputSelectionForTrack (
  const structure::tracks::Track * track)
{
  if (track == nullptr)
    return nullptr;

  return get_or_create_audio_input_selection (track->get_uuid ());
}

dsp::MidiInputSelection *
ProjectUiState::get_or_create_midi_input_selection (
  const structure::tracks::Track::Uuid &uuid)
{
  auto it = midi_input_selections_.find (uuid);
  if (it != midi_input_selections_.end ())
    return it->second.get ();

  auto   sel = utils::make_qobject_unique<dsp::MidiInputSelection> (this);
  auto * raw = sel.get ();

  QObject::connect (
    raw, &dsp::MidiInputSelection::deviceIdentifierChanged, this,
    &ProjectUiState::midiInputDeviceChanged);

  midi_input_selections_.emplace (uuid, std::move (sel));
  return raw;
}

dsp::MidiInputSelection *
ProjectUiState::find_midi_input_selection (
  const structure::tracks::Track::Uuid &uuid) const
{
  auto it = midi_input_selections_.find (uuid);
  if (it != midi_input_selections_.end ())
    return it->second.get ();
  return nullptr;
}

dsp::MidiInputSelection *
ProjectUiState::midiInputSelectionForTrack (
  const structure::tracks::Track * track)
{
  if (track == nullptr)
    return nullptr;

  return get_or_create_midi_input_selection (track->get_uuid ());
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
      if (utils::contains (p.project_.get_registry (), uuid))
        {
          selections_json.push_back (nlohmann::json::array ({ uuid, *sel }));
        }
      else
        {
          z_debug ("pruning audio input selection for removed track {}", uuid);
        }
    }
  if (!selections_json.empty ())
    {
      j[ProjectUiState::kAudioInputSelectionsKey] = selections_json;
    }

  nlohmann::json midi_selections_json = nlohmann::json::array ();
  for (const auto &[uuid, sel] : p.midi_input_selections_)
    {
      if (utils::contains (p.project_.get_registry (), uuid))
        {
          midi_selections_json.push_back (
            nlohmann::json::array ({ uuid, *sel }));
        }
      else
        {
          z_debug ("pruning midi input selection for removed track {}", uuid);
        }
    }
  if (!midi_selections_json.empty ())
    {
      j[ProjectUiState::kMidiInputSelectionsKey] = midi_selections_json;
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
          auto * sel = p.get_or_create_audio_input_selection (uuid);
          from_json (entry[1], *sel);
        }
    }
  if (j.contains (ProjectUiState::kMidiInputSelectionsKey))
    {
      p.midi_input_selections_.clear ();
      for (const auto &entry : j.at (ProjectUiState::kMidiInputSelectionsKey))
        {
          if (!entry.is_array () || entry.size () < 2)
            {
              z_warning ("Skipping malformed MIDI input selection entry");
              continue;
            }
          structure::tracks::Track::Uuid uuid;
          entry[0].get_to (uuid);
          auto * sel = p.get_or_create_midi_input_selection (uuid);
          from_json (entry[1], *sel);
        }
    }
}

} // namespace zrythm::structure::project
