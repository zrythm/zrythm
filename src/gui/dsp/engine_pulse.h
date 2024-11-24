// SPDX-FileCopyrightText: © 2020 Ryan Gonzalez <rymg19 at gmail dot com>
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "zrythm-config.h"

#if HAVE_PULSEAUDIO

#  ifndef __AUDIO_ENGINE_PULSE_H__
#    define __AUDIO_ENGINE_PULSE_H__

#    include "gtk_wrapper.h"
#    include "utils/types.h"
#    include <pulse/pulseaudio.h>

class AudioEngine;

/**
 * @addtogroup dsp
 *
 * @{
 */

/**
 * Set up PulseAudio.
 */
int
engine_pulse_setup (AudioEngine * self);

void
engine_pulse_activate (AudioEngine * self, gboolean activate);

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
