/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
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
 * Appends the events from src to dest
 *
 * @param queued Append queued events instead of
 *   main events.
 */
void
midi_events_append (
  MidiEvents * src,
  MidiEvents * dest,
  int          queued)
{
  /* queued not implemented yet */
  g_warn_if_fail (!queued);

  int dest_index = dest->num_events;
  MidiEvent * src_ev, * dest_ev;
  for (int i = 0; i < src->num_events; i++)
    {
      src_ev = &src->events[i];
      dest_ev = &dest->events[i + dest_index];
      midi_event_copy  (src_ev, dest_ev);
    }
  dest->num_events += src->num_events;
}

/**
 * Clears midi events.
 */
void
midi_events_clear (
  MidiEvents * self,
  int          queued)
{
  if (queued)
    {
      self->num_queued_events = 0;
    }
  else
    {
      self->num_events = 0;
    }
}

/**
 * Returns if a note on event for the given note
 * exists in the given events.
 */
int
midi_events_check_for_note_on (
  MidiEvents * self,
  int          note,
  int          queued)
{
  g_message ("waiting check note on");
  zix_sem_wait (&self->access_sem);

  MidiEvent * ev;
  for (int i = 0;
       i < queued ?
         self->num_queued_events :
         self->num_events;
       i++)
    {
      if (queued)
        ev = &self->queued_events[i];
      else
        ev = &self->events[i];

      if (ev->type == MIDI_EVENT_TYPE_NOTE_ON &&
          ev->note_pitch == note)
        return 1;
    }

  zix_sem_post (&self->access_sem);
  g_message ("posted check note on");

  return 0;
}

/**
 * Deletes the midi event with a note on signal
 * from the queue, and returns if it deleted or not.
 */
int
midi_events_delete_note_on (
  MidiEvents * self,
  int note,
  int queued)
{
  g_message ("waiting delete note on");
  zix_sem_wait (&self->access_sem);

  MidiEvent * ev, * ev2, * next_ev;
  int match = 0;
  for (int i = queued ?
         self->num_queued_events - 1 :
         self->num_events - 1;
       i >= 0; i--)
    {
      ev = queued ?
        &self->queued_events[i] :
        &self->events[i];
      if (ev->type == MIDI_EVENT_TYPE_NOTE_ON &&
          ev->note_pitch == note)
        {
          match = 1;

          if (queued)
            --self->num_queued_events;
          else
            --self->num_events;

          for (int ii = i;
               ii < queued ?
                 self->num_queued_events :
                 self->num_events;
               ii++)
            {
              if (queued)
                {
                  ev2 = &self->queued_events[ii];
                  next_ev =
                    &self->queued_events[ii + 1];
                }
              else
                {
                  ev2 = &self->events[ii];
                  next_ev =
                    &self->events[ii + 1];
                }
              midi_event_copy (next_ev, ev2);
            }
        }
    }

  zix_sem_post (&self->access_sem);
  g_message ("posted delete note on");

  return match;
}

/**
 * Inits the MidiEvents struct.
 */
void
midi_events_init (
  MidiEvents * self)
{
  self->num_events = 0;
  self->num_queued_events = 0;

  zix_sem_init (&self->access_sem, 1);
}

/**
 * Allocates and inits a MidiEvents struct.
 *
 * @param port Owner Port.
 */
MidiEvents *
midi_events_new (
  Port * port)
{
  MidiEvents * self =
    calloc (1, sizeof(MidiEvents));

  midi_events_init (self);
  self->port = port;

  return self;
}

/**
 * Returrns if the MidiEvents have any note on
 * events.
 *
 * @param check_main Check the main events.
 * @param check_queued Check the queued events.
 */
int
midi_events_has_note_on (
  MidiEvents * self,
  int          check_main,
  int          check_queued)
{
  if (check_main)
    {
      for (int i = 0; i < self->num_events; i++)
        {
          if (self->events[i].type ==
                MIDI_EVENT_TYPE_NOTE_ON)
            return 1;
        }
    }
  if (check_queued)
    {
      for (int i = 0;
           i < self->num_queued_events; i++)
        {
          if (self->queued_events[i].type ==
                MIDI_EVENT_TYPE_NOTE_ON)
            return 1;
        }
    }

  return 0;
}

/**
 * Copies the queue contents to the original struct
 */
void
midi_events_dequeue (
  MidiEvents * self)
{
  /*g_message ("waiting dequeue");*/
  zix_sem_wait (&self->access_sem);

  MidiEvent * ev, * q_ev;
  for (int i = 0;
       i < self->num_queued_events; i++)
    {
      q_ev = &self->queued_events[i];
      ev = &self->events[i];

      midi_event_copy (q_ev, ev);
    }

  self->num_events = self->num_queued_events;
  self->num_queued_events = 0;

  zix_sem_post (&self->access_sem);
  /*g_message ("posted dequeue");*/
}

/**
 * Queues MIDI note off to event queue.
 */
void
midi_events_panic (
  MidiEvents * self,
  int          queued)
{
  g_message ("waiting panic");
  zix_sem_wait (&self->access_sem);

  MidiEvent * ev;
  if (queued)
    ev =
      &self->queued_events[0];
  else
    ev =
      &self->events[0];

  ev->type = MIDI_EVENT_TYPE_ALL_NOTES_OFF;
  ev->time = 0;
  ev->raw_buffer[0] = MIDI_CH1_CTRL_CHANGE;
  ev->raw_buffer[1] = MIDI_ALL_NOTES_OFF;
  ev->raw_buffer[2] = 0x00;

  if (queued)
    self->num_queued_events = 1;
  else
    self->num_events = 1;

  zix_sem_post (&self->access_sem);
  g_message ("posted panic");
}

/**
 * Parses a MidiEvent from a raw MIDI buffer.
 */
void
midi_events_add_event_from_buf (
  MidiEvents * self,
  uint32_t     time,
  uint8_t *    buf,
  int          buf_size)
{
  uint8_t type = buf[0] & 0xf0;
  uint8_t channel = buf[0] & 0xf;
  switch (type)
    {
    case MIDI_CH1_NOTE_ON:
      midi_events_add_note_on (
        self,
        channel,
        buf[1],
        buf[2],
        time, 0);
      break;
    case MIDI_CH1_NOTE_OFF:
      midi_events_add_note_off (
        self,
        channel,
        buf[1],
        time, 0);
      break;
    case MIDI_CH1_CTRL_CHANGE:
      midi_events_add_control_change (
        self,
        1, buf[1],
        buf[2],
        time, 0);
      break;
    default:
      g_warning ("Unknown MIDI event received");
      break;
    }
}

/**
 * Queues MIDI note off to event queues.
 *
 * @param queued Send the event to queues instead
 *   of main events.
 */
void
midi_panic_all (
  int queued)
{
  midi_events_panic (
    AUDIO_ENGINE->midi_editor_manual_press->
      midi_events, queued);

  Channel * ch;
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      ch = TRACKLIST->tracks[i]->channel;

      if (ch)
        midi_events_panic (
          ch->piano_roll->midi_events, queued);
    }
}
