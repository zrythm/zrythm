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

#include "zrythm-config.h"

#ifdef HAVE_JACK

#ifndef __AUDIO_ENGINE_JACK_H__
#define __AUDIO_ENGINE_JACK_H__

#include <stdlib.h>

#define JACK_PORT_T(exp) ((jack_port_t *) exp)

typedef struct AudioEngine AudioEngine;
typedef enum AudioEngineJackTransportType
  AudioEngineJackTransportType;

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
 * Refreshes the list of external ports.
 */
void
engine_jack_rescan_ports (
  AudioEngine * self);

/**
 * Prepares for processing.
 *
 * Called at the start of each process cycle.
 */
void
engine_jack_prepare_process (
  AudioEngine * self);

/**
 * Updates the JACK Transport type.
 */
void
engine_jack_set_transport_type (
  AudioEngine * self,
  AudioEngineJackTransportType type);

/**
 * Fills the external out bufs.
 */
void
engine_jack_fill_out_bufs (
  AudioEngine *   self,
  const nframes_t nframes);

/**
 * Sets up the MIDI engine to use jack.
 *
 * @param loading Loading a Project or not.
 */
int
engine_jack_midi_setup (
  AudioEngine * self);

/**
 * Sets up the audio engine to use jack.
 *
 * @param loading Loading a Project or not.
 */
int
engine_jack_setup (
  AudioEngine * self);
/**
 * Copies the error message corresponding to \p
 * status in \p msg.
 */
void
engine_jack_get_error_message (
  jack_status_t status,
  char *        msg);

void
engine_jack_tear_down (
  AudioEngine * self);

int
engine_jack_activate (
  AudioEngine * self,
  bool          activate);

/**
 * Returns the JACK type string.
 */
const char *
engine_jack_get_jack_type (
  PortType type);

#endif
#endif /* HAVE_JACK */
