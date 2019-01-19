/*
 * audio/engine.h - Audio engine
 *
 * Copyright (C) 2018 Alexandros Theodotou
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

#ifndef __AUDIO_ENGINE_JACK_H__
#define __AUDIO_ENGINE_JACK_H__

#include <stdlib.h>

/** Jack sample rate callback. */
int
jack_sample_rate_cb (uint32_t nframes, void * data);

/** Jack buffer size callback. */
int
jack_buffer_size_cb (uint32_t nframes, void* data);

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
 * Sets up the audio engine to use jack.
 */
void
jack_setup (AudioEngine * engine);

#endif
