// clang-format off
// SPDX-FileCopyrightText: © 2020-2021, 2023 Alexandros Theodotou <alex@zrythm.org>
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
// clang-format on

#include <math.h>

#include "dsp/curve.h"
#include "utils/debug.h"
#include "utils/math.h"
#include "utils/objects.h"
#include "utils/string.h"

#include <glib/gi18n.h>

const char **
curve_algorithm_get_strings (void)
{
  static const char * curve_algorithm_strings[] = {
    N_ ("Exponent"), N_ ("Superellipse"), N_ ("Vital"),
    N_ ("Pulse"),    N_ ("Logarithmic"),
  };
  return curve_algorithm_strings;
}

const char *
curve_algorithm_to_str (CurveAlgorithm algo)
{
  const char ** strs = curve_algorithm_get_strings ();

  return strs[static_cast<int> (algo)];
}

void
curve_opts_init (CurveOptions * opts)
{
  opts->schema_version = CURVE_OPTIONS_SCHEMA_VERSION;
}

/**
 * Stores the localized name of the algorithm in
 * \ref buf.
 */
void
curve_algorithm_get_localized_name (CurveAlgorithm algo, char * buf)
{
  switch (algo)
    {
    case CurveAlgorithm::EXPONENT:
      sprintf (buf, _ ("Exponent"));
      break;
    case CurveAlgorithm::SUPERELLIPSE:
      sprintf (buf, _ ("Superellipse"));
      break;
    case CurveAlgorithm::VITAL:
      sprintf (buf, "Vital");
      break;
    case CurveAlgorithm::PULSE:
      sprintf (buf, _ ("Pulse"));
      break;
    case CurveAlgorithm::LOGARITHMIC:
      sprintf (buf, _ ("Logarithmic"));
      break;
    default:
      g_return_if_reached ();
    }
  return;
}

static const char *
curve_algorithm_get_string_id (CurveAlgorithm algo)
{
  switch (algo)
    {
    case CurveAlgorithm::EXPONENT:
      return "exponent";
    case CurveAlgorithm::SUPERELLIPSE:
      return "superellipse";
    case CurveAlgorithm::VITAL:
      return "vital";
    case CurveAlgorithm::PULSE:
      return "pulse";
    case CurveAlgorithm::LOGARITHMIC:
      return "logarithmic";
    default:
      return "invalid";
    }
}

static CurveAlgorithm
curve_algorithm_get_from_string_id (const char * str)
{
  if (string_is_equal (str, "exponent"))
    return CurveAlgorithm::EXPONENT;
  else if (string_is_equal (str, "superellipse"))
    return CurveAlgorithm::SUPERELLIPSE;
  else if (string_is_equal (str, "vital"))
    return CurveAlgorithm::VITAL;
  else if (string_is_equal (str, "pulse"))
    return CurveAlgorithm::PULSE;
  else if (string_is_equal (str, "logarithmic"))
    return CurveAlgorithm::LOGARITHMIC;

  g_return_val_if_reached (CurveAlgorithm::SUPERELLIPSE);
}

gboolean
curve_algorithm_get_g_settings_mapping (
  GValue *   value,
  GVariant * variant,
  gpointer   user_data)
{
  const char * str = g_variant_get_string (variant, NULL);

  CurveAlgorithm val = curve_algorithm_get_from_string_id (str);
  g_value_set_uint (value, static_cast<guint> (val));

  return true;
}

GVariant *
curve_algorithm_set_g_settings_mapping (
  const GValue *       value,
  const GVariantType * expected_type,
  gpointer             user_data)
{
  guint val = g_value_get_uint (value);

  const char * str =
    curve_algorithm_get_string_id (static_cast<CurveAlgorithm> (val));

  return g_variant_new_string (str);
}

/**
 * Returns the Y value on a curve specified by
 * \ref algo.
 *
 * @param x X-coordinate, normalized.
 * @param opts Curve options.
 * @param start_higher Start at higher point.
 */
