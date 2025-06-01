// SPDX-FileCopyrightText: © 2018-2019, 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "zrythm-config.h"

#include "engine/device_io/engine.h"

#ifdef HAVE_JACK

#  include <cstdlib>

#  define JACK_PORT_T(exp) (static_cast<jack_port_t *> (exp))

#  if 0
/**
 * Tests if JACK is working properly.
 *
 * Returns 0 if ok, non-null if has errors.
 *
 * If win is not null, it displays error messages
 * to it.
 */
int
engine_jack_test (GtkWindow * win);
#  endif

/**
 * Refreshes the list of external ports.
 */
void
engine_jack_rescan_ports (engine::device_io::AudioEngine * self);

/**
 * Disconnects and reconnects the monitor output
 * port to the selected devices.
 *
 * @throw ZrythmException on error.
 */
void
engine_jack_reconnect_monitor (engine::device_io::AudioEngine * self, bool left);

void
engine_jack_handle_position_change (engine::device_io::AudioEngine * self);

void
engine_jack_handle_start (engine::device_io::AudioEngine * self);

void
engine_jack_handle_stop (engine::device_io::AudioEngine * self);

void
engine_jack_handle_buf_size_change (
  engine::device_io::AudioEngine * self,
  uint32_t                         frames);

void
engine_jack_handle_sample_rate_change (
  engine::device_io::AudioEngine * self,
  uint32_t                         samplerate);

/**
 * Prepares for processing.
 *
 * Called at the start of each process cycle.
 */
void
engine_jack_prepare_process (engine::device_io::AudioEngine * self);

/**
 * Updates the JACK Transport type.
 */
void
engine_jack_set_transport_type (
  engine::device_io::AudioEngine *                  self,
  engine::device_io::AudioEngine::JackTransportType type);

/**
 * Fills the external out bufs.
 */
void
engine_jack_fill_out_bufs (
  engine::device_io::AudioEngine * self,
  const nframes_t                  nframes);

/**
 * Sets up the MIDI engine to use jack.
 *
 * @param loading Loading a Project or not.
 */
int
engine_jack_midi_setup (engine::device_io::AudioEngine * self);

/**
 * Sets up the audio engine to use jack.
 *
 * @param loading Loading a Project or not.
 */
int
engine_jack_setup (engine::device_io::AudioEngine * self);

void
engine_jack_tear_down (engine::device_io::AudioEngine * self);

int
engine_jack_activate (engine::device_io::AudioEngine * self, bool activate);

/** Jack buffer size callback. */
int
engine_jack_buffer_size_cb (
  uint32_t                         nframes,
  engine::device_io::AudioEngine * self);

/**
 * Returns if this is a pipewire session.
 */
bool
engine_jack_is_pipewire (engine::device_io::AudioEngine * self);

#endif // HAVE_JACK
