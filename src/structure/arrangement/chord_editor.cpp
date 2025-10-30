// SPDX-FileCopyrightText: © 2019-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/chord_editor.h"

namespace zrythm::structure::arrangement
{
ChordEditor::ChordEditor (QObject * parent)
    : QObject (parent),
      editor_settings_ (utils::make_qobject_unique<EditorSettings> (this))
{
}

void
ChordEditor::init ()
{
  for (const auto index : std::views::iota (0, CHORD_EDITOR_NUM_CHORDS))
    {
      add_chord_descriptor (
        { (MusicalNote) ((int) MusicalNote::C + index), true,
          (MusicalNote) ((int) MusicalNote::C + index), ChordType::Major,
          ChordAccent::None, 0 });
    }
}

void
ChordEditor::apply_single_chord (
  const ChordDescriptor &chord,
  const int              idx,
  bool                   undoable)
{
#if 0
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
      replace_chord_descriptor (idx, ChordDescriptor{ chord });
    }
#endif
}

void
ChordEditor::apply_chords (
  const std::vector<ChordDescriptor> &chords,
  bool                                undoable)
{
#if 0
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
      for (const auto i : std::views::iota (0, CHORD_EDITOR_NUM_CHORDS))
        {
          replace_chord_descriptor (i, ChordDescriptor{ chords[i] });
        }
    }

  // /* EVENTS_PUSH (EventType::ET_CHORDS_UPDATED, nullptr); */
#endif
}

void
ChordEditor::apply_preset_from_scale (
  MusicalScale::ScaleType scale,
  MusicalNote             root_note,
  bool                    undoable)
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
      new_chords.emplace_back (
        MusicalNote::C, false, MusicalNote::C, ChordType::None,
        ChordAccent::None, 0);
    }

  apply_chords (new_chords, undoable);
}

void
ChordEditor::transpose_chords (bool up, bool undoable)
{
  std::vector<ChordDescriptor> new_chords;
  new_chords.reserve (CHORD_EDITOR_NUM_CHORDS);
  for (const auto i : std::views::iota (0, CHORD_EDITOR_NUM_CHORDS))
    {
      new_chords.push_back (get_chord_at_index (i));
      ChordDescriptor &descr = new_chords.back ();

      int add = (up ? 1 : 11);
      descr.root_note_ = (MusicalNote) ((int) descr.root_note_ + add);
      descr.bass_note_ = (MusicalNote) ((int) descr.bass_note_ + add);

      descr.root_note_ = (MusicalNote) ((int) descr.root_note_ % 12);
      descr.bass_note_ = (MusicalNote) ((int) descr.bass_note_ % 12);
    }

  apply_chords (new_chords, undoable);
}

ChordEditor::ChordDescriptor *
ChordEditor::get_chord_from_note_number (midi_byte_t note_number)
{
  if (note_number < 60 || note_number >= 72)
    return nullptr;

  return &get_chord_at_index (note_number - 60);
}

int
ChordEditor::get_chord_index (const ChordDescriptor &chord) const
{
  for (const auto i : std::views::iota (0, CHORD_EDITOR_NUM_CHORDS))
    {
      if (get_chord_at_index (i) == chord)
        return i;
    }

  z_return_val_if_reached (-1);
}
}
