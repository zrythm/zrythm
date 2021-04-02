/*
 * Copyright (C) 2018-2021 Alexandros Theodotou <alex at zrythm dot org>
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
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * Copyright (C) 2015 Tim Mayberry <mojofunk@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <inttypes.h>
#include <math.h>
#include <stdlib.h>
#include <signal.h>

#include "audio/channel.h"
#include "audio/chord_descriptor.h"
#include "audio/engine.h"
#include "audio/midi_event.h"
#include "audio/router.h"
#include "audio/transport.h"
#include "project.h"
#include "utils/objects.h"

static const char * midi_event_type_strings[] =
{
  "pitchbend",
  "controller",
  "note off",
  "note on",
  "all notes off",
};

/**
 * Appends the events from src to dest
 *
 * @param queued Append queued events instead of
 *   main events.
 * @param local_offset The start frame offset from 0
 *   in this cycle.
 * @param nframes Number of frames to process.
 */
void
midi_events_append (
  MidiEvents *    src,
  MidiEvents *    dest,
  const nframes_t local_offset,
  const nframes_t nframes,
  bool            queued)
{
  midi_events_append_w_filter (
    src, dest, NULL, local_offset, nframes, queued);
}

/**
 * Appends the events from src to dest
 *
 * @param queued Append queued events instead of
 *   main events.
 * @param channels Allowed channels (array of 16
 *   booleans).
 * @param local_offset The start frame offset from 0
 *   in this cycle.
 * @param nframes Number of frames to process.
 */
void
midi_events_append_w_filter (
  MidiEvents *    src,
  MidiEvents *    dest,
  int *           channels,
  const nframes_t local_offset,
  const nframes_t nframes,
  bool            queued)
{
  /* queued not implemented yet */
  g_return_if_fail (!queued);

  MidiEvent * src_ev, * dest_ev;
  for (int i = 0; i < src->num_events; i++)
    {
      src_ev = &src->events[i];

      /* only copy events inside the current time
       * range */
      if (src_ev->time < local_offset ||
          src_ev->time >= local_offset + nframes)
        {
          g_debug (
            "skipping event: time %" PRIu8
            " (local offset %" PRIu32
            " nframes %" PRIu32 ")",
            src_ev->time, local_offset, nframes);
          continue;
        }

      /* if filtering, skip disabled channels */
      if (channels)
        {
          midi_byte_t channel =
            src_ev->raw_buffer[0] & 0xf;
          if (!channels[channel])
            {
              continue;
            }
        }

      dest_ev =
        &dest->events[dest->num_events++];
      midi_event_copy (dest_ev, src_ev);
    }
}

/**
 * Sets the given MIDI channel on all applicable
 * MIDI events.
 *
 * @param channel Channel, starting from 1.
 */
void
midi_events_set_channel (
  MidiEvents *      self,
  const int         queued,
  const midi_byte_t channel)
{
  /* queued not implemented yet */
  g_return_if_fail (!queued);

  for (int i = 0; i < self->num_events; i++)
    {
      MidiEvent * ev = &self->events[i];
      ev->channel = channel;

      /* do this on all MIDI events that have
       * channels */
#define SET_CHANNEL(byte) \
      if ((ev->raw_buffer[0] & byte) == byte) \
        { \
          ev->raw_buffer[0] = \
            (ev->raw_buffer[0] & byte) | \
            (channel - 1); \
          continue; \
        }

      /* skip MIDI events starting with the given
       * byte */
#define SKIP_CHANNEL(byte) \
      if ((ev->raw_buffer[0] & byte) == byte) \
        continue;

      SKIP_CHANNEL (0xF0);
      SET_CHANNEL (0xE0);
      SET_CHANNEL (0xD0);
      SET_CHANNEL (0xC0);
      SET_CHANNEL (0xB0);
      SET_CHANNEL (0xA0);
      SET_CHANNEL (0x90);
      SET_CHANNEL (0x80);

#undef SET_CHANNEL
#undef SKIP_CHANNEL
    }
}

/**
 * Clears midi events.
 */
REALTIME
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
  /*g_message ("waiting check note on");*/
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
  /*g_message ("posted check note on");*/

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
  /*g_message ("waiting delete note on");*/
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
              midi_event_copy (ev2, next_ev);
            }
        }
    }

  zix_sem_post (&self->access_sem);

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
  MidiEvents * self = object_new (MidiEvents);

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
REALTIME
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

      midi_event_copy (ev, q_ev);
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
midi_events_add_all_notes_off (
  MidiEvents * self,
  midi_byte_t  channel,
  midi_time_t  time,
  bool         queued)
{
  MidiEvent * ev;
  if (queued)
    ev =
      &self->queued_events[self->num_queued_events];
  else
    ev =
      &self->events[self->num_events];

  ev->type = MIDI_EVENT_TYPE_ALL_NOTES_OFF;
  ev->channel = channel;
  ev->time = time;
  ev->raw_buffer[0] =
    (midi_byte_t)
    (MIDI_CH1_CTRL_CHANGE | (channel - 1));
  ev->raw_buffer[1] = MIDI_ALL_NOTES_OFF;
  ev->raw_buffer[2] = 0x00;

  if (queued)
    self->num_queued_events++;
  else
    self->num_events++;
}

