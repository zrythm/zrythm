// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
 */

/**
 * \file
 *
 * Track logic specific to bus tracks.
 */

#include <stdlib.h>

#include "audio/automation_tracklist.h"
#include "audio/midi_bus_track.h"
#include "project.h"

void
midi_bus_track_init (Track * self)
{
  self->type = TRACK_TYPE_MIDI_BUS;
  /* GTK color picker color */
  gdk_rgba_parse (&self->color, "#F5C211");
  self->icon_name = g_strdup ("signal-midi");
}

void
midi_bus_track_setup (MidiBusTrack * self)
{
  channel_track_setup ((ChannelTrack *) self);
}
