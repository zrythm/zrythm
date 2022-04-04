// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 */

#include <stdlib.h>

#include "audio/automation_tracklist.h"
#include "audio/master_track.h"
#include "project.h"

#include <gtk/gtk.h>

void
master_track_init (Track * self)
{
  self->type = TRACK_TYPE_MASTER;
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
