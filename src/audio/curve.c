/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version approved by
 * Alexandros Theodotou in a public statement of acceptance.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <math.h>

#include "audio/curve.h"
#include "utils/math.h"

#include <glib/gi18n.h>

void
curve_opts_init (
  CurveOptions * opts)
{
  opts->schema_version =
    CURVE_OPTIONS_SCHEMA_VERSION;
}

/**
 * Stores the localized name of the algorithm in
 * \ref buf.
 */
void
curve_algorithm_get_localized_name (
  CurveAlgorithm algo,
  char *         buf)
{
  switch (algo)
    {
    case CURVE_ALGORITHM_EXPONENT:
      sprintf (buf, _("Exponent"));
      break;
    case CURVE_ALGORITHM_SUPERELLIPSE:
      sprintf (buf, _("Superellipse"));
      break;
    case CURVE_ALGORITHM_VITAL:
      sprintf (buf, _("Vital"));
      break;
    case CURVE_ALGORITHM_PULSE:
      sprintf (buf, _("Pulse"));
      break;
    default:
      g_return_if_reached ();
    }
  return;
}

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
  int            start_higher)
{
  g_return_val_if_fail (x >= 0.0 && x <= 1.0, 0.0);

  int curve_up = opts->curviness >= 0;

  double val = -1.0;
  switch (opts->algo)
    {
    case CURVE_ALGORITHM_EXPONENT:
      {
        /* convert curviness to bound */
        double curviness_for_calc =
          opts->curviness *
          CURVE_EXPONENT_CURVINESS_BOUND;

        curviness_for_calc =
          1.0 - fabs (curviness_for_calc);
        if (!start_higher)
          x = 1.0 - x;
        if (curve_up)
          x = 1.0 - x;

        /* if curviness is 0, it is a straight line */
        if (math_doubles_equal (
              curviness_for_calc, 0.0000))
          {
            val = x;
          }
        else
          {
            val = pow (x, curviness_for_calc);
          }

        if (!curve_up)
          val = 1.0 - val;
      }
      break;
    case CURVE_ALGORITHM_SUPERELLIPSE:
      {
        /* convert curviness to bound */
        double curviness_for_calc =
          opts->curviness *
          CURVE_SUPERELLIPSE_CURVINESS_BOUND;

        curviness_for_calc =
          1.0 - fabs (curviness_for_calc);
        if (!start_higher)
          x = 1.0 - x;
        if (curve_up)
          x = 1.0 - x;

        /* if curviness is 0, it is a straight line */
        if (math_doubles_equal (
              curviness_for_calc, 0.0000))
          {
            val = x;
          }
        else
          {
            val =
              pow (
                1.0 - pow (x, curviness_for_calc),
                (1.0 / curviness_for_calc));
          }

        if (curve_up)
          val = 1.0 - val;
      }
      break;
    case CURVE_ALGORITHM_VITAL:
      {
        /* convert curviness to bound */
        double curviness_for_calc =
          opts->curviness *
          CURVE_VITAL_CURVINESS_BOUND;

        curviness_for_calc =
          - curviness_for_calc * 10.0;
        if (start_higher)
          x = 1.0 - x;

        /* if curviness is 0, it is a straight line */
        if (math_doubles_equal (
              curviness_for_calc, 0.0000))
          {
            val = x;
          }
        else
          {
            val =
              expm1 (curviness_for_calc * x) /
              expm1 (curviness_for_calc);
          }
      }
      break;
    case CURVE_ALGORITHM_PULSE:
      {
        val =
          ((1.0 + opts->curviness) / 2.0) > x ?
            0.0 : 1.0;

        if (start_higher)
          val = 1.0 - val;
      }
      break;
    default:
      g_warn_if_reached ();
      break;
    }
  return val;
}

bool
curve_options_are_equal (
  CurveOptions * a,
  CurveOptions * b)
{
  return
    a->algo == b->algo &&
    math_doubles_equal (
      a->curviness, b->curviness);
}
