// SPDX-FileCopyrightText: © 2020, 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef ZRYTHM_GUI_CURVE_PRESET_H
#define ZRYTHM_GUI_CURVE_PRESET_H

#include <string>
#include <utility>
#include <vector>

#include "dsp/curve.h"

struct CurvePreset final
{
  // Rule of 0
  CurvePreset () = default;
  CurvePreset (
    std::string                          id,
    QString                              label,
    zrythm::dsp::CurveOptions::Algorithm algo,
    double                               curviness);

  /**
   * Returns an array of CurveFadePreset.
   */
  static std::vector<CurvePreset> get_fade_presets ();

  zrythm::dsp::CurveOptions opts_;
  std::string               id_;
  QString                   label_;
};

#if 0
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
#endif

#endif
