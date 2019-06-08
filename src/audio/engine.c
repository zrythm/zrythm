/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

/** \file
 * The audio engine of zyrthm. */

#include <math.h>
#include <stdlib.h>
#include <signal.h>

#include "config.h"

#include "audio/automation_track.h"
#include "audio/automation_tracklist.h"
#include "audio/channel.h"
#include "audio/engine.h"
#include "audio/engine_alsa.h"
#include "audio/engine_dummy.h"
#include "audio/engine_jack.h"
#include "audio/engine_pa.h"
#include "audio/midi.h"
#include "audio/mixer.h"
#include "audio/routing.h"
#include "audio/transport.h"
#include "plugins/plugin.h"
#include "plugins/plugin_manager.h"
#include "plugins/lv2_plugin.h"
#include "project.h"
#include "zrythm.h"

#include <gtk/gtk.h>

#ifdef HAVE_JACK
#include <jack/jack.h>
#include <jack/midiport.h>
#endif

 void
engine_update_frames_per_tick (int beats_per_bar,
                               int bpm,
                               int sample_rate)
{
  AUDIO_ENGINE->frames_per_tick =
    (sample_rate * 60.f * beats_per_bar) /
    (bpm * TRANSPORT->ticks_per_bar);

  /* update positions */
  transport_update_position_frames (&AUDIO_ENGINE->transport);
}

static void
init_midi (
  AudioEngine * self,
  int           loading)
{
  g_message ("initializing MIDI...");

  if (!loading)
    {
      self->midi_editor_manual_press =
        port_new_with_type (
          TYPE_EVENT,
          FLOW_INPUT,
          "MIDI Editor Manual Press");
    }

  /* init MIDI queues for manual press */
  self->midi_editor_manual_press->midi_events =
    midi_events_new (1);
}

static void
init_audio (
  AudioEngine * self,
  int           loading)
{
  g_message ("initializing audio...");

  if (loading)
    {
      stereo_ports_init_loaded (self->stereo_in);
      stereo_ports_init_loaded (self->stereo_out);
    }
  else
    {

    }
}

/**
 * Init audio engine.
 *
 * loading is 1 if loading a project.
 */
void
engine_init (AudioEngine * self,
             int           loading)
{
  g_message ("Initializing audio engine...");

  transport_init (&self->transport,
                  loading);

  /* get audio backend */
  int ab_code =
    g_settings_get_enum (
      S_PREFERENCES,
      "audio-backend");
  self->audio_backend = AUDIO_BACKEND_DUMMY;
  /* use ifdef's so that dummy is used if the
   * selected backend isn't available */
  switch (ab_code)
    {
    case AUDIO_BACKEND_DUMMY: // no backend
      break;
#ifdef HAVE_JACK
    case AUDIO_BACKEND_JACK:
      self->audio_backend = AUDIO_BACKEND_JACK;
      break;
#endif
#ifdef __linux__
    case AUDIO_BACKEND_ALSA:
      self->audio_backend = AUDIO_BACKEND_ALSA;
      break;
#endif
#ifdef HAVE_PORT_AUDIO
    case AUDIO_BACKEND_PORT_AUDIO:
      self->audio_backend = AUDIO_BACKEND_PORT_AUDIO;
      break;
#endif
    default:
      g_warn_if_reached ();
      break;
    }

  /* get midi backend */
  int mb_code =
    g_settings_get_enum (
      S_PREFERENCES,
      "midi-backend");
  self->midi_backend = MIDI_BACKEND_DUMMY;
  switch (mb_code)
    {
    case MIDI_BACKEND_DUMMY: // no backend
      break;
#ifdef __linux__
    case MIDI_BACKEND_ALSA:
      self->midi_backend = MIDI_BACKEND_ALSA;
      break;
#endif
#ifdef HAVE_JACK
    case MIDI_BACKEND_JACK:
      self->midi_backend = MIDI_BACKEND_JACK;
      break;
#endif
    default:
      break;
    }

  self->pan_law =
    g_settings_get_enum (
      S_PREFERENCES,
      "pan-law");
  self->pan_algo =
    g_settings_get_enum (
      S_PREFERENCES,
      "pan-algo");

  /* init semaphores */
  zix_sem_init (&self->port_operation_lock, 1);

  if (loading)
    {
      /* load */
      mixer_init_loaded ();
    }

  /* init audio */
  init_audio (self, loading);

  switch (self->audio_backend)
    {
    case AUDIO_BACKEND_DUMMY:
      engine_dummy_setup (self, loading);
      break;
#ifdef __linux__
    case AUDIO_BACKEND_ALSA:
	    alsa_setup(self, loading);
	    break;
#endif
#ifdef HAVE_JACK
    case AUDIO_BACKEND_JACK:
      jack_setup (self, loading);
      break;
#endif
#ifdef HAVE_PORT_AUDIO
    case AUDIO_BACKEND_PORT_AUDIO:
      pa_setup (self, loading);
      break;
#endif
    default:
      g_warn_if_reached ();
      break;
    }

  /* init midi */
  init_midi (self, loading);

  /* set up midi */
  switch (self->midi_backend)
    {
    case MIDI_BACKEND_DUMMY:
      engine_dummy_midi_setup (self, loading);
      break;
#ifdef __linux__
    case MIDI_BACKEND_ALSA:
      jack_midi_setup (self, loading);
      break;
#endif
#ifdef HAVE_JACK
    case MIDI_BACKEND_JACK:
      jack_midi_setup (self, loading);
      break;
#endif
    default:
      g_warn_if_reached ();
      break;
    }

#ifdef HAVE_JACK
  if (self->audio_backend == AUDIO_BACKEND_JACK)
    engine_jack_activate (self);
#endif

  self->buf_size_set = false;
}

