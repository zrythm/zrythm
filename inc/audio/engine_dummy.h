/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

#ifndef __AUDIO_ENGINE_DUMMY_H__
#define __AUDIO_ENGINE_DUMMY_H__

#include <stdbool.h>

typedef struct AudioEngine AudioEngine;

/**
 * Sets up a dummy audio engine.
 */
int
engine_dummy_setup (AudioEngine * self);

int
engine_dummy_process (AudioEngine * self);

/**
 * Sets up a dummy MIDI engine.
 */
int
engine_dummy_midi_setup (AudioEngine * self);

int
engine_dummy_activate (AudioEngine * self, bool activate);

void
engine_dummy_tear_down (AudioEngine * self);

#endif
