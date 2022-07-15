/*
 * Copyright (C) 2018-2021 Alexandros Theodotou <alex at zrythm dot org>
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
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * MIDI note schema.
 */

#ifndef __SCHEMAS_AUDIO_MIDI_NOTE_H__
#define __SCHEMAS_AUDIO_MIDI_NOTE_H__

#include "schemas/gui/backend/arranger_object.h"

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
  YAML_FIELD_MAPPING_EMBEDDED (
    MidiNote_v1,
    base,
    arranger_object_fields_schema_v1),
  YAML_FIELD_INT (MidiNote_v1, schema_version),
  YAML_FIELD_MAPPING_PTR (
    MidiNote_v1,
    vel,
    velocity_fields_schema_v1),
  YAML_FIELD_UINT (MidiNote_v1, val),
  YAML_FIELD_INT (MidiNote_v1, muted),
  YAML_FIELD_INT (MidiNote_v1, pos),
  CYAML_FIELD_END
};

static const cyaml_schema_value_t midi_note_schema_v1 = {
  YAML_VALUE_PTR (MidiNote_v1, midi_note_fields_schema_v1),
};

#endif
