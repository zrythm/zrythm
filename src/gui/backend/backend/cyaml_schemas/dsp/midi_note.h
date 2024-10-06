// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * MIDI note schema.
 */

#ifndef __SCHEMAS_AUDIO_MIDI_NOTE_H__
#define __SCHEMAS_AUDIO_MIDI_NOTE_H__

#include "gui/backend/backend/cyaml_schemas/dsp/velocity.h"
#include "gui/backend/backend/cyaml_schemas/gui/backend/arranger_object.h"

typedef struct MidiNote_v1
{
  ArrangerObject_v1 base;
  int               schema_version;
  Velocity_v1 *     vel;
  uint8_t           val;
  int               muted;
  int               pos;
} MidiNote_v1;

static const cyaml_schema_field_t midi_note_fields_schema_v1[] = {
  YAML_FIELD_MAPPING_EMBEDDED (MidiNote_v1, base, arranger_object_fields_schema_v1),
  YAML_FIELD_INT (MidiNote_v1, schema_version),
  YAML_FIELD_MAPPING_PTR (MidiNote_v1, vel, velocity_fields_schema_v1),
  YAML_FIELD_UINT (MidiNote_v1, val),
  YAML_FIELD_INT (MidiNote_v1, muted),
  YAML_FIELD_INT (MidiNote_v1, pos),
  CYAML_FIELD_END
};

static const cyaml_schema_value_t midi_note_schema_v1 = {
  YAML_VALUE_PTR_NULLABLE (MidiNote_v1, midi_note_fields_schema_v1),
};

#endif
