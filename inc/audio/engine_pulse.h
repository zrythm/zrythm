/*
 * Copyright (C) 2020 Ryan Gonzalez <rymg19 at gmail dot com>
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

#ifdef HAVE_PULSEAUDIO

#  ifndef __AUDIO_ENGINE_PULSE_H__
#    define __AUDIO_ENGINE_PULSE_H__

#    include "utils/types.h"

#    include <gtk/gtk.h>

#    include <pulse/pulseaudio.h>

typedef struct AudioEngine AudioEngine;

/**
 * @addtogroup audio
 *
 * @{
 */

/**
 * Set up PulseAudio.
 */
int
engine_pulse_setup (AudioEngine * self);

void
engine_pulse_activate (
  AudioEngine * self,
  gboolean      activate);

/**
 * Tests if PulseAudio is working properly.
 *
 * Returns 0 if ok, non-null if has errors.
 *
 * If win is not null, it displays error messages
 * to it.
 */
int
engine_pulse_test (GtkWindow * win);

/**
 * Closes PulseAudio.
 */
void
engine_pulse_tear_down (AudioEngine * engine);

/**
 * @}
 */

#  endif
#endif // HAVE_PULSEAUDIO
