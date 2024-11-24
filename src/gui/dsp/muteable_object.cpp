// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/dsp/muteable_object.h"

#include "utils/rt_thread_id.h"

void
MuteableObject::copy_members_from (const MuteableObject &other)
{
  muted_ = other.muted_;
}

void
MuteableObject::init_loaded_base ()
{
}

void
MuteableObject::set_muted (bool muted, bool fire_events)
{
  muted_ = muted;

  if (fire_events)
    {
      // EVENTS_PUSH (EventType::ET_ARRANGER_OBJECT_CHANGED, this);
    }
}
