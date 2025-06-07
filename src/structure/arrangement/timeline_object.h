// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/arrangement/arranger_object.h"

namespace zrythm::structure::arrangement
{

/**
 * @brief Represents an object on the timeline.
 *
 * The TimelineObject class represents an object that can be placed on the
 * timeline, such as an audio clip or MIDI event. It inherits from the
 * ArrangerObject class, which provides common functionality for objects in the
 * arranger.
 */
class TimelineObject : virtual public ArrangerObject
{
protected:
  TimelineObject () noexcept { }

public:
  ~TimelineObject () noexcept override = default;
  Z_DISABLE_COPY_MOVE (TimelineObject)

protected:
  friend void init_from (
    TimelineObject        &obj,
    const TimelineObject  &other,
    utils::ObjectCloneType clone_type);

  BOOST_DESCRIBE_CLASS (TimelineObject, (ArrangerObject), (), (), ())
};

using TimelineObjectVariant = std::
  variant<ScaleObject, MidiRegion, AudioRegion, ChordRegion, AutomationRegion, Marker>;
using TimelineObjectPtrVariant = to_pointer_variant<TimelineObjectVariant>;

}
