// SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Chord editor backend.
 */

#ifndef __GUI_BACKEND_CHORD_EDITOR_H__
#define __GUI_BACKEND_CHORD_EDITOR_H__

#include "dsp/chord_descriptor.h"
#include "dsp/musical_scale.h"
#include "gui/backend/backend/editor_settings.h"
#include "utils/icloneable.h"

class ChordPreset;

/**
 * @addtogroup gui_backend
 *
 * @{
 */

using namespace zrythm;

#define CHORD_EDITOR (CLIP_EDITOR->chord_editor_)

constexpr int CHORD_EDITOR_NUM_CHORDS = 12;

/**
 * Backend for the chord editor.
 */
class ChordEditor final
    : public QObject,
      public EditorSettings,
      public ICloneable<ChordEditor>,
      public utils::serialization::ISerializable<ChordEditor>
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_EDITOR_SETTINGS_QML_PROPERTIES
public:
  using ChordDescriptor = dsp::ChordDescriptor;
  using ChordAccent = dsp::ChordAccent;
  using ChordType = dsp::ChordType;
  using MusicalScale = dsp::MusicalScale;
  using MusicalNote = dsp::MusicalNote;

  ChordEditor (QObject * parent = nullptr) : QObject (parent) { }

  /**
   * Initializes the ChordEditor.
   */
  void init ();

  void
  apply_single_chord (const ChordDescriptor &chord, int idx, bool undoable);

  void apply_chords (const std::vector<ChordDescriptor> &chords, bool undoable);

  void apply_preset (const ChordPreset &pset, bool undoable);

  void apply_preset_from_scale (
    MusicalScale::Type scale,
    MusicalNote        root_note,
    bool               undoable);

  void transpose_chords (bool up, bool undoable);

  /**
   * Returns the ChordDescriptor for the given note number, otherwise NULL if
   * the given note number is not in the proper range.
   */
  ChordDescriptor * get_chord_from_note_number (midi_byte_t note_number);

  int get_chord_index (const ChordDescriptor &chord) const;

  auto &get_chord_at_index (size_t index) { return chords_.at (index); }
  auto &get_chord_at_index (size_t index) const { return chords_.at (index); }

  void init_after_cloning (const ChordEditor &other, ObjectCloneType clone_type)
    override
  {
    static_cast<EditorSettings &> (*this) =
      static_cast<const EditorSettings &> (other);
  }

  void add_chord_descriptor (ChordDescriptor &&chord_descr)
  {
    chords_.emplace_back (std::move (chord_descr));
    chords_.back ().update_notes ();
  }

  void
  replace_chord_descriptor (const auto index, ChordDescriptor &&chord_descr)
  {
    auto removed_id = chords_.at (index);
    chords_.erase (chords_.begin () + index);
    chords_.insert (chords_.begin () + index, std::move (chord_descr));
    get_chord_at_index (index).update_notes ();
  }

  DECLARE_DEFINE_FIELDS_METHOD ();

public:
  /**
   * The chords to show on the left.
   *
   * Currently fixed to 12 chords whose order cannot be edited. Chords cannot
   * be added or removed.
   */
  std::vector<ChordDescriptor> chords_;
};

/**
 * @}
 */

#endif
