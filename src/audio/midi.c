/*
 * Copyright (C) 2018-2020 Alexandros Theodotou <alex at zrythm dot org>
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
#include "audio/engine.h"
#include "audio/router.h"
#include "audio/midi.h"
#include "audio/transport.h"
#include "project.h"
#include "utils/objects.h"

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
  bool          queued)
{
  /* queued not implemented yet */
  g_return_if_fail (!queued);

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
 * Saves a string representation of the given
 * control change event in the given buffer.
 *
 * @param buf The string buffer to fill, or NULL
 *   to only get the channel.
 *
 * @return The MIDI channel, or -1 if not ctrl
 *   change.
 */
int
midi_ctrl_change_get_ch_and_description (
  midi_byte_t * ctrl_change,
  char *        buf)
{
  /* assert the given event is a ctrl change event */
  if (ctrl_change[0] < 0xB0 ||
      ctrl_change[0] > 0xBF)
    {
      return -1;
    }

  if (buf)
    {
      if (ctrl_change[1] >= 0x08 &&
          ctrl_change[1] <= 0x1F)
        {
          sprintf (
            buf, "Continuous controller #%u",
            ctrl_change[1]);
        }
      else if (ctrl_change[1] >= 0x28 &&
               ctrl_change[1] <= 0x3F)
        {
          sprintf (
            buf, "Continuous controller #%u",
            ctrl_change[1] - 0x28);
        }
      else
        {
          switch (ctrl_change[1])
            {
            case 0x00:
            case 0x20:
              strcpy (
                buf, "Continuous controller #0");
              break;
            case 0x01:
            case 0x21:
              strcpy (buf, "Modulation wheel");
              break;
            case 0x02:
            case 0x22:
              strcpy (buf, "Breath control");
              break;
            case 0x03:
            case 0x23:
              strcpy (
                buf, "Continuous controller #3");
              break;
            case 0x04:
            case 0x24:
              strcpy (buf, "Foot controller");
              break;
            case 0x05:
            case 0x25:
              strcpy (buf, "Portamento time");
              break;
            case 0x06:
            case 0x26:
              strcpy (buf, "Data Entry");
              break;
            case 0x07:
            case 0x27:
              strcpy (buf, "Main Volume");
              break;
            default:
              strcpy (buf, "Unknown");
              break;
            }
        }
    }
  return (ctrl_change[0] - 0xB0) + 1;
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
 * Used for MIDI controls whose values are split
 * between MSB/LSB.
 *
 * @param lsb First byte (pos 1).
 * @param msb Second byte (pos 2).
 */
int
midi_combine_bytes_to_int (
  midi_byte_t lsb,
  midi_byte_t msb)
{
  /* http://midi.teragonaudio.com/tech/midispec/wheel.htm */
  return (int) ((msb << 7) | lsb);
}

/**
 * Used for MIDI controls whose values are split
 * between MSB/LSB.
 *
 * @param lsb First byte (pos 1).
 * @param msb Second byte (pos 2).
 */
void
midi_get_bytes_from_int (
  int           val,
  midi_byte_t * lsb,
  midi_byte_t * msb)
{
  /* https://arduino.stackexchange.com/questions/18955/how-to-send-a-pitch-bend-midi-message-using-arcore */
  *lsb = val & 0x7F;
  *msb = val >> 7;
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

static int
cmpfunc (
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
  const int    queued)
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
         cmpfunc);
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
  g_message (
    "%s: ch %"PRIu8", pitch %"PRIu8", vel %"PRIu8
    ", time %"PRIu32,
    __func__, channel, note_pitch, velocity, time);

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
 *
 * This must be a full 3-byte message. If in
 * 'running status' mode, the caller is responsible
 * for prepending the status byte.
 */
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
      g_warning (
        "buf size of %d received (%"PRIu8" %"
        PRIu8" %"PRIu8"), expected 3",
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
midi_event_print (
  const MidiEvent * ev)
{
  g_message (
    "~MIDI EVENT~\n"
    "Type: %u\n"
    "Channel: %u\n"
    "Pitch: %u\n"
    "Velocity: %u\n"
    "Time: %u\n"
    "Raw: %hhx %hhx %hhx",
    ev->type, ev->channel, ev->note_pitch,
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
                    &arr[k + 1], &arr[k]);
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
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      track = TRACKLIST->tracks[i];

      if (track_has_piano_roll (track))
        midi_events_panic (
          track->processor->piano_roll->midi_events,
          queued);
    }
}

/**
 * Returns the length of the MIDI message based on
 * the status byte.
 *
 * TODO move this and other functions to utils/midi,
 * split separate files for MidiEvents and MidiEvent.
 */
int
midi_get_msg_length (
  uint8_t status_byte)
{
  // define these with meaningful names
  switch (status_byte & 0xf0) {
  case 0x80:
  case 0x90:
  case 0xa0:
  case 0xb0:
  case 0xe0:
    return 3;
  case 0xc0:
  case 0xd0:
    return 2;
  case 0xf0:
    switch (status_byte) {
    case 0xf0:
      return 0;
    case 0xf1:
    case 0xf3:
      return 2;
    case 0xf2:
      return 3;
    case 0xf4:
    case 0xf5:
    case 0xf7:
    case 0xfd:
      break;
    default:
      return 1;
    }
  }
  g_return_val_if_reached (-1);
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
