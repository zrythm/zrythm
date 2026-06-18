// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "commands/chord_pad_commands.h"
#include "dsp/chord_descriptor.h"

namespace zrythm::commands
{

void
AddChordPadCommand::redo ()
{
  bank_.addChord (
    state_.rootNote, state_.type, state_.accent, state_.inversion, state_.bass);
}

void
AddChordPadCommand::undo ()
{
  bank_.removeChord (bank_.rowCount () - 1);
}

void
RemoveChordPadCommand::redo ()
{
  bank_.removeChord (index_);
}

void
RemoveChordPadCommand::undo ()
{
  bank_.insertChord (
    index_, state_.rootNote, state_.type, state_.accent, state_.inversion,
    state_.bass);
}

void
MoveChordPadCommand::redo ()
{
  bank_.moveChord (from_, to_);
}

void
MoveChordPadCommand::undo ()
{
  bank_.moveChord (to_, from_);
}

void
ReplaceAllChordPadsCommand::redo ()
{
  apply_states (bank_, after_);
}

void
ReplaceAllChordPadsCommand::undo ()
{
  apply_states (bank_, before_);
}

void
ReplaceAllChordPadsCommand::apply_states (
  structure::project::ChordPadBank &bank,
  const std::vector<ChordPadState> &states)
{
  while (bank.rowCount () > 0)
    bank.removeChord (0);
  for (const auto &s : states)
    {
      bank.addChord (s.rootNote, s.type, s.accent, s.inversion, s.bass);
    }
}

} // namespace zrythm::commands
