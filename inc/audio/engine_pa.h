/*
 * audio/engine_pa.c - Port Audio engine
 *
 * Copyright (C) 2019 Alexandros Theodotou
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

#ifndef __AUDIO_ENGINE_PA_H__
#define __AUDIO_ENGINE_PA_H__

#include <portaudio.h>

typedef struct AudioEngine AudioEngine;

/**
 * Set up Port Audio.
 */
void
pa_setup (AudioEngine * engine);

/**
 * This routine will be called by the PortAudio engine when
 * audio is needed. It may called at interrupt level on some
 * machines so don't do anything that could mess up the system
 * like calling malloc() or free().
*/
int
pa_stream_cb (const void *                    input,
              void *                          output,
              unsigned long                   nframes,
              const PaStreamCallbackTimeInfo* time_info,
              PaStreamCallbackFlags           status_flags,
              void *                          user_data);

/**
 * Opens a Port Audio stream.
 */
PaStream *
pa_open_stream (AudioEngine * engine);

/**
 * Closes Port Audio.
 */
void
pa_terminate (AudioEngine * engine);

#endif
