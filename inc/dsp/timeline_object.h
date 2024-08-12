// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_TIMELINE_OBJECT_H__
#define __AUDIO_TIMELINE_OBJECT_H__

#include "dsp/arranger_object.h"

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
  virtual ~TimelineObject () = default;

  ArrangerWidget * get_arranger () const final;

protected:
  void copy_members_from (const TimelineObject &other) { }

public:
};

inline bool
operator== (const TimelineObject &rhs, const TimelineObject &lhs)
{
  return true;
}

#endif // __AUDIO_TIMELINE_OBJECT_H__
