// SPDX-FileCopyrightText: © 2019-2021, 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Piano roll backend.
 */

#ifndef __GUI_BACKEND_PIANO_ROLL_H__
#define __GUI_BACKEND_PIANO_ROLL_H__

#include "gui/cpp/backend/editor_settings.h"

#include "common/utils/icloneable.h"

class Track;

/**
 * @addtogroup gui_backend
 *
 * @{
 */

#define PIANO_ROLL (&CLIP_EDITOR->piano_roll_)

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

class Region;

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
  inline std::string get_custom_name () const { return custom_name_; }

  void set_custom_name (const std::string &str) { custom_name_ = str; }

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
  std::string custom_name_;

  /** Name of the note, from C-2 to B8. */
  std::string note_name_;

  /** Note name with extra formatting. */
  std::string note_name_pango_;

  /** Whether the note is highlighted/marked or not. */
  bool marked_ = false;
};

/**
 * Piano roll serializable backend.
 *
 * The actual widgets should reflect the information here.
 */
class PianoRoll final
    : public EditorSettings,
      public ICloneable<PianoRoll>,
      public ISerializable<PianoRoll>
{
public:
  /**
   * Highlighting for the piano roll.
   */
  enum class Highlighting
  {
    None,
    Chord,
    Scale,
    Both,
  };

  enum class NoteNotation
  {
    Musical,
    Pitch,
  };

public:
  DECLARE_DEFINE_FIELDS_METHOD ();

  /**
   * Returns if the key is black.
   */
  bool is_key_black (int note);

  bool is_next_key_black (int note) { return is_key_black (note + 1); }

  bool is_prev_key_black (int note) { return is_key_black (note - 1); }

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

  /**
   * Returns the current track whose regions are being shown in the piano roll.
   */
  Track * get_current_track () const;

  void set_notes_zoom (float notes_zoom, bool fire_events);

  /**
   * Inits the PianoRoll after a Project has been loaded.
   */
  void init_loaded ();

  /**
   * Returns the MidiNoteDescriptor matching the value (0-127).
   */
  const MidiNoteDescriptor *
  find_midi_note_descriptor_by_val (bool drum_mode, const uint8_t val);

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

    for (int i = 0; i < 128; i++)
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

  void init_after_cloning (const PianoRoll &other) override { *this = other; }

private:
  /**
   * Inits the descriptors to the default values.
   *
   * FIXME move them to tracks since each track might have different
   * arrangement of drums.
   */
  void init_descriptors ();

public:
  /** Notes zoom level. */
  float notes_zoom_ = 1.0f;

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

/**
 * @}
 */

#endif
