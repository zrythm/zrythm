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

/**
 * \file
 *
 * MIDI related structs, defines and enums.
 */

#ifndef __AUDIO_MIDI_H__
#define __AUDIO_MIDI_H__

#include "config.h"

#include <stdint.h>

#include "zix/sem.h"

#ifdef HAVE_JACK
#include <jack/midiport.h>
#endif

/**
 * @addtogroup audio
 *
 * @{
 */

/* see http://www.onicos.com/staff/iz/formats/midi-event.html */
#define MIDI_CH1_NOTE_ON 0x90
#define MIDI_CH1_NOTE_OFF 0x80
#define MIDI_CH1_POLY_AFTERTOUCH 0xA0
#define MIDI_CH1_CTRL_CHANGE 0xB0
#define MIDI_CH1_PROG_CHANGE 0xC0
#define MIDI_CH1_CHAN_AFTERTOUCH 0xD0
#define MIDI_CH1_PITCH_WHEEL_RANGE 0xE0
#define MIDI_ALL_NOTES_OFF 0x7B
#define MIDI_ALL_SOUND_OFF 0x78
#define MAX_MIDI_EVENTS 128
#define MIDI_SYSTEM_MESSAGE 0xF0

/**
 * Type of MIDI event.
 */
typedef enum MidiEventType
{
  MIDI_EVENT_TYPE_PITCHBEND,
  MIDI_EVENT_TYPE_CONTROLLER,
  MIDI_EVENT_TYPE_NOTE_ON,
  MIDI_EVENT_TYPE_NOTE_OFF,
  MIDI_EVENT_TYPE_ALL_NOTES_OFF,
} MidiEventType;

/**
 * Backend-agnostic MIDI event descriptor.
 */
typedef struct MidiEvent
{
  /** The values below will be filled in
   * depending on what event this is. */
  MidiEventType  type;

  /** -8192 to 8191. */
  int            pitchbend;

  /** The controller, for control events. */
  uint8_t        controller;

  /** Control value (also used for modulation
   * wheel (0 ~ 127). */
  uint8_t        control;

  /** MIDI channel. */
  uint8_t        channel;

  /** Note value (0 ~ 127). */
  uint8_t        note_pitch;

  /** Velocity (0 ~ 127). */
  uint8_t        velocity;

  /** Time of the MIDI event, in frames from the
   * start of the current cycle. */
  uint32_t       time;

  /** Raw MIDI data. */
  uint8_t        raw_buffer[3];

} MidiEvent;

typedef struct MidiEvents MidiEvents;
typedef struct Port Port;

/**
 * Container for passing midi events through ports.
 * This should be passed in the data field of MIDI Ports
 */
typedef struct MidiEvents
{
  /** Event count. */
  volatile int num_events;

  /** Events to use in this cycle. */
  MidiEvent  events[MAX_MIDI_EVENTS];

  /**
   * For queueing events from the GUI or from ALSA
   * at random times, since they run in different
   * threads.
   *
   * Engine will copy them to the
   * original MIDI events when ready to be processed
   *
   * Also has other uses.
   */
  MidiEvent  queued_events[MAX_MIDI_EVENTS];
  volatile int num_queued_events;

  /** Semaphore for exclusive read/write. */
  ZixSem     access_sem;

  /** Cache, pointer back to owner Port. */
  /* FIXME not needed */
  Port *     port;

} MidiEvents;

/**
 * Inits the MidiEvents struct.
 */
void
midi_events_init (
  MidiEvents * self);

/**
 * Allocates and inits a MidiEvents struct.
 *
 * @param port Owner Port.
 */
MidiEvents *
midi_events_new (
  Port * port);

/**
 * Copies the members from one MidiEvent to another.
 */
static inline void
midi_event_copy (
  MidiEvent * src,
  MidiEvent * dest)
{
  dest->pitchbend = src->pitchbend;
  dest->controller = src->controller;
  dest->control = src->control;
  dest->channel = src->channel;
  dest->note_pitch = src->note_pitch;
  dest->velocity = src->velocity;
  dest->time = src->time;
  for (int i = 0; i < 3; i++)
    dest->raw_buffer[i] = src->raw_buffer[i];
}

/**
 * Sorts the MIDI events by time ascendingly.
 */
void
midi_events_sort_by_time (
  MidiEvents * self);

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
  const int    start_frame,
  const int    nframes,
  int          queued);

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
  int          queued);

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
  int          check_queued);

/**
 * Parses a MidiEvent from a raw MIDI buffer.
 */
void
midi_events_add_event_from_buf (
  MidiEvents * self,
  uint32_t     time,
  uint8_t *    buf,
  int          buf_size);

/**
 * Adds a note off event to the given MidiEvents.
 *
 * @param channel MIDI channel starting from 1.
 * @param queued Add to queued events instead.
 */
void
midi_events_add_note_off (
  MidiEvents * self,
  uint8_t      channel,
  uint8_t      note_pitch,
  uint32_t     time,
  int          queued);

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
  int          queued);

/**
 * Adds a control event to the given MidiEvents.
 *
 * @param channel MIDI channel starting from 1.
 * @param queued Add to queued events instead.
 */
void
midi_events_add_pitchbend (
  MidiEvents * self,
  uint8_t      channel,
  int          pitchbend,
  uint32_t     time,
  int          queued);

/**
 * Queues MIDI note off to event queue.
 */
void
midi_events_panic (
  MidiEvents * self,
  int          queued);

/**
 * Clears midi events.
 *
 * @param queued Clear queued events instead.
 */
void
midi_events_clear (
  MidiEvents * midi_events,
  int          queued);

/**
 * Copies the queue contents to the original struct
 */
void
midi_events_dequeue (
  MidiEvents * midi_events);

/**
 * Returns if a note on event for the given note
 * exists in the given events.
 */
int
midi_events_check_for_note_on (
  MidiEvents * midi_events,
  int          note,
  int          queued);

/**
 * Deletes the midi event with a note on signal
 * from the queue, and returns if it deleted or not.
 */
int
midi_events_delete_note_on (
  MidiEvents * midi_events,
  int          note,
  int          queued);

#ifdef HAVE_JACK
/**
 * Writes the events to the given JACK buffer.
 */
void
midi_events_copy_to_jack (
  MidiEvents * self,
  void *       buff);
#endif

/**
 * Frees the MIDI events.
 */
void
midi_events_free (
  MidiEvents * self);

/**
 * Queues MIDI note off to event queues.
 *
 * @param queued Send the event to queues instead
 *   of main events.
 */
void
midi_panic_all (
  int queued);

/**
 * @}
 */

#endif
