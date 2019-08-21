/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
 * Copyright (C) 1999-2002 Paul Davis
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
#include "audio/metronome.h"
#include "audio/midi.h"
#include "audio/mixer.h"
#include "audio/routing.h"
#include "audio/sample_playback.h"
#include "audio/sample_processor.h"
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
  transport_update_position_frames (
    &AUDIO_ENGINE->transport);

  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      track_update_frames (TRACKLIST->tracks[i]);
    }
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
      self->midi_editor_manual_press->
        identifier.flags |=
          PORT_FLAG_MANUAL_PRESS;
    }

  /* init MIDI queues for manual press */
  self->midi_editor_manual_press->midi_events =
    midi_events_new (
      self->midi_editor_manual_press);
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
engine_init (
  AudioEngine * self,
  int           loading)
{
  g_message ("Initializing audio engine...");

  transport_init (&self->transport,
                  loading);
  sample_processor_init (SAMPLE_PROCESSOR);

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
      alsa_midi_setup (self, loading);
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

  metronome_init (METRONOME);

  /* connect the sample processor to the engine
   * output */
  port_connect (
    SAMPLE_PROCESSOR->stereo_out->l,
    self->stereo_out->l, 1);
  port_connect (
    SAMPLE_PROCESSOR->stereo_out->r,
    self->stereo_out->r, 1);
}

void
engine_realloc_port_buffers (
  AudioEngine * self,
  int           nframes)
{
  int i, j;

  AUDIO_ENGINE->block_length = nframes;
  AUDIO_ENGINE->buf_size_set = true;
  g_message (
    "Block length changed to %d",
    AUDIO_ENGINE->block_length);

  /** reallocate port buffers to new size */
  Port * port;
  Channel * ch;
  Plugin * pl;
  Port * ports[60000];
  int num_ports;
  port_get_all (ports, &num_ports);
  for (i = 0; i < num_ports; i++)
    {
      port = ports[i];
      g_warn_if_fail (port);

      port->buf =
        realloc (port->buf,
                 nframes * sizeof (float));
      /* TODO memset */
    }
  for (i = 0; i < TRACKLIST->num_tracks; i++)
    {
      ch = TRACKLIST->tracks[i]->channel;

      if (!ch)
        continue;

      for (j = 0; j < STRIP_SIZE; j++)
        {
          if (ch->plugins[j])
            {
              pl = ch->plugins[j];
              if (pl->descr->protocol == PROT_LV2)
                {
                  lv2_allocate_port_buffers (
                    (Lv2Plugin *)pl->lv2);
                }
            }
        }
    }
  AUDIO_ENGINE->nframes = nframes;
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
      g_message ("pause requested handled");
      TRANSPORT->play_state = PLAYSTATE_PAUSED;
      /*zix_sem_post (&TRANSPORT->paused);*/
      if (self->audio_backend == AUDIO_BACKEND_JACK)
        jack_transport_stop (
          self->client);
    }
  else if (TRANSPORT->play_state ==
           PLAYSTATE_ROLL_REQUESTED)
    {
      TRANSPORT->play_state = PLAYSTATE_ROLLING;
      self->remaining_latency_preroll =
        router_get_max_playback_latency (
          &MIXER->router);
      if (self->audio_backend == AUDIO_BACKEND_JACK)
        jack_transport_start (
          self->client);
    }

  switch (self->audio_backend)
    {
    case AUDIO_BACKEND_JACK:
#ifdef HAVE_JACK
      engine_jack_prepare_process (self);
#endif
      break;
    case AUDIO_BACKEND_ALSA:
      engine_alsa_prepare_process (self);
      break;
    default:
      break;
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
  port_clear_buffer (self->midi_out);
  port_clear_buffer (self->midi_editor_manual_press);

  sample_processor_prepare_process (
    &self->sample_processor, nframes);

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
          /* FIXME passing playhead doesn't take
           * into account the latency compensation.
           * maybe automation should be a port/
           * processor in the signal chain. */
          val =
            automation_track_get_normalized_val_at_pos (
              at, PLAYHEAD);
          /*g_message ("val received %f",*/
                     /*val);*/
          /* if there was an automation event
           * at the playhead position, val is
           * positive */
          if (val >= 0.f)
            {
              automatable_set_val_from_normalized (
                at->automatable,
                val, 1);
            }
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
      port_receive_midi_events_from_jack (
        self->midi_in, 0, nframes);
      break;
#endif
#ifdef __linux__
    case MIDI_BACKEND_ALSA:
      engine_alsa_receive_midi_events (
        self, print);
      break;
#endif
    default:
      break;
    }

  if (self->midi_in->midi_events->num_events > 0)
    self->trigger_midi_activity = 1;
}

