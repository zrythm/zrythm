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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "audio/engine.h"
#include "audio/engine_alsa.h"
#include "audio/port.h"
#include "project.h"

#include <alsa/asoundlib.h>
#include <pthread.h>

void
engine_alsa_fill_stereo_out_buffs (
  AudioEngine * engine)
{
  int nframes = engine->nframes;
  for (int i = 0; i < nframes; i++)
    {
      engine->alsa_out_buf[i * 2] =
        MIXER->master->channel->
          stereo_out->l->buf[i];
      engine->alsa_out_buf[i * 2 + 1] =
        MIXER->master->channel->
          stereo_out->r->buf[i];
    }
}

static int
set_sw_params (AudioEngine * self)
{
  int err;

  /* get current swparams */
  err =
    snd_pcm_sw_params_current (
      self->playback_handle, self->sw_params);
  if (err < 0)
    {
      g_warning (
        "Cannot init sw params: %s",
        snd_strerror (err));
      return err;
    }

  /* allow the transfer when at least period_size
   * samples can be processed */
  g_warn_if_fail (self->block_length > 0);
  err =
    snd_pcm_sw_params_set_avail_min (
      self->playback_handle, self->sw_params,
      self->block_length);
  if (err < 0)
    {
      g_warning (
        "Cannot set avail min: %s",
        snd_strerror (err));
      return err;
    }

  /* start the transfer whena period is full */
  err =
    snd_pcm_sw_params_set_start_threshold (
      self->playback_handle,
      self->sw_params,
      self->block_length);
  if (err < 0)
    {
      g_warning (
        "Cannot set start threshold: %s",
        snd_strerror (err));
      return err;
    }

  /* write the parameters to the playback device */
  err =
    snd_pcm_sw_params (
      self->playback_handle, self->sw_params);
  if (err < 0)
    {
      g_warning (
        "Cannot set sw params: %s",
        snd_strerror (err));
      return err;
    }

  return 0;
}

static void
set_hw_params (
  AudioEngine * self)
{
  int err, dir;

  /* choose all parameters */
  err =
    snd_pcm_hw_params_any (
      self->playback_handle, self->hw_params);
  if (err < 0)
    g_warning (
      "Failed to choose all parameters: %s",
      snd_strerror (err));

  /* set interleaved read/write format */
  err =
    snd_pcm_hw_params_set_access (
      self->playback_handle, self->hw_params,
      SND_PCM_ACCESS_RW_INTERLEAVED);
  if (err < 0)
    g_warning (
      "Cannot set access type: %s",
      snd_strerror (err));

  /* set the sample format */
  err =
    snd_pcm_hw_params_set_format (
      self->playback_handle, self->hw_params,
      SND_PCM_FORMAT_FLOAT_LE);
  if (err < 0)
    g_warning (
      "Cannot set format: %s",
      snd_strerror (err));

  /* set num channels */
  err =
    snd_pcm_hw_params_set_channels (
      self->playback_handle, self->hw_params, 2);
  if (err < 0)
    g_warning (
      "Cannot set channels: %s",
      snd_strerror (err));

  /* set sample rate */
  err =
    snd_pcm_hw_params_set_rate_near (
      self->playback_handle, self->hw_params,
      &self->sample_rate, 0);
  if (err < 0)
    g_warning (
      "Cannot set sample rate: %s",
      snd_strerror (err));

  /* set periods */
  snd_pcm_hw_params_set_periods (
    self->playback_handle, self->hw_params, 2, 0);

  snd_pcm_uframes_t period_size =
    self->block_length;
  dir = 0;
  err =
    snd_pcm_hw_params_set_period_size_near (
      self->playback_handle, self->hw_params,
      &period_size, &dir);
  if (err < 0)
    g_warning (
      "Unable to set perriod size %lu for playback: "
      "%s",
      period_size,
      snd_strerror (err));
  dir = 0;
  err =
    snd_pcm_hw_params_get_period_size (
      self->hw_params, &period_size, &dir);
  if (err < 0)
    g_warning (
      "Unable to perriod size %lu for playback: %s",
      period_size,
      snd_strerror (err));

  snd_pcm_uframes_t buffer_size =
    period_size * 2;
  err =
    snd_pcm_hw_params_set_buffer_size_near (
      self->playback_handle, self->hw_params,
      &buffer_size);
  if (err < 0)
    g_warning (
      "Unable to set buffer size %lu for playback: %s",
      buffer_size,
      snd_strerror (err));
  err =
    snd_pcm_hw_params_get_buffer_size (
      self->hw_params, &buffer_size);

  if (2 * period_size  > buffer_size)
    g_warning ("Buffer %lu too small",
               buffer_size);

	// write the parameters to device
  err =
    snd_pcm_hw_params (
      self->playback_handle, self->hw_params);
  if (err < 0)
    g_warning (
      "Cannot set hw params: %s",
      snd_strerror (err));
}

