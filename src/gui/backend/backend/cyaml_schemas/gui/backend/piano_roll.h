// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Piano roll backend.
 */

#ifndef __SCHEMAS_GUI_BACKEND_PIANO_ROLL_H__
#define __SCHEMAS_GUI_BACKEND_PIANO_ROLL_H__

#include "gui/backend/backend/cyaml_schemas/gui/backend/editor_settings.h"
#include "utils/yaml.h"

typedef enum MidiModifier_v1
{
  MIDI_MODIFIER_VELOCITY_v1,
  MIDI_MODIFIER_PITCH_WHEEL_v1,
  MIDI_MODIFIER_MOD_WHEEL_v1,
  MIDI_MODIFIER_AFTERTOUCH_v1,
} MidiModifier_v1;

static const cyaml_strval_t midi_modifier_strings_v1[] = {
  { "Velocity",    MIDI_MODIFIER_VELOCITY_v1    },
  { "Pitch Wheel", MIDI_MODIFIER_PITCH_WHEEL_v1 },
  { "Mod Wheel",   MIDI_MODIFIER_MOD_WHEEL_v1   },
  { "Aftertouch",  MIDI_MODIFIER_AFTERTOUCH_v1  },
};

typedef struct MidiNoteDescriptor_v1
{
  int    index;
  int    value;
  int    visible;
  char * custom_name;
  char * note_name;
  char * note_name_pango;
  int    marked;
} MidiNoteDescriptor_v1;

typedef struct PianoRoll_v1
{
  int               schema_version;
  float             notes_zoom;
  MidiModifier_v1   midi_modifier;
  EditorSettings_v1 editor_settings;
} PianoRoll_v1;

static const cyaml_schema_field_t piano_roll_fields_schema_v1[] = {
  YAML_FIELD_INT (PianoRoll_v1, schema_version),
  YAML_FIELD_FLOAT (PianoRoll_v1, notes_zoom),
  YAML_FIELD_ENUM (PianoRoll_v1, midi_modifier, midi_modifier_strings_v1),
  YAML_FIELD_MAPPING_EMBEDDED (
    PianoRoll_v1,
    editor_settings,
    editor_settings_fields_schema_v1),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t piano_roll_schema_v1 = {
  YAML_VALUE_PTR (PianoRoll_v1, piano_roll_fields_schema_v1),
};

#endif