void
audio_engine_close (AudioEngine * self)
{
  g_message ("closing audio engine...");

  switch (self->audio_backend)
    {
    case AUDIO_BACKEND_DUMMY:
      break;
#ifdef HAVE_JACK
    case AUDIO_BACKEND_JACK:
      jack_client_close (AUDIO_ENGINE->client);
      break;
#endif
#ifdef HAVE_PORT_AUDIO
    case AUDIO_BACKEND_PORT_AUDIO:
      pa_terminate (AUDIO_ENGINE);
      break;
#endif
    case NUM_AUDIO_BACKENDS:
    default:
      g_warn_if_reached ();
      break;
    }
}

/**
 * Clears the underlying backend's output buffers.
 */
static void
clear_output_buffers (
  AudioEngine * self)
{
  switch (self->audio_backend)
    {
    case AUDIO_BACKEND_JACK:
#ifdef HAVE_JACK
      engine_jack_clear_output_buffers (self);
#endif
      break;
    default:
      break;
    }
}

/**
 * To be called by each implementation to prepare the
 * structures before processing.
 *
 * Clears buffers, marks all as unprocessed, etc.
 */
void
engine_process_prepare (
  AudioEngine * self,
  uint32_t nframes)
{
  int i, j;

  self->last_time_taken =
    g_get_monotonic_time ();
  self->nframes = nframes;

  if (TRANSPORT->play_state ==
      PLAYSTATE_PAUSE_REQUESTED)
    {
      /*g_message ("pause requested handled");*/
      /*TRANSPORT->play_state = PLAYSTATE_PAUSED;*/
      /*zix_sem_post (&TRANSPORT->paused);*/
    }
  else if (TRANSPORT->play_state ==
           PLAYSTATE_ROLL_REQUESTED)
    {
      TRANSPORT->play_state = PLAYSTATE_ROLLING;
    }

  int ret =
    zix_sem_try_wait (
      &self->port_operation_lock);

  if (!ret && !self->exporting)
    {
      self->skip_cycle = 1;
      return;
    }

  /* reset all buffers */
  port_clear_buffer (self->midi_in);

  /* prepare channels for this cycle */
  Channel * ch;
  for (i = 0; i < TRACKLIST->num_tracks; i++)
    {
      ch = TRACKLIST->tracks[i]->channel;

      if (ch)
        channel_prepare_process (ch);
    }

  /* for each automation track, update the val */
  /* TODO move to routing process node */
  AutomationTracklist * atl;
  AutomationTrack * at;
  float val;
  for (i = 0; i < TRACKLIST->num_tracks; i++)
    {
      atl =
        track_get_automation_tracklist (
          TRACKLIST->tracks[i]);
      if (!atl)
        continue;
      for (j = 0; j < atl->num_ats; j++)
        {
          at = atl->ats[j];
          val =
            automation_track_get_normalized_val_at_pos (
              at, &PLAYHEAD);
          /*g_message ("val received %f",*/
                     /*val);*/
          /* if there was an automation event
           * at the playhead position, val is
           * positive */
          if (val >= 0.f)
            automatable_set_val_from_normalized (
              at->automatable,
              val);
        }
    }

  self->filled_stereo_out_bufs = 0;
}

