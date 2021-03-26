/*
 * Copyright (C) 2019 Alexandros Theodotou
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdlib.h>

#include "audio/automation_tracklist.h"
#include "audio/master_track.h"
#include "project.h"

#include <gtk/gtk.h>

void
master_track_init (
  Track * self)
{
  self->type = TRACK_TYPE_MASTER;
  gdk_rgba_parse (&self->color, "#f01010");
  self->icon_name = g_strdup ("effect");
}

void
master_track_setup (MasterTrack * self)
{
  AudioBusTrack * track = (AudioBusTrack *) self;

  audio_bus_track_setup (track);
}
