// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "commands/command.h"
#include "dsp/chord_descriptor.h"
#include "structure/project/chord_pad_bank.h"

namespace zrythm::commands
{

struct ChordPadState
{
  dsp::MusicalNote                rootNote = dsp::MusicalNote::C;
  dsp::ChordType                  type = dsp::ChordType::None;
  dsp::ChordAccent                accent = dsp::ChordAccent::None;
  int                             inversion = 0;
  std::optional<dsp::MusicalNote> bass = std::nullopt;

  static ChordPadState from_descriptor (const dsp::ChordDescriptor &d)
  {
    return {
      d.rootNote (), d.chordType (), d.chordAccent (), d.inversion (),
      d.hasBass () ? std::optional (d.bassNote ()) : std::nullopt
    };
  }

  void apply_to (dsp::ChordDescriptor &d) const
  {
    d.setRootNote (rootNote);
    d.setChordType (type);
    d.setChordAccent (accent);
    d.setInversion (inversion);
    if (bass.has_value ())
      {
        d.setBassNote (*bass);
      }
    else
      {
        d.setHasBass (false);
      }
  }

  bool operator== (const ChordPadState &other) const = default;
};

class EditChordPadCommand : public Command
{
public:
  EditChordPadCommand (
    structure::project::ChordPadBank &bank,
    int                               index,
    ChordPadState                     after)
      : Command (QObject::tr ("Edit Chord Pad"), EngineRequirement::None),
        bank_ (bank), index_ (index),
        before_ (ChordPadState::from_descriptor (*bank.chordAt (index))),
        after_ (std::move (after))
  {
  }

  void undo () override { before_.apply_to (*bank_.chordAt (index_)); }
  void redo () override { after_.apply_to (*bank_.chordAt (index_)); }

private:
  structure::project::ChordPadBank &bank_;
  int                               index_;
  ChordPadState                     before_;
  ChordPadState                     after_;
};

class AddChordPadCommand : public Command
{
public:
  AddChordPadCommand (structure::project::ChordPadBank &bank, ChordPadState state)
      : Command (QObject::tr ("Add Chord Pad"), EngineRequirement::None),
        bank_ (bank), state_ (std::move (state))
  {
  }

  void redo () override;
  void undo () override;

private:
  structure::project::ChordPadBank &bank_;
  ChordPadState                     state_;
};

class RemoveChordPadCommand : public Command
{
public:
  RemoveChordPadCommand (structure::project::ChordPadBank &bank, int index)
      : Command (QObject::tr ("Remove Chord Pad"), EngineRequirement::None),
        bank_ (bank), index_ (index),
        state_ (ChordPadState::from_descriptor (*bank.chordAt (index)))
  {
  }

  void redo () override;
  void undo () override;

private:
  structure::project::ChordPadBank &bank_;
  int                               index_;
  ChordPadState                     state_;
};

class MoveChordPadCommand : public Command
{
public:
  MoveChordPadCommand (structure::project::ChordPadBank &bank, int from, int to)
      : Command (QObject::tr ("Move Chord Pad"), EngineRequirement::None),
        bank_ (bank), from_ (from), to_ (to)
  {
  }

  void redo () override;
  void undo () override;

private:
  structure::project::ChordPadBank &bank_;
  int                               from_;
  int                               to_;
};

class ReplaceAllChordPadsCommand : public Command
{
public:
  ReplaceAllChordPadsCommand (
    structure::project::ChordPadBank &bank,
    std::vector<ChordPadState>        after,
    const QString                    &text = QObject::tr ("Replace Chord Pads"))
      : Command (text, EngineRequirement::None), bank_ (bank),
        after_ (std::move (after))
  {
    const auto count = bank.rowCount ();
    before_.reserve (count);
    for (int i = 0; i < count; ++i)
      {
        before_.push_back (ChordPadState::from_descriptor (*bank.chordAt (i)));
      }
  }

  void redo () override;
  void undo () override;

private:
  static void apply_states (
    structure::project::ChordPadBank &bank,
    const std::vector<ChordPadState> &states);

  structure::project::ChordPadBank &bank_;
  std::vector<ChordPadState>        before_;
  std::vector<ChordPadState>        after_;
};

} // namespace zrythm::commands
