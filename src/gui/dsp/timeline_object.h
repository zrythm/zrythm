// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_TIMELINE_OBJECT_H__
#define __AUDIO_TIMELINE_OBJECT_H__

#include "gui/dsp/arranger_object.h"

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
public:
  ~TimelineObject () override = default;

protected:
  void
  copy_members_from (const TimelineObject &other, ObjectCloneType clone_type);

  void init_loaded_base ();

public:
};

inline bool
operator== (const TimelineObject &rhs, const TimelineObject &lhs)
{
  return true;
}

using TimelineObjectVariant = std::
  variant<ScaleObject, MidiRegion, AudioRegion, ChordRegion, AutomationRegion, Marker>;
using TimelineObjectPtrVariant = to_pointer_variant<TimelineObjectVariant>;

#endif // __AUDIO_TIMELINE_OBJECT_H__
