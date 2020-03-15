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

#include <math.h>

#include "audio/curve.h"
#include "utils/math.h"

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
              (exp (curviness_for_calc * x) - 1) /
              (exp (curviness_for_calc) - 1);
          }
      }
      break;
    }
  return val;
}
