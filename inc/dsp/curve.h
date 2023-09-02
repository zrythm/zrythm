// SPDX-FileCopyrightText: © 2020, 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Curves.
 *
 * Can find more at tracktion_AudioFadeCurve.h.
 */

#ifndef __AUDIO_CURVE_H__
#define __AUDIO_CURVE_H__

#include <stdbool.h>

#include "utils/yaml.h"

#include <glib/gi18n.h>

/**
 * @addtogroup dsp
 *
 * @{
 */

#define CURVE_OPTIONS_SCHEMA_VERSION 1

/** Bounds for each algorithm. */
#define CURVE_SUPERELLIPSE_CURVINESS_BOUND 0.82
#define CURVE_EXPONENT_CURVINESS_BOUND 0.95
#define CURVE_VITAL_CURVINESS_BOUND 1.00

/**
 * The algorithm to use for curves.
 *
 * See https://www.desmos.com/calculator/typjsyykvb
 */
typedef enum CurveAlgorithm
{
  /**
   * y = x^n
   * 0 < n <= 1,
   *   where the whole thing is tilting up and 0 is
   *   full tilt and 1 is straight line (when starting
   *   at lower point).
   */
  CURVE_ALGORITHM_EXPONENT,

  /**
   * y = 1 - (1 - x^n)^(1/n)
   * 0 < n <= 1,
   *   where the whole thing is tilting up and 0 is
   *   full tilt and 1 is straight line (when starting
   *   at lower point).
   *
   * See https://stackoverflow.com/questions/17623152/how-map-tween-a-number-based-on-a-dynamic-curve
   */
  CURVE_ALGORITHM_SUPERELLIPSE,

  /**
   * (e^(nx) - 1) / (e^n - 1)
   * -10 <= n <= 10
   *    where positive tilts down and negative tilts
   *    up (when starting at lower point).
   *
   * See https://www.desmos.com/calculator/2dnuiptiqc.
   *
   * Taken from Vital synth.
   */
  CURVE_ALGORITHM_VITAL,

  /**
   * Pulse (square).
   */
  CURVE_ALGORITHM_PULSE,

  /**
   * a = log (n)
   * b = 1 / (log (1 + (1  / n)))
   * # smaller numbers tilt up
   * y1 = (log (x + n) - a) * b
   * # smaller numbers tilt down
   * y2 = (a - log (x + n)) * b
   * where 0 < n <= 10
   *
   * See https://www.desmos.com/calculator/tdedahsdz8.
   *
   * Taken from Ardour.
   */
  CURVE_ALGORITHM_LOGARITHMIC,

  NUM_CURVE_ALGORITHMS,
} CurveAlgorithm;

static const cyaml_strval_t curve_algorithm_strings[] = {
  {N_ ("Exponent"),      CURVE_ALGORITHM_EXPONENT    },
  { N_ ("Superellipse"), CURVE_ALGORITHM_SUPERELLIPSE},
  { N_ ("Vital"),        CURVE_ALGORITHM_VITAL       },
  { N_ ("Pulse"),        CURVE_ALGORITHM_PULSE       },
  { N_ ("Logarithmic"),  CURVE_ALGORITHM_LOGARITHMIC },
};

/**
 * Curve options.
 */
typedef struct CurveOptions
{
  int schema_version;

  /** Curve algorithm to use. */
  CurveAlgorithm algo;

  /** Curviness between -1 and 1, where < 0 tils downwards, > 0
   * tilts upwards and 0 is a straight line. */
  double curviness;
} CurveOptions;

static const cyaml_schema_field_t curve_options_fields_schema[] = {
  YAML_FIELD_INT (CurveOptions, schema_version),
  YAML_FIELD_ENUM (CurveOptions, algo, curve_algorithm_strings),
  YAML_FIELD_FLOAT (CurveOptions, curviness),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t curve_options_schema = {
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
    CurveOptions,
    curve_options_fields_schema),
};

typedef struct CurveFadePreset
{
  char *       id;
  char *       label;
  CurveOptions opts;
} CurveFadePreset;

NONNULL void
curve_opts_init (CurveOptions * opts);

/**
 * Returns an array of CurveFadePreset.
 */
RETURNS_NONNULL
GPtrArray *
curve_get_fade_presets (void);

/**
 * Stores the localized name of the algorithm in
 * \ref buf.
 */
void
curve_algorithm_get_localized_name (
  CurveAlgorithm algo,
  char *         buf);

gboolean
curve_algorithm_get_g_settings_mapping (
  GValue *   value,
  GVariant * variant,
  gpointer   user_data);

GVariant *
curve_algorithm_set_g_settings_mapping (
  const GValue *       value,
  const GVariantType * expected_type,
  gpointer             user_data);

/**
 * Returns the Y value on a curve specified by
 * \ref algo.
 *
 * @param x X-coordinate, normalized.
 * @param opts Curve options.
 * @param start_higher Start at higher point.
 */
HOT NONNULL double
curve_get_normalized_y (
  double         x,
  CurveOptions * opts,
  int            start_higher);

PURE bool
curve_options_are_equal (
  const CurveOptions * a,
  const CurveOptions * b);

/**
 * @}
 */

#endif
