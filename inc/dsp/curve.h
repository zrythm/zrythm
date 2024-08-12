// SPDX-FileCopyrightText: © 2020, 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_CURVE_H__
#define __AUDIO_CURVE_H__

#include <string>
#include <utility>
#include <vector>

#include "io/serialization/iserializable.h"
#include "utils/format.h"

#include <glib/gi18n.h>

/**
 * @addtogroup dsp
 *
 * @{
 */

/** Bounds for each algorithm. */
constexpr double CURVE_SUPERELLIPSE_CURVINESS_BOUND = 0.82;
constexpr double CURVE_EXPONENT_CURVINESS_BOUND = 0.95;
constexpr double CURVE_VITAL_CURVINESS_BOUND = 1.00;

/**
 * Curve options.
 *
 * Can find more at tracktion_AudioFadeCurve.h.
 */
class CurveOptions : public ISerializable<CurveOptions>
{
public:
  /**
   * The algorithm to use for curves.
   *
   * See https://www.desmos.com/calculator/typjsyykvb
   */
  enum class Algorithm
  {
    /**
     * y = x^n
     * 0 < n <= 1,
     *   where the whole thing is tilting up and 0 is
     *   full tilt and 1 is straight line (when starting
     *   at lower point).
     */
    Exponent,

    /**
     * y = 1 - (1 - x^n)^(1/n)
     * 0 < n <= 1,
     *   where the whole thing is tilting up and 0 is
     *   full tilt and 1 is straight line (when starting
     *   at lower point).
     *
     * See
     * https://stackoverflow.com/questions/17623152/how-map-tween-a-number-based-on-a-dynamic-curve
     */
    SuperEllipse,

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
    Vital,

    /**
     * Pulse (square).
     */
    Pulse,

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
    Logarithmic,
  };

public:
  // Rule of 0
  CurveOptions () = default;
  CurveOptions (double curviness, Algorithm algo)
      : curviness_ (curviness), algo_ (algo){};

  /**
   * Returns the Y value on a curve.
   *
   * @param x X-coordinate, normalized.
   * @param start_higher Start at higher point.
   */
  HOT double get_normalized_y (double x, bool start_higher) const;

  DECLARE_DEFINE_FIELDS_METHOD ();

public:
  /** Curviness between -1 and 1, where < 0 tils downwards, > 0
   * tilts upwards and 0 is a straight line. */
  double curviness_ = 0.0;

  /** Curve algorithm to use. */
  Algorithm algo_ = (Algorithm) 0;
};

bool
operator== (const CurveOptions &a, const CurveOptions &b);

struct CurveFadePreset
{
  // Rule of 0
  CurveFadePreset () = default;
  CurveFadePreset (
    std::string             id,
    std::string             label,
    CurveOptions::Algorithm algo,
    double                  curviness)
      : opts_ (curviness, algo), id_ (std::move (id)), label_ (std::move (label))
  {
  }

  /**
   * Returns an array of CurveFadePreset.
   */
  static std::vector<CurveFadePreset> get_fade_presets ();

  CurveOptions opts_;
  std::string  id_;
  std::string  label_;
};

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
 * Gets the normalized Y for a normalized X.
 *
 * @param x Normalized x.
 * @param fade_in 1 for in, 0 for out.
 */
inline double
fade_get_y_normalized (const CurveOptions &opts, double x, bool fade_in)
{
  return opts.get_normalized_y (x, !fade_in);
}

DEFINE_ENUM_FORMATTER (
  CurveOptions::Algorithm,
  CurveOptions_Algorithm,
  N_ ("Exponent"),
  N_ ("Superellipse"),
  "Vital",
  N_ ("Pulse"),
  N_ ("Logarithmic"));

/**
 * @}
 */

#endif
