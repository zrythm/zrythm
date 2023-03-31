// SPDX-FileCopyrightText: © 2019-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Piano roll backend.
 */

#ifndef __GUI_BACKEND_PIANO_ROLL_H__
#define __GUI_BACKEND_PIANO_ROLL_H__

#include <stdbool.h>

#include "gui/backend/editor_settings.h"

#include <cyaml/cyaml.h>

typedef struct Track Track;

/**
 * @addtogroup gui_backend
 *
 * @{
 */

#define PIANO_ROLL_SCHEMA_VERSION 1

#define PIANO_ROLL (CLIP_EDITOR->piano_roll)

#define DRUM_LABELS \
  static const char * drum_labels[47] = { \
    "Acoustic Bass Drum", \
    "Bass Drum 1", \
    "Side Stick", \
    "Acoustic Snare", \
    "Hand Clap", \
    "Electric Snare", \
    "Low Floor Tom", \
    "Closed Hi Hat", \
    "High Floor Tom", \
    "Pedal Hi-Hat", \
    "Low Tom", \
    "Open Hi-Hat", \
    "Low-Mid Tom", \
    "Hi-Mid Tom", \
    "Crash Cymbal 1", \
    "High Tom", \
    "Ride Cymbal 1", \
    "Chinese Cymbal", \
    "Ride Bell", \
    "Tambourine", \
    "Splash Cymbal", \
    "Cowbell", \
    "Crash Cymbal 2", \
    "Vibraslap", \
    "Ride Cymbal 2", \
    "Hi Bongo", \
    "Low Bongo", \
    "Mute Hi Conga", \
    "Open Hi Conga", \
    "Low Conga", \
    "High Timbale", \
    "Low Timbale", \
    "High Agogo", \
    "Low Agogo", \
    "Cabasa", \
    "Maracas", \
    "Short Whistle", \
    "Long Whistle", \
    "Short Guiro", \
    "Long Guiro", \
    "Claves", \
    "Hi Wood Block", \
    "Low Wood Block", \
    "Mute Cuica", \
    "Open Cuica", \
    "Mute Triangle", \
    "Open Triangle" \
  }

/**
 * A MIDI modifier to use to display data for.
 */
typedef enum MidiModifier
{
  MIDI_MODIFIER_VELOCITY,
  MIDI_MODIFIER_PITCH_WHEEL,
  MIDI_MODIFIER_MOD_WHEEL,
  MIDI_MODIFIER_AFTERTOUCH,
} MidiModifier;

/**
 * Highlighting for the piano roll.
 */
typedef enum PianoRollHighlighting
{
  PR_HIGHLIGHT_NONE,
  PR_HIGHLIGHT_CHORD,
  PR_HIGHLIGHT_SCALE,
  PR_HIGHLIGHT_BOTH,
} PianoRollHighlighting;

typedef struct ZRegion ZRegion;

/**
 * A descriptor for a MidiNote, used by the piano
 * roll.
 *
 * Notes will only be draggable and reorderable in
 * drum mode.
 *
 * In normal mode, only visibility can be changed.
 */
typedef struct MidiNoteDescriptor
{
  /**
   * The index to display the note at.
   */
  int index;

  /**
   * The actual value (0-127).
   *
   * Must be unique in the array.
   */
  int value;

  /** Whether the note is visible or not. */
  int visible;

  /**
   * Custom name, from midnam or GM MIDI specs, etc.
   *
   * This is only used in drum mode.
   */
  char * custom_name;

  /** Name of the note, from C-2 to B8. */
  char * note_name;

  /** Note name with extra formatting. */
  char * note_name_pango;

  /** Whether the note is highlighted/marked or not.
   */
  int marked;
} MidiNoteDescriptor;

MidiNoteDescriptor *
midi_note_descriptor_new (void);

void
midi_note_descriptor_free (MidiNoteDescriptor * self);

/**
 * Piano roll serializable backend.
 *
 * The actual widgets should reflect the information here.
 */
typedef struct PianoRoll
{
  int schema_version;

  /** Notes zoom level. */
  float notes_zoom;

  /** Selected MidiModifier. */
  MidiModifier midi_modifier;

  /** Currently pressed notes (used only at runtime). */
  int current_notes[128];
  int num_current_notes;

  /**
   * Piano roll mode descriptors.
   *
   * For performance purposes, invisible notes must
   * be sorted at the end of the array.
   */
  MidiNoteDescriptor * piano_descriptors[128];

  /**
   * Highlighting notes depending on the current
   * chord or scale.
   */
  PianoRollHighlighting highlighting;

  /**
   * Drum mode descriptors.
   *
   * These must be sorted by index at all times.
   *
   * For performance purposes, invisible notes must
   * be sorted at the end of the array.
   */
  MidiNoteDescriptor * drum_descriptors[128];

  EditorSettings editor_settings;
} PianoRoll;

