// SPDX-FileCopyrightText: © 2019-2021, 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

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

#define MA_SELECTIONS_SCHEMA_VERSION 1

#define MA_SELECTIONS (PROJECT->midi_arranger_selections)

/**
 * Selections to be used for the midi_arranger's
 * current selections, copying, undoing, etc.
 */
typedef struct MidiArrangerSelections
{
  /** Base struct. */
  ArrangerSelections base;

  int schema_version;

  /** Selected notes. */
  MidiNote ** midi_notes;
  int         num_midi_notes;
  size_t      midi_notes_size;

} MidiArrangerSelections;

static const cyaml_schema_field_t
  midi_arranger_selections_fields_schema[] = {
    YAML_FIELD_MAPPING_EMBEDDED (
      MidiArrangerSelections,
      base,
      arranger_selections_fields_schema),
    YAML_FIELD_INT (MidiArrangerSelections, schema_version),
    YAML_FIELD_DYN_ARRAY_VAR_COUNT (
      MidiArrangerSelections,
      midi_notes,
      midi_note_schema),

    CYAML_FIELD_END
  };

static const cyaml_schema_value_t
  midi_arranger_selections_schema = {
    YAML_VALUE_PTR (
      MidiArrangerSelections,
      midi_arranger_selections_fields_schema),
  };

MidiNote *
midi_arranger_selections_get_highest_note (
  MidiArrangerSelections * mas);

MidiNote *
midi_arranger_selections_get_lowest_note (
  MidiArrangerSelections * mas);

/**
 * Sets the listen status of notes on and off based
 * on changes in the previous selections and the
 * current selections.
 */
void
midi_arranger_selections_unlisten_note_diff (
  MidiArrangerSelections * prev,
  MidiArrangerSelections * mas);

/**
 * Returns if the selections can be pasted.
 *
 * @param pos Position to paste to.
 * @param region ZRegion to paste to.
 */
int
midi_arranger_selections_can_be_pasted (
  MidiArrangerSelections * ts,
  Position *               pos,
  ZRegion *                region);

NONNULL void
midi_arranger_selections_sort_by_pitch (
  MidiArrangerSelections * self,
  bool                     desc);

/**
* @}
*/

#endif
