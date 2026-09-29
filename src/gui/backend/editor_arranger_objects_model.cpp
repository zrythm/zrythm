// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/editor_arranger_objects_model.h"
#include "structure/arrangement/arranger_object_list_model.h"
#include "structure/arrangement/automation_clip.h"
#include "structure/arrangement/chord_clip.h"
#include "structure/arrangement/midi_clip.h"
#include "structure/project/clip_editor.h"

namespace zrythm::gui
{

namespace
{

structure::arrangement::ArrangerObjectListModel *
clip_objects_model_of (const structure::arrangement::Clip * clip)
{
  if (clip == nullptr)
    return nullptr;
  if (
    const auto * midi_clip =
      dynamic_cast<const structure::arrangement::MidiClip *> (clip))
    {
      return midi_clip->midiNotes ();
    }
  if (
    const auto * chord_clip =
      dynamic_cast<const structure::arrangement::ChordClip *> (clip))
    {
      return chord_clip->chordObjects ();
    }
  if (
    const auto * automation_clip =
      dynamic_cast<const structure::arrangement::AutomationClip *> (clip))
    {
      return automation_clip->automationPoints ();
    }
  return nullptr;
}

} // namespace

EditorArrangerObjectsModel::EditorArrangerObjectsModel (QObject * parent)
    : UnifiedProxyModel (parent)
{
}

structure::project::ClipEditor *
EditorArrangerObjectsModel::clipEditor () const
{
  return clip_editor_;
}

void
EditorArrangerObjectsModel::setClipEditor (
  structure::project::ClipEditor * editor)
{
  if (clip_editor_ == editor)
    return;

  if (clip_editor_ != nullptr)
    {
      QObject::disconnect (clip_editor_, nullptr, this, nullptr);
    }
  clip_editor_ = editor;
  Q_EMIT clipEditorChanged ();
  if (clip_editor_ != nullptr)
    {
      QObject::connect (
        clip_editor_, &structure::project::ClipEditor::clipObjectChanged, this,
        &EditorArrangerObjectsModel::sync_with_open_clip);
    }

  sync_with_open_clip ();
}

void
EditorArrangerObjectsModel::sync_with_open_clip ()
{
  auto * clip = clip_editor_ != nullptr ? clip_editor_->clip () : nullptr;
  auto * model = clip_objects_model_of (clip);

  if (model == source_)
    return;

  if (source_ != nullptr)
    {
      QObject::disconnect (
        source_,
        &structure::arrangement::ArrangerObjectListModel::aboutToBeDestroyed,
        this, nullptr);
      removeSourceModel (source_);
      source_ = nullptr;
    }

  if (model == nullptr)
    return;

  source_ = model;
  QObject::connect (
    model, &structure::arrangement::ArrangerObjectListModel::aboutToBeDestroyed,
    this, [this, model] () {
      if (source_ != model)
        return;
      source_ = nullptr;
      removeSourceModel (model);
    });
  addSourceModel (model);
}

} // namespace zrythm::gui
