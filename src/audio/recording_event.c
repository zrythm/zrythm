/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version approved by
 * Alexandros Theodotou in a public statement of acceptance.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "audio/recording_event.h"

static const char *
recording_event_type_strings[] =
{
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
  return calloc (1, sizeof (RecordingEvent));
}

void
recording_event_print (
  RecordingEvent * self)
{
  g_message (
    "%p: type %s track %s", self,
    recording_event_type_strings[self->type],
    self->track_name);
}

void
recording_event_free (
  RecordingEvent * self)
{
  free (self);
}
