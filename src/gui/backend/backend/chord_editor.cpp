// SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

# include "gui/dsp/chord_descriptor.h"
#include "utils/logger.h"
#include "utils/rt_thread_id.h"
#include "gui/backend/backend/actions/chord_action.h"
#include "gui/backend/backend/chord_editor.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/settings/chord_preset.h"
#include "gui/backend/backend/zrythm.h"

void
ChordEditor::init ()
{
  chords_.resize (CHORD_EDITOR_NUM_CHORDS);
  for (int i = 0; i < CHORD_EDITOR_NUM_CHORDS; i++)
    {
      chords_[i] = ChordDescriptor (
        (MusicalNote) ((int) MusicalNote::C + i), true,
        (MusicalNote) ((int) MusicalNote::C + i), ChordType::Major,
        ChordAccent::None, 0);
    }
}

void
ChordEditor::apply_single_chord (
  const ChordDescriptor &chord,
  const int              idx,
  bool                   undoable)
{
  if (undoable)
    {
      try
        {
          UNDO_MANAGER->perform (new gui::actions::ChordAction (chord, idx));
        }
      catch (const ZrythmException &e)
        {
          e.handle ("Failed to apply single chord");
          return;
        }
    }
  else
    {
      chords_[idx] = chord;
      chords_[idx].update_notes ();
    }
}

void
ChordEditor::apply_chords (
  const std::vector<ChordDescriptor> &chords,
  bool                                undoable)
{
  if (undoable)
    {
      try
        {
          UNDO_MANAGER->perform (
            new gui::actions::ChordAction (chords_, chords));
        }
      catch (const ZrythmException &e)
        {
          e.handle ("Failed to apply chords");
          return;
        }
    }
  else
    {
      for (int i = 0; i < CHORD_EDITOR_NUM_CHORDS; i++)
        {
          chords_[i] = chords[i];
          chords_[i].update_notes ();
        }
    }

  // /* EVENTS_PUSH (EventType::ET_CHORDS_UPDATED, nullptr); */
}

void
ChordEditor::apply_preset (const ChordPreset &pset, bool undoable)
{
  apply_chords (pset.descr_, undoable);
}

void
ChordEditor::apply_preset_from_scale (
  MusicalScale::Type scale,
  MusicalNote        root_note,
  bool               undoable)
{
  z_debug (
    "applying preset from scale %s, root note %s",
    MusicalScale::type_to_string (scale),
    ChordDescriptor::note_to_string (root_note));
  const auto   triads = MusicalScale::get_triad_types_for_type (scale, true);
  const bool * notes = MusicalScale::get_notes_for_type (scale, true);
  int          cur_chord = 0;
  std::vector<ChordDescriptor> new_chords;
  new_chords.reserve (CHORD_EDITOR_NUM_CHORDS);
  for (int i = 0; i < 12; i++)
    {
      /* skip notes not in scale */
      if (!notes[i])
        continue;

      new_chords.emplace_back (
        (MusicalNote) ((int) root_note + i), false,
        (MusicalNote) ((int) root_note + i), triads.at (cur_chord),
        ChordAccent::None, 0);
    }

  /* fill remaining chords with default data */
  while (cur_chord < CHORD_EDITOR_NUM_CHORDS)
    {
      new_chords.push_back (ChordDescriptor (
        MusicalNote::C, false, MusicalNote::C, ChordType::None,
        ChordAccent::None, 0));
    }

  apply_chords (new_chords, undoable);
}

void
ChordEditor::transpose_chords (bool up, bool undoable)
{
  std::vector<ChordDescriptor> new_chords;
  new_chords.reserve (CHORD_EDITOR_NUM_CHORDS);
  for (int i = 0; i < CHORD_EDITOR_NUM_CHORDS; ++i)
    {
      new_chords.push_back (chords_[i]);
      ChordDescriptor &descr = new_chords.back ();

      int add = (up ? 1 : 11);
      descr.root_note_ = (MusicalNote) ((int) descr.root_note_ + add);
      descr.bass_note_ = (MusicalNote) ((int) descr.bass_note_ + add);

      descr.root_note_ = (MusicalNote) ((int) descr.root_note_ % 12);
      descr.bass_note_ = (MusicalNote) ((int) descr.bass_note_ % 12);
    }

  apply_chords (new_chords, undoable);
}

ChordDescriptor *
ChordEditor::get_chord_from_note_number (midi_byte_t note_number)
{
  if (note_number < 60 || note_number >= 72)
    return nullptr;

  return &chords_[note_number - 60];
}

int
ChordEditor::get_chord_index (const ChordDescriptor &chord) const
{
  for (int i = 0; i < CHORD_EDITOR_NUM_CHORDS; i++)
    {
      if (chords_[i] == chord)
        return i;
    }

  z_return_val_if_reached (-1);
}
