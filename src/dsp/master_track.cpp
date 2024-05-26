// SPDX-FileCopyrightText: © 2019 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cstdlib>

#include "dsp/automation_tracklist.h"
#include "dsp/master_track.h"
#include "project.h"

#include <gtk/gtk.h>

void
master_track_init (Track * self)
{
  self->type = TrackType::TRACK_TYPE_MASTER;
  /* GTK color picker color */
  gdk_rgba_parse (&self->color, "#C01C28");
  self->icon_name = g_strdup ("effect");
}

void
master_track_setup (MasterTrack * self)
{
  AudioBusTrack * track = (AudioBusTrack *) self;

  audio_bus_track_setup (track);
}
