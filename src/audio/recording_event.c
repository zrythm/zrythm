// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "audio/recording_event.h"
#include "utils/objects.h"

static const char * recording_event_type_strings[] = {
  "start track recording",
  "start automation recording",
  "MIDI",
  "audio",
  "automation",
  "pause track recording",
  "pause automation recording",
  "stop track recording",
  "stop automation recording",
};

RecordingEvent *
recording_event_new (void)
{
  return object_new_unresizable (RecordingEvent);
}

void
recording_event_print (RecordingEvent * self)
{
  g_message (
    "%p: type %s track name hash %u", self,
    recording_event_type_strings[self->type],
    self->track_name_hash);
}

void
recording_event_free (RecordingEvent * self)
{
  object_zero_and_free_unresizable (RecordingEvent, self);
}
