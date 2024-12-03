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

#include <cmath>

#include "gui/backend/curve_preset.h"
#include "utils/debug.h"
#include "utils/math.h"
#include "utils/string.h"

using namespace zrythm::dsp;

#if 0
static const char *
curve_algorithm_get_string_id (CurveOptions::Algorithm algo)
{
  using Algorithm = CurveOptions::Algorithm;
  switch (algo)
    {
    case Algorithm::Exponent:
      return "exponent";
    case Algorithm::SuperEllipse:
      return "superellipse";
    case Algorithm::Vital:
      return "vital";
    case Algorithm::Pulse:
      return "pulse";
    case Algorithm::Logarithmic:
      return "logarithmic";
    default:
      return "invalid";
    }
}

static CurveOptions::Algorithm
curve_algorithm_get_from_string_id (const char * str)
{
  using Algorithm = CurveOptions::Algorithm;
  if (string_is_equal (str, "exponent"))
    return Algorithm::Exponent;
  else if (string_is_equal (str, "superellipse"))
    return Algorithm::SuperEllipse;
  else if (string_is_equal (str, "vital"))
    return Algorithm::Vital;
  else if (string_is_equal (str, "pulse"))
    return Algorithm::Pulse;
  else if (string_is_equal (str, "logarithmic"))
    return Algorithm::Logarithmic;

  z_return_val_if_reached (Algorithm::SuperEllipse);
}
#endif

#if 0
gboolean
curve_algorithm_get_g_settings_mapping (
  GValue *   value,
  GVariant * variant,
  gpointer   user_data)
{
  const char * str = g_variant_get_string (variant, nullptr);

  CurveOptions::Algorithm val = curve_algorithm_get_from_string_id (str);
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
    curve_algorithm_get_string_id (static_cast<CurveOptions::Algorithm> (val));

  return g_variant_new_string (str);
}
#endif

CurvePreset::CurvePreset (
  std::string             id,
  QString                 label,
  CurveOptions::Algorithm algo,
  double                  curviness)
    : opts_ (curviness, algo), id_ (std::move (id)), label_ (std::move (label))
{
}

std::vector<CurvePreset>
CurvePreset::get_fade_presets ()
{
  std::vector<CurvePreset> presets;

  presets.emplace_back (
    "linear", QObject::tr ("Linear"), CurveOptions::Algorithm::SuperEllipse, 0);
  presets.emplace_back (
    "exponential", QObject::tr ("Exponential"),
    CurveOptions::Algorithm::Exponent, -0.6);
  presets.emplace_back (
    "elliptic", QObject::tr ("Elliptic"), CurveOptions::Algorithm::SuperEllipse,
    -0.5);
  presets.emplace_back (
    "logarithmic", QObject::tr ("Logarithmic"),
    CurveOptions::Algorithm::Logarithmic, -0.5);
  presets.emplace_back (
    "vital", QObject::tr ("Vital"), CurveOptions::Algorithm::Vital, -0.5);

  return presets;
}

CurvePreset::~CurvePreset () noexcept { }
