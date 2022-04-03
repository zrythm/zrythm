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
 */

#ifndef __AUDIO_ENGINE_RTMIDI_H__
#define __AUDIO_ENGINE_RTMIDI_H__

#include "zrythm-config.h"

#ifdef HAVE_RTMIDI

typedef struct AudioEngine AudioEngine;

/**
 * Tests if the backend is working properly.
 *
 * Returns 0 if ok, non-null if has errors.
 *
 * If win is not null, it displays error messages
 * to it.
 */
int
engine_rtmidi_test (GtkWindow * win);

/**
 * Refreshes the list of external ports.
 */
void
engine_rtmidi_rescan_ports (AudioEngine * self);

/**
 * Sets up the MIDI engine to use Rtmidi.
 *
 * @param loading Loading a Project or not.
 */
int
engine_rtmidi_setup (AudioEngine * self);

/**
 * Gets the number of input ports (devices).
 */
unsigned int
engine_rtmidi_get_num_in_ports (AudioEngine * self);

/**
 * Creates an input port, optionally opening it with
 * the given device ID (from rtmidi port count) and
 * label.
 */
RtMidiPtr
engine_rtmidi_create_in_port (
  AudioEngine * self,
  int           open_port,
  unsigned int  device_id,
  const char *  label);

void
engine_rtmidi_tear_down (AudioEngine * self);

int
engine_rtmidi_activate (
  AudioEngine * self,
  bool          activate);

#endif /* HAVE_RTMIDI */
#endif
