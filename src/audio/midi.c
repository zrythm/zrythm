/*
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

/** \file
 * MIDI functions. */

#include <math.h>
#include <stdlib.h>
#include <signal.h>

#include "audio/channel.h"
#include "audio/mixer.h"
#include "audio/midi.h"
#include "audio/transport.h"

/**
 * Appends the events from src to dest
 */
void
midi_events_append (MidiEvents * src, MidiEvents * dest)
{
  int dest_index = dest->num_events;
  for (int i = 0; i < src->num_events; i++)
    {
      jack_midi_event_t * se = &src->jack_midi_events[i];
      jack_midi_event_t * de = &dest->jack_midi_events[i + dest_index++];
      de->time = se->time;
      de->size = se->size;
      de->buffer = se->buffer;
    }
  dest->num_events += src->num_events;
}

/**
 * Clears midi events.
 */
void
midi_events_clear (MidiEvents * midi_events)
{
  midi_events->num_events = 0;
}

/**
 * Copies the queue contents to the original struct
 */
void
midi_events_dequeue (MidiEvents * midi_events)
{
  midi_events->num_events = midi_events->queue->num_events;
  for (int i = 0; i < midi_events->num_events; i++)
    {
      midi_events->jack_midi_events[i] = midi_events->queue->jack_midi_events[i];
    }

  midi_events->queue->num_events = 0;
}

/**
 * Queues MIDI note off to event queue.
 */
void
midi_panic (MidiEvents * events)
{
  MidiEvents * queue = events->queue;
  queue->num_events = 0;
  for (int j = 0; j < 1; j++)
    {
      jack_midi_event_t * ev = &queue->jack_midi_events[j];
      /*ev->time = position_to_frames (&TRANSPORT->playhead_pos);*/
      ev->time = j * 2;
      ev->size = 3;
      if (!ev->buffer)
        ev->buffer = calloc (3, sizeof (jack_midi_data_t));
      ev->buffer[0] = 0xB0; /* All sounds off */
      ev->buffer[1] = 0x7B;
      ev->buffer[2] = 0x00;
      queue->num_events++;
    }
}