static void
receive_midi_events (
  AudioEngine * self,
  uint32_t      nframes,
  int           print)
{
  switch (self->midi_backend)
    {
#ifdef HAVE_JACK
    case MIDI_BACKEND_JACK:
      engine_jack_receive_midi_events (
        self, nframes, print);
      break;
#endif
    case NUM_MIDI_BACKENDS:
    default:
      break;
    }
}

static long count = 0;
/**
 * Processes current cycle.
 *
 * To be called by each implementation in its
 * callback.
 */
int
engine_process (
  AudioEngine * self,
  uint32_t      nframes)
{
  /* Clear output buffers just in case we have to
   * return early */
  clear_output_buffers (self);

  if (!g_atomic_int_get (&self->run))
    {
      g_message ("ENGINE NOT RUNNING");
      return 0;
    }

  count++;
  self->cycle = count;

  /* run pre-process code */
  engine_process_prepare (self, nframes);

  if (AUDIO_ENGINE->skip_cycle)
    {
      AUDIO_ENGINE->skip_cycle = 0;
      return 0;
    }

  /* puts MIDI in events in the MIDI in port */
  receive_midi_events (self, nframes, 1);

  /* this will keep looping until everything was
   * processed in this cycle */
  /*mixer_process (nframes);*/
  /*g_message ("==========================================================================");*/
  router_start_cycle (&MIXER->router);
  /*g_message ("end==========================================================================");*/

  /* run post-process code */
  engine_post_process (self);

  /*
   * processing finished, return 0 (OK)
   */
  return 0;
}

/**
 * To be called after processing for common logic.
 */
void
engine_post_process (AudioEngine * self)
{
  zix_sem_post (&self->port_operation_lock);

  /* stop panicking */
  if (self->panic)
    {
      self->panic = 0;
    }

  /* already processed recording start */
  /*if (IS_TRANSPORT_ROLLING && TRANSPORT->starting_recording)*/
    /*{*/
      /*TRANSPORT->starting_recording = 0;*/
    /*}*/

  /* loop position back if about to exit loop */
  if (TRANSPORT->loop && /* if looping */
      IS_TRANSPORT_ROLLING && /* if rolling */
      (TRANSPORT->playhead_pos.frames <=  /* if current pos is inside loop */
          TRANSPORT->loop_end_pos.frames) &&
      ((long int)(TRANSPORT->playhead_pos.frames + self->nframes) > /* if next pos will be outside loop */
          TRANSPORT->loop_end_pos.frames))
    {
      transport_move_playhead (
        &TRANSPORT->loop_start_pos,
        1);
    }
  else if (IS_TRANSPORT_ROLLING)
    {
      /* move playhead as many samples as processed */
      transport_add_to_playhead (self->nframes);
    }

  AUDIO_ENGINE->last_time_taken =
    g_get_monotonic_time () - AUDIO_ENGINE->last_time_taken;
  /*g_message ("last time taken: %ld");*/
  if (AUDIO_ENGINE->max_time_taken <
      AUDIO_ENGINE->last_time_taken)
    AUDIO_ENGINE->max_time_taken =
      AUDIO_ENGINE->last_time_taken;
  /*g_message ("last time %ld, max time %ld",*/
             /*AUDIO_ENGINE->last_time_taken,*/
             /*AUDIO_ENGINE->max_time_taken);*/
}

/**
 * Closes any connections and free's data.
 */
void
engine_tear_down ()
{
  g_message ("tearing down audio engine...");

#ifdef HAVE_JACK
  if (AUDIO_ENGINE->audio_backend ==
        AUDIO_BACKEND_JACK)
    jack_tear_down ();
#endif

  /* TODO free data */
}