void
midi_events_panic (
  MidiEvents * self,
  bool         queued)
{
  zix_sem_wait (&self->access_sem);
  /*g_message ("sending PANIC");*/
  for (midi_byte_t i = 0; i < 16; i++)
    {
      midi_events_add_all_notes_off (
        self, i, 0, queued);
    }
  zix_sem_post (&self->access_sem);
}

static int
sort_events_func (
  const void * _a, const void * _b)
{
  MidiEvent * a = (MidiEvent *) _a;
  MidiEvent * b = (MidiEvent *) _b;

  /* if same time put note ons first */
  if (a->time == b->time)
    {
      return
        a->type == MIDI_EVENT_TYPE_NOTE_ON ?
          -1 : 1;
    }

  return (int) a->time - (int) b->time;
}

/**
 * Sorts the MIDI events by time ascendingly.
 */
void
midi_events_sort_by_time (
  MidiEvents * self)
{
  qsort (
    self->events, (size_t) self->num_events,
    sizeof (MidiEvent), sort_events_func);
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
 * FIXME should be wrapped in access sem?
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
 * @param pitchbend -8192 to 8191
 * @param queued Add to queued events instead.
 */
void
midi_events_add_pitchbend (
  MidiEvents * self,
  midi_byte_t  channel,
  int          pitchbend,
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
  midi_get_bytes_from_int (
    pitchbend + 8192, &ev->raw_buffer[1],
    &ev->raw_buffer[2]);

  if (queued)
    self->num_queued_events++;
  else
    self->num_events++;
}

HOT
static int
midi_event_cmpfunc (
  const void * _a,
  const void * _b)
{
  const MidiEvent * a =
    (MidiEvent const *) _a;
  const MidiEvent * b =
    (MidiEvent const *) _b;
  if (a->time == b->time)
    {
      return (int) a->type - (int) b->type;
    }
  return (int) a->time - (int) b->time;
}

/**
 * Sorts the MidiEvents by time.
 */
void
midi_events_sort (
  MidiEvents * self,
  const bool   queued)
{
  MidiEvent * events;
  size_t num_events;
  if (queued)
    {
      events = self->queued_events;
      num_events = (size_t) self->num_queued_events;
    }
  else
    {
      events = self->events;
      num_events = (size_t) self->num_events;
    }
  qsort (events, num_events, sizeof (MidiEvent),
         midi_event_cmpfunc);
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
  midi_time_t  time,
  int          queued)
{
#if 0
  g_message (
    "%s: ch %"PRIu8", pitch %"PRIu8", vel %"PRIu8
    ", time %"PRIu32,
    __func__, channel, note_pitch, velocity, time);
#endif

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
 * Adds a note on for each note in the chord.
 */
void
midi_events_add_note_ons_from_chord_descr (
  MidiEvents *      self,
  ChordDescriptor * descr,
  midi_byte_t       channel,
  midi_byte_t       velocity,
  midi_time_t       _time,
  bool              queued)
{
#if 0
  g_message (
    "%s: vel %"PRIu8", time %"PRIu32,
    __func__, velocity, _time);
#endif

  for (int i = 0; i < CHORD_DESCRIPTOR_MAX_NOTES;
       i++)
    {
      if (descr->notes[i])
        {
          midi_events_add_note_on (
            self, channel, i + 36,
            velocity, _time, queued);
        }
    }
}

/**
 * Adds a note off for each note in the chord.
 */
void
midi_events_add_note_offs_from_chord_descr (
  MidiEvents *      self,
  ChordDescriptor * descr,
  midi_byte_t       channel,
  midi_time_t       time,
  bool              queued)
{
  for (int i = 0; i < CHORD_DESCRIPTOR_MAX_NOTES;
       i++)
    {
      if (descr->notes[i])
        {
          midi_events_add_note_off (
            self, channel, i + 36, time, queued);
        }
    }
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
      g_message (
        "Unknown MIDI event %#x %#x %#x"
        " received", buf[0], buf[1], buf[2]);
    }
  else if (buf_size == 2)
    {
      g_message (
        "Unknown MIDI event %#x %#x"
        " received", buf[0], buf[1]);
    }
  else if (buf_size == 1)
    {
      g_message (
        "Unknown MIDI event %#x"
        " received", buf[0]);
    }
  else
    {
      g_message (
        "Unknown MIDI event of size %d"
        " received", buf_size);
    }
}

/**
 * Parses a MidiEvent from a raw MIDI buffer.
 *
 * This must be a full 3-byte message. If in
 * 'running status' mode, the caller is responsible
 * for prepending the status byte.
 */
REALTIME
void
midi_events_add_event_from_buf (
  MidiEvents *  self,
  midi_time_t   time,
  midi_byte_t * buf,
  int           buf_size,
  int           queued)
{
  if (buf_size != 3)
    {
      g_debug (
        "buf size of %d received (%"PRIu8" %"
        PRIu8" %"PRIu8"), expected 3, skipping...",
        buf_size > 0 ? buf[0] : 0,
        buf_size > 1 ? buf[1] : 0,
        buf_size > 2 ? buf[2] : 0, buf_size);
      return;
    }

  midi_byte_t type = buf[0] & 0xf0;
  midi_byte_t channel =
    (midi_byte_t) ((buf[0] & 0xf) + 1);
  switch (type)
    {
    case MIDI_CH1_NOTE_ON:
      /* velocity == 0 means note off */
      if (buf[2] == 0)
        goto note_off;

      midi_events_add_note_on (
        self, channel, buf[1], buf[2], time,
        queued);
      break;
    case MIDI_CH1_NOTE_OFF:
note_off:
      midi_events_add_note_off (
        self, channel, buf[1], time, queued);
      break;
    case MIDI_CH1_PITCH_WHEEL_RANGE:
      midi_events_add_pitchbend (
        self, channel,
        midi_combine_bytes_to_int (buf[1], buf[2]),
        time, queued);
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
        self, 1, buf[1], buf[2], time, queued);
      break;
    default:
      print_unknown_event_message (
        buf, buf_size);
      break;
    }
}

