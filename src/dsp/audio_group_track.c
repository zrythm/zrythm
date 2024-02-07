// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * SPDX-FileCopyrightText: © 2018-2019 Alexandros Theodotou <alex@zrythm.org>
 */

/**
 * \file
 *
 * Track logic specific to bus tracks.
 */

#include <stdlib.h>

#include "dsp/audio_group_track.h"
#include "dsp/automation_tracklist.h"
#include "dsp/channel_track.h"
#include "project.h"

void
audio_group_track_init (Track * self)
{
  self->type = TRACK_TYPE_AUDIO_GROUP;
  /* GTK color picker color */
  gdk_rgba_parse (&self->color, "#26A269");
  self->icon_name = g_strdup ("effect");
}

void
audio_group_track_setup (Track * self)
{
  channel_track_setup (self);
}
