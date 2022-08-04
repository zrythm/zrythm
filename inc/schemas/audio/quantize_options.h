// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Quantize options.
 */

#ifndef __SCHEMAS_AUDIO_QUANTIZE_OPTIONS_H__
#define __SCHEMAS_AUDIO_QUANTIZE_OPTIONS_H__

#include "schemas/audio/position.h"
#include "schemas/audio/snap_grid.h"

typedef struct QuantizeOptions_v1
{
  int           schema_version;
  NoteLength_v1 note_length;
  NoteType_v1   note_type;
  float         amount;
  int           adj_start;
  int           adj_end;
  float         swing;
  double        rand_ticks;
  Position_v1   q_points[120096];
  int           num_q_points;
} QuantizeOptions_v1;

static const cyaml_schema_field_t
  quantize_options_fields_schema_v1[] = {
    YAML_FIELD_INT (QuantizeOptions_v1, schema_version),
    YAML_FIELD_ENUM (
      QuantizeOptions_v1,
      note_length,
      note_length_strings),
    YAML_FIELD_ENUM (
      QuantizeOptions_v1,
      note_type,
      note_type_strings),
    YAML_FIELD_FLOAT (QuantizeOptions_v1, amount),
    YAML_FIELD_INT (QuantizeOptions_v1, adj_start),
    YAML_FIELD_INT (QuantizeOptions_v1, adj_end),
    YAML_FIELD_FLOAT (QuantizeOptions_v1, swing),
    YAML_FIELD_FLOAT (QuantizeOptions_v1, rand_ticks),

    CYAML_FIELD_END
  };

static const cyaml_schema_value_t quantize_options_schema_v1 = {
  YAML_VALUE_PTR (
    QuantizeOptions_v1,
    quantize_options_fields_schema_v1),
};

#endif
