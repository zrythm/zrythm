/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * API for selections in the piano roll.
 */

#ifndef __GUI_BACKEND_MA_SELECTIONS_H__
#define __GUI_BACKEND_MA_SELECTIONS_H__

#include "audio/midi_note.h"
#include "gui/backend/arranger_selections.h"
#include "utils/yaml.h"

/**
 * @addtogroup gui_backend
 *
 * @{
 */

#define MA_SELECTIONS \
  (&PROJECT->midi_arranger_selections)

/**
 * Selections to be used for the midi_arranger's
 * current selections, copying, undoing, etc.
 */
typedef struct MidiArrangerSelections
{
  /** Selected notes. */
  MidiNote *               midi_notes[600];
  int                      num_midi_notes;

} MidiArrangerSelections;

static const cyaml_schema_field_t
  midi_arranger_selections_fields_schema[] =
{
  CYAML_FIELD_SEQUENCE_COUNT (
    "midi_notes", CYAML_FLAG_DEFAULT,
    MidiArrangerSelections, midi_notes, num_midi_notes,
    &midi_note_schema, 0, CYAML_UNLIMITED),

	CYAML_FIELD_END
};

static const cyaml_schema_value_t
midi_arranger_selections_schema = {
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
    MidiArrangerSelections,
    midi_arranger_selections_fields_schema),
};

ARRANGER_SELECTIONS_DECLARE_FUNCS (
  MidiArranger, midi_arranger);
ARRANGER_SELECTIONS_DECLARE_OBJ_FUNCS (
  MidiArranger, midi_arranger, MidiNote, midi_note);

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

/**
 * Code to run after deserializing.
 */
void
midi_arranger_selections_post_deserialize (
  MidiArrangerSelections * ts);

/**
 * Returns if the selections can be pasted.
 *
 * @param pos Position to paste to.
 * @param region Region to paste to.
 */
int
midi_arranger_selections_can_be_pasted (
  MidiArrangerSelections * ts,
  Position *               pos,
  Region *                 region);

/**
 * Adds a Velocity (MidiNote) to the selections.
 */
#define midi_arranger_selections_add_velocity( \
  mas, vel) \
  midi_arranger_selections_add_midi_note ( \
    mas, vel->midi_note)

/**
 * Removes a Velocity (MidiNote) from the
 * selections.
 */
#define midi_arranger_selections_remove_velocity( \
  mas, vel) \
  midi_arranger_selections_remove_midi_note ( \
    mas, vel->midi_note)

SERIALIZE_INC (MidiArrangerSelections,
               midi_arranger_selections)
DESERIALIZE_INC (MidiArrangerSelections,
                 midi_arranger_selections)
PRINT_YAML_INC (MidiArrangerSelections,
                midi_arranger_selections)

/**
* @}
*/

#endif
