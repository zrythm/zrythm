/*
 * SPDX-FileCopyrightText: © 2018-2023 Alexandros Theodotou <alex@zrythm.org>
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * ---
 *
 * Copyright (C) 2015 Tim Mayberry <mojofunk@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * ---
 */

#include <inttypes.h>
#include <math.h>
#include <signal.h>
#include <stdlib.h>

#include "dsp/channel.h"
#include "dsp/chord_descriptor.h"
#include "dsp/engine.h"
#include "dsp/midi_event.h"
#include "dsp/router.h"
#include "dsp/transport.h"
#include "project.h"
#include "utils/objects.h"
#include "zrythm_app.h"

/**
 * Type of MIDI event.
 *
 * These are in order of precedence when sorting.
 *
 * See libs/ardour/midi_buffer.cc for more.
 */
typedef enum MidiEventType
{
  MIDI_EVENT_TYPE_NOTE_ON,
  MIDI_EVENT_TYPE_NOTE_OFF,
  MIDI_EVENT_TYPE_ALL_NOTES_OFF,
  MIDI_EVENT_TYPE_CONTROLLER,
  MIDI_EVENT_TYPE_PITCHBEND,
  MIDI_EVENT_TYPE_START,
  MIDI_EVENT_TYPE_CONTINUE,
  MIDI_EVENT_TYPE_SONG_POS,
  MIDI_EVENT_TYPE_CLOCK,
  MIDI_EVENT_TYPE_STOP,

  /** Unknown type. */
  MIDI_EVENT_TYPE_RAW,
} MidiEventType;

static const char * midi_event_type_strings[] = {
  "pitchbend", "controller", "note off", "note on", "all notes off",
};

/**
 * Appends the events from src to dest.
 *
 * @param queued Append queued events instead of
 *   main events.
 * @param local_offset The start frame offset from
 *   0 in this cycle.
 * @param nframes Number of frames to process.
 */
void
midi_events_append (
  MidiEvents *    dest,
  MidiEvents *    src,
  const nframes_t local_offset,
  const nframes_t nframes,
  bool            queued)
{
  midi_events_append_w_filter (dest, src, NULL, local_offset, nframes, queued);
}

/**
 * Transforms the given MIDI input to the MIDI
 * notes of the corresponding chord.
 *
 * Only C0~B0 are considered.
 */
