// SPDX-FileCopyrightText: © 2019 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

namespace zrythm::engine::device_io
{
class AudioEngine;
}

/**
 * Sets up a dummy audio engine.
 */
int
engine_dummy_setup (zrythm::engine::device_io::AudioEngine * self);

int
engine_dummy_process (zrythm::engine::device_io::AudioEngine * self);

/**
 * Sets up a dummy MIDI engine.
 */
int
engine_dummy_midi_setup (zrythm::engine::device_io::AudioEngine * self);

int
engine_dummy_activate (
  zrythm::engine::device_io::AudioEngine * self,
  bool                                     activate);

void
engine_dummy_tear_down (zrythm::engine::device_io::AudioEngine * self);
