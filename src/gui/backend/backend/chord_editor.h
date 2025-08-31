// SPDX-FileCopyrightText: © 2019-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Chord editor backend.
 */

#pragma once

#include "dsp/chord_descriptor.h"
#include "dsp/musical_scale.h"
#include "gui/backend/arranger_object_selection_manager.h"
#include "gui/backend/backend/editor_settings.h"
#include "structure/arrangement/arranger_object_all.h"
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
class ChordEditor : public QObject
{
  Q_OBJECT
  Q_PROPERTY (
    gui::backend::EditorSettings * editorSettings READ getEditorSettings
      CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::gui::backend::ArrangerObjectSelectionManager * selectionManager READ
      selectionManager CONSTANT)
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  using ChordDescriptor = dsp::ChordDescriptor;
  using ChordAccent = dsp::ChordAccent;
  using ChordType = dsp::ChordType;
  using MusicalScale = dsp::MusicalScale;
  using MusicalNote = dsp::MusicalNote;

  ChordEditor (
    const structure::arrangement::ArrangerObjectRegistry &registry,
    QObject *                                             parent = nullptr)
      : QObject (parent),
        selection_manager_ (
          utils::make_qobject_unique<
            gui::backend::ArrangerObjectSelectionManager> (registry, this))
  {
  }

  // =========================================================
  // QML interface
  // =========================================================

  gui::backend::EditorSettings * getEditorSettings () const
  {
    return editor_settings_.get ();
  }

  gui::backend::ArrangerObjectSelectionManager * selectionManager () const
  {
    return selection_manager_.get ();
  }

  // =========================================================

  /**
   * Initializes the ChordEditor.
   */
  void init ();

  void
  apply_single_chord (const ChordDescriptor &chord, int idx, bool undoable);

  void apply_chords (const std::vector<ChordDescriptor> &chords, bool undoable);

  void apply_preset (const ChordPreset &pset, bool undoable);

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

  auto &get_chord_at_index (size_t index) { return chords_.at (index); }
  auto &get_chord_at_index (size_t index) const { return chords_.at (index); }

  friend void init_from (
    ChordEditor           &obj,
    const ChordEditor     &other,
    utils::ObjectCloneType clone_type)

  {
    obj.editor_settings_ =
      utils::clone_unique_qobject (*other.editor_settings_, &obj);
  }

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
    get_chord_at_index (index).update_notes ();
  }

private:
  static constexpr auto kEditorSettingsKey = "editorSettings"sv;
  static constexpr auto kChordsKey = "chords"sv;
  friend void           to_json (nlohmann::json &j, const ChordEditor &editor)
  {
    j[kEditorSettingsKey] = editor.editor_settings_;
    j[kChordsKey] = editor.chords_;
  }
  friend void from_json (const nlohmann::json &j, ChordEditor &editor)
  {
    j.at (kEditorSettingsKey).get_to (editor.editor_settings_);
    j.at (kChordsKey).get_to (editor.chords_);
  }

public:
  /**
   * The chords to show on the left.
   *
   * Currently fixed to 12 chords whose order cannot be edited. Chords cannot
   * be added or removed.
   */
  std::vector<ChordDescriptor> chords_;

private:
  utils::QObjectUniquePtr<gui::backend::EditorSettings> editor_settings_;

  utils::QObjectUniquePtr<gui::backend::ArrangerObjectSelectionManager>
    selection_manager_;
};

/**
 * @}
 */
