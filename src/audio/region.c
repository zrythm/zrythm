/*
 * audio/region.c - A region in the timeline having a start
 *   and an end
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

#include "audio/region.h"
#include "audio/track.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/region.h"

Region *
region_new (Track * track,
            Position * start_pos,
            Position * end_pos)
{
  Region * region = calloc (1, sizeof (Region));

  g_message ("creating region");
  region->start_pos.bars = start_pos->bars;
  region->start_pos.beats = start_pos->beats;
  region->start_pos.quarter_beats = start_pos->quarter_beats;
  region->start_pos.ticks = start_pos->ticks;
  region->end_pos.bars = end_pos->bars;
  region->end_pos.beats = end_pos->beats;
  region->end_pos.quarter_beats = end_pos->quarter_beats;
  region->end_pos.ticks = end_pos->ticks;
  region->track = track;
  region->widget = region_widget_new (region);
  region->name = g_strdup ("Region 1");

  return region;
}

/**
 * Clamps position then sets it.
 * TODO
 */
void
region_set_start_pos (Region * region,
                      Position * pos)
{
  position_set_to_pos (&region->start_pos,
                       pos);
}

/**
 * Clamps position then sets it.
 */
void
region_set_end_pos (Region * region,
                    Position * pos)
{
  position_set_to_pos (&region->end_pos,
                       pos);
}

/**
 * Returns the region at the given position in the given channel
 */
Region *
region_at_position (Track    * track, ///< the track to look in
                    Position * pos) ///< the position
{
  for (int i = 0; i < track->num_regions; i++)
    {
      if (position_compare (pos,
                            &track->regions[i]->start_pos) >= 0 &&
          position_compare (pos,
                            &track->regions[i]->end_pos) <= 0)
        {
          return track->regions[i];
        }
    }
  return NULL;
}

/**
 * Adds midi note to region
 */
void
region_add_midi_note (Region * region,
                      MidiNote * midi_note)
{
  region->midi_notes[region->num_midi_notes++] = midi_note;
}

