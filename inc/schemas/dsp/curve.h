// SPDX-FileCopyrightText: © 2021-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Curve schema.
 */

#ifndef __SCHEMAS_AUDIO_CURVE_H__
#define __SCHEMAS_AUDIO_CURVE_H__

#include "utils/yaml.h"

typedef enum CurveAlgorithm_v1
{
  CURVE_ALGORITHM_EXPONENT_v1,
  CURVE_ALGORITHM_SUPERELLIPSE_v1,
  CURVE_ALGORITHM_VITAL_v1,
  CURVE_ALGORITHM_PULSE_v1,
  CURVE_ALGORITHM_LOGARITHMIC_v1,
  NUM_CURVE_ALGORITHMS_v1,
} CurveAlgorithm_v1;

static const cyaml_strval_t curve_algorithm_strings_v1[] = {
  { ("Exponent"),     CURVE_ALGORITHM_EXPONENT_v1     },
  { ("Superellipse"), CURVE_ALGORITHM_SUPERELLIPSE_v1 },
  { ("Vital"),        CURVE_ALGORITHM_VITAL_v1        },
  { ("Pulse"),        CURVE_ALGORITHM_PULSE_v1        },
  { ("Logarithmic"),  CURVE_ALGORITHM_LOGARITHMIC_v1  },
};

typedef struct CurveOptions_v1
{
  int               schema_version;
  CurveAlgorithm_v1 algo;
  double            curviness;
} CurveOptions_v1;

static const cyaml_schema_field_t curve_options_fields_schema_v1[] = {
  YAML_FIELD_INT (CurveOptions_v1, schema_version),
  YAML_FIELD_ENUM (CurveOptions_v1, algo, curve_algorithm_strings_v1),
  YAML_FIELD_FLOAT (CurveOptions_v1, curviness),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t curve_options_schema_v1 = {
  YAML_VALUE_PTR (CurveOptions_v1, curve_options_fields_schema_v1),
};

#endif