void
midi_events_transform_chord_and_append (
  MidiEvents *    dest,
  MidiEvents *    src,
  const nframes_t local_offset,
  const nframes_t nframes,
  bool            queued)
{
  g_return_if_fail (src);
  g_return_if_fail (dest);

  /* queued not implemented yet */
  g_return_if_fail (!queued);

  for (int i = 0; i < src->num_events; i++)
    {
      MidiEvent * src_ev = &src->events[i];

      /* only copy events inside the current time
       * range */
      if (
        ZRYTHM_TESTING
        && (src_ev->time < local_offset || src_ev->time >= local_offset + nframes))
        {
          g_debug (
            "skipping event: time %" PRIu8 " (local offset %" PRIu32
            " nframes %" PRIu32 ")",
            src_ev->time, local_offset, nframes);
          continue;
        }

      /* make sure there is enough space */
      g_return_if_fail (dest->num_events < MAX_MIDI_EVENTS - 6);

      const uint8_t * buf = src_ev->raw_buffer;

      if (!midi_is_note_on (buf) && !midi_is_note_off (buf))
        continue;

      /* only use middle octave */
      midi_byte_t             note_number = midi_get_note_number (buf);
      const ChordDescriptor * descr =
        chord_editor_get_chord_from_note_number (CHORD_EDITOR, note_number);
      if (!descr)
        continue;

      if (midi_is_note_on (buf))
        {
          midi_events_add_note_ons_from_chord_descr (
            dest, descr, 1, VELOCITY_DEFAULT, src_ev->time, queued);
        }
      else if (midi_is_note_off (buf))
        {
          midi_events_add_note_offs_from_chord_descr (
            dest, descr, 1, src_ev->time, queued);
        }
    }

  /* clear duplicates */
  midi_events_clear_duplicates (dest, queued);
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
  MidiEvents *    dest,
  MidiEvents *    src,
  int *           channels,
  const nframes_t local_offset,
  const nframes_t nframes,
  bool            queued)
{
  g_return_if_fail (src);
  g_return_if_fail (dest);

  /* queued not implemented yet */
  g_return_if_fail (!queued);

  MidiEvent *src_ev, *dest_ev;
  for (int i = 0; i < src->num_events; i++)
    {
      src_ev = &src->events[i];

      /* only copy events inside the current time
       * range */
      if (
        ZRYTHM_TESTING
        && (src_ev->time < local_offset || src_ev->time >= local_offset + nframes))
        {
          g_debug (
            "skipping event: time %" PRIu8 " (local offset %" PRIu32
            " nframes %" PRIu32 ")",
            src_ev->time, local_offset, nframes);
          continue;
        }

      /* if filtering, skip disabled channels */
      if (channels)
        {
          midi_byte_t channel = src_ev->raw_buffer[0] & 0xf;
          if (!channels[channel])
            {
              continue;
            }
        }

      g_return_if_fail (dest->num_events < MAX_MIDI_EVENTS);

      dest_ev = &dest->events[dest->num_events++];
      midi_event_copy (dest_ev, src_ev);
    }

  /* clear duplicates */
  midi_events_clear_duplicates (dest, queued);
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

      /* do this on all MIDI events that have
       * channels */
#define SET_CHANNEL(byte) \
  if ((ev->raw_buffer[0] & byte) == byte) \
    { \
      ev->raw_buffer[0] = (ev->raw_buffer[0] & byte) | (channel - 1); \
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
midi_events_clear (MidiEvents * self, int queued)
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
midi_events_check_for_note_on (MidiEvents * self, int note, int queued)
{
  /*g_message ("waiting check note on");*/
  zix_sem_wait (&self->access_sem);

  MidiEvent * ev;
  for (int i = 0; i < queued ? self->num_queued_events : self->num_events; i++)
    {
      if (queued)
        ev = &self->queued_events[i];
      else
        ev = &self->events[i];

      midi_byte_t * buf = ev->raw_buffer;

      if (midi_is_note_on (buf) && midi_get_note_number (buf) == note)
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
midi_events_delete_note_on (MidiEvents * self, int note, int queued)
{
  /*g_message ("waiting delete note on");*/
  zix_sem_wait (&self->access_sem);

  MidiEvent *ev, *ev2, *next_ev;
  int        match = 0;
  for (
    int i = queued ? self->num_queued_events - 1 : self->num_events - 1; i >= 0;
    i--)
    {
      ev = queued ? &self->queued_events[i] : &self->events[i];
      midi_byte_t * buf = ev->raw_buffer;
      if (midi_is_note_on (buf) && midi_get_note_number (buf) == note)
        {
          match = 1;

          if (queued)
            --self->num_queued_events;
          else
            --self->num_events;

          for (
            int ii = i;
            ii < queued ? self->num_queued_events : self->num_events; ii++)
            {
              if (queued)
                {
                  ev2 = &self->queued_events[ii];
                  next_ev = &self->queued_events[ii + 1];
                }
              else
                {
                  ev2 = &self->events[ii];
                  next_ev = &self->events[ii + 1];
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
midi_events_init (MidiEvents * self)
{
  self->num_events = 0;
  self->num_queued_events = 0;

  zix_sem_init (&self->access_sem, 1);
}

/**
 * Allocates and inits a MidiEvents struct.
 */
MidiEvents *
midi_events_new (void)
{
  MidiEvents * self = object_new (MidiEvents);

  midi_events_init (self);

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
midi_events_has_note_on (MidiEvents * self, int check_main, int check_queued)
{
  if (check_main)
    {
      for (int i = 0; i < self->num_events; i++)
        {
          if (midi_is_note_on (self->events[i].raw_buffer))
            {
              return 1;
            }
        }
    }
  if (check_queued)
    {
      for (int i = 0; i < self->num_queued_events; i++)
        {
          if (midi_is_note_on (self->queued_events[i].raw_buffer))
            {
              return 1;
            }
        }
    }

  return 0;
}

/**
 * Copies the queue contents to the original struct
 */
REALTIME
NONNULL void
midi_events_dequeue (MidiEvents * self)
{
  /*g_message ("waiting dequeue");*/
  zix_sem_wait (&self->access_sem);

  MidiEvent *ev, *q_ev;
  for (int i = 0; i < self->num_queued_events; i++)
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
  g_return_if_fail (channel > 0);
  MidiEvent * ev;
  if (queued)
    ev = &self->queued_events[self->num_queued_events];
  else
    ev = &self->events[self->num_events];

  ev->time = time;
  ev->raw_buffer[0] = (midi_byte_t) (MIDI_CH1_CTRL_CHANGE | (channel - 1));
  ev->raw_buffer[1] = MIDI_ALL_NOTES_OFF;
  ev->raw_buffer[2] = 0x00;
  ev->raw_buffer_sz = 3;
  g_return_if_fail (midi_is_all_notes_off (ev->raw_buffer));

  if (queued)
    self->num_queued_events++;
  else
    self->num_events++;
}

/**
 * Adds a note off message to every MIDI channel.
 */
void
midi_events_panic_without_lock (MidiEvents * self, bool queued)
{
  /*g_message ("sending PANIC");*/
  for (midi_byte_t i = 1; i < 17; i++)
    {
      midi_events_add_all_notes_off (self, i, 0, queued);
    }
}

/**
 * Must only be called from the UI thread.
 */
void
midi_events_panic (MidiEvents * self, bool queued)
{
  if (zrythm_app)
    {
      g_return_if_fail (g_thread_self () == zrythm_app->gtk_thread);
    }

  while (zix_sem_try_wait (&self->access_sem) != ZIX_STATUS_SUCCESS)
    {
      g_usleep (10);
    }
  midi_events_panic_without_lock (self, queued);
  zix_sem_post (&self->access_sem);
}

void
midi_events_write_to_midi_file (
  const MidiEvents * self,
  MIDI_FILE *        mf,
  int                midi_track)
{
  g_return_if_fail (midi_track > 0);

  int last_time = 0;
  for (int i = 0; i < self->num_events; i++)
    {
      const MidiEvent * ev = &self->events[i];

      BYTE buf[] = { ev->raw_buffer[0], ev->raw_buffer[1], ev->raw_buffer[2] };
      int  delta_time = (int) ev->time - last_time;
      midiTrackAddRaw (
        mf, midi_track, 3, buf,
        /* move ptr */
        true, delta_time);
      last_time += delta_time;
    }
}

#ifdef HAVE_JACK
/**
 * Writes the events to the given JACK buffer.
 */
void
midi_events_copy_to_jack (
  MidiEvents *    self,
  const nframes_t local_start_frames,
  const nframes_t nframes,
  void *          buff)
{
  /*jack_midi_clear_buffer (buff);*/

  MidiEvent *      ev;
  jack_midi_data_t midi_data[3];
  for (int i = 0; i < self->num_events; i++)
    {
      ev = &self->events[i];

      if (
        ev->time < local_start_frames
        || ev->time >= local_start_frames + nframes)
        {
          continue;
        }

      for (size_t j = 0; j < ev->raw_buffer_sz; j++)
        {
          midi_data[j] = ev->raw_buffer[j];
        }
      jack_midi_event_write (buff, ev->time, midi_data, ev->raw_buffer_sz);
#  if 0
      g_message (
        "wrote MIDI event to JACK MIDI out at %d",
        ev->time);
#  endif
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
  g_return_if_fail (channel > 0);
  MidiEvent * ev;
  if (queued)
    ev = &self->queued_events[self->num_queued_events];
  else
    ev = &self->events[self->num_events];

  ev->time = time;
  ev->raw_buffer[0] = (midi_byte_t) (MIDI_CH1_NOTE_OFF | (channel - 1));
  ev->raw_buffer[1] = note_pitch;
  ev->raw_buffer[2] = 90;
  ev->raw_buffer_sz = 3;
  g_return_if_fail (midi_is_note_off (ev->raw_buffer));

  if (queued)
    self->num_queued_events++;
  else
    self->num_events++;
}

/**
 * Adds a song position event to the queue.
 *
 * @param total_sixteenths Total sixteenths.
 */
void
midi_events_add_song_pos (
  MidiEvents * self,
  int64_t      total_sixteenths,
  midi_time_t  time,
  bool         queued)
{
  uint8_t buf[3];
  buf[0] = MIDI_SONG_POSITION;
  buf[1] = total_sixteenths & 0x7f;        // LSB
  buf[2] = (total_sixteenths >> 7) & 0x7f; // MSB
  midi_events_add_raw (self, buf, 3, time, queued);
}

void
midi_events_add_raw (
  MidiEvents * self,
  uint8_t *    buf,
  size_t       buf_sz,
  midi_time_t  time,
  bool         queued)
{
  if (buf_sz > 3)
    {
      midi_print (buf, buf_sz);
      g_return_if_reached ();
    }

  MidiEvent * ev;
  if (queued)
    ev = &self->queued_events[self->num_queued_events];
  else
    ev = &self->events[self->num_events];

  ev->time = time;
  for (size_t i = 0; i < buf_sz; i++)
    {
      ev->raw_buffer[i] = buf[i];
    }
  ev->raw_buffer_sz = buf_sz;

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
    ev = &self->queued_events[self->num_queued_events];
  else
    ev = &self->events[self->num_events];

  ev->time = time;
  ev->raw_buffer[0] = (midi_byte_t) (MIDI_CH1_CTRL_CHANGE | (channel - 1));
  ev->raw_buffer[1] = controller;
  ev->raw_buffer[2] = control;
  ev->raw_buffer_sz = 3;

  if (queued)
    self->num_queued_events++;
  else
    self->num_events++;
}

void
midi_events_add_pitchbend (
  MidiEvents * self,
  midi_byte_t  channel,
  uint32_t     pitchbend,
  midi_time_t  time,
  bool         queued)
{
  g_return_if_fail (pitchbend < 0x4000 && channel > 0);

  MidiEvent * ev;
  if (queued)
    ev = &self->queued_events[self->num_queued_events];
  else
    ev = &self->events[self->num_events];

  ev->time = time;
  ev->raw_buffer[0] = (midi_byte_t) (MIDI_CH1_PITCH_WHEEL_RANGE | (channel - 1));
  midi_get_bytes_from_combined (
    pitchbend, &ev->raw_buffer[1], &ev->raw_buffer[2]);
  ev->raw_buffer_sz = 3;

  if (queued)
    self->num_queued_events++;
  else
    self->num_events++;
}

void
midi_events_add_channel_pressure (
  MidiEvents * self,
  midi_byte_t  channel,
  midi_byte_t  value,
  midi_time_t  time,
  bool         queued)
{
  g_return_if_fail (channel > 0);

  MidiEvent * ev;
  if (queued)
    ev = &self->queued_events[self->num_queued_events];
  else
    ev = &self->events[self->num_events];

  ev->time = time;
  ev->raw_buffer[0] = (midi_byte_t) (MIDI_CH1_CHAN_AFTERTOUCH | (channel - 1));
  ev->raw_buffer[1] = value;
  ev->raw_buffer_sz = 2;

  if (queued)
    self->num_queued_events++;
  else
    self->num_events++;
}

static inline MidiEventType
get_event_type (const midi_byte_t short_msg[3])
{
  if (midi_is_note_off (short_msg))
    return MIDI_EVENT_TYPE_NOTE_OFF;
  else if (midi_is_note_on (short_msg))
    return MIDI_EVENT_TYPE_NOTE_ON;
  /* note: this is also a controller */
  else if (midi_is_all_notes_off (short_msg))
    return MIDI_EVENT_TYPE_ALL_NOTES_OFF;
  /* note: this is also a controller */
  else if (midi_is_pitch_wheel (short_msg))
    return MIDI_EVENT_TYPE_PITCHBEND;
  else if (midi_is_controller (short_msg))
    return MIDI_EVENT_TYPE_CONTROLLER;
  else if (midi_is_song_position_pointer (short_msg))
    return MIDI_EVENT_TYPE_SONG_POS;
  else if (midi_is_start (short_msg))
    return MIDI_EVENT_TYPE_START;
  else if (midi_is_stop (short_msg))
    return MIDI_EVENT_TYPE_STOP;
  else if (midi_is_continue (short_msg))
    return MIDI_EVENT_TYPE_CONTINUE;
  else if (midi_is_clock (short_msg))
    return MIDI_EVENT_TYPE_CLOCK;
  else
    return MIDI_EVENT_TYPE_RAW;
}

HOT static int
midi_event_cmpfunc (const void * _a, const void * _b)
{
  const MidiEvent * a = (MidiEvent const *) _a;
  const MidiEvent * b = (MidiEvent const *) _b;
  if (a->time == b->time)
    {
      MidiEventType a_type = get_event_type (a->raw_buffer);
      MidiEventType b_type = get_event_type (b->raw_buffer);
      (void) midi_event_type_strings;
#if 0
      g_debug ("a type %s, b type %s",
        midi_event_type_strings[a_type],
        midi_event_type_strings[b_type]);
#endif
      return (int) a_type - (int) b_type;
    }
  return (int) a->time - (int) b->time;
}

/**
 * Sorts the MidiEvents by time.
 */
void
midi_events_sort (MidiEvents * self, const bool queued)
{
  MidiEvent * events;
  size_t      num_events;
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
  qsort (events, num_events, sizeof (MidiEvent), midi_event_cmpfunc);
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
  g_return_if_fail (channel > 0);
#if 0
  g_message (
    "%s: ch %"PRIu8", pitch %"PRIu8", vel %"PRIu8
    ", time %"PRIu32,
    __func__, channel, note_pitch, velocity, time);
#endif

  MidiEvent * ev;
  if (queued)
    ev = &self->queued_events[self->num_queued_events];
  else
    ev = &self->events[self->num_events];

  ev->time = time;
  ev->raw_buffer[0] = (midi_byte_t) (MIDI_CH1_NOTE_ON | (channel - 1));
  ev->raw_buffer[1] = note_pitch;
  ev->raw_buffer[2] = velocity;
  ev->raw_buffer_sz = 3;
  g_return_if_fail (midi_is_note_on (ev->raw_buffer));

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
  MidiEvents *            self,
  const ChordDescriptor * descr,
  midi_byte_t             channel,
  midi_byte_t             velocity,
  midi_time_t             time,
  bool                    queued)
{
#if 0
  g_message (
    "%s: vel %"PRIu8", time %"PRIu32,
    __func__, velocity, _time);
#endif

  for (int i = 0; i < CHORD_DESCRIPTOR_MAX_NOTES; i++)
    {
      if (descr->notes[i])
        {
          midi_events_add_note_on (
            self, channel, i + 36, velocity, time, queued);
        }
    }
}

/**
 * Adds a note off for each note in the chord.
 */
void
midi_events_add_note_offs_from_chord_descr (
  MidiEvents *            self,
  const ChordDescriptor * descr,
  midi_byte_t             channel,
  midi_time_t             time,
  bool                    queued)
{
  for (int i = 0; i < CHORD_DESCRIPTOR_MAX_NOTES; i++)
    {
      if (descr->notes[i])
        {
          midi_events_add_note_off (self, channel, i + 36, time, queued);
        }
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
  if (G_UNLIKELY (buf_size != 3))
    {
#if 0
      g_debug (
        "buf size of %d received (%"PRIu8" %"
        PRIu8" %"PRIu8"), expected 3, skipping...",
        buf_size > 0 ? buf[0] : 0,
        buf_size > 1 ? buf[1] : 0,
        buf_size > 2 ? buf[2] : 0, buf_size);
#endif

      return;
    }

  midi_byte_t type = buf[0] & 0xf0;
  midi_byte_t channel = (midi_byte_t) ((buf[0] & 0xf) + 1);
  switch (type)
    {
    case MIDI_CH1_NOTE_ON:
      /* velocity == 0 means note off */
      if (buf[2] == 0)
        goto note_off;

      midi_events_add_note_on (self, channel, buf[1], buf[2], time, queued);
      break;
    case MIDI_CH1_NOTE_OFF:
note_off:
      midi_events_add_note_off (self, channel, buf[1], time, queued);
      break;
    case MIDI_CH1_PITCH_WHEEL_RANGE:
      midi_events_add_pitchbend (
        self, channel, midi_get_14_bit_value (buf), time, queued);
      break;
    case MIDI_CH1_CHAN_AFTERTOUCH:
      midi_events_add_channel_pressure (self, channel, buf[1], time, queued);
      break;
    case MIDI_SYSTEM_MESSAGE:
      /* ignore active sensing */
      if (buf[0] != 0xFE)
        {
#if 0
          print_unknown_event_message (
            buf, buf_size);
#endif
        }
      break;
    case MIDI_CH1_CTRL_CHANGE:
      midi_events_add_control_change (self, 1, buf[1], buf[2], time, queued);
      break;
    case MIDI_CH1_POLY_AFTERTOUCH:
      /* TODO unimplemented */
      /* fallthrough */
    default:
      midi_events_add_raw (self, buf, (size_t) buf_size, time, queued);
      break;
    }
}

void
midi_events_delete_event (
  MidiEvents *      self,
  const MidiEvent * ev,
  const bool        queued)
{
  MidiEvent * arr = queued ? self->queued_events : self->events;
#define NUM_EVENTS (queued ? self->num_queued_events : self->num_events)

  for (int i = 0; i < NUM_EVENTS; i++)
    {
      MidiEvent * cur_ev = &arr[i];
      if (cur_ev == ev)
        {
          for (int k = i; k < NUM_EVENTS - 1; k++)
            {
              midi_event_copy (&arr[k + 1], &arr[k]);
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
midi_event_set_velocity (MidiEvent * ev, midi_byte_t vel)
{
  ev->raw_buffer[2] = vel;
}

void
midi_event_print (const MidiEvent * ev)
{
  char msg[400];
  midi_print_to_str (ev->raw_buffer, ev->raw_buffer_sz, msg);
  g_message ("%s | time: %u", msg, ev->time);
}

void
midi_events_print (MidiEvents * self, const int queued)
{
  MidiEvent * arr = queued ? self->queued_events : self->events;
  MidiEvent * ev1;
#define NUM_EVENTS (queued ? self->num_queued_events : self->num_events)

  int i;
  for (i = 0; i < NUM_EVENTS; i++)
    {
      ev1 = &arr[i];
      midi_event_print (ev1);
    }
}

/**
 * Queues MIDI note off to event queues.
 *
 * @param queued Send the event to queues instead
 *   of main events.
 */
void
midi_events_panic_all (const bool queued)
{
  g_message ("~ midi panic all (queued: %d) ~", queued);

  midi_events_panic (
    AUDIO_ENGINE->midi_editor_manual_press->midi_events, queued);

  Track * track;
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      track = TRACKLIST->tracks[i];

      if (
        track_type_has_piano_roll (track->type)
        || track->type == TrackType::TRACK_TYPE_CHORD)
        {
          midi_events_panic (track->processor->piano_roll->midi_events, queued);
        }
    }
}

/**
 * Clears duplicates.
 *
 * @param queued Clear duplicates from queued events
 * instead.
 */
void
midi_events_clear_duplicates (MidiEvents * self, const int queued)
{
  g_return_if_fail (self);

  MidiEvent * arr = queued ? self->queued_events : self->events;
  MidiEvent * ev1, *ev2;
#define NUM_EVENTS (queued ? self->num_queued_events : self->num_events)

  int i, j, k;
  for (i = 0; i < NUM_EVENTS; i++)
    {
      ev1 = &arr[i];

      for (j = i + 1; j < NUM_EVENTS; j++)
        {
          ev2 = &arr[j];

          if (midi_events_are_equal (ev1, ev2))
            {
#if 0
              g_message (
                "removing duplicate MIDI event");
#endif
              for (k = j; k < NUM_EVENTS; k++)
                {
                  midi_event_copy (&arr[k], &arr[k + 1]);
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
midi_events_free (MidiEvents * self)
{
  zix_sem_destroy (&self->access_sem);

  object_zero_and_free (self);
}
