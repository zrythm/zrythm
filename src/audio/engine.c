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
    (bpm * TICKS_PER_BAR);

  /* update positions */
  transport_update_position_frames (&AUDIO_ENGINE->transport);
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

  self->backend =
    g_settings_get_enum (
      S_PREFERENCES,
      "audio-backend");

  /* init semaphores */
  zix_sem_init (&self->port_operation_lock, 1);
  /*zix_sem_init (&MIXER->channel_process_sem, 1);*/

  /* load ports from IDs */
  if (loading)
    {
      mixer_init_loaded ();

      stereo_ports_init_loaded (self->stereo_in);
      stereo_ports_init_loaded (self->stereo_out);
      self->midi_in =
        project_get_port (
          self->midi_in_id);
      self->midi_editor_manual_press =
        project_get_port (
          self->midi_editor_manual_press_id);
    }

#ifdef HAVE_JACK
  if (self->backend == ENGINE_BACKEND_JACK)
    jack_setup (self, loading);
#endif
#ifdef HAVE_PORT_AUDIO
  if (self->backend == ENGINE_BACKEND_PORT_AUDIO)
    pa_setup (self);
#endif

  self->buf_size_set = false;
}

void
close_audio_engine ()
{
  g_message ("closing audio engine...");

#ifdef HAVE_JACK
  if (AUDIO_ENGINE->backend == ENGINE_BACKEND_JACK)
    jack_client_close (AUDIO_ENGINE->client);
#endif
#ifdef HAVE_PORT_AUDIO
  if (AUDIO_ENGINE->backend == ENGINE_BACKEND_PORT_AUDIO)
    pa_terminate (AUDIO_ENGINE);
#endif
}

/**
 * To be called by each implementation to prepare the
 * structures before processing.
 *
 * Clears buffers, marks all as unprocessed, etc.
 */
void
engine_process_prepare (uint32_t nframes)
{
  int i, j;
  AUDIO_ENGINE->last_time_taken =
    g_get_monotonic_time ();
  AUDIO_ENGINE->nframes = nframes;

  if (TRANSPORT->play_state == PLAYSTATE_PAUSE_REQUESTED)
    {
      g_message ("pause requested handled");
      /*TRANSPORT->play_state = PLAYSTATE_PAUSED;*/
      /*zix_sem_post (&TRANSPORT->paused);*/
    }
  else if (TRANSPORT->play_state == PLAYSTATE_ROLL_REQUESTED)
    {
      TRANSPORT->play_state = PLAYSTATE_ROLLING;
    }

  int ret =
    zix_sem_try_wait (
      &AUDIO_ENGINE->port_operation_lock);
  if (!ret)
    {
      AUDIO_ENGINE->skip_cycle = 1;
      return;
    }

  /* reset all buffers */
  port_clear_buffer (AUDIO_ENGINE->midi_in);

  /* prepare channels for this cycle */
  for (i = -1; i < MIXER->num_channels; i++)
    {
      Channel * channel;
      if (i == -1)
        channel = MIXER->master;
      else
        channel = MIXER->channels[i];
      /*for (int j = 0; j < STRIP_SIZE; j++)*/
        /*{*/
          /*if (channel->plugins[j])*/
            /*{*/
              /*g_atomic_int_set (*/
                /*&channel->plugins[j]->processed, 0);*/
            /*}*/
        /*}*/
      /*g_atomic_int_set (*/
        /*&channel->processed, 0);*/
      channel_prepare_process (channel);
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
      for (j = 0;
           j < atl->num_automation_tracks;
           j++)
        {
          at = atl->automation_tracks[j];
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
      ((TRANSPORT->playhead_pos.frames + self->nframes) > /* if next pos will be outside loop */
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
  if (AUDIO_ENGINE->backend == ENGINE_BACKEND_JACK)
    jack_tear_down ();
#endif

  /* TODO free data */
}
