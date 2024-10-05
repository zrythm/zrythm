// SPDX-FileCopyrightText: © 2019 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_ENGINE_DUMMY_H__
#define __AUDIO_ENGINE_DUMMY_H__

class AudioEngine;

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
