// SPDX-FileCopyrightText: © 2020, 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef ZRYTHM_DSP_CURVE_H
#define ZRYTHM_DSP_CURVE_H

#include <string>
#include <utility>
#include <vector>

#include "utils/format.h"
#include "utils/iserializable.h"

namespace zrythm::dsp
{

/**
 * Curve options.
 *
 * Can find more at tracktion_AudioFadeCurve.h.
 */
class CurveOptions
    : public zrythm::utils::serialization::ISerializable<CurveOptions>
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

  /** Bounds for each algorithm. */
  static constexpr double SUPERELLIPSE_CURVINESS_BOUND = 0.82;
  static constexpr double EXPONENT_CURVINESS_BOUND = 0.95;
  static constexpr double VITAL_CURVINESS_BOUND = 1.00;

public:
  // Rule of 0
  CurveOptions () = default;
  CurveOptions (double curviness, Algorithm algo);
  ~CurveOptions () noexcept override;

  /**
   * Returns the Y value on a curve.
   *
   * @param x X-coordinate, normalized.
   * @param start_higher Start at higher point.
   */
  [[gnu::hot]] double get_normalized_y (double x, bool start_higher) const;

  /**
   * Gets the normalized Y for a normalized X, for a fade.
   *
   * @param x Normalized x.
   * @param fade_in 1 for in, 0 for out.
   */
  double get_normalized_y_for_fade (double x, bool fade_in) const
  {
    return get_normalized_y (x, !fade_in);
  }

  DECLARE_DEFINE_FIELDS_METHOD ();

public:
  /** Curviness between -1 and 1, where < 0 tils downwards, > 0
   * tilts upwards and 0 is a straight line. */
  double curviness_{ 0.0 };

  /** Curve algorithm to use. */
  Algorithm algo_{};
};

bool
operator== (const CurveOptions &a, const CurveOptions &b);

} // namespace zrythm::dsp

DEFINE_ENUM_FORMATTER (
  zrythm::dsp::CurveOptions::Algorithm,
  CurveOptions_Algorithm,
  QT_TR_NOOP_UTF8 ("Exponent"),
  QT_TR_NOOP_UTF8 ("Superellipse"),
  "Vital",
  QT_TR_NOOP_UTF8 ("Pulse"),
  QT_TR_NOOP_UTF8 ("Logarithmic"));

#endif
