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
  /** MIDI notes acting on */
  MidiNote *               midi_notes[600];
  int                      num_midi_notes;

  /** Highest selected MIDI note */
  MidiNote *               top_midi_note;

  /** Lowest selected MIDI note */
  MidiNote *               bot_midi_note;
} MidiArrangerSelections;

static const cyaml_schema_field_t
  midi_arranger_selections_fields_schema[] =
{
  CYAML_FIELD_SEQUENCE_COUNT (
    "midi_notes", CYAML_FLAG_OPTIONAL,
      MidiArrangerSelections, midi_notes, num_midi_notes,
      &midi_note_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_MAPPING_PTR (
    "top_midi_note", CYAML_FLAG_OPTIONAL | CYAML_FLAG_POINTER,
    MidiArrangerSelections, top_midi_note, midi_note_fields_schema),
  CYAML_FIELD_MAPPING_PTR (
    "bot_region", CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    MidiArrangerSelections, bot_midi_note, midi_note_fields_schema),

	CYAML_FIELD_END
};

static const cyaml_schema_value_t
midi_arranger_selections_schema = {
	CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
			MidiArrangerSelections, midi_arranger_selections_fields_schema),
};

/**
 * Shift note pos for selected noted
 */
void
midi_arranger_selections_shift_pos (int delta);

/**
 * Shift note value for selected noted
 */
void
midi_arranger_selections_shift_val (int delta);
/**
 * Clone the struct for copying, undoing, etc.
 */
MidiArrangerSelections *
midi_arranger_selections_clone ();

/**
 * Returns the position of the leftmost object.
 */
void
midi_arranger_selections_get_start_pos (
  MidiArrangerSelections * ts,
  Position *                pos); ///< position to fill in

void
midi_arranger_selections_paste_to_pos (
  MidiArrangerSelections * ts,
  Position *           pos);

void
midi_arranger_selections_free (MidiArrangerSelections * self);

SERIALIZE_INC (MidiArrangerSelections,
               midi_arranger_selections)
DESERIALIZE_INC (MidiArrangerSelections,
                 midi_arranger_selections)
PRINT_YAML_INC (MidiArrangerSelections,
                midi_arranger_selections)

#endif
