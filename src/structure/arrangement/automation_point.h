// SPDX-FileCopyrightText: © 2018-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/curve.h"
#include "structure/arrangement/arranger_object.h"
#include "utils/icloneable.h"

namespace zrythm::structure::arrangement
{
/**
 * An automation point inside an AutomationTrack.
 */
class AutomationPoint final : public ArrangerObject
{
  Q_OBJECT
  Q_PROPERTY (float value READ value WRITE setValue NOTIFY valueChanged)
  Q_PROPERTY (dsp::CurveOptionsQmlAdapter * curveOpts READ curveOpts CONSTANT)
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  AutomationPoint (const dsp::TempoMap &tempo_map, QObject * parent = nullptr);
  Z_DISABLE_COPY_MOVE (AutomationPoint)
  ~AutomationPoint () override = default;

  // ========================================================================
  // QML Interface
  // ========================================================================

  float value () const { return normalized_value_; }
  void  setValue (float dval)
  {
    const auto val = dval;
    if (qFuzzyCompare (normalized_value_, val))
      return;

    normalized_value_ = val;
    Q_EMIT valueChanged (dval);
  }
  Q_SIGNAL void valueChanged (float);

  dsp::CurveOptionsQmlAdapter * curveOpts () const
  {
    return curve_opts_adapter_.get ();
  }

  // ========================================================================

private:
  friend void init_from (
    AutomationPoint       &obj,
    const AutomationPoint &other,
    utils::ObjectCloneType clone_type);

  static constexpr auto kNormalizedValueKey = "normalized_value"sv;
  static constexpr auto kCurveOptionsKey = "curve_options"sv;
  friend void to_json (nlohmann::json &j, const AutomationPoint &point)
  {
    to_json (j, static_cast<const ArrangerObject &> (point));
    j[kNormalizedValueKey] = point.normalized_value_;
    j[kCurveOptionsKey] = point.curve_opts_;
  }
  friend void from_json (const nlohmann::json &j, AutomationPoint &point)
  {
    from_json (j, static_cast<ArrangerObject &> (point));
    j.at (kNormalizedValueKey).get_to (point.normalized_value_);
    j.at (kCurveOptionsKey).get_to (point.curve_opts_);
  }

private:
  /** Normalized value (0 to 1) used as a cache. */
  float normalized_value_ = 0.f;

  dsp::CurveOptions                                    curve_opts_;
  utils::QObjectUniquePtr<dsp::CurveOptionsQmlAdapter> curve_opts_adapter_;

  BOOST_DESCRIBE_CLASS (
    AutomationPoint,
    (ArrangerObject),
    (),
    (),
    (normalized_value_, curve_opts_))
};

} // namespace zrythm::structure::arrangement
