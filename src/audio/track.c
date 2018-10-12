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

#include "audio/midi_note.h"
#include "audio/position.h"
#include "audio/region.h"
#include "audio/track.h"
#include "gui/widgets/track.h"

Track *
track_new (Channel * channel)
{
  Track * track = calloc (1, sizeof (Track));

  track->channel = channel;
  track->widget = track_widget_new (track);
  Position start = { 1, 1, 1, 0, 0 };
  Position end = { 1, 3, 1, 9, 123456 };
  track->regions[track->num_regions++] = region_new (track,
                                  &start,
                                  &end);
  start.bars = 2;
  end.bars = 6;
  track->regions[track->num_regions++] = region_new (track,
                                  &start,
                                  &end);

  return track;
}

/**
 * NOTE: real time func
 */
void
track_fill_midi_events (Track      * track,
                        Position   * pos, ///< start position to check
                        nframes_t  nframes, ///< n of frames from start pos
                        Midi_Events * midi_events) ///< midi events to fill
{
  Position end_pos;
  position_set_to_pos (&end_pos, pos);
  position_add_frames (&end_pos, nframes);

  midi_events->queue->num_events = 0;
  for (int i = 0; i < track->num_regions; i++)
    {
      Region * region = track->regions[i];
      for (int j = 0; j < region->num_midi_notes; j++)
        {
          MidiNote * midi_note = region->midi_notes[j];

          /* note on event */
          if (position_compare (&midi_note->start_pos,
                                pos) >= 0 &&
              position_compare (&midi_note->start_pos,
                                &end_pos) <= 0)
            {
              jack_midi_event_t * ev =
                &midi_events->queue->jack_midi_events[
                  midi_events->queue->num_events++];
              ev->time = position_to_frames (&midi_note->start_pos) -
                position_to_frames (pos);
              ev->size = 3;
              if (!ev->buffer)
                ev->buffer = calloc (3, sizeof (jack_midi_data_t));
              ev->buffer[0] = 0x90; /* status byte, 0x90 is note on */
              ev->buffer[1] = midi_note->val; /* note number 0-127 */
              ev->buffer[2] = midi_note->vel; /* velocity 0-127 */
            }

          /* note off event */
          if (position_compare (&midi_note->end_pos,
                                pos) >= 0 &&
              position_compare (&midi_note->end_pos,
                                &end_pos) <= 0)
            {
              jack_midi_event_t * ev =
                &midi_events->queue->jack_midi_events[
                  midi_events->queue->num_events++];
              ev->time = position_to_frames (&midi_note->end_pos) -
                position_to_frames (pos);
              ev->size = 3;
              if (!ev->buffer)
                ev->buffer = calloc (3, sizeof (jack_midi_data_t));
              ev->buffer[0] = 0x80; /* status byte, 0x80 is note off */
              ev->buffer[1] = midi_note->val; /* note number 0-127 */
              ev->buffer[2] = midi_note->vel; /* velocity 0-127 */
            }
        }
    }
}
