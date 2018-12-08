/*
 * audio/track.c - the back end for a timeline track
 *
 * Copyright (C) 2018 Alexandros Theodotou
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

#include <stdlib.h>

#include "audio/track.h"
#include "audio/instrument_track.h"

void
track_init (Track * track)
{
}

/**
 * Wrapper for each track type.
 */
void
track_update_automation_tracks (Track * track)
{
  switch (track->type)
    {
    case TRACK_TYPE_INSTRUMENT:
      instrument_track_update_automation_tracks (
        (InstrumentTrack *) track);
      break;
    case TRACK_TYPE_MASTER:
    case TRACK_TYPE_AUDIO:
    case TRACK_TYPE_CHORD:
    case TRACK_TYPE_BUS:
      break;
    }
}

/**
 * Wrapper for each track type.
 */
void
track_free (Track * track)
{
  switch (track->type)
    {
    case TRACK_TYPE_INSTRUMENT:
      instrument_track_free (
        (InstrumentTrack *) track);
      break;
    case TRACK_TYPE_MASTER:
    case TRACK_TYPE_AUDIO:
    case TRACK_TYPE_CHORD:
    case TRACK_TYPE_BUS:
      break;
    }
}
