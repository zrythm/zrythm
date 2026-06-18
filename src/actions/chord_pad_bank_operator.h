// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "commands/chord_pad_commands.h"
#include "dsp/chord_preset.h"
#include "structure/project/chord_pad_bank.h"
#include "undo/undo_stack.h"

#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::actions
{

class ChordPadBankOperator : public QObject
{
  Q_OBJECT
  Q_PROPERTY (
    zrythm::structure::project::ChordPadBank * chordPadBank READ chordPadBank
      WRITE setChordPadBank NOTIFY chordPadBankChanged)
  Q_PROPERTY (
    zrythm::undo::UndoStack * undoStack READ undoStack WRITE setUndoStack NOTIFY
      undoStackChanged)
  QML_ELEMENT

public:
  explicit ChordPadBankOperator (QObject * parent = nullptr) : QObject (parent)
  {
  }

  structure::project::ChordPadBank * chordPadBank () const { return bank_; }
  void setChordPadBank (structure::project::ChordPadBank * bank)
  {
    if (bank_ != bank)
      {
        bank_ = bank;
        Q_EMIT chordPadBankChanged ();
      }
  }
  Q_SIGNAL void chordPadBankChanged ();

  undo::UndoStack * undoStack () const { return undo_stack_; }
  void              setUndoStack (undo::UndoStack * stack)
  {
    if (undo_stack_ != stack)
      {
        undo_stack_ = stack;
        Q_EMIT undoStackChanged ();
      }
  }
  Q_SIGNAL void undoStackChanged ();

  Q_INVOKABLE void editPad (
    int                      index,
    zrythm::dsp::MusicalNote rootNote,
    zrythm::dsp::ChordType   type,
    zrythm::dsp::ChordAccent accent,
    int                      inversion,
    bool                     hasBass,
    zrythm::dsp::MusicalNote bassNote);
  Q_INVOKABLE void addPad (
    zrythm::dsp::MusicalNote rootNote,
    zrythm::dsp::ChordType   type,
    zrythm::dsp::ChordAccent accent = zrythm::dsp::ChordAccent::None,
    int                      inversion = 0,
    bool                     hasBass = false,
    zrythm::dsp::MusicalNote bassNote = zrythm::dsp::MusicalNote::C);
  Q_INVOKABLE void removePad (int index);
  Q_INVOKABLE void movePad (int from, int to);
  Q_INVOKABLE void transposePads (bool up);
  Q_INVOKABLE void applyPresetFromScale (
    zrythm::dsp::MusicalScale::ScaleType scale,
    zrythm::dsp::MusicalNote             rootNote);
  Q_INVOKABLE void applyPreset (ChordPreset * preset);

private:
  structure::project::ChordPadBank * bank_{};
  undo::UndoStack *                  undo_stack_{};
};

}