static const cyaml_strval_t midi_modifier_strings[] = {
  {"Velocity",     MIDI_MODIFIER_VELOCITY   },
  { "Pitch Wheel", MIDI_MODIFIER_PITCH_WHEEL},
  { "Mod Wheel",   MIDI_MODIFIER_MOD_WHEEL  },
  { "Aftertouch",  MIDI_MODIFIER_AFTERTOUCH },
};

static const cyaml_schema_field_t piano_roll_fields_schema[] = {
  YAML_FIELD_INT (PianoRoll, schema_version),
  YAML_FIELD_FLOAT (PianoRoll, notes_zoom),
  YAML_FIELD_ENUM (
    PianoRoll,
    midi_modifier,
    midi_modifier_strings),
  YAML_FIELD_MAPPING_EMBEDDED (
    PianoRoll,
    editor_settings,
    editor_settings_fields_schema),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t piano_roll_schema = {
  YAML_VALUE_PTR (PianoRoll, piano_roll_fields_schema),
};

/**
 * Returns if the key is black.
 */
int
piano_roll_is_key_black (int note);

#define piano_roll_is_next_key_black(x) \
  piano_roll_is_key_black (x + 1)

#define piano_roll_is_prev_key_black(x) \
  piano_roll_is_key_black (x - 1)

/**
 * Adds the note if it doesn't exist in the array.
 */
void
piano_roll_add_current_note (PianoRoll * self, int note);

/**
 * Removes the note if it exists in the array.
 */
void
piano_roll_remove_current_note (PianoRoll * self, int note);

/**
 * Returns 1 if it contains the given note, 0
 * otherwise.
 */
int
piano_roll_contains_current_note (PianoRoll * self, int note);

/**
 * Returns the current track whose regions are
 * being shown in the piano roll.
 */
Track *
piano_roll_get_current_track (const PianoRoll * self);

void
piano_roll_set_notes_zoom (
  PianoRoll * self,
  float       notes_zoom,
  int         fire_events);

/**
 * Inits the PianoRoll after a Project has been
 * loaded.
 */
void
piano_roll_init_loaded (PianoRoll * self);

/**
 * Returns the MidiNoteDescriptor matching the value
 * (0-127).
 */
const MidiNoteDescriptor *
piano_roll_find_midi_note_descriptor_by_val (
  PianoRoll *   self,
  bool          drum_mode,
  const uint8_t val);

static inline char *
midi_note_descriptor_get_custom_name (
  MidiNoteDescriptor * descr)
{
  return descr->custom_name;
}

void
midi_note_descriptor_set_custom_name (
  MidiNoteDescriptor * descr,
  char *               str);

/**
 * Updates the highlighting and notifies the UI.
 */
void
piano_roll_set_highlighting (
  PianoRoll *           self,
  PianoRollHighlighting highlighting);

/**
 * Sets the MIDI modifier.
 */
void
piano_roll_set_midi_modifier (
  PianoRoll *  self,
  MidiModifier modifier);

/**
 * Gets the visible notes.
 */
static inline void
piano_roll_get_visible_notes (
  PianoRoll *          self,
  bool                 drum_mode,
  MidiNoteDescriptor * arr,
  int *                num)
{
  *num = 0;

  MidiNoteDescriptor * descr;
  for (int i = 0; i < 128; i++)
    {
      if (drum_mode)
        descr = self->drum_descriptors[i];
      else
        descr = self->piano_descriptors[i];

      if (descr->visible)
        {
          arr[*num].index = descr->index;
          arr[*num].value = descr->value;
          arr[*num].marked = descr->marked;
          arr[*num].visible = descr->visible;
          arr[*num].custom_name = descr->custom_name;
          arr[*num].note_name = descr->note_name;
          (*num)++;
        }
    }
}

/**
 * Initializes the PianoRoll.
 */
void
piano_roll_init (PianoRoll * self);

/**
 * Only clones what is needed for serialization.
 */
PianoRoll *
piano_roll_clone (const PianoRoll * src);

PianoRoll *
piano_roll_new (void);

void
piano_roll_free (PianoRoll * self);

/**
 * @}
 */

#endif
