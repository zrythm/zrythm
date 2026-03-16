// SPDX-FileCopyrightText: © 2018-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/automation_point.h"

namespace zrythm::structure::arrangement
{
AutomationPoint::AutomationPoint (
  const dsp::TempoMap &tempo_map,
  QObject *            parent)
    : ArrangerObject (Type::AutomationPoint, tempo_map, {}, parent),
      curve_opts_adapter_ (
        utils::make_qobject_unique<dsp::CurveOptionsQmlAdapter> (curve_opts_, this))
{
  QObject::connect (
    this, &AutomationPoint::valueChanged, this,
    &ArrangerObject::propertiesChanged);
  QObject::connect (
    curveOpts (), &dsp::CurveOptionsQmlAdapter::algorithmChanged, this,
    &ArrangerObject::propertiesChanged);
  QObject::connect (
    curveOpts (), &dsp::CurveOptionsQmlAdapter::curvinessChanged, this,
    &ArrangerObject::propertiesChanged);
}

void
init_from (
  AutomationPoint       &obj,
  const AutomationPoint &other,
  utils::ObjectCloneType clone_type)
{
  obj.normalized_value_ = other.normalized_value_;
  obj.curve_opts_ = other.curve_opts_;
  init_from (
    static_cast<ArrangerObject &> (obj),
    static_cast<const ArrangerObject &> (other), clone_type);
}

AutomationPoint::~AutomationPoint () = default;

void
to_json (nlohmann::json &j, const AutomationPoint &point)
{
  to_json (j, static_cast<const ArrangerObject &> (point));
  j[AutomationPoint::kNormalizedValueKey] = point.normalized_value_;
  j[AutomationPoint::kCurveOptionsKey] = point.curve_opts_;
}
void
from_json (const nlohmann::json &j, AutomationPoint &point)
{
  from_json (j, static_cast<ArrangerObject &> (point));
  j.at (AutomationPoint::kNormalizedValueKey).get_to (point.normalized_value_);
  j.at (AutomationPoint::kCurveOptionsKey).get_to (point.curve_opts_);
}
}
