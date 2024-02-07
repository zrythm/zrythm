/*
 * SPDX-FileCopyrightText: © 2019-2021 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

/**
 * \file
 *
 * API for selections in the piano roll.
 */

#ifndef __SCHEMAS_GUI_BACKEND_MA_SELECTIONS_H__
#define __SCHEMAS_GUI_BACKEND_MA_SELECTIONS_H__

#include "schemas/dsp/midi_note.h"
#include "schemas/gui/backend/arranger_selections.h"
#include "utils/yaml.h"

typedef struct MidiArrangerSelections_v1
{
  ArrangerSelections_v1 base;
  int                   schema_version;
  MidiNote_v1 **        midi_notes;
  int                   num_midi_notes;
  size_t                midi_notes_size;
} MidiArrangerSelections_v1;

static const cyaml_schema_field_t midi_arranger_selections_fields_schema_v1[] = {
  YAML_FIELD_MAPPING_EMBEDDED (
    MidiArrangerSelections_v1,
    base,
    arranger_selections_fields_schema_v1),
  YAML_FIELD_INT (MidiArrangerSelections_v1, schema_version),
  YAML_FIELD_DYN_ARRAY_VAR_COUNT (
    MidiArrangerSelections_v1,
    midi_notes,
    midi_note_schema_v1),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t midi_arranger_selections_schema_v1 = {
  YAML_VALUE_PTR (
    MidiArrangerSelections_v1,
    midi_arranger_selections_fields_schema_v1),
};

#endif
