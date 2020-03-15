/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
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
 * Curves.
 */

#ifndef __AUDIO_CURVE_H__
#define __AUDIO_CURVE_H__

#include "utils/yaml.h"

/**
 * @addtogroup audio
 *
 * @{
 */

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
   */
  CURVE_ALGORITHM_VITAL,
} CurveAlgorithm;

static const cyaml_strval_t
  curve_algorithm_strings[] =
{
  { "exponent",
    CURVE_ALGORITHM_EXPONENT },
  { "superellipse",
    CURVE_ALGORITHM_SUPERELLIPSE },
  { "vital",
    CURVE_ALGORITHM_VITAL },
};

/**
 * Curve options.
 */
typedef struct CurveOptions
{
  /** Curve algorithm to use. */
  CurveAlgorithm    algo;

  /** Curviness between -1 and 1, where < 0 tils
   * downwards, > 0 tilts upwards and 0 is a
   * straight line. */
  double            curviness;
} CurveOptions;

static const cyaml_schema_field_t
  curve_options_fields_schema[] =
{
  YAML_FIELD_ENUM (
    CurveOptions, algo,
    curve_algorithm_strings),
  YAML_FIELD_FLOAT (
    CurveOptions, curviness),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
  curve_options_schema =
{
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
    CurveOptions, curve_options_fields_schema),
};

/**
 * Returns the Y value on a curve spcified by
 * \ref algo.
 *
 * @param x X-coordinate, normalized.
 * @param opts Curve options.
 * @param start_higher Start at higher point.
 */
double
curve_get_normalized_y (
  double         x,
  CurveOptions * opts,
  int            start_higher);

/**
 * @}
 */

#endif
