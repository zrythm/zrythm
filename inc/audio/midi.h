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
 */

#ifndef __AUDIO_MIDI_H__
#define __AUDIO_MIDI_H__

#include "config.h"

/* see http://www.onicos.com/staff/iz/formats/midi-event.html */
#define MIDI_CH1_NOTE_ON 0x90
#define MIDI_CH2_NOTE_ON 0x91
#define MIDI_CH1_NOTE_OFF 0x80
#define MIDI_CH2_NOTE_OFF 0x81
#define MIDI_CH1_POLY_AFTERTOUCH 0xA0
#define MIDI_CH1_CTRL_CHANGE 0xB0
#define MIDI_CH1_PROG_CHANGE 0xC0
#define MIDI_CH1_CHAN_AFTERTOUCH 0xD0
#define MIDI_CH1_PITCH_WHEEL_RANGE 0xE0
#define MIDI_ALL_NOTES_OFF 0x7B
#define MIDI_ALL_SOUND_OFF 0x78
#define MAX_MIDI_EVENTS 128

#include "utils/sem.h"

#ifdef HAVE_JACK
#include <jack/midiport.h>
#endif

typedef struct MidiEvents MidiEvents;

/**
 * Container for passing midi events through ports.
 * This should be passed in the data field of MIDI Ports
 */
typedef struct MidiEvents
{
  int        num_events;       ///< number of events
#ifdef HAVE_JACK
  jack_midi_event_t   jack_midi_events[MAX_MIDI_EVENTS];       ///< jack midi events
#endif
  MidiEvents           * queue; ///< for queing events. engine will copy them to the
                              ///< original MIDI events when ready to be processed
                              //
  /** Dequeued already or not. */
  int                 processed;

  //ZixSem              processed_sem;
} MidiEvents;

/**
 * Returns a MidiEvents instance with pre-allocated
 * buffers for each midi event.
 *
 * Optionally allocates/initializes the queue.
 */
MidiEvents *
midi_events_new (int init_queue);

/**
 * Appends the events from src to dest
 */
void
midi_events_append (MidiEvents * src, MidiEvents * dest);

/**
 * Clears midi events.
 */
void
midi_events_clear (MidiEvents * midi_events);

/**
 * Copies the queue contents to the original struct
 */
void
midi_events_dequeue (MidiEvents * midi_events);

/**
 * Returns if a note on event for the given note
 * exists in the given events.
 */
int
midi_events_check_for_note_on (
  MidiEvents * midi_events, int note);

/**
 * Deletes the midi event with a note on signal
 * from the queue, and returns if it deleted or not.
 */
int
midi_events_delete_note_on (
  MidiEvents * midi_events, int note);

/**
 * Queues MIDI note off to event queue.
 */
void
midi_panic (MidiEvents * events);

/**
 * Queues MIDI note off to event queue.
 */
void
midi_panic_all ();

#endif
