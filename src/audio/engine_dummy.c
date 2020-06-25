/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/engine.h"
#include "audio/engine_dummy.h"
#include "audio/router.h"
#include "audio/port.h"
#include "audio/tempo_track.h"
#include "project.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>

static gpointer
process_cb (gpointer data)
{
  AudioEngine * self = (AudioEngine *) data;

  gulong sleep_time =
    (gulong)
    (((double) self->block_length /
       self->sample_rate) *
       1000.0 * 1000);

  g_message (
    "Running dummy audio engine for first time");

  while (1)
    {
      if (self->stop_dummy_audio_thread)
        break;

      engine_process (self, self->block_length);
      g_usleep (sleep_time);
    }

  return NULL;
}

int
engine_dummy_setup (
  AudioEngine * self)
{
  /* Set audio engine properties */
  self->sample_rate = 44100;
  self->midi_buf_size = 4096;

  if (ZRYTHM_HAVE_UI && zrythm_app->buf_size)
    {
      self->block_length =
        (nframes_t)
        strtol (
          zrythm_app->buf_size, (char **)NULL, 10);
    }
  else
    {
      self->block_length = 256;
    }

  g_warn_if_fail (
    TRANSPORT && TRANSPORT->beats_per_bar > 1);

  g_message ("Dummy Engine set up");

  return 0;
}

int
engine_dummy_midi_setup (
  AudioEngine * self)
{
  self->midi_buf_size = 4096;

  return 0;
}

int
engine_dummy_activate (
  AudioEngine * self,
  bool          activate)
{
  if (activate)
    {
      g_message ("%s: activating...", __func__);

      self->stop_dummy_audio_thread = false;
      self->dummy_audio_thread =
        g_thread_new (
          "process_cb", process_cb, self);
    }
  else
    {
      g_message ("%s: deactivating...", __func__);

      self->stop_dummy_audio_thread = true;
      g_thread_join (self->dummy_audio_thread);
    }

  g_message ("%s: done", __func__);

  return 0;
}

void
engine_dummy_tear_down (
  AudioEngine * self)
{
}