void
midi_events_delete_event (
  MidiEvents *      self,
  const MidiEvent * ev,
  const bool        queued)
{
  MidiEvent * arr =
    queued ?
      self->queued_events :
      self->events;
#define NUM_EVENTS \
  (queued ? self->num_queued_events : \
   self->num_events)

  for (int i = 0; i < NUM_EVENTS; i++)
    {
      MidiEvent * cur_ev = &arr[i];
      if (cur_ev == ev)
        {
          for (int k = i; k < NUM_EVENTS - 1; k++)
            {
              midi_event_copy (
                &arr[k + 1], &arr[k]);
            }
          if (queued)
            self->num_queued_events--;
          else
            self->num_events--;
          i--;
        }
    }
}

void
midi_event_set_velocity (
  MidiEvent * ev,
  midi_byte_t vel)
{
  ev->velocity = vel;
  ev->raw_buffer[2] = vel;
}

void
midi_event_print (
  const MidiEvent * ev)
{
  g_message (
    "~MIDI EVENT~\n"
    "Type: %s\n"
    "Channel: %u\n"
    "Pitch: %u\n"
    "Velocity: %u\n"
    "Time: %u\n"
    "Raw: %hhx %hhx %hhx",
    midi_event_type_strings[ev->type], ev->channel,
    ev->note_pitch,
    ev->velocity, ev->time, ev->raw_buffer[0],
    ev->raw_buffer[1], ev->raw_buffer[2]);
}

int
midi_events_are_equal (
  const MidiEvent * src,
  const MidiEvent * dest)
{
  int ret =
    dest->type == src->type &&
    dest->pitchbend == src->pitchbend &&
    dest->controller == src->controller &&
    dest->control == src->control &&
    dest->channel == src->channel &&
    dest->note_pitch == src->note_pitch &&
    (src->type != MIDI_EVENT_TYPE_NOTE_ON ||
      (src->type == MIDI_EVENT_TYPE_NOTE_ON &&
       dest->velocity == src->velocity)) &&
    dest->time == src->time &&
    dest->raw_buffer[0] == src->raw_buffer[0] &&
    dest->raw_buffer[1] == src->raw_buffer[1] &&
    dest->raw_buffer[2] == src->raw_buffer[2];
  return ret;
}

void
midi_events_print (
  MidiEvents * self,
  const int    queued)
{
  MidiEvent * arr =
    queued ?
      self->queued_events :
      self->events;
  MidiEvent * ev1;
#define NUM_EVENTS \
  (queued ? self->num_queued_events : \
   self->num_events)

  int i;
  for (i = 0; i < NUM_EVENTS; i++)
    {
      ev1 = &arr[i];
      midi_event_print (ev1);
    }
}

/**
 * Clears duplicates.
 *
 * @param queued Clear duplicates from queued events
 * instead.
 */
void
midi_events_clear_duplicates (
  MidiEvents * self,
  const int    queued)
{
  MidiEvent * arr =
    queued ?
      self->queued_events :
      self->events;
  MidiEvent * ev1, * ev2;
#define NUM_EVENTS \
  (queued ? self->num_queued_events : \
   self->num_events)

  int i, j, k;
  for (i = 0; i < NUM_EVENTS; i++)
    {
      ev1 = &arr[i];

      for (j = i + 1; j < NUM_EVENTS; j++)
        {
          ev2 = &arr[j];

          if (midi_events_are_equal (ev1, ev2))
            {
              g_message (
                "removing duplicate MIDI event");
              for (k = j; k < NUM_EVENTS; k++)
                {
                  midi_event_copy (
                    &arr[k], &arr[k + 1]);
                }
              if (queued)
                self->num_queued_events--;
              else
                self->num_events--;
              j--;
            }
        }
    }
}

/**
 * Frees the MIDI events.
 */
void
midi_events_free (
  MidiEvents * self)
{
  zix_sem_destroy (&self->access_sem);

  object_zero_and_free (self);
}
