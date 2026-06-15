// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "commands/chord_pad_commands.h"
#include "structure/arrangement/chord_object.h"

#include <QPointer>
#include <QUndoCommand>

namespace zrythm::commands
{

/**
 * @brief Edits the ChordDescriptor of one or more ChordObjects.
 *
 * Captures the descriptor state by value at construction time and tracks the
 * ChordObject via QPointer, so undo/redo remain safe even if the model rebuilds
 * or resets during the operation.
 */
class EditChordObjectCommand : public QUndoCommand
{
public:
  EditChordObjectCommand (
    structure::arrangement::ChordObject * chord_object,
    const dsp::ChordDescriptor           &new_descriptor)
      : object_ (chord_object),
        before_ (
          ChordPadState::from_descriptor (*chord_object->chordDescriptor ())),
        after_ (ChordPadState::from_descriptor (new_descriptor))
  {
    setText (QObject::tr ("Edit chord"));
  }

  int id () const override { return 1762954987; }

  bool mergeWith (const QUndoCommand * other) override
  {
    if (other->id () != id ())
      return false;
    const auto * o = static_cast<const EditChordObjectCommand *> (other);
    if (object_ == nullptr || o->object_ == nullptr)
      return false;
    if (object_.data () != o->object_.data ())
      return false;
    after_ = o->after_;
    return true;
  }

  void undo () override
  {
    if (object_)
      before_.apply_to (*object_->chordDescriptor ());
  }

  void redo () override
  {
    if (object_)
      after_.apply_to (*object_->chordDescriptor ());
  }

private:
  QPointer<structure::arrangement::ChordObject> object_;
  ChordPadState                                 before_;
  ChordPadState                                 after_;
};

} // namespace zrythm::commands
