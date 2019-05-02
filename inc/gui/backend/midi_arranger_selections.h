/*
 * Copyright (C) 2019 Alexandros Theodotou
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __ACTIONS_MIDI_ARRANGER_SELECTIONS_H__
#define __ACTIONS_MIDI_ARRANGER_SELECTIONS_H__

#include "audio/midi_note.h"
#include "utils/yaml.h"

#define MIDI_ARRANGER_SELECTIONS \
  (&PROJECT->midi_arranger_selections)

/**
 * Selections to be used for the midi_arranger's
 * current selections, copying, undoing, etc.
 */
typedef struct MidiArrangerSelections
{
  /** Original selected notes. */
  MidiNote *               midi_notes[600];
  int                      midi_note_ids[600];
  int                      num_midi_notes;

  /** MIDI notes acting on.
   *
   * These are newly cloned notes (transient).
   * Use actual_note to get the original note ID. */
  MidiNote *               transient_notes[600];
} MidiArrangerSelections;

static const cyaml_schema_field_t
  midi_arranger_selections_fields_schema[] =
{
  CYAML_FIELD_SEQUENCE_COUNT (
    "midi_note_ids", CYAML_FLAG_OPTIONAL,
      MidiArrangerSelections, midi_note_ids, num_midi_notes,
      &int_schema, 0, CYAML_UNLIMITED),

	CYAML_FIELD_END
};

static const cyaml_schema_value_t
midi_arranger_selections_schema = {
	CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
			MidiArrangerSelections, midi_arranger_selections_fields_schema),
};

void
midi_arranger_selections_init_loaded (
  MidiArrangerSelections * self);

/**
 * Creates transient notes for notes added
 * to selections without transients.
 */
void
midi_arranger_selections_create_missing_transients (
  MidiArrangerSelections * mas);

/**
 * Clone the struct for copying, undoing, etc.
 */
MidiArrangerSelections *
midi_arranger_selections_clone (
  MidiArrangerSelections * mas);

/**
 * Returns the position of the leftmost object.
 */
void
midi_arranger_selections_get_start_pos (
  MidiArrangerSelections * ts,
  Position *                pos); ///< position to fill in

/**
 * Gets first (position-wise) MidiNote.
 *
 * If transient is 1, the transient notes are
 * checked instead.
 */
MidiNote *
midi_arranger_selections_get_first_midi_note (
  MidiArrangerSelections * mas,
  int                      transient);

/**
 * Gets last (position-wise) MidiNote.
 *
 * If transient is 1, the transient notes are
 * checked instead.
 */
MidiNote *
midi_arranger_selections_get_last_midi_note (
  MidiArrangerSelections * mas,
  int                      transient);

MidiNote *
midi_arranger_selections_get_highest_note (
  MidiArrangerSelections * mas,
  int                      transient);

MidiNote *
midi_arranger_selections_get_lowest_note (
  MidiArrangerSelections * mas,
  int                      transient);

void
midi_arranger_selections_paste_to_pos (
  MidiArrangerSelections * ts,
  Position *           pos);

/**
 * Only removes transients from their regions and
 * frees them.
 */
void
midi_arranger_selections_remove_transients (
  MidiArrangerSelections * mas);

/**
 * Adds a note to the selections.
 *
 * Optionally adds a transient note (if moving /
 * copy-moving).
 */
void
midi_arranger_selections_add_note (
  MidiArrangerSelections * mas,
  MidiNote *               note,
  int                      transient);

void
midi_arranger_selections_remove_note (
  MidiArrangerSelections * mas,
  MidiNote *               note);

/**
 * Clears selections.
 */
void
midi_arranger_selections_clear (
  MidiArrangerSelections * mas);

void
midi_arranger_selections_free (
  MidiArrangerSelections * self);

SERIALIZE_INC (MidiArrangerSelections,
               midi_arranger_selections)
DESERIALIZE_INC (MidiArrangerSelections,
                 midi_arranger_selections)
PRINT_YAML_INC (MidiArrangerSelections,
                midi_arranger_selections)

#endif