static inline int
process_cb (
  AudioEngine* self,
  snd_pcm_sframes_t nframes)
{
  self->nframes = nframes;

  memset(self->alsa_out_buf, 0, nframes * 2);
  engine_process (self, nframes);

  return
    snd_pcm_writei (
      self->playback_handle,
      self->alsa_out_buf,
      nframes);
}

static void *
audio_thread (void * _self)
{
  AudioEngine * self = (AudioEngine *) _self;

  int err;
  err =
    snd_pcm_open(
      &self->playback_handle, "default",
      SND_PCM_STREAM_PLAYBACK, 0);
  if (err < 0)
    g_warning (
      "Cannot open audio device: %s",
      snd_strerror (err));

  /* set hw params */
  err =
    snd_pcm_hw_params_malloc (&self->hw_params);
  if (err < 0)
    g_warning (
      "Cannot allocate hw params: %s",
      snd_strerror (err));
  set_hw_params (self);

  /* set sw params */
  err = snd_pcm_sw_params_malloc (&self->sw_params);
  if (err < 0)
    g_warning (
      "Cannot allocate sw params: %s",
      snd_strerror (err));
  set_sw_params (self);
  g_warn_if_fail (self->block_length > 0);

  struct pollfd *pfds;
  int l1, nfds =
    snd_pcm_poll_descriptors_count (
      self->playback_handle);
  pfds =
    (struct pollfd *)
    alloca(nfds * sizeof(struct pollfd));
  snd_pcm_poll_descriptors (
    self->playback_handle, pfds, nfds);
  while (1)
    {
      if (poll (pfds, nfds, 1000) > 0)
        {
          for (l1 = 0; l1 < nfds; l1++)
            {
              if (pfds[l1].revents > 0)
                {
                  if (process_cb (self, self->block_length) < self->block_length)
                    {
                      g_warning ("XRUN");
                      snd_pcm_prepare (
                        self->playback_handle);
                    }
                }
            }
        }
    }
  return NULL;
}

void alsa_setup (
  AudioEngine *self, int loading)
{
  self->block_length = 512;
  self->sample_rate = 44100;
  self->alsa_out_buf =
    (float *) malloc (
      2 * sizeof (float) * self->block_length);

  Port *stereo_out_l, *stereo_out_r,
      *stereo_in_l, *stereo_in_r;

  stereo_out_l =
    port_new_with_type (
      TYPE_AUDIO, FLOW_OUTPUT,
      "ALSA Stereo Out / L");
  stereo_out_r =
    port_new_with_type (
      TYPE_AUDIO, FLOW_OUTPUT,
      "ALSA Stereo Out / R");
  stereo_in_l =
    port_new_with_type (
      TYPE_AUDIO, FLOW_INPUT,
      "ALSA Stereo In / L");
  stereo_in_r =
    port_new_with_type (
      TYPE_AUDIO, FLOW_INPUT,
      "ALSA Stereo In / R");

  stereo_in_l->identifier.owner_type =
    PORT_OWNER_TYPE_BACKEND;
  stereo_in_r->identifier.owner_type =
    PORT_OWNER_TYPE_BACKEND;
  stereo_out_l->identifier.owner_type =
    PORT_OWNER_TYPE_BACKEND;
  stereo_out_r->identifier.owner_type =
    PORT_OWNER_TYPE_BACKEND;

  self->stereo_out =
    stereo_ports_new (stereo_out_l, stereo_out_r);
  self->stereo_in =
    stereo_ports_new (stereo_in_l, stereo_in_r);

  pthread_t thread_id;
  pthread_create(
    &thread_id, NULL,
    &audio_thread, self);

  g_message ("ALSA setup complete");

  return;
}
