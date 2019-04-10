/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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
#include "audio/engine.h"
#include "audio/mixer.h"
#include "audio/midi.h"
#include "audio/transport.h"
#include "project.h"


/**
 * Returns a MidiEvents instance with pre-allocated
 * buffers for each midi event.
 */
MidiEvents *
midi_events_new (int init_queue)
{
  MidiEvents * self =
    calloc (1, sizeof (MidiEvents));
  for (int i = 0; i < MAX_MIDI_EVENTS; i++)
    {
#ifdef HAVE_JACK
      jack_midi_event_t * ev =
        &self->jack_midi_events[i];
      ev->buffer =
        calloc (3, sizeof (jack_midi_data_t));
#endif
    }
  if (init_queue)
    self->queue = midi_events_new (0);

  return self;
}

/**
 * Appends the events from src to dest
 */
void
midi_events_append (MidiEvents * src, MidiEvents * dest)
{
  int dest_index = dest->num_events;
  for (int i = 0; i < src->num_events; i++)
    {
#ifdef HAVE_JACK
      jack_midi_event_t * se =
        &src->jack_midi_events[i];
      jack_midi_event_t * de =
        &dest->jack_midi_events[i + dest_index];
      de->time = se->time;
      de->size = se->size;
      de->buffer = se->buffer;
#endif
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
 * Returns if a note on event for the given note
 * exists in the given events.
 */
int
midi_events_check_for_note_on (
  MidiEvents * midi_events, int note)
{
#ifdef HAVE_JACK
  jack_midi_event_t * ev;
  for (int i = 0; i < midi_events->num_events; i++)
    {
      ev = &midi_events->jack_midi_events[i];
      if (ev->buffer[0] == MIDI_CH1_NOTE_ON &&
          ev->buffer[1] == note)
        return 1;
    }
#endif
  return 0;
}

/**
 * Deletes the midi event with a note on signal
 * from the queue, and returns if it deleted or not.
 */
int
midi_events_delete_note_on (
  MidiEvents * midi_events, int note)
{
#ifdef HAVE_JACK
  jack_midi_event_t * ev;
  for (int i = 0; i < midi_events->num_events; i++)
    {
      ev = &midi_events->jack_midi_events[i];
      if (ev->buffer[0] == MIDI_CH1_NOTE_ON &&
          ev->buffer[1] == note)
        {
          for (int ii = 0;
               ii < midi_events->num_events; ii++)
            {
              if (&midi_events->jack_midi_events[ii] == ev)
                {
                  --midi_events->num_events;
                  for (int jj = ii; jj < midi_events->num_events; jj++)
                    {
                      midi_events->jack_midi_events[jj].time = midi_events->jack_midi_events[jj + 1].time;
                      midi_events->jack_midi_events[jj].size = midi_events->jack_midi_events[jj + 1].size;
                      midi_events->jack_midi_events[jj].buffer[0] = midi_events->jack_midi_events[jj + 1].buffer[0];
                      midi_events->jack_midi_events[jj].buffer[1] = midi_events->jack_midi_events[jj + 1].buffer[1];
                      midi_events->jack_midi_events[jj].buffer[2] = midi_events->jack_midi_events[jj + 1].buffer[2];
                    }
                  break;
                }
            }
          return 1;
          break;
        }
    }
#endif

  return 0;
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
#ifdef HAVE_JACK
      midi_events->jack_midi_events[i].time =
        midi_events->queue->jack_midi_events[i].time;
      midi_events->jack_midi_events[i].size =
        midi_events->queue->jack_midi_events[i].size;
      midi_events->jack_midi_events[i].buffer =
        midi_events->queue->jack_midi_events[i].buffer;
#endif
    }
  midi_events->processed = 1;

  midi_events->queue->num_events = 0;
}

/**
 * Queues MIDI note off to event queue.
 */
void
midi_panic (MidiEvents * events)
{
  MidiEvents * queue = events->queue;
  queue->num_events = 1;
#ifdef HAVE_JACK
  jack_midi_event_t * ev = &queue->jack_midi_events[0];
  ev->time = 0;
  ev->size = 3;
  if (!ev->buffer)
    ev->buffer = calloc (3, sizeof (jack_midi_data_t));
  ev->buffer[0] = MIDI_CH1_CTRL_CHANGE;
  ev->buffer[1] = MIDI_ALL_NOTES_OFF;
  ev->buffer[2] = 0x00;
#endif
}

/**
 * Queues MIDI note off to event queue.
 */
void
midi_panic_all ()
{
  midi_panic (
    AUDIO_ENGINE->midi_editor_manual_press->
      midi_events);

  for (int i = 0; i < MIXER->num_channels; i++)
    {
      Channel * channel = MIXER->channels[i];
      midi_panic (
        channel->piano_roll->midi_events);
    }
}