double
curve_get_normalized_y (double x, CurveOptions * opts, int start_higher)
{
  z_return_val_if_fail_cmp (x, >=, 0.0, 0.0);
  z_return_val_if_fail_cmp (x, <=, 1.0, 0.0);

  int curve_up = opts->curviness >= 0;

  double val = -1.0;
  switch (opts->algo)
    {
    case CurveAlgorithm::EXPONENT:
      {
        /* convert curviness to bound */
        double curviness_for_calc =
          opts->curviness * CURVE_EXPONENT_CURVINESS_BOUND;

        curviness_for_calc = 1.0 - fabs (curviness_for_calc);
        if (!start_higher)
          x = 1.0 - x;
        if (curve_up)
          x = 1.0 - x;

        /* if curviness is 0, it is a straight line */
        if (math_doubles_equal (curviness_for_calc, 0.0000))
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
    case CurveAlgorithm::SUPERELLIPSE:
      {
        /* convert curviness to bound */
        double curviness_for_calc =
          opts->curviness * CURVE_SUPERELLIPSE_CURVINESS_BOUND;

        curviness_for_calc = 1.0 - fabs (curviness_for_calc);
        if (!start_higher)
          x = 1.0 - x;
        if (curve_up)
          x = 1.0 - x;

        /* if curviness is 0, it is a straight line */
        if (math_doubles_equal (curviness_for_calc, 0.0000))
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
    case CurveAlgorithm::VITAL:
      {
        /* convert curviness to bound */
        double curviness_for_calc =
          opts->curviness * CURVE_VITAL_CURVINESS_BOUND;

        curviness_for_calc = -curviness_for_calc * 10.0;
        if (start_higher)
          x = 1.0 - x;

        /* if curviness is 0, it is a straight line */
        if (math_doubles_equal (curviness_for_calc, 0.0000))
          {
            val = x;
          }
        else
          {
            val = expm1 (curviness_for_calc * x) / expm1 (curviness_for_calc);
          }
      }
      break;
    case CurveAlgorithm::PULSE:
      {
        val = ((1.0 + opts->curviness) / 2.0) > x ? 0.0 : 1.0;

        if (start_higher)
          val = 1.0 - val;
      }
      break;
    case CurveAlgorithm::LOGARITHMIC:
      {
        /* convert curviness to bound */
        static const float bound = 1e-12f;
        float              s =
          CLAMP (fabsf ((float) opts->curviness), 0.01f, 1 - bound) * 10.f;
        float curviness_for_calc =
          CLAMP ((10.f - s) / (powf (s, s)), bound, 10.f);

        if (!start_higher)
          x = 1.0 - x;
        if (curve_up)
          x = 1.0 - x;

#if 0
        g_message (
          "curviness (z): %f (for calc %f), x %f",
          opts->curviness,
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
            const float a = math_fast_log (curviness_for_calc);
            const float b =
              1.f / math_fast_log (1.f + (1.f / curviness_for_calc));

            float fval;
            if (curve_up)
              {
                fval = (math_fast_log ((float) x + curviness_for_calc) - a) * b;
              }
            else
              {
                fval =
                  (a - math_fast_log ((float) x + curviness_for_calc)) * b + 1.f;
              }
            val = (double) fval;
          }
      }
      break;
    default:
      g_return_val_if_reached (-1);
    }
  return CLAMP (val, 0.0, 1.0);
}

static CurveFadePreset *
curve_fade_preset_create (
  const char *   id,
  const char *   label,
  CurveAlgorithm algo,
  double         curviness)
{
  CurveFadePreset * preset = object_new (CurveFadePreset);

  preset->id = g_strdup (id);
  preset->label = g_strdup (label);
  preset->opts.algo = algo;
  preset->opts.curviness = curviness;

  return preset;
}

static void
curve_fade_preset_free (gpointer data)
{
  CurveFadePreset * preset = (CurveFadePreset *) data;
  g_free_and_null (preset->id);
  g_free_and_null (preset->label);
  object_zero_and_free (preset);
}

/**
 * Returns an array of CurveFadePreset.
 */
GPtrArray *
curve_get_fade_presets (void)
{
  GPtrArray * arr = g_ptr_array_new_with_free_func (curve_fade_preset_free);
  g_ptr_array_add (
    arr,
    curve_fade_preset_create (
      "linear", _ ("Linear"), CurveAlgorithm::SUPERELLIPSE, 0));
  g_ptr_array_add (
    arr,
    curve_fade_preset_create (
      "exponential", _ ("Exponential"), CurveAlgorithm::EXPONENT, -0.6));
  g_ptr_array_add (
    arr,
    curve_fade_preset_create (
      "elliptic", _ ("Elliptic"), CurveAlgorithm::SUPERELLIPSE, -0.5));
  g_ptr_array_add (
    arr,
    curve_fade_preset_create (
      "logarithmic", _ ("Logarithmic"), CurveAlgorithm::LOGARITHMIC, -0.5));
  g_ptr_array_add (
    arr,
    curve_fade_preset_create ("vital", _ ("Vital"), CurveAlgorithm::VITAL, -0.5));

  return arr;
}

bool
curve_options_are_equal (const CurveOptions * a, const CurveOptions * b)
{
  return a->algo == b->algo && math_doubles_equal (a->curviness, b->curviness);
}
