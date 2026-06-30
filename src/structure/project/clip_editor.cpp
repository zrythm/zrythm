// SPDX-FileCopyrightText: © 2019-2021, 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/project/clip_editor.h"
#include "structure/tracks/track_all.h"
#include "utils/registry_utils.h"

namespace zrythm::structure::project
{

ClipEditor::ClipEditor (utils::IObjectRegistry &reg, QObject * parent)
    : QObject (parent), object_registry_ (reg),
      midi_editor_ (utils::make_qobject_unique<MidiEditor> (this)),
      audio_clip_editor_ (utils::make_qobject_unique<AudioClipEditor> (this)),
      automation_editor_ (utils::make_qobject_unique<AutomationEditor> (this)),
      chord_editor_ (utils::make_qobject_unique<ChordEditor> (this))
{
}

void
ClipEditor::init_loaded ()
{
}

arrangement::Clip *
ClipEditor::clip () const
{
  if (has_clip ())
    {
      auto * obj = get_clip_and_track ()->first;
      return qobject_cast<arrangement::Clip *> (obj);
    }

  return nullptr;
}
QVariant
ClipEditor::track () const
{
  if (has_clip ())
    {
      return QVariant::fromValue (get_clip_and_track ()->second);
    }

  return {};
}

void
ClipEditor::setClip (QVariant clip, QVariant track)
{
  auto * r = dynamic_cast<ArrangerObject *> (clip.value<QObject *> ());
  auto * t =
    dynamic_cast<structure::tracks::Track *> (track.value<QObject *> ());
  set_clip (r->get_uuid (), t->get_uuid ());
}

void
ClipEditor::set_clip (ArrangerObject::Uuid clip_id, TrackUuid track_id)
{
  if (clip_id_.has_value () && clip_id_.value ().first == clip_id)
    return;

  clip_id_ = { clip_id, track_id };
  Q_EMIT clipObjectChanged ();
}

void
ClipEditor::unsetClip ()
{
  if (!clip_id_)
    return;

  clip_id_.reset ();
  Q_EMIT clipObjectChanged ();
}

auto
ClipEditor::get_clip_and_track () const
  -> std::optional<std::pair<ArrangerObject *, Track *>>
{
  auto &clip_obj =
    utils::get_typed<ArrangerObject> (object_registry_, clip_id_->first);
  auto &track_obj = utils::get_typed<Track> (object_registry_, clip_id_->second);
  return std::make_pair (&clip_obj, &track_obj);
}

void
ClipEditor::init ()
{
  midi_editor_->init ();
  // the rest of the editors are initialized in their respective classes
}

void
init_from (
  ClipEditor            &obj,
  const ClipEditor      &other,
  utils::ObjectCloneType clone_type)

{
  obj.clip_id_ = other.clip_id_;
  init_from (*obj.audio_clip_editor_, *other.audio_clip_editor_, clone_type);
  init_from (*obj.automation_editor_, *other.automation_editor_, clone_type);
  init_from (*obj.chord_editor_, *other.chord_editor_, clone_type);
  init_from (*obj.midi_editor_, *other.midi_editor_, clone_type);
}

void
to_json (nlohmann::json &j, const ClipEditor &editor)
{
  j[ClipEditor::kClipIdKey] = editor.clip_id_;
  j[ClipEditor::kMidiEditorKey] = editor.midi_editor_;
  j[ClipEditor::kAutomationEditorKey] = editor.automation_editor_;
  j[ClipEditor::kChordEditorKey] = editor.chord_editor_;
  j[ClipEditor::kAudioClipEditorKey] = editor.audio_clip_editor_;
}

void
from_json (const nlohmann::json &j, ClipEditor &editor)
{
  j.at (ClipEditor::kClipIdKey).get_to (editor.clip_id_);
  j.at (ClipEditor::kMidiEditorKey).get_to (*editor.midi_editor_);
  j.at (ClipEditor::kAutomationEditorKey).get_to (*editor.automation_editor_);
  j.at (ClipEditor::kChordEditorKey).get_to (*editor.chord_editor_);
  j.at (ClipEditor::kAudioClipEditorKey).get_to (*editor.audio_clip_editor_);
}

} // namespace zrythm::structure::project
