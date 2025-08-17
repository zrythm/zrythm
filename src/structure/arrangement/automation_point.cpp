// SPDX-FileCopyrightText: © 2018-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/automation_point.h"

namespace zrythm::structure::arrangement
{
AutomationPoint::AutomationPoint (
  const dsp::TempoMap &tempo_map,
  QObject *            parent)
    : ArrangerObject (Type::AutomationPoint, tempo_map, parent),
      curve_opts_adapter_ (
        utils::make_qobject_unique<dsp::CurveOptionsQmlAdapter> (curve_opts_, this))
{
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
}
