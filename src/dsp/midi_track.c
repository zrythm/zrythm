// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * Copyright (C) 2018-2021 Alexandros Theodotou <alex at zrythm dot org>
 */

#include "zrythm-config.h"

#include <inttypes.h>
#include <stdlib.h>

#include "dsp/automation_track.h"
#include "dsp/automation_tracklist.h"
#include "dsp/midi_event.h"
#include "dsp/midi_note.h"
#include "dsp/midi_track.h"
#include "dsp/position.h"
#include "dsp/region.h"
#include "dsp/track.h"
#include "dsp/velocity.h"
#include "gui/widgets/track.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/math.h"
#include "utils/stoat.h"

#include <gtk/gtk.h>

/**
 * Initializes an midi track.
 */
void
midi_track_init (Track * self)
{
  self->type = TRACK_TYPE_MIDI;
  gdk_rgba_parse (&self->color, "#F79616");

  self->icon_name = g_strdup ("signal-midi");
}

void
midi_track_setup (Track * self)
{
  channel_track_setup (self);
}

/**
 * Frees the track.
 *
 * TODO
 */
void
midi_track_free (Track * track)
{
}
