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

#include "config.h"

#ifdef HAVE_JACK

#ifndef __AUDIO_ENGINE_JACK_H__
#define __AUDIO_ENGINE_JACK_H__

#include <stdlib.h>

#define JACK_PORT_T(exp) ((jack_port_t *) exp)
#define MIDI_IN_EVENT(i) \
  (AUDIO_ENGINE->midi_in->midi_events-> \
   jack_midi_events[i])
#define MIDI_IN_NUM_EVENTS \
  AUDIO_ENGINE->midi_in->midi_events->num_events

typedef struct AudioEngine AudioEngine;

/** Jack sample rate callback. */
int
jack_sample_rate_cb (uint32_t nframes, void * data);

/** Jack buffer size callback. */
int
jack_buffer_size_cb (uint32_t nframes, void* data);

/**
 * Receives MIDI events from JACK MIDI and puts them
 * in the JACK MIDI in port.
 */
void
engine_jack_receive_midi_events (
  AudioEngine * self,
  uint32_t      nframes,
  int           print);

/**
 * The process callback for this JACK application is
 * called in a special realtime thread once for each audio
 * cycle.
 */
int
jack_process_cb (uint32_t nframes, ///< the number of frames to fill
                 void *    data); ///< user data

/**
 * JACK calls this shutdown_callback if the server ever
 * shuts down or decides to disconnect the client.
 */
void
jack_shutdown_cb (void *arg);

/**
 * Tests if JACK is working properly.
 *
 * Returns 0 if ok, non-null if has errors.
 *
 * If win is not null, it displays error messages
 * to it.
 */
int
engine_jack_test (
  GtkWindow * win);

/**
 * Zero's out the output buffers.
 */
void
engine_jack_clear_output_buffers (
  AudioEngine * self);

/**
 * Sets up the MIDI engine to use jack.
 *
 * @param loading Loading a Project or not.
 */
int
jack_midi_setup (
  AudioEngine * self,
  int           loading);

/**
 * Sets up the audio engine to use jack.
 *
 * @param loading Loading a Project or not.
 */
int
jack_setup (AudioEngine * self,
            int           loading);

const char *
engine_jack_get_error_message (
  jack_status_t status);

void
jack_tear_down ();

int
engine_jack_activate (
  AudioEngine * self);

#endif
#endif /* HAVE_JACK */
