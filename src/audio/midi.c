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
 * @param start_frame The start frame offset from 0
 *   in this cycle.
 * @param nframes Number of frames to process.
 */
void
midi_events_append (
  MidiEvents * src,
  MidiEvents * dest,
  const nframes_t    start_frame,
  const nframes_t    nframes,
  int          queued)
{
  /* queued not implemented yet */
  g_warn_if_fail (!queued);

  int dest_index = dest->num_events;
  MidiEvent * src_ev, * dest_ev;
  for (int i = 0; i < src->num_events; i++)
    {
      src_ev = &src->events[i];
      if (src_ev->time < nframes)
        {
          dest_ev = &dest->events[i + dest_index];
          midi_event_copy  (src_ev, dest_ev);
          dest->num_events++;
        }
      else
        {
          g_warn_if_reached ();
        }
    }
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

static int
sort_events_func (const void *a, const void *b)
{
  const MidiEvent * aa = (const MidiEvent *) a;
  const MidiEvent * bb = (const MidiEvent *) b;
  return aa->time > bb->time;
}

/**
 * Sorts the MIDI events by time ascendingly.
 */
void
midi_events_sort_by_time (
  MidiEvents * self)
{
  qsort (self->events,
         (size_t) self->num_events,
         sizeof (MidiEvent),
         sort_events_func);
}

#ifdef HAVE_JACK
/**
 * Writes the events to the given JACK buffer.
 */
void
midi_events_copy_to_jack (
  MidiEvents * self,
  void *       buff)
{
  jack_midi_clear_buffer (buff);

  MidiEvent * ev;
  jack_midi_data_t midi_data[3];
  for (int i = 0; i < self->num_events; i++)
    {
      ev = &self->events[i];

      midi_data[0] = ev->raw_buffer[0];
      midi_data[1] = ev->raw_buffer[1];
      midi_data[2] = ev->raw_buffer[2];
      jack_midi_event_write (
        buff, ev->time, midi_data, 3);
      g_message (
        "wrote MIDI event to JACK MIDI out at %d",
        ev->time);
    }
}
#endif

/**
 * Adds a note off event to the given MidiEvents.
 *
 * @param channel MIDI channel starting from 1.
 * @param queued Add to queued events instead.
 */
void
midi_events_add_note_off (
  MidiEvents * self,
  midi_byte_t  channel,
  midi_byte_t  note_pitch,
  midi_time_t  time,
  int          queued)
{
  MidiEvent * ev;
  if (queued)
    ev =
      &self->queued_events[self->num_queued_events];
  else
    ev =
      &self->events[self->num_events];

  ev->type = MIDI_EVENT_TYPE_NOTE_OFF;
  ev->channel = channel;
  ev->note_pitch = note_pitch;
  ev->time = time;
  ev->raw_buffer[0] =
    (midi_byte_t)
    (MIDI_CH1_NOTE_OFF | (channel - 1));
  ev->raw_buffer[1] = note_pitch;
  ev->raw_buffer[2] = 90;

  if (queued)
    self->num_queued_events++;
  else
    self->num_events++;
}

/**
 * Adds a control event to the given MidiEvents.
 *
 * @param channel MIDI channel starting from 1.
 * @param queued Add to queued events instead.
 */
void
midi_events_add_control_change (
  MidiEvents * self,
  uint8_t      channel,
  uint8_t      controller,
  uint8_t      control,
  uint32_t     time,
  int          queued)
{
  MidiEvent * ev;
  if (queued)
    ev =
      &self->queued_events[self->num_queued_events];
  else
    ev =
      &self->events[self->num_events];

  ev->type = MIDI_EVENT_TYPE_CONTROLLER;
  ev->channel = channel;
  ev->controller = controller;
  ev->control = control;
  ev->time = time;
  ev->raw_buffer[0] =
    (midi_byte_t)
    (MIDI_CH1_CTRL_CHANGE | (channel - 1));
  ev->raw_buffer[1] = controller;
  ev->raw_buffer[2] = control;

  if (queued)
    self->num_queued_events++;
  else
    self->num_events++;
}

/**
 * Adds a control event to the given MidiEvents.
 *
 * @param channel MIDI channel starting from 1.
 * @param queued Add to queued events instead.
 */
void
midi_events_add_pitchbend (
  MidiEvents * self,
  midi_byte_t  channel,
  midi_byte_t  pitchbend,
  midi_time_t  time,
  int          queued)
{
  MidiEvent * ev;
  if (queued)
    ev =
      &self->queued_events[self->num_queued_events];
  else
    ev =
      &self->events[self->num_events];

  ev->type = MIDI_EVENT_TYPE_PITCHBEND;
  ev->pitchbend = pitchbend;
  ev->time = time;
  ev->raw_buffer[0] =
    (midi_byte_t)
    (MIDI_CH1_PITCH_WHEEL_RANGE | (channel - 1));
  ev->raw_buffer[1] = 1;
  ev->raw_buffer[2] = pitchbend;

  if (queued)
    self->num_queued_events++;
  else
    self->num_events++;
}

/**
 * Adds a note on event to the given MidiEvents.
 *
 * @param channel MIDI channel starting from 1.
 * @param queued Add to queued events instead.
 */
void
midi_events_add_note_on (
  MidiEvents * self,
  uint8_t      channel,
  uint8_t      note_pitch,
  uint8_t      velocity,
  uint32_t     time,
  int          queued)
{
  MidiEvent * ev;
  if (queued)
    ev =
      &self->queued_events[self->num_queued_events];
  else
    ev =
      &self->events[self->num_events];

  ev->type = MIDI_EVENT_TYPE_NOTE_ON;
  ev->channel = channel;
  ev->note_pitch = note_pitch;
  ev->velocity = velocity;
  ev->time = time;
  ev->raw_buffer[0] =
    (midi_byte_t)
    (MIDI_CH1_NOTE_ON | (channel - 1));
  ev->raw_buffer[1] = note_pitch;
  ev->raw_buffer[2] = velocity;

  if (queued)
    self->num_queued_events++;
  else
    self->num_events++;
}

/**
 * Prints a message saying unknown event, with
 * information about the given event.
 */
static void
print_unknown_event_message (
  uint8_t * buf,
  const int buf_size)
{
  if (buf_size == 3)
    {
      g_warning (
        "Unknown MIDI event %#x %#x %#x"
        " received", buf[0], buf[1], buf[2]);
    }
  else if (buf_size == 2)
    {
      g_warning (
        "Unknown MIDI event %#x %#x"
        " received", buf[0], buf[1]);
    }
  else if (buf_size == 1)
    {
      g_warning (
        "Unknown MIDI event %#x"
        " received", buf[0]);
    }
  else
    {
      g_warning (
        "Unknown MIDI event of size %d"
        " received", buf_size);
    }
}

/**
 * Parses a MidiEvent from a raw MIDI buffer.
 */
void
midi_events_add_event_from_buf (
  MidiEvents *  self,
  midi_time_t   time,
  midi_byte_t * buf,
  int           buf_size)
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
    case MIDI_SYSTEM_MESSAGE:
      /* ignore active sensing */
      if (buf[0] != 0xFE)
        {
          print_unknown_event_message (
            buf, buf_size);
        }
      break;
    case MIDI_CH1_CTRL_CHANGE:
      midi_events_add_control_change (
        self,
        1, buf[1],
        buf[2],
        time, 0);
      break;
    default:
      print_unknown_event_message (
        buf, buf_size);
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

  Track * track;
  Channel * ch;
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      track = TRACKLIST->tracks[i];
      ch = track->channel;

      if (ch && track_has_piano_roll (track))
        midi_events_panic (
          ch->piano_roll->midi_events, queued);
    }
}

/**
 * Frees the MIDI events.
 */
void
midi_events_free (
  MidiEvents * self)
{
  free (self);
}
