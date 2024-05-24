// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/recording_event.h"
#include "utils/objects.h"

RecordingEvent *
recording_event_new (void)
{
  return object_new_unresizable (RecordingEvent);
}

void
recording_event_print (RecordingEvent * self)
{
  g_message (
    "%p: type %s track name hash %u", self, ENUM_NAME (self->type),
    self->track_name_hash);
}

void
recording_event_free (RecordingEvent * self)
{
  object_zero_and_free_unresizable (RecordingEvent, self);
}
