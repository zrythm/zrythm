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

#include "audio/engine.h"
#include "audio/engine_dummy.h"
#include "audio/mixer.h"
#include "audio/port.h"
#include "project.h"

#include <gtk/gtk.h>

static gpointer
process_cb (gpointer data)
{
  AudioEngine * self = (AudioEngine *) data;
  while (1)
    {
      engine_process (self, self->block_length);
      g_usleep (((double) self->block_length /
                 self->sample_rate) *
                1000.0 * 1000);
    }

  return NULL;
}

void
engine_dummy_setup (
  AudioEngine * self,
  int           loading)
{
  /* Set audio engine properties */
  self->sample_rate   = 44100;
  self->block_length  = 256;
  self->midi_buf_size = 4096;

  g_warn_if_fail (TRANSPORT &&
                  TRANSPORT->beats_per_bar > 1);

  engine_update_frames_per_tick (
    TRANSPORT->beats_per_bar,
    TRANSPORT->bpm,
    self->sample_rate);

  /* create ports */
  Port * stereo_out_l, * stereo_out_r,
       * stereo_in_l, * stereo_in_r;

  if (loading)
    {
    }
  else
    {
      stereo_out_l =
        port_new_with_data (
          INTERNAL_PA_PORT,
          TYPE_AUDIO,
          FLOW_OUTPUT,
          "Dummy Stereo Out / L",
          NULL);
      stereo_out_r =
        port_new_with_data (
          INTERNAL_PA_PORT,
          TYPE_AUDIO,
          FLOW_OUTPUT,
          "Dummy Stereo Out / R",
          NULL);
      stereo_in_l =
        port_new_with_data (
          INTERNAL_PA_PORT,
          TYPE_AUDIO,
          FLOW_INPUT,
          "Dummy Stereo In / L",
          NULL);
      stereo_in_r =
        port_new_with_data (
          INTERNAL_PA_PORT,
          TYPE_AUDIO,
          FLOW_INPUT,
          "Dummy Stereo In / R",
          NULL);

      self->stereo_in  =
        stereo_ports_new (stereo_in_l,
                          stereo_in_r);
      self->stereo_out =
        stereo_ports_new (stereo_out_l,
                          stereo_out_r);
    }

  g_thread_new ("process_cb",
                process_cb,
                self);


  g_message ("Dummy Engine set up");
}

void
engine_dummy_midi_setup (
  AudioEngine * self,
  int           loading)
{
  self->midi_buf_size = 4096;

  if (loading)
    {
    }
  else
    {
      self->midi_in =
        port_new_with_type (
          TYPE_EVENT,
          FLOW_INPUT,
          "Dummy MIDI In");
      self->midi_in->identifier.owner_type =
        PORT_OWNER_TYPE_BACKEND;
    }
}
