// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * SPDX-FileCopyrightText: © 2018-2021 Alexandros Theodotou <alex@zrythm.org>
 */

#include "zrythm-config.h"

#include <cstdlib>

#include <inttypes.h>

#include "dsp/automation_track.h"
#include "dsp/automation_tracklist.h"
#include "dsp/channel_track.h"
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

#include "gtk_wrapper.h"

/**
 * Initializes an midi track.
 */
void
midi_track_init (Track * self)
{
  self->type = TrackType::TRACK_TYPE_MIDI;
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