/**
 * Finds all metronome events (beat and bar changes)
 * within the given range and adds them to the
 * queue of the sample processor.
 *
 * @param loffset Local offset.
 */
static void
find_and_queue_metronome (
  const Position * start_pos,
  const Position * end_pos,
  const int        loffset)
{
  /* find each bar / beat change from start
   * to finish */
  int num_bars_before =
    position_get_total_bars (start_pos);
  int num_bars_after =
    position_get_total_bars (end_pos);
  if (end_pos->beats == 1 &&
      end_pos->sixteenths == 1 &&
      end_pos->ticks == 0)
    num_bars_after--;


  Position bar_pos;
  int bar_offset;
  for (int i =
         start_pos->beats == 1 &&
         start_pos->sixteenths == 1 &&
         start_pos->ticks == 0 ?
         num_bars_before :
         num_bars_before + 1;
       i <= num_bars_after;
       i++)
    {
      position_set_to_bar (
        &bar_pos, i + 1);
      bar_offset =
        bar_pos.frames - start_pos->frames;
      sample_processor_queue_metronome (
        SAMPLE_PROCESSOR,
        METRONOME_TYPE_EMPHASIS,
        bar_offset + loffset);
    }

  int num_beats_before =
    position_get_total_beats (start_pos);
  int num_beats_after =
    position_get_total_beats (end_pos);
  if (end_pos->sixteenths == 1 &&
      end_pos->ticks == 0)
    num_beats_after--;

  Position beat_pos;
  int beat_offset;
  for (int i =
         start_pos->sixteenths == 1 &&
         start_pos->ticks == 0 ?
         num_beats_before :
         num_beats_before + 1;
       i <= num_beats_after;
       i++)
    {
      position_set_to_bar (
        &beat_pos,
        i / TRANSPORT->beats_per_bar);
      position_set_beat (
        &beat_pos,
        i % TRANSPORT->beats_per_bar + 1);
      if (beat_pos.beats != 1)
        {
          beat_offset =
            beat_pos.frames - start_pos->frames;
          sample_processor_queue_metronome (
            SAMPLE_PROCESSOR,
            METRONOME_TYPE_NORMAL,
            beat_offset + loffset);
        }
    }
}

/**
 * Queues metronome events.
 *
 * @param loffset Local offset in this cycle.
 */
static void
queue_metronome_events (
 AudioEngine * self,
 const int     loffset,
 const int     nframes)
{
  Position pos, bar_pos, beat_pos, unlooped_playhead;
  position_init (&bar_pos);
  position_init (&beat_pos);
  position_set_to_pos (&pos, PLAYHEAD);
  position_set_to_pos (&unlooped_playhead, PLAYHEAD);
  transport_position_add_frames (
    TRANSPORT, &pos, nframes);
  position_add_frames (&unlooped_playhead, nframes);
  int loop_crossed =
    unlooped_playhead.frames !=
    pos.frames;
  if (loop_crossed)
    {
      /* find each bar / beat change until loop
       * end */
      find_and_queue_metronome (
        PLAYHEAD, &TRANSPORT->loop_end_pos,
        loffset);

      /* find each bar / beat change after loop
       * start */
      find_and_queue_metronome (
        &TRANSPORT->loop_start_pos, &pos,
        loffset +
          (TRANSPORT->loop_end_pos.frames -
           PLAYHEAD->frames));
    }
  else /* loop not crossed */
    {
      /* find each bar / beat change from start
       * to finish */
      find_and_queue_metronome (
        PLAYHEAD, &pos,
        loffset);
    }
}

