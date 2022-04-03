// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
 */

#include <stdlib.h>

#include "audio/automation_tracklist.h"
#include "audio/channel_track.h"

void
channel_track_setup (ChannelTrack * self)
{
  Track * track = (Track *) self;

  automation_tracklist_init (
    &self->automation_tracklist, track);
}

/**
 * Frees the track.
 *
 * TODO
 */
void
channel_track_free (ChannelTrack * track)
{
}
