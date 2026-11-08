// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "commands/chord_pad_commands.h"
#include "commands/command.h"
#include "structure/arrangement/chord_object.h"

#include <QPointer>

namespace zrythm::commands
{

/**
 * @brief Edits the ChordDescriptor of one or more ChordObjects.
 *
 * Captures the descriptor state by value at construction time and tracks the
 * ChordObject via QPointer, so undo/redo remain safe even if the model rebuilds
 * or resets during the operation.
 */
class EditChordObjectCommand : public Command
{
public:
  EditChordObjectCommand (
    structure::arrangement::ChordObject * chord_object,
    const dsp::ChordDescriptor           &new_descriptor)
      : Command (QObject::tr ("Edit chord"), EngineRequirement::None),
        object_ (chord_object),
        before_ (
          ChordPadState::from_descriptor (*chord_object->chordDescriptor ())),
        after_ (ChordPadState::from_descriptor (new_descriptor))
  {
  }

  bool merge_with (const Command &other) override
  {
    const auto * o = dynamic_cast<const EditChordObjectCommand *> (&other);
    if (o == nullptr)
      return false;
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
