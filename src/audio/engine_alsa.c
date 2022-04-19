// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
 */

#include "zrythm-config.h"

#if 0

#  ifdef HAVE_ALSA

#    include <math.h>
#    include <stdio.h>
#    include <stdlib.h>

#    include "audio/engine.h"
#    include "audio/engine_alsa.h"
#    include "audio/port.h"
#    include "project.h"
#    include "utils/midi.h"
#    include "utils/ui.h"

#    include <glib/gi18n.h>

#    include <alsa/asoundlib.h>
#    include <pthread.h>

#    define POLY 10

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
      "Unable to set period size %lu for playback: "
      "%s",
      period_size,
      snd_strerror (err));
  dir = 0;
  err =
    snd_pcm_hw_params_get_period_size (
      self->hw_params, &period_size, &dir);
  if (err < 0)
    g_warning (
      "Unable to period size %lu for playback: %s",
      period_size,
      snd_strerror (err));
  self->block_length = (nframes_t) period_size;

  snd_pcm_uframes_t buffer_size =
    period_size * 2; /* 2 channels */
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

static inline snd_pcm_sframes_t
process_cb (
  AudioEngine* self,
  snd_pcm_sframes_t nframes)
{
  self->nframes = (nframes_t) nframes;

  memset (
    self->alsa_out_buf, 0, (size_t) (nframes * 2));
  engine_process (self, self->nframes);

  return
    snd_pcm_writei (
      self->playback_handle,
      self->alsa_out_buf,
      (snd_pcm_uframes_t) nframes);
}

/**
 * Called when an alsa MIDI event is received on
 * any registered port.
 */
static inline int
on_midi_event (
  AudioEngine* self)
{
  snd_seq_event_t *ev;
  Port * port;

  uint32_t time = 0;

  do
    {
      /* get alsa seq event */
      snd_seq_event_input (
        self->seq_handle, &ev);

      /* find the zrythm port by the alsa seq
       * port ID */
      port =
        port_find_by_alsa_seq_id (
          ev->dest.port);

      g_return_val_if_fail (port, -1);

      zix_sem_wait (
        &port->midi_events->access_sem);

      switch (ev->type)
        {
        /* see https://www.alsa-project.org/alsa-doc/alsa-lib/group___seq_events.html for more */
        case SND_SEQ_EVENT_PITCHBEND:
          g_message ("pitch %d",
                     ev->data.control.value);
          midi_events_add_pitchbend (
            port->midi_events, 1,
            (midi_byte_t)
            ev->data.control.value,
            time++, 1);
          break;
        case SND_SEQ_EVENT_CONTROLLER:
          g_message ("modulation %d",
                     ev->data.control.value);
          midi_events_add_control_change (
            port->midi_events,
            1, (midi_byte_t) ev->data.control.param,
            (midi_byte_t) ev->data.control.value,
            time++, 1);
          break;
        case SND_SEQ_EVENT_NOTEON:
          g_message ("note on: note %d vel %d",
                     ev->data.note.note,
                     ev->data.note.velocity);
          /*g_message ("time %u:%u",*/
                     /*ev->time.time.tv_sec,*/
                     /*ev->time.time.tv_nsec);*/
          midi_events_add_note_on (
            port->midi_events,
            1, ev->data.note.note,
            ev->data.note.velocity,
            time++, 1);
          break;
        case SND_SEQ_EVENT_NOTEOFF:
          g_message ("note off: note %d",
                     ev->data.note.note);
          midi_events_add_note_off (
            port->midi_events,
            1, ev->data.note.note,
            time++, 1);
          /* FIXME passing ticks, should pass
           * frames */
          break;
        default:
          g_message ("Unknown MIDI event received");
          break;
      }

      zix_sem_post (
        &port->midi_events->access_sem);

      snd_seq_free_event(ev);
    } while (
        snd_seq_event_input_pending (
          self->seq_handle, 0) > 0);

  return 0;
}

/**
 * Tests if ALSA works.
 *
 * @param win If window is non-null, it will display
 *   a message to it.
 * @return 0 for OK, non-zero for not ok.
 */
int
engine_alsa_test (
  GtkWindow * win)
{
  snd_pcm_t * playback_handle;
  int err =
    snd_pcm_open(
      &playback_handle, "default",
      SND_PCM_STREAM_PLAYBACK, 0);
  if (err < 0)
    {
      char * msg =
        g_strdup_printf (
          _("ALSA Error: %s"),
          snd_strerror (err));
      ui_show_error_message (
        win, msg);
      g_free (msg);
      return 1;
    }

  return err;
}

