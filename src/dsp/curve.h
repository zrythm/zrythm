// SPDX-FileCopyrightText: © 2020, 2023-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "utils/format.h"

#include <QtQmlIntegration>

#include <nlohmann/json.hpp>

namespace zrythm::dsp
{

/**
 * Curve options.
 *
 * Can find more at tracktion_AudioFadeCurve.h.
 */
class CurveOptions final
{
  Q_GADGET

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
  Q_ENUM (Algorithm)

  /** Bounds for each algorithm. */
  static constexpr double SUPERELLIPSE_CURVINESS_BOUND = 0.82;
  static constexpr double EXPONENT_CURVINESS_BOUND = 0.95;
  static constexpr double VITAL_CURVINESS_BOUND = 1.00;

public:
  // Rule of 0
  CurveOptions () = default;
  CurveOptions (double curviness, Algorithm algo);

  /**
   * Returns the Y value on a curve.
   *
   * @param x X-coordinate, normalized.
   * @param start_higher Start at higher point.
   */
  [[gnu::hot]] double get_normalized_y (double x, bool start_higher) const;

  friend bool operator== (const CurveOptions &a, const CurveOptions &b);

  NLOHMANN_DEFINE_TYPE_INTRUSIVE (CurveOptions, algo_, curviness_)

public:
  /** Curviness between -1 and 1, where < 0 tils downwards, > 0
   * tilts upwards and 0 is a straight line. */
  double curviness_{ 0.0 };

  /** Curve algorithm to use. */
  Algorithm algo_{};

  BOOST_DESCRIBE_CLASS (CurveOptions, (), (curviness_, algo_), (), ())
};

/**
 * @brief QML adapter for CurveOptions.
 */
class CurveOptionsQmlAdapter : public QObject
{
  Q_OBJECT
  Q_PROPERTY (
    double curviness READ curviness WRITE setCurviness NOTIFY curvinessChanged)
  Q_PROPERTY (
    zrythm::dsp::CurveOptions::Algorithm algorithm READ algorithm WRITE
      setAlgorithm NOTIFY algorithmChanged)
  QML_NAMED_ELEMENT (CurveOptions)
  QML_UNCREATABLE ("")

public:
  CurveOptionsQmlAdapter (CurveOptions &options, QObject * parent = nullptr)
      : QObject (parent), options_ (options)
  {
  }

  // ========================================================================
  // QML Interface
  // ========================================================================

  double curviness () const { return options_.curviness_; }
  void   setCurviness (double curviness)
  {
    if (qFuzzyCompare (options_.curviness_, curviness))
      return;

    curviness = std::clamp (curviness, -1.0, 1.0);
    options_.curviness_ = curviness;
    Q_EMIT curvinessChanged (curviness);
  }
  Q_SIGNAL void curvinessChanged (double curviness);

  zrythm::dsp::CurveOptions::Algorithm algorithm () const
  {
    return options_.algo_;
  }
  void setAlgorithm (zrythm::dsp::CurveOptions::Algorithm algorithm)
  {
    if (options_.algo_ == algorithm)
      return;

    options_.algo_ = algorithm;
    Q_EMIT algorithmChanged (algorithm);
  }
  Q_SIGNAL void
  algorithmChanged (zrythm::dsp::CurveOptions::Algorithm algorithm);

  Q_INVOKABLE double normalizedY (double x, bool startHigher) const
  {
    return options_.get_normalized_y (x, startHigher);
  }

  // ========================================================================

private:
  CurveOptions &options_;
};

} // namespace zrythm::dsp

DEFINE_ENUM_FORMATTER (
  zrythm::dsp::CurveOptions::Algorithm,
  CurveOptions_Algorithm,
  QT_TR_NOOP_UTF8 ("Exponent"),
  QT_TR_NOOP_UTF8 ("Superellipse"),
  "Vital",
  QT_TR_NOOP_UTF8 ("Pulse"),
  QT_TR_NOOP_UTF8 ("Logarithmic"));
