// SPDX-FileCopyrightText: © 2019-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/chord_descriptor.h"
#include "dsp/musical_scale.h"
#include "structure/project/editor_settings.h"
#include "utils/midi.h"

#include <QtQmlIntegration/qqmlintegration.h>

#include <nlohmann/json_fwd.hpp>

class ChordPreset;

namespace zrythm::structure::project
{

constexpr int CHORD_EDITOR_NUM_CHORDS = 12;

class ChordEditor : public EditorSettings
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  using ChordDescriptor = dsp::ChordDescriptor;
  using ChordAccent = dsp::ChordAccent;
  using ChordType = dsp::ChordType;
  using MusicalScale = dsp::MusicalScale;
  using MusicalNote = dsp::MusicalNote;

  ChordEditor (QObject * parent = nullptr);

  /**
   * Initializes the ChordEditor.
   */
  void init ();

  void
  apply_single_chord (const ChordDescriptor &chord, int idx, bool undoable);

  void apply_chords (const std::vector<ChordDescriptor> &chords, bool undoable);

  void apply_preset_from_scale (
    MusicalScale::ScaleType scale,
    MusicalNote             root_note,
    bool                    undoable);

  void transpose_chords (bool up, bool undoable);

  /**
   * Returns the ChordDescriptor for the given note number, otherwise NULL if
   * the given note number is not in the proper range.
   */
  ChordDescriptor * get_chord_from_note_number (midi_byte_t note_number);

  int get_chord_index (const ChordDescriptor &chord) const;

  auto &chord_at_index (size_t index) { return chords_.at (index); }
  auto &chord_at_index (size_t index) const { return chords_.at (index); }

  void add_chord_descriptor (ChordDescriptor &&chord_descr)
  {
    chords_.emplace_back (std::move (chord_descr));
    chords_.back ().update_notes ();
  }

  void
  replace_chord_descriptor (const auto index, ChordDescriptor &&chord_descr)
  {
    chords_.erase (chords_.begin () + index);
    chords_.insert (chords_.begin () + index, std::move (chord_descr));
    chord_at_index (index).update_notes ();
  }

private:
  friend void init_from (
    ChordEditor           &obj,
    const ChordEditor     &other,
    utils::ObjectCloneType clone_type);
  friend void to_json (nlohmann::json &j, const ChordEditor &editor);
  friend void from_json (const nlohmann::json &j, ChordEditor &editor);

private:
  static constexpr auto kChordsKey = "chords"sv;

public:
  /**
   * The chords to show on the left.
   *
   * Currently fixed to 12 chords whose order cannot be edited. Chords cannot
   * be added or removed.
   */
  std::vector<ChordDescriptor> chords_;
};

}