/**
 * Fill the output buffers at the end of the
 * cycle.
 */
void
engine_alsa_fill_out_bufs (
  AudioEngine *   self,
  const nframes_t nframes)
{
  for (unsigned int i = 0; i < nframes; i++)
    {
      self->alsa_out_buf[i * 2] =
        self->monitor_out->l->buf[i];
      self->alsa_out_buf[i * 2 + 1] =
        self->monitor_out->l->buf[i];
    }
}

static void *
audio_thread (void * _self)
{
  AudioEngine * self = (AudioEngine *) _self;

  int err;
  err =
    snd_pcm_open(
      &self->playback_handle, "plughw:0,0",
      SND_PCM_STREAM_PLAYBACK, 0);
  if (err < 0)
    g_warning (
      "%s [ALSA]: Cannot open audio device: %s",
      __func__, snd_strerror (err));

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

  g_usleep (8000);

  struct pollfd *pfds;
  int nfds =
    snd_pcm_poll_descriptors_count (
      self->playback_handle);
  pfds =
    (struct pollfd *)
    alloca (
      (size_t) nfds * sizeof (struct pollfd));
  snd_pcm_poll_descriptors (
    self->playback_handle, pfds,
    (unsigned int) nfds);
  nframes_t frames_processed;
  int l1;
  while (1)
    {
      if (poll (pfds, (nfds_t) nfds, 1000) > 0)
        {
          for (l1 = 0; l1 < nfds; l1++)
            {
              while (pfds[l1].revents > 0)
                {
                  frames_processed =
                    (nframes_t)
                    process_cb (
                      self, self->block_length);
                  if (frames_processed <
                        self->block_length)
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

static void *
midi_thread (void * _self)
{
  AudioEngine * self = (AudioEngine *) _self;

  int err =
    snd_seq_open (
      &self->seq_handle, "default",
      SND_SEQ_OPEN_DUPLEX, 0);
  if (err < 0)
    {
      g_warning (
        "Error opening ALSA sequencer: %s",
        snd_strerror (err));
    }

  snd_seq_set_client_name (
    self->seq_handle, "Zrythm");

  int seq_nfds, l1;
  struct pollfd *pfds;
  seq_nfds =
    snd_seq_poll_descriptors_count (
      self->seq_handle, POLLIN);
  pfds =
    (struct pollfd *)
    alloca (
      sizeof (struct pollfd) * (size_t) seq_nfds);
  snd_seq_poll_descriptors (
    self->seq_handle, pfds,
    (unsigned int) seq_nfds, POLLIN);

  while (1)
    {
      if (poll (pfds, (nfds_t) seq_nfds, 1000) > 0)
        {
          for (l1 = 0; l1 < seq_nfds; l1++)
            {
               if (pfds[l1].revents > 0)
                 {
                   if (!PROJECT->loaded ||
                       !AUDIO_ENGINE->run)
                     {
                       g_usleep (1000);
                     }
                   on_midi_event (self);
                 }
            }
        }
    }

  return NULL;
}

/**
 * Prepares for processing.
 *
 * Called at the start of each process cycle.
 */
void
engine_alsa_prepare_process (
  AudioEngine * self)
{
  memset (
    self->alsa_out_buf, 0,
    2 * AUDIO_ENGINE->block_length *
      sizeof (float));
}

int
engine_alsa_setup (
  AudioEngine *self)
{
  g_message ("setting up ALSA...");

  self->block_length = 512;
  self->sample_rate = 44100;
  self->midi_buf_size = 4096;
  self->alsa_out_buf =
    (float *) malloc (
      2 * sizeof (float) * self->block_length);

  pthread_t thread_id;
  int ret =
    pthread_create (
      &thread_id, NULL,
      &audio_thread, self);
  if (ret)
    {
      perror (
        "Failed to create ALSA audio thread:");
      return -1;
    }

  /* wait for the thread to initialize */
  g_usleep (100000);

  g_message ("ALSA setup complete");

  return 0;
}

int
engine_alsa_midi_setup (
  AudioEngine * self,
  int           loading)
{
  /*if (loading)*/
    /*{*/
    /*}*/
  /*else*/
    /*{*/
    /*}*/
  g_message ("creating ALSA midi thread");

  pthread_t thread_id;
  pthread_create (
    &thread_id, NULL,
    &midi_thread, self);

  /* wait for the thread to initialize the seq */
  g_usleep (100000);

  return 0;
}

#  endif

#endif
