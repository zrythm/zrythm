// SPDX-FileCopyrightText: © 2019-2022, 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/project/chord_editor.h"
#include "utils/logger.h"
#include "utils/qt.h"
#include "utils/utf8_string.h"

#include <nlohmann/json.hpp>

namespace zrythm::structure::project
{

ChordEditor::ChordEditor (QObject * parent) : EditorSettings (parent) { }

void
init_from (
  ChordEditor           &obj,
  const ChordEditor     &other,
  utils::ObjectCloneType clone_type)
{
  init_from (
    static_cast<EditorSettings &> (obj),
    static_cast<const EditorSettings &> (other), clone_type);
}

void
ChordEditor::init ()
{
  for (const auto index : std::views::iota (0, CHORD_EDITOR_NUM_CHORDS))
    {
      add_chord_descriptor (
        (MusicalNote) ((int) MusicalNote::C + index), ChordType::Major);
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
      auto * target = chord_at_index (idx);
      target->setRootNote (chord.rootNote ());
      target->setChordType (chord.chordType ());
      target->setChordAccent (chord.chordAccent ());
      target->setInversion (chord.inversion ());
      if (chord.hasBass ())
        target->setBassNote (chord.bassNote ());
    }
#endif
}

void
ChordEditor::apply_chords (
  const std::vector<utils::QObjectUniquePtr<ChordDescriptor>> &chords,
  bool                                                         undoable)
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
      for (const auto i :
           std::views::iota (size_t{ 0 }, std::min (chords_.size (), chords.size ())))
        {
          auto * src = chords[i].get ();
          auto * dst = chord_at_index (i);
          dst->setRootNote (src->rootNote ());
          dst->setHasBass (src->hasBass ());
          dst->setBassNote (src->bassNote ());
          dst->setChordType (src->chordType ());
          dst->setChordAccent (src->chordAccent ());
          dst->setInversion (src->inversion ());
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
  std::vector<utils::QObjectUniquePtr<ChordDescriptor>> new_chords;
  for (int i = 0; i < 12; i++)
    {
      /* skip notes not in scale */
      if (!notes[i])
        continue;

      new_chords.push_back (
        utils::make_qobject_unique<ChordDescriptor> (
          (MusicalNote) ((int) root_note + i), triads.at (cur_chord)));
    }

  /* fill remaining chords with default data */
  while (cur_chord < CHORD_EDITOR_NUM_CHORDS)
    {
      new_chords.push_back (
        utils::make_qobject_unique<ChordDescriptor> (
          MusicalNote::C, ChordType::None));
    }

  apply_chords (new_chords, undoable);
}

void
ChordEditor::transpose_chords (bool up, bool undoable)
{
  for (const auto i : std::views::iota (0, CHORD_EDITOR_NUM_CHORDS))
    {
      auto * descr = chord_at_index (i);

      int add = (up ? 1 : 11);
      descr->setRootNote ((MusicalNote) (((int) descr->rootNote () + add) % 12));
      if (descr->hasBass ())
        descr->setBassNote (
          (MusicalNote) (((int) descr->bassNote () + add) % 12));
    }
}

ChordEditor::ChordDescriptor *
ChordEditor::get_chord_from_note_number (midi_byte_t note_number)
{
  if (note_number < 60 || note_number >= 72)
    return nullptr;

  return chord_at_index (note_number - 60);
}

int
ChordEditor::get_chord_index (const ChordDescriptor &chord) const
{
  for (const auto i : std::views::iota (size_t{ 0 }, chords_.size ()))
    {
      if (chord_at_index (i)->isEquivalent (chord))
        return static_cast<int> (i);
    }

  z_return_val_if_reached (-1);
}

void
to_json (nlohmann::json &j, const ChordEditor &editor)
{
  to_json (j, static_cast<const EditorSettings &> (editor));
  nlohmann::json chords_arr = nlohmann::json::array ();
  for (const auto &chord_ptr : editor.chords_)
    {
      nlohmann::json chord_json;
      to_json (chord_json, *chord_ptr);
      chords_arr.push_back (chord_json);
    }
  j[ChordEditor::kChordsKey] = chords_arr;
}

void
from_json (const nlohmann::json &j, ChordEditor &editor)
{
  from_json (j, static_cast<EditorSettings &> (editor));
  if (j.contains (ChordEditor::kChordsKey))
    {
      editor.chords_.clear ();
      for (const auto &chord_json : j[ChordEditor::kChordsKey])
        {
          auto ptr = utils::make_qobject_unique<dsp::ChordDescriptor> ();
          from_json (chord_json, *ptr);
          editor.chords_.push_back (std::move (ptr));
        }
    }
}

}
