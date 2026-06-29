// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/tick_types.h"
#include "structure/arrangement/arranger_object_all.h"
#include "utils/units.h"

#include "arranger_object_helper.h"

namespace zrythm::gui::qquick
{

double
ArrangerObjectHelper::timelineTicks (
  structure::arrangement::ArrangerObject * obj)
{
  if (obj == nullptr)
    return 0.0;
  return structure::arrangement::timeline_ticks (*obj).asDouble ();
}

double
ArrangerObjectHelper::timelineEndTicks (
  structure::arrangement::ArrangerObject * obj)
{
  if (obj == nullptr)
    return 0.0;
  return structure::arrangement::timeline_end_ticks (*obj).asDouble ();
}

bool
ArrangerObjectHelper::isClip (structure::arrangement::ArrangerObject * obj)
{
  if (obj == nullptr)
    return false;
  return structure::arrangement::is_clip (*obj);
}

void
ArrangerObjectHelper::setEndFromTimelineTicks (
  structure::arrangement::ArrangerObject * obj,
  double                                   timeline_end_ticks)
{
  if (obj == nullptr)
    return;
  structure::arrangement::set_end_from_timeline_ticks (
    *obj, dsp::TimelineTick{ units::ticks (timeline_end_ticks) });
}

} // namespace zrythm::gui::qquick
