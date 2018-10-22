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

#include "audio/channel.h"
#include "audio/midi_note.h"
#include "audio/region.h"
#include "audio/track.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/region.h"

/**
 * Creates region (used when loading projects).
 */
Region *
region_get_or_create_blank (int id)
{
  if (PROJECT->regions[id])
    {
      return PROJECT->regions[id];
    }
  else
    {
      Region * region = calloc (1, sizeof (Region));

      region->id = id;
      PROJECT->regions[id] = region;
      PROJECT->num_regions++;
      region->widget = region_widget_new (region);

      g_message ("[region_new] Creating blank region %d", id);

      return region;
    }
}

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
  region->id = PROJECT->num_regions;
  region->name = g_strdup_printf ("Region %d", region->id);
  PROJECT->regions[PROJECT->num_regions++] = region;

  return region;
}

/**
 * Clamps position then sets it.
 */
void
region_set_start_pos (Region * region,
                      Position * pos,
                      int      moved) ///< region moved or not (to move notes as
                                          ///< well)
{
  Position prev;
  position_set_to_pos (&prev, &region->start_pos);

  position_set_to_pos (&region->start_pos,
                       pos);
  if (moved)
    {
      int prev_frames = position_to_frames (&prev);
      int now_frames = position_to_frames (pos);
      int frames = now_frames - prev_frames;
      for (int i = 0; i < region->num_midi_notes; i++)
        {
          MidiNote * note = region->midi_notes[i];
          position_add_frames (&note->start_pos, frames);
          position_add_frames (&note->end_pos, frames);
        }
    }
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

/**
 * Generates the filename for this region.
 *
 * MUST be free'd.
 */
char *
region_generate_filename (Region * region)
{
  return g_strdup_printf (REGION_PRINTF_FILENAME,
                          region->id,
                          region->track->channel->name,
                          region->name);
}
