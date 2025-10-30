// SPDX-FileCopyrightText: © 2019-2021, 2023-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/arrangement/editor_settings.h"
#include "utils/icloneable.h"

namespace zrythm::structure::arrangement
{
/**
 * A MIDI modifier to use to display data for.
 */
enum class MidiModifier
{
  Velocity,
  PitchWheel,
  ModWheel,
  Aftertouch,
};

/**
 * A descriptor for a MidiNote, used by the piano roll.
 *
 * Notes will only be draggable and reorderable in drum mode.
 *
 * In normal mode, only visibility can be changed.
 */
class MidiNoteDescriptor
{
public:
  auto get_custom_name () const { return custom_name_; }

  void set_custom_name (const utils::Utf8String &str) { custom_name_ = str; }

public:
  /**
   * The index to display the note at.
   */
  int index_ = 0;

  /**
   * The actual value (0-127).
   *
   * Must be unique in the array.
   */
  int value_ = 0;

  /** Whether the note is visible or not. */
  bool visible_ = true;

  /**
   * Custom name, from midnam or GM MIDI specs, etc.
   *
   * This is only used in drum mode.
   */
  utils::Utf8String custom_name_;

  /** Name of the note, from C-2 to B8. */
  utils::Utf8String note_name_;

  /** Note name with extra formatting. */
  utils::Utf8String note_name_pango_;

  /** Whether the note is highlighted/marked or not. */
  bool marked_ = false;
};

/**
 * Piano roll serializable backend.
 *
 * The actual widgets should reflect the information here.
 */
class PianoRoll : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY (
    zrythm::structure::arrangement::EditorSettings * editorSettings READ
      getEditorSettings CONSTANT FINAL)
  Q_PROPERTY (int keyHeight READ getKeyHeight NOTIFY keyHeightChanged)
  QML_UNCREATABLE ("")

public:
  /**
   * Highlighting for the piano roll.
   */
  enum class Highlighting : std::uint8_t
  {
    None,
    Chord,
    Scale,
    Both,
  };

  PianoRoll (QObject * parent = nullptr);

private:
  static constexpr std::array<bool, 12> BLACK_NOTES = {
    false, true,  false, true,  false, false,
    true,  false, true,  false, true,  false
  };

public:
  // ============================================================================
  // QML Interface
  // ============================================================================

  auto getEditorSettings () const { return editor_settings_.get (); }

  int getKeyHeight () const { return note_height_; }

  Q_SIGNAL void keyHeightChanged ();

  Q_INVOKABLE int getKeyAtY (double y) const;
  /**
   * Returns if the key is black.
   */
  Q_INVOKABLE static constexpr bool isBlackKey (int note)
  {
    note = std::clamp (note, 0, 127);
    return BLACK_NOTES.at (static_cast<size_t> (note) % 12);
  }
  Q_INVOKABLE static constexpr bool isWhiteKey (int note)
  {
    return !isBlackKey (note);
  }

  Q_INVOKABLE static constexpr bool isNextKeyBlack (int note)
  {
    return isBlackKey (note + 1);
  }
  Q_INVOKABLE static constexpr bool isNextKeyWhite (int note)
  {
    return isWhiteKey (note + 1);
  }

  Q_INVOKABLE static constexpr bool isPrevKeyBlack (int note)
  {
    return isBlackKey (note - 1);
  }
  Q_INVOKABLE static constexpr bool isPrevKeyWhite (int note)
  {
    return isWhiteKey (note - 1);
  }

  // ============================================================================

  /**
   * Adds the note if it doesn't exist in @ref current_notes_.
   */
  void add_current_note (int note);

  /**
   * Removes the note if it exists in @ref current_notes_.
   */
  void remove_current_note (int note);

  /**
   * Returns whether the note exists in @ref current_notes_.
   */
  bool contains_current_note (int note);

  void set_notes_zoom (float notes_zoom, bool fire_events);

  /**
   * Returns the MidiNoteDescriptor matching the value (0-127).
   */
  const MidiNoteDescriptor *
  find_midi_note_descriptor_by_val (bool drum_mode, uint8_t val);

  /**
   * Updates the highlighting and notifies the UI.
   */
  void set_highlighting (Highlighting highlighting);

  /**
   * Sets the MIDI modifier.
   */
  void set_midi_modifier (MidiModifier modifier);

  /**
   * Gets the visible notes.
   */
  void get_visible_notes (bool drum_mode, std::vector<MidiNoteDescriptor> &vec)
  {
    vec.clear ();

    for (const auto i : std::views::iota (0_zu, 128_zu))
      {
        MidiNoteDescriptor * descr;
        if (drum_mode)
          descr = &drum_descriptors_[i];
        else
          descr = &piano_descriptors_[i];

        if (descr->visible_)
          {
            vec.push_back (*descr);
          }
      }
  }

  /**
   * Initializes the PianoRoll.
   */
  void init ();

  friend void init_from (
    PianoRoll             &obj,
    const PianoRoll       &other,
    utils::ObjectCloneType clone_type)
  {
    obj.editor_settings_ =
      utils::clone_unique_qobject (*other.editor_settings_, &obj);
  }

private:
  static constexpr auto kEditorSettingsKey = "editorSettings"sv;
  static constexpr auto kNotesZoomKey = "notesZoom"sv;
  static constexpr auto kMidiModifierKey = "midiModifier"sv;
  friend void           to_json (nlohmann::json &j, const PianoRoll &piano_roll)
  {
    j[kEditorSettingsKey] = piano_roll.editor_settings_;
    j[kNotesZoomKey] = piano_roll.notes_zoom_;
    j[kMidiModifierKey] = piano_roll.midi_modifier_;
  }
  friend void from_json (const nlohmann::json &j, PianoRoll &piano_roll)
  {
    j.at (kEditorSettingsKey).get_to (piano_roll.editor_settings_);
    j.at (kNotesZoomKey).get_to (piano_roll.notes_zoom_);
    j.at (kMidiModifierKey).get_to (piano_roll.midi_modifier_);
  }

  /**
   * Inits the descriptors to the default values.
   *
   * FIXME move them to tracks since each track might have different
   * arrangement of drums.
   */
  void init_descriptors ();

public:
  utils::QObjectUniquePtr<EditorSettings> editor_settings_;

  /** Notes zoom level. */
  float notes_zoom_ = 1.0f;

  /// Visual height per key in pixels
  int note_height_{ 16 };

  /** Selected MidiModifier. */
  MidiModifier midi_modifier_ = MidiModifier::Velocity;

  /** Currently pressed notes (used only at runtime). */
  std::vector<int> current_notes_;

  /**
   * Piano roll mode descriptors.
   *
   * For performance purposes, invisible notes must be sorted at the end of
   * the array.
   */
  std::vector<MidiNoteDescriptor> piano_descriptors_ =
    std::vector<MidiNoteDescriptor> (128);

  /**
   * Highlighting notes depending on the current chord or scale.
   */
  Highlighting highlighting_ = Highlighting::None;

  /**
   * Drum mode descriptors.
   *
   * These must be sorted by index at all times.
   *
   * For performance purposes, invisible notes must be sorted at the end of
   * the array.
   */
  std::vector<MidiNoteDescriptor> drum_descriptors_ =
    std::vector<MidiNoteDescriptor> (128);
};
}
