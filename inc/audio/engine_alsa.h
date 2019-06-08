/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

#ifdef __linux__

#ifndef __AUDIO_ENGINE_ALSA_H__
#define __AUDIO_ENGINE_ALSA_H__

#include <stdlib.h>

typedef struct AudioEngine AudioEngine;

void
engine_alsa_fill_stereo_out_buffs (
  AudioEngine * engine);

/**
 * The process callback for this ALSA application is
 * called in a special realtime thread once for each audio
 * cycle.
 */
int
alsa_process_cb (AudioEngine * self);

/**
 * ALSA calls this shutdown_callback if the
 * server ever
 * shuts down or decides to disconnect the client.
 */
void
alsa_shutdown_cb (void *arg);

/**
 * Sets up the audio engine to use alsa.
 */
void
alsa_setup (AudioEngine * self,
            int           loading);

void
alsa_tear_down ();

#endif // header guard
#endif // __linux__
