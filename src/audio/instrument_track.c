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

#include "audio/automatable.h"
#include "audio/automation_track.h"
#include "audio/automation_tracklist.h"
#include "audio/instrument_track.h"
#include "audio/midi_note.h"
#include "audio/position.h"
#include "audio/region.h"
#include "audio/track.h"
#include "audio/velocity.h"
#include "plugins/lv2/control.h"
#include "plugins/lv2_plugin.h"
#include "project.h"
#include "gui/widgets/track.h"
#include "gui/widgets/automation_lane.h"
#include "utils/arrays.h"

#include <gtk/gtk.h>

InstrumentTrack *
instrument_track_new (Channel * channel)
{
  InstrumentTrack * self =
    calloc (1, sizeof (InstrumentTrack));

  Track * track = (Track *) self;
  track->type = TRACK_TYPE_INSTRUMENT;
  gdk_rgba_parse (&track->color, "#F79616");
  track_init ((Track *) self);
  project_add_track (track);

  ChannelTrack * ct = (ChannelTrack *) self;
  ct->channel = channel;
  self->ui_active = 1;

  return self;
}

void
instrument_track_setup (InstrumentTrack * self)
{
  ChannelTrack * track = (ChannelTrack *) self;

  channel_track_setup (track);
}

/**
 * Fills MIDI events from track.
 *
 * NOTE: real time func
 */
void
instrument_track_fill_midi_events (InstrumentTrack      * track,
                        Position   * pos, ///< start position to check
                        nframes_t  nframes, ///< n of frames from start pos
                        MidiEvents * midi_events) ///< midi events to fill
{
  Position end_pos;
  position_set_to_pos (&end_pos, pos);
  position_add_frames (&end_pos, nframes);

  midi_events->queue->num_events = 0;
  for (int i = 0; i < track->num_regions; i++)
    {
      MidiRegion * region = track->regions[i];
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
              ev->buffer[0] = MIDI_CH1_NOTE_ON; /* status byte */
              ev->buffer[1] = midi_note->val; /* note number 0-127 */
              ev->buffer[2] = midi_note->vel->vel; /* velocity 0-127 */
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
              ev->buffer[0] = MIDI_CH1_NOTE_OFF; /* status byte */
              ev->buffer[1] = midi_note->val; /* note number 0-127 */
              ev->buffer[2] = midi_note->vel->vel; /* velocity 0-127 */
            }
        }
    }
}

void
instrument_track_add_region (InstrumentTrack      * track,
                  MidiRegion     * region)
{
  g_message ("midi region num notes %d",
             region->num_midi_notes);
  array_append (track->regions,
                track->num_regions,
                region);
}

void
instrument_track_remove_region (InstrumentTrack    * track,
                     MidiRegion   * region)
{
  array_delete (track->regions,
                track->num_regions,
                region);
}

/**
 * Frees the track.
 *
 * TODO
 */
void
instrument_track_free (InstrumentTrack * track)
{

}
