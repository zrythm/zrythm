// SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Chord editor backend.
 */

#ifndef __GUI_BACKEND_CHORD_EDITOR_H__
#define __GUI_BACKEND_CHORD_EDITOR_H__

#include "gui/backend/backend/editor_settings.h"
#include "gui/dsp/chord_descriptor.h"
#include "gui/dsp/scale.h"

#include "utils/icloneable.h"

class ChordDescriptor;
class ChordPreset;

/**
 * @addtogroup gui_backend
 *
 * @{
 */

#define CHORD_EDITOR (&CLIP_EDITOR->chord_editor_)

constexpr int CHORD_EDITOR_NUM_CHORDS = 12;

/**
 * Backend for the chord editor.
 */
class ChordEditor final
    : public EditorSettings,
      public ICloneable<ChordEditor>,
      public zrythm::utils::serialization::ISerializable<ChordEditor>
{
public:
  /**
   * Initializes the ChordEditor.
   */
  void init ();

  void
  apply_single_chord (const ChordDescriptor &chord, const int idx, bool undoable);

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

  void init_after_cloning (const ChordEditor &other) override { *this = other; }

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
