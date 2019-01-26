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
#include "audio/midi_region.h"
#include "audio/instrument_track.h"
#include "audio/region.h"
#include "audio/track.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_region.h"
#include "gui/widgets/region.h"
#include "project.h"
#include "utils/yaml.h"

/**
 * Only to be used by implementing structs.
 */
void
region_init (Region *   region,
             RegionType type,
             Track *    track,
             Position * start_pos,
             Position * end_pos)
{
  g_message ("creating region");
  region->start_pos.bars = start_pos->bars;
  region->start_pos.beats = start_pos->beats;
  region->start_pos.sixteenths = start_pos->sixteenths;
  region->start_pos.ticks = start_pos->ticks;
  region->end_pos.bars = end_pos->bars;
  region->end_pos.beats = end_pos->beats;
  region->end_pos.sixteenths = end_pos->sixteenths;
  region->end_pos.ticks = end_pos->ticks;
  region->track_id = track->id;
  region->track = track;
  Channel * chan = track_get_channel (track);
  region->name = g_strdup_printf ("%s (%d)",
                                  chan->name,
                                  region->id);
  region->linked_region_id = -1;
  if (track->type == TRACK_TYPE_AUDIO)
    region->type = REGION_TYPE_AUDIO;
  else if (track->type == TRACK_TYPE_INSTRUMENT)
    {
      region->type = REGION_TYPE_MIDI;
      region->widget = Z_REGION_WIDGET (
        midi_region_widget_new (
          (MidiRegion *) region));
    }
  project_add_region (region);
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
  if (moved && region->type == REGION_TYPE_MIDI)
    {
      MidiRegion * midi_region = (MidiRegion *) region;
      int prev_frames = position_to_frames (&prev);
      int now_frames = position_to_frames (pos);
      int frames = now_frames - prev_frames;
      for (int i = 0; i < midi_region->num_midi_notes; i++)
        {
          MidiNote * note = midi_region->midi_notes[i];
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
  int num_regions = 0;
  MidiRegion ** midi_regions;
  if (track->type == TRACK_TYPE_AUDIO)
    {

    }
  else if (track->type == TRACK_TYPE_INSTRUMENT)
    {
      num_regions = ((InstrumentTrack *)track)->num_regions;
      midi_regions = ((InstrumentTrack *)track)->regions;
    }
  for (int i = 0; i < num_regions; i++)
    {
      Region * region;
      if (track->type == TRACK_TYPE_AUDIO)
        {

        }
      else if (track->type == TRACK_TYPE_INSTRUMENT)
        {
          region = (Region *)midi_regions[i];
        }

      if (position_compare (pos,
                            &region->start_pos) >= 0 &&
          position_compare (pos,
                            &region->end_pos) <= 0)
        {
          return region;
        }
    }
  return NULL;
}

/**
 * Generates the filename for this region.
 *
 * MUST be free'd.
 */
char *
region_generate_filename (Region * region)
{
  Channel * channel = track_get_channel (region->track);
  return g_strdup_printf (REGION_PRINTF_FILENAME,
                          region->id,
                          channel->name,
                          region->name);
}

/**
 * Serializes the region.
 *
 * MUST be free'd.
 */
char *
region_serialize (Region * region)
{
  cyaml_err_t err;

  char * output =
    calloc (1200, sizeof (char));
  size_t output_len;
  err =
    cyaml_save_data (
      &output,
      &output_len,
      &config,
      &region_schema,
      region,
      0);
  if (err != CYAML_OK)
    {
      g_message ("error %s",
                 cyaml_strerror (err));
    }
  return output;
}
