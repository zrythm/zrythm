// SPDX-FileCopyrightText: © 2018-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/region.h"

namespace zrythm::structure::arrangement
{

RegionMixin::RegionMixin (const dsp::AtomicPositionQmlAdapter &start_position)
    : bounds_ (
        utils::make_qobject_unique<ArrangerObjectBounds> (start_position, this)),
      loop_range_ (
        utils::make_qobject_unique<ArrangerObjectLoopRange> (*bounds_, this)),
      name_ (utils::make_qobject_unique<ArrangerObjectName> (this)),
      color_ (utils::make_qobject_unique<ArrangerObjectColor> (this)),
      mute_ (utils::make_qobject_unique<ArrangerObjectMuteFunctionality> (this))
{
}
}
