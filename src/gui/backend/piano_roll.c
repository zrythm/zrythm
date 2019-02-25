/*
 * Copyright (C) 2018-2019 Alexandros Theodotou
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Piano roll backend.
 *
 * This is meant to be serialized along with each project.
 */

#include <stdlib.h>

#include "audio/channel.h"
#include "gui/backend/piano_roll.h"
#include "audio/track.h"
#include "gui/widgets/piano_roll.h"
#include "project.h"

/**
 * Sets the track and refreshes the piano roll widgets.
 *
 * To be called only from GTK threads.
 */
void
piano_roll_set_region (Region * region)
{
  PianoRoll * self = PIANO_ROLL;

  if (region->track->type != TRACK_TYPE_INSTRUMENT)
    {
      g_warning ("Track is not an instrument track");
      return;
    }
  /*InstrumentTrack * it = (InstrumentTrack *) channel->track;*/
  if (self->region)
    {
      channel_reattach_midi_editor_manual_press_port (
        track_get_channel (region->track),
        0);
    }
  channel_reattach_midi_editor_manual_press_port (
    track_get_channel (region->track),
    1);
  self->region = region;
  self->region_id = region->id;

  piano_roll_widget_region_updated ();
}

void
piano_roll_init (PianoRoll * self)
{
  self->notes_zoom = 1; /* FIXME */

  self->midi_modifier = MIDI_MODIFIER_VELOCITY;
}
