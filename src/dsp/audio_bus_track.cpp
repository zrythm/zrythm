// SPDX-FileCopyrightText: © 2018-2019 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Track logic specific to bus tracks.
 */

#include <cstdlib>

#include "dsp/audio_bus_track.h"
#include "dsp/automation_tracklist.h"
#include "project.h"

void
audio_bus_track_init (Track * self)
{
  self->type = TrackType::TRACK_TYPE_AUDIO_BUS;
  /* GTK color picker color */
  gdk_rgba_parse (&self->color, "#33D17A");
  self->icon_name = g_strdup ("effect");
}

void
audio_bus_track_setup (AudioBusTrack * self)
{
  channel_track_setup ((ChannelTrack *) self);
}
