// SPDX-FileCopyrightText: © 2020-2021, 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * This file incorporates work covered by the following copyright and
 * permission notices:
 *
 * ---
 *
 * Copyright (C) 2005-2006 Taybin Rutkin <taybin@taybin.com>
 * Copyright (C) 2007-2013 Paul Davis <paul@linuxaudiosystems.com>
 * Copyright (C) 2009 David Robillard <d@drobilla.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * ---
 */

#include <cmath>

#include "dsp/curve.h"
#include "utils/debug.h"
#include "utils/math.h"
#include "utils/string.h"

namespace zrythm::dsp
{

CurveOptions::CurveOptions (double curviness, Algorithm algo)
    : curviness_ (curviness), algo_ (algo)
{
}

double
CurveOptions::get_normalized_y (double x, bool start_higher) const
{
  z_return_val_if_fail_cmp (x, >=, 0.0, 0.0);
  z_return_val_if_fail_cmp (x, <=, 1.0, 0.0);

  int curve_up = curviness_ >= 0;

  double val = -1.0;
  switch (algo_)
    {
    case CurveOptions::Algorithm::Exponent:
      {
        /* convert curviness to bound */
        double curviness_for_calc = curviness_ * EXPONENT_CURVINESS_BOUND;

        curviness_for_calc = 1.0 - fabs (curviness_for_calc);
        if (!start_higher)
          x = 1.0 - x;
        if (curve_up)
          x = 1.0 - x;

        /* if curviness is 0, it is a straight line */
        if (utils::math::floats_equal (curviness_for_calc, 0.0000))
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
    case CurveOptions::Algorithm::SuperEllipse:
      {
        /* convert curviness to bound */
        double curviness_for_calc = curviness_ * SUPERELLIPSE_CURVINESS_BOUND;

        curviness_for_calc = 1.0 - fabs (curviness_for_calc);
        if (!start_higher)
          x = 1.0 - x;
        if (curve_up)
          x = 1.0 - x;

        /* if curviness is 0, it is a straight line */
        if (utils::math::floats_equal (curviness_for_calc, 0.0000))
          {
            val = x;
          }
        else
          {
            val = pow (
              1.0 - pow (x, curviness_for_calc), (1.0 / curviness_for_calc));
          }

        if (curve_up)
          val = 1.0 - val;
      }
      break;
    case CurveOptions::Algorithm::Vital:
      {
        /* convert curviness to bound */
        double curviness_for_calc = curviness_ * VITAL_CURVINESS_BOUND;

        curviness_for_calc = -curviness_for_calc * 10.0;
        if (start_higher)
          x = 1.0 - x;

        /* if curviness is 0, it is a straight line */
        if (utils::math::floats_equal (curviness_for_calc, 0.0000))
          {
            val = x;
          }
        else
          {
            val = expm1 (curviness_for_calc * x) / expm1 (curviness_for_calc);
          }
      }
      break;
    case CurveOptions::Algorithm::Pulse:
      {
        val = ((1.0 + curviness_) / 2.0) > x ? 0.0 : 1.0;

        if (start_higher)
          val = 1.0 - val;
      }
      break;
    case CurveOptions::Algorithm::Logarithmic:
      {
        /* convert curviness to bound */
        static const float bound = 1e-12f;
        float              s =
          std::clamp (
            static_cast<float> (std::fabs (curviness_)), 0.01f, 1.f - bound)
          * 10.f;
        float curviness_for_calc =
          std::clamp ((10.f - s) / (std::pow<float> (s, s)), bound, 10.f);

        if (!start_higher)
          x = 1.0 - x;
        if (curve_up)
          x = 1.0 - x;

#if 0
        z_info (
          "curviness (z): {:f} (for calc {:f}), x {:f}",
          curviness_,
          (double) curviness_for_calc, x);
#endif

        /* if close to the center, use precise
         * log (fast_log doesn't handle that well) */
        if (curviness_for_calc >= 0.02f)
          {
            /* required vals */
            const float a = logf (curviness_for_calc);
            const float b = 1.f / logf (1.f + (1.f / curviness_for_calc));

            float fval;
            if (curve_up)
              {
                fval = (logf ((float) x + curviness_for_calc) - a) * b;
              }
            else
              {
                fval = (a - logf ((float) x + curviness_for_calc)) * b + 1.f;
              }
            val = (double) fval;
          }
        else
          {
            /* required vals */
            const float a = utils::math::fast_log (curviness_for_calc);
            const float b =
              1.f / utils::math::fast_log (1.f + (1.f / curviness_for_calc));

            float fval;
            if (curve_up)
              {
                fval =
                  (utils::math::fast_log ((float) x + curviness_for_calc) - a)
                  * b;
              }
            else
              {
                fval =
                  (a - utils::math::fast_log ((float) x + curviness_for_calc)) * b
                  + 1.f;
              }
            val = (double) fval;
          }
      }
      break;
    default:
      z_return_val_if_reached (-1);
    }
  return std::clamp (val, 0.0, 1.0);
}

void
CurveOptions::define_fields (const Context &ctx)
{
  serialize_fields (
    ctx, make_field ("algorithm", algo_), make_field ("curviness", curviness_));
}

CurveOptions::~CurveOptions () noexcept = default;

bool
operator== (const CurveOptions &a, const CurveOptions &b)
{
  return utils::math::floats_equal (a.curviness_, b.curviness_)
         && a.algo_ == b.algo_;
}

} // namespace zrythm::dsp
