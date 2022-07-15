/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
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
 * Quantize options.
 */

#ifndef __SCHEMAS_AUDIO_QUANTIZE_OPTIONS_H__
#define __SCHEMAS_AUDIO_QUANTIZE_OPTIONS_H__

#include "audio/position.h"
#include "audio/snap_grid.h"

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
