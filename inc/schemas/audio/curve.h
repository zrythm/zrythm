/*
 * Copyright (C) 2020-2021 Alexandros Theodotou <alex at zrythm dot org>
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
 * Curve schema.
 */

#ifndef __SCHEMAS_AUDIO_CURVE_H__
#define __SCHEMAS_AUDIO_CURVE_H__

#include <stdbool.h>

#include "utils/yaml.h"

typedef enum CurveAlgorithm_v1
{
  CURVE_ALGORITHM_EXPONENT_v1,
  CURVE_ALGORITHM_SUPERELLIPSE_v1,
  CURVE_ALGORITHM_VITAL_v1,
  CURVE_ALGORITHM_PULSE_v1,
  NUM_CURVE_ALGORITHMS_v1,
} CurveAlgorithm_v1;

static const cyaml_strval_t
  curve_algorithm_strings_v1[] = {
    {__ ("Exponent"),      CURVE_ALGORITHM_EXPONENT_v1},
    { __ ("Superellipse"),
     CURVE_ALGORITHM_SUPERELLIPSE_v1                  },
    { __ ("Vital"),        CURVE_ALGORITHM_VITAL_v1   },
    { __ ("Pulse"),        CURVE_ALGORITHM_PULSE_v1   },
};

typedef struct CurveOptions_v1
{
  int               schema_version;
  CurveAlgorithm_v1 algo;
  double            curviness;
} CurveOptions;

static const cyaml_schema_field_t
  curve_options_fields_schema_v1[] = {
    YAML_FIELD_INT (CurveOptions_v1, schema_version),
    YAML_FIELD_ENUM (
      CurveOptions_v1,
      algo,
      curve_algorithm_strings_v1),
    YAML_FIELD_FLOAT (CurveOptions_v1, curviness),

    CYAML_FIELD_END
  };

static const cyaml_schema_value_t
  curve_options_schema_v1 = {
    YAML_VALUE_PTR (
      CurveOptions_v1,
      curve_options_fields_schema_v1),
  };

#endif