/*static long count = 0;*/
/**
 * Processes current cycle.
 *
 * To be called by each implementation in its
 * callback.
 */
int
engine_process (
  AudioEngine * self,
  uint32_t      _nframes)
{
  /* Clear output buffers just in case we have to
   * return early */
  clear_output_buffers (self);

  if (!g_atomic_int_get (&self->run))
    {
      g_message ("ENGINE NOT RUNNING");
      return 0;
    }

  /*count++;*/
  /*self->cycle = count;*/

  uint32_t nframes = _nframes;

  /* run pre-process code */
  engine_process_prepare (self, nframes);

  if (AUDIO_ENGINE->skip_cycle)
    {
      AUDIO_ENGINE->skip_cycle = 0;
      return 0;
    }

  /* puts MIDI in events in the MIDI in port */
  receive_midi_events (self, nframes, 1);

  long route_latency = 0;
  GraphNode * start_node;
  int i, num_samples;
  while (self->remaining_latency_preroll > 0)
    {
      num_samples =
        MIN (
          nframes, self->remaining_latency_preroll);
      for (i = 0;
           i < MIXER->router.graph2->
             n_init_triggers;
           i++)
        {
          start_node =
            MIXER->router.graph2->
              init_trigger_list[i];
          route_latency =
            start_node->playback_latency;

          if (self->remaining_latency_preroll >
              route_latency + num_samples)
            {
              /* this route will no-roll for the
               * complete pre-roll cycle */
              continue;
            }

          if (self->remaining_latency_preroll >
              route_latency)
            {
              /* route may need partial no-roll
               * and partial roll from
               * (transport_sample -
               *  remaining_latency_preroll) .. +
               * num_samples.
               * shorten and split the process
               * cycle */
              num_samples =
                MIN (
                  num_samples,
                  self->remaining_latency_preroll -
                    route_latency);
            }
          else
            {
              /* route will do a normal roll for the
               * complete pre-roll cycle */
            }
        } // end foreach trigger node


      /* this will keep looping until everything was
       * processed in this cycle */
      /*mixer_process (nframes);*/
      /*g_message (*/
        /*"======== processing at %d for %d samples "*/
        /*"(preroll: %ld)",*/
        /*_nframes - nframes,*/
        /*num_samples,*/
        /*self->remaining_latency_preroll);*/
      router_start_cycle (
        &MIXER->router, num_samples,
        _nframes - nframes,
        PLAYHEAD);

      self->remaining_latency_preroll -=
        num_samples;
      nframes -= num_samples;

      if (nframes == 0)
        break;
    }

  if (nframes > 0)
    {
      /* queue metronome if met within this cycle */
      if (TRANSPORT->metronome_enabled &&
          IS_TRANSPORT_ROLLING)
        {
          queue_metronome_events (
            self, _nframes - nframes, nframes);
        }

      /*g_message (*/
        /*"======== processing at %d for %d samples "*/
        /*"(preroll: %ld)",*/
        /*_nframes - nframes,*/
        /*nframes,*/
        /*self->remaining_latency_preroll);*/
      router_start_cycle (
        &MIXER->router, nframes,
        _nframes - nframes,
        PLAYHEAD);
    }
  /*g_message ("end====================");*/

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

  /* move the playhead if rolling and not
   * pre-rolling */
  if (IS_TRANSPORT_ROLLING &&
      self->remaining_latency_preroll == 0)
    {
      transport_add_to_playhead (
        TRANSPORT, self->nframes);
      if (self->audio_backend ==
            AUDIO_BACKEND_JACK &&
          self->transport_type ==
            AUDIO_ENGINE_JACK_TIMEBASE_MASTER)
        {
          jack_transport_locate (
            self->client, PLAYHEAD->frames);
        }
    }

  AUDIO_ENGINE->last_time_taken =
    g_get_monotonic_time () -
    AUDIO_ENGINE->last_time_taken;
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
