// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * SPDX-FileCopyrightText: © 2019 Alexandros Theodotou <alex@zrythm.org>
 */

/**
 * \file
 *
 * Track logic specific to MIDI group tracks.
 */

#include <stdlib.h>

#include "dsp/automation_tracklist.h"
#include "dsp/midi_group_track.h"
#include "project.h"

void
midi_group_track_init (Track * self)
{
  self->type = TrackType::TRACK_TYPE_MIDI_GROUP;
  /* GTK color picker color */
  gdk_rgba_parse (&self->color, "#E66100");
  self->icon_name = g_strdup ("signal-midi");
}

void
midi_group_track_setup (Track * self)
{
  channel_track_setup ((ChannelTrack *) self);
}
