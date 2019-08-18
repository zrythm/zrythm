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

#include "config.h"

#ifdef HAVE_JACK

#ifndef __AUDIO_ENGINE_JACK_H__
#define __AUDIO_ENGINE_JACK_H__

#include <stdlib.h>

#define JACK_PORT_T(exp) ((jack_port_t *) exp)

typedef struct AudioEngine AudioEngine;

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
