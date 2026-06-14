// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cassert>

#include "actions/chord_pad_bank_operator.h"
#include "dsp/chord_descriptor.h"
#include "dsp/musical_scale.h"

namespace zrythm::actions
{

void
ChordPadBankOperator::editPad (
  int              index,
  dsp::MusicalNote rootNote,
  dsp::ChordType   type,
  dsp::ChordAccent accent,
  int              inversion,
  bool             hasBass,
  dsp::MusicalNote bassNote)
{
  assert (bank_ != nullptr);
  assert (undo_stack_ != nullptr);
  commands::ChordPadState after{
    rootNote, type, accent, inversion,
    hasBass ? std::optional (bassNote) : std::nullopt
  };
  undo_stack_->push (
    new commands::EditChordPadCommand (*bank_, index, std::move (after)));
}

void
ChordPadBankOperator::addPad (
  dsp::MusicalNote rootNote,
  dsp::ChordType   type,
  dsp::ChordAccent accent,
  int              inversion,
  bool             hasBass,
  dsp::MusicalNote bassNote)
{
  assert (bank_ != nullptr);
  assert (undo_stack_ != nullptr);
  commands::ChordPadState state{
    rootNote, type, accent, inversion,
    hasBass ? std::optional (bassNote) : std::nullopt
  };
  undo_stack_->push (
    new commands::AddChordPadCommand (*bank_, std::move (state)));
}

void
ChordPadBankOperator::removePad (int index)
{
  assert (bank_ != nullptr);
  assert (undo_stack_ != nullptr);
  undo_stack_->push (new commands::RemoveChordPadCommand (*bank_, index));
}

void
ChordPadBankOperator::movePad (int from, int to)
{
  assert (bank_ != nullptr);
  assert (undo_stack_ != nullptr);
  undo_stack_->push (new commands::MoveChordPadCommand (*bank_, from, to));
}

void
ChordPadBankOperator::transposePads (bool up)
{
  assert (bank_ != nullptr);
  assert (undo_stack_ != nullptr);
  const auto count = bank_->rowCount ();
  if (count == 0)
    return;

  std::vector<commands::ChordPadState> after;
  after.reserve (count);
  for (int i = 0; i < count; ++i)
    {
      auto state =
        commands::ChordPadState::from_descriptor (*bank_->chordAt (i));
      state.rootNote = dsp::chords::transpose_note (state.rootNote, up);
      if (state.bass.has_value ())
        state.bass = dsp::chords::transpose_note (*state.bass, up);
      after.push_back (std::move (state));
    }

  undo_stack_->push (new commands::ReplaceAllChordPadsCommand (
    *bank_, std::move (after), QObject::tr ("Transpose Chord Pads")));
}

void
ChordPadBankOperator::applyPresetFromScale (
  dsp::MusicalScale::ScaleType scale,
  dsp::MusicalNote             rootNote)
{
  assert (bank_ != nullptr);
  assert (undo_stack_ != nullptr);
  const auto triads = dsp::MusicalScale::get_diatonic_triads (scale, rootNote);

  std::vector<commands::ChordPadState> after;
  for (const auto &triad : triads)
    {
      after.push_back (
        commands::ChordPadState{
          triad.root_note, triad.chord_type, dsp::ChordAccent::None, 0,
          std::nullopt });
    }

  undo_stack_->push (new commands::ReplaceAllChordPadsCommand (
    *bank_, std::move (after), QObject::tr ("Apply Scale Preset")));
}

void
ChordPadBankOperator::applyPreset (ChordPreset * preset)
{
  assert (bank_ != nullptr);
  assert (undo_stack_ != nullptr);
  if (preset == nullptr)
    return;

  std::vector<commands::ChordPadState> after;
  after.reserve (preset->descriptors ().size ());
  for (const auto &descr : preset->descriptors ())
    {
      after.push_back (commands::ChordPadState::from_descriptor (*descr));
    }

  undo_stack_->push (new commands::ReplaceAllChordPadsCommand (
    *bank_, std::move (after), QObject::tr ("Apply Chord Preset")));
}

}
