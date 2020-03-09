/*
 * Copyright (C) 2018-2020 Alexandros Theodotou <alex at zrythm dot org>
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
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * Copyright (C) 1999-2002 Paul Davis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
#include "audio/control_port.h"
#include "audio/engine.h"
#include "audio/engine_alsa.h"
#include "audio/engine_dummy.h"
#include "audio/engine_jack.h"
#include "audio/engine_pa.h"
#include "audio/engine_rtaudio.h"
#include "audio/engine_rtmidi.h"
#include "audio/engine_sdl.h"
#include "audio/engine_windows_mme.h"
#include "audio/metronome.h"
#include "audio/midi.h"
#include "audio/midi_mapping.h"
#include "audio/mixer.h"
#include "audio/pool.h"
#include "audio/routing.h"
#include "audio/sample_playback.h"
#include "audio/sample_processor.h"
#include "audio/transport.h"
#include "gui/widgets/main_window.h"
#include "plugins/plugin.h"
#include "plugins/plugin_manager.h"
#include "plugins/lv2_plugin.h"
#include "project.h"
#include "utils/ui.h"
#include "zrythm.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#ifdef HAVE_JACK
#include <jack/jack.h>
#include <jack/midiport.h>
#endif

/**
 * Updates frames per tick based on the time sig,
 * the BPM, and the sample rate
 */
void
engine_update_frames_per_tick (
  AudioEngine *       self,
  const int           beats_per_bar,
  const bpm_t         bpm,
  const sample_rate_t sample_rate)
{
  self->frames_per_tick =
    (((double) sample_rate * 60.0 *
       (double) beats_per_bar) /
    ((double) bpm *
       (double) TRANSPORT->ticks_per_bar));

  /* update positions */
  transport_update_position_frames (
    &self->transport);

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

  if (loading)
    {
    }
  else
    {
      /* init midi editor manual press */
      self->midi_editor_manual_press =
        port_new_with_type (
          TYPE_EVENT,
          FLOW_INPUT,
          "MIDI Editor Manual Press");
      self->midi_editor_manual_press->id.flags |=
        PORT_FLAG_MANUAL_PRESS;

      /* init midi in */
      self->midi_in =
        port_new_with_type (
          TYPE_EVENT,
          FLOW_INPUT,
          "MIDI in");
    }

  /* init MIDI queues */
  self->midi_editor_manual_press->midi_events =
    midi_events_new (
      self->midi_editor_manual_press);
  self->midi_in->midi_events =
    midi_events_new (self->midi_in);

  /* Expose midi in */
  port_set_expose_to_backend (
    self->midi_in, 1);
}

static void
init_audio (
  AudioEngine * self,
  int           loading)
{
  g_message ("initializing audio...");

  control_room_init (CONTROL_ROOM, loading);

#ifdef TRIAL_VER
  self->zrythm_start_time =
    g_get_monotonic_time ();
#endif

  /*if (loading)*/
    /*{*/
      /*stereo_ports_init_loaded (self->stereo_in);*/
      /*stereo_ports_init_loaded (self->monitor_out);*/
    /*}*/
  /*else*/
    /*{*/
    /*}*/
  g_message ("Finished initializing audio");
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

  transport_init (
    &self->transport, loading);

  /* get audio backend */
  AudioBackend ab_code =
    ZRYTHM_TESTING ?
      AUDIO_BACKEND_DUMMY :
      (AudioBackend)
      g_settings_get_enum (
        S_PREFERENCES,
        "audio-backend");

  /* use ifdef's so that dummy is used if the
   * selected backend isn't available */
  switch (ab_code)
    {
    case AUDIO_BACKEND_DUMMY:
      self->audio_backend = AUDIO_BACKEND_DUMMY;
      break;
#ifdef HAVE_JACK
    case AUDIO_BACKEND_JACK:
      self->audio_backend = AUDIO_BACKEND_JACK;
      break;
#endif
#ifdef HAVE_ALSA
    case AUDIO_BACKEND_ALSA:
      self->audio_backend = AUDIO_BACKEND_ALSA;
      break;
#endif
#ifdef HAVE_PORT_AUDIO
    case AUDIO_BACKEND_PORT_AUDIO:
      self->audio_backend =
        AUDIO_BACKEND_PORT_AUDIO;
      break;
#endif
#ifdef HAVE_SDL
    case AUDIO_BACKEND_SDL:
      self->audio_backend = AUDIO_BACKEND_SDL;
      break;
#endif
#ifdef HAVE_RTAUDIO
    case AUDIO_BACKEND_RTAUDIO:
      self->audio_backend = AUDIO_BACKEND_RTAUDIO;
      break;
#endif
    default:
      self->audio_backend = AUDIO_BACKEND_DUMMY;
      g_warn_if_reached ();
      break;
    }

  /* get midi backend */
  MidiBackend mb_code =
    ZRYTHM_TESTING ?
      MIDI_BACKEND_DUMMY :
      (MidiBackend)
      g_settings_get_enum (
        S_PREFERENCES,
        "midi-backend");
  switch (mb_code)
    {
    case MIDI_BACKEND_DUMMY:
      self->midi_backend = MIDI_BACKEND_DUMMY;
      break;
#ifdef HAVE_ALSA
    case MIDI_BACKEND_ALSA:
      self->midi_backend = MIDI_BACKEND_ALSA;
      break;
#endif
#ifdef HAVE_JACK
    case MIDI_BACKEND_JACK:
      self->midi_backend = MIDI_BACKEND_JACK;
      break;
#endif
#ifdef _WOE32
    case MIDI_BACKEND_WINDOWS_MME:
      self->midi_backend = MIDI_BACKEND_WINDOWS_MME;
      break;
#endif
#ifdef HAVE_RTMIDI
    case MIDI_BACKEND_RTMIDI:
      self->midi_backend = MIDI_BACKEND_RTMIDI;
      break;
#endif
    default:
      self->midi_backend = MIDI_BACKEND_DUMMY;
      g_warn_if_reached ();
      break;
    }

  self->pan_law =
    ZRYTHM_TESTING ?
      PAN_LAW_MINUS_3DB :
      (PanLaw)
      g_settings_get_enum (
        S_PREFERENCES,
        "pan-law");
  self->pan_algo =
    ZRYTHM_TESTING ?
      PAN_ALGORITHM_SINE_LAW :
      (PanAlgorithm)
      g_settings_get_enum (
        S_PREFERENCES,
        "pan-algo");

  int ret = 0;
  switch (self->audio_backend)
    {
    case AUDIO_BACKEND_DUMMY:
      ret =
        engine_dummy_setup (self, loading);
      break;
#ifdef HAVE_ALSA
    case AUDIO_BACKEND_ALSA:
      ret =
        engine_alsa_setup(self, loading);
      break;
#endif
#ifdef HAVE_JACK
    case AUDIO_BACKEND_JACK:
      ret =
        engine_jack_setup (self, loading);
      break;
#endif
#ifdef HAVE_PORT_AUDIO
    case AUDIO_BACKEND_PORT_AUDIO:
      ret =
        engine_pa_setup (self, loading);
      break;
#endif
#ifdef HAVE_SDL
    case AUDIO_BACKEND_SDL:
      ret =
        engine_sdl_setup (self, loading);
      break;
#endif
#ifdef HAVE_RTAUDIO
    case AUDIO_BACKEND_RTAUDIO:
      ret =
        engine_rtaudio_setup (self, loading);
      break;
#endif
    default:
      g_warn_if_reached ();
      break;
    }
  if (ret)
    {
      if (!ZRYTHM_TESTING)
        {
          char str[600];
          sprintf (
            str,
            _("Failed to initialize the %s audio "
              "backend. Will use the dummy backend "
              "instead. Please check your backend "
              "settings in the Preferences."),
            engine_audio_backend_to_string (
              self->audio_backend));
          ui_show_message_full (
            GTK_WINDOW (MAIN_WINDOW),
            GTK_MESSAGE_WARNING, str);
        }

      self->audio_backend =
        AUDIO_BACKEND_DUMMY;
      self->midi_backend =
        MIDI_BACKEND_DUMMY;
      engine_dummy_setup (self, loading);
    }

  /* init semaphores */
  zix_sem_init (&self->port_operation_lock, 1);

  if (loading)
    {
      /* load */
      mixer_init_loaded ();
    }

  /* init audio */
  init_audio (self, loading);

  /* connect fader to monitor out */
  g_return_if_fail (
    MONITOR_FADER &&
    MONITOR_FADER->stereo_out &&
    MONITOR_FADER->stereo_out->l &&
    MONITOR_FADER->stereo_out->r &&
    self->monitor_out &&
    self->monitor_out->l &&
    self->monitor_out->r);
  stereo_ports_connect (
    MONITOR_FADER->stereo_out,
    self->monitor_out, 1);

  /* set up midi */
  int mret = 0;
  switch (self->midi_backend)
    {
    case MIDI_BACKEND_DUMMY:
      mret =
        engine_dummy_midi_setup (self, loading);
      break;
#ifdef HAVE_ALSA
    case MIDI_BACKEND_ALSA:
      mret =
        engine_alsa_midi_setup (self, loading);
      break;
#endif
#ifdef HAVE_JACK
    case MIDI_BACKEND_JACK:
      mret =
        engine_jack_midi_setup (self, loading);
      break;
#endif
#ifdef _WOE32
    case MIDI_BACKEND_WINDOWS_MME:
      mret =
        engine_windows_mme_setup (self, loading);
      break;
#endif
#ifdef HAVE_RTMIDI
    case MIDI_BACKEND_RTMIDI:
      mret =
        engine_rtmidi_setup (self, loading);
      break;
#endif
    default:
      g_warn_if_reached ();
      break;
    }
  if (mret)
    {
      if (!ZRYTHM_TESTING)
        {
          char * str =
            g_strdup_printf (
              _("Failed to initialize the %s MIDI "
                "backend. Will use the dummy backend "
                "instead. Please check your backend "
                "settings in the Preferences."),
              engine_midi_backend_to_string (
                self->midi_backend));
          ui_show_message_full (
            GTK_WINDOW (MAIN_WINDOW),
            GTK_MESSAGE_WARNING, str);
          g_free (str);
        }

      self->midi_backend =
        MIDI_BACKEND_DUMMY;
      engine_dummy_midi_setup (self, loading);
    }

  /* init midi */
  init_midi (self, loading);

  self->buf_size_set = false;

  metronome_init (METRONOME);
  sample_processor_init (&self->sample_processor);

  /* connect the sample processor to the engine
   * output */
  stereo_ports_connect (
    self->sample_processor.stereo_out,
    self->control_room.monitor_fader.stereo_in, 1);

  /** init audio pool */
  if (loading)
    {
      audio_pool_init_loaded (self->pool);
    }
  else
    {
      self->pool = audio_pool_new ();
    }

  engine_realloc_port_buffers (
    self, self->block_length);
  engine_update_frames_per_tick (
    self,
    TRANSPORT->beats_per_bar,
    TRANSPORT->bpm,
    self->sample_rate);
}

/**
 * Activates the audio engine to start receiving
 * events.
 */
void
engine_activate (
  AudioEngine * self)
{
  g_message ("activating engine...");

#ifdef HAVE_JACK
  if (self->audio_backend == AUDIO_BACKEND_JACK &&
      self->midi_backend == MIDI_BACKEND_JACK)
    engine_jack_activate (self);
#endif
  if (self->audio_backend == AUDIO_BACKEND_DUMMY)
    {
      engine_dummy_activate (self);
    }
#ifdef _WOE32
  if (self->midi_backend ==
        MIDI_BACKEND_WINDOWS_MME)
    {
      engine_windows_mme_start_known_devices (self);
    }
#endif
#ifdef HAVE_RTMIDI
  if (self->midi_backend ==
        MIDI_BACKEND_RTMIDI)
    {
      engine_rtmidi_activate (self);
    }
#endif
#ifdef HAVE_SDL
  if (self->audio_backend == AUDIO_BACKEND_SDL)
    engine_sdl_activate (self);
#endif
#ifdef HAVE_RTAUDIO
  if (self->audio_backend == AUDIO_BACKEND_RTAUDIO)
    engine_rtaudio_activate (self);
#endif
}

void
engine_realloc_port_buffers (
  AudioEngine * self,
  nframes_t     nframes)
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
                  lv2_plugin_allocate_port_buffers (
                    pl->lv2);
                }
            }
        }
    }
  AUDIO_ENGINE->nframes = nframes;
}

/*void*/
/*audio_engine_close (*/
  /*AudioEngine * self)*/
/*{*/
  /*g_message ("closing audio engine...");*/

  /*switch (self->audio_backend)*/
    /*{*/
    /*case AUDIO_BACKEND_DUMMY:*/
      /*break;*/
/*#ifdef HAVE_JACK*/
    /*case AUDIO_BACKEND_JACK:*/
      /*jack_client_close (AUDIO_ENGINE->client);*/
      /*break;*/
/*#endif*/
/*#ifdef HAVE_PORT_AUDIO*/
    /*case AUDIO_BACKEND_PORT_AUDIO:*/
      /*pa_terminate (AUDIO_ENGINE);*/
      /*break;*/
/*#endif*/
    /*case NUM_AUDIO_BACKENDS:*/
    /*default:*/
      /*g_warn_if_reached ();*/
      /*break;*/
    /*}*/
/*}*/

/**
 * Clears the underlying backend's output buffers.
 */
static void
clear_output_buffers (
  AudioEngine * self,
  nframes_t     nframes)
{
  switch (self->audio_backend)
    {
    case AUDIO_BACKEND_JACK:
      /* nothing special needed, already handled
       * by clearing the monitor outs */
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
#ifdef HAVE_JACK
      if (self->audio_backend == AUDIO_BACKEND_JACK)
        jack_transport_stop (
          self->client);
#endif
    }
  else if (TRANSPORT->play_state ==
           PLAYSTATE_ROLL_REQUESTED)
    {
      TRANSPORT->play_state = PLAYSTATE_ROLLING;
      self->remaining_latency_preroll =
        router_get_max_playback_latency (
          &MIXER->router);
#ifdef HAVE_JACK
      if (self->audio_backend == AUDIO_BACKEND_JACK)
        jack_transport_start (
          self->client);
#endif
    }

  switch (self->audio_backend)
    {
#ifdef HAVE_JACK
    case AUDIO_BACKEND_JACK:
      engine_jack_prepare_process (self);
      break;
#endif
#ifdef HAVE_ALSA
    case AUDIO_BACKEND_ALSA:
      engine_alsa_prepare_process (self);
      break;
#endif
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
  fader_clear_buffers (MONITOR_FADER);
  port_clear_buffer (self->midi_in);
  port_clear_buffer (
    self->midi_editor_manual_press);
  port_clear_buffer (self->monitor_out->l);
  port_clear_buffer (self->monitor_out->r);

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
          /* FIXME this is the most processor hungry
           * part of zrythm */
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
              Port * port =
                automation_track_get_port (at);
              control_port_set_val_from_normalized (
                port, val, 1);
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
#ifdef HAVE_ALSA
    case MIDI_BACKEND_ALSA:
      /*engine_alsa_receive_midi_events (*/
        /*self, print);*/
      break;
#endif
    default:
      break;
    }

  MidiEvents * events =
    self->midi_in->midi_events;
  if (events->num_events > 0)
    {
      self->trigger_midi_activity = 1;

      /* capture cc if capturing */
      if (self->capture_cc)
        {
          memcpy (
            self->last_cc,
            events->events[
              events->num_events - 1].raw_buffer,
            sizeof (midi_byte_t) * 3);
        }

      /* send cc to mapped ports */
      for (int i = 0; i < events->num_events; i++)
        {
          MidiEvent * ev = &events->events[i];
          midi_mappings_apply (
            MIDI_MAPPINGS, ev->raw_buffer);
        }
    }
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
  const nframes_t  loffset)
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
  nframes_t bar_offset;
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
        (nframes_t)
        (bar_pos.frames - start_pos->frames);
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
  nframes_t beat_offset;
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
          /* adjust position because even though
           * the start and beat pos have the same
           * ticks, their frames differ (the beat
           * position might be before the start
           * position in frames) */
          if (beat_pos.frames < start_pos->frames)
            beat_pos.frames = start_pos->frames;

          beat_offset =
            (nframes_t)
            (beat_pos.frames - start_pos->frames);
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
 AudioEngine *   self,
 const nframes_t loffset,
 const nframes_t nframes)
{
  Position pos, bar_pos, beat_pos,
           unlooped_playhead;
  position_init (&bar_pos);
  position_init (&beat_pos);
  position_set_to_pos (&pos, PLAYHEAD);
  position_set_to_pos (
    &unlooped_playhead, PLAYHEAD);
  transport_position_add_frames (
    TRANSPORT, &pos, nframes);
  position_add_frames (
    &unlooped_playhead, (long) nframes);
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
          (nframes_t)
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
  nframes_t     _nframes)
{
  /* calculate timestamps (used for synchronizing
   * external events like Windows MME MIDI) */
  self->timestamp_start =
    g_get_monotonic_time ();
  self->timestamp_end =
    self->timestamp_start +
    (_nframes * 1000000) / self->sample_rate;

  /* Clear output buffers just in case we have to
   * return early */
  clear_output_buffers (self, _nframes);

  if (!g_atomic_int_get (&self->run))
    {
      /*g_message ("ENGINE NOT RUNNING");*/
      return 0;
    }

  /*count++;*/
  /*self->cycle = count;*/

  /* run pre-process code */
  engine_process_prepare (self, _nframes);

  if (AUDIO_ENGINE->skip_cycle)
    {
      AUDIO_ENGINE->skip_cycle = 0;
      return 0;
    }

  /* puts MIDI in events in the MIDI in port */
  receive_midi_events (self, _nframes, 1);

  nframes_t route_latency = 0;
  GraphNode * start_node;
  size_t i;
  nframes_t num_samples;
  nframes_t nframes = _nframes;
  while (self->remaining_latency_preroll > 0)
    {
      num_samples =
        MIN (
          _nframes, self->remaining_latency_preroll);
      for (i = 0;
           i < MIXER->router.graph->
             n_init_triggers;
           i++)
        {
          start_node =
            MIXER->router.graph->
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
          TRANSPORT_IS_ROLLING)
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
  engine_post_process (self, nframes);

#ifdef TRIAL_VER
  /* go silent if limit reached */
  if (self->timestamp_start -
        self->zrythm_start_time > 600000000)
    {
      if (!self->limit_reached)
        {
          EVENTS_PUSH (
            ET_TRIAL_LIMIT_REACHED, NULL);
          self->limit_reached = 1;
        }
    }
#endif

  /*
   * processing finished, return 0 (OK)
   */
  return 0;
}

/**
 * To be called after processing for common logic.
 */
void
engine_post_process (
  AudioEngine * self,
  const nframes_t nframes)
{
  zix_sem_post (&self->port_operation_lock);

  if (!self->exporting)
    {
      /* fill in the external buffers */
      engine_fill_out_bufs (
        self, nframes);
    }

  /* stop panicking */
  if (self->panic)
    {
      self->panic = 0;
    }

  /* move the playhead if rolling and not
   * pre-rolling */
  if (TRANSPORT_IS_ROLLING &&
      self->remaining_latency_preroll == 0)
    {
      transport_add_to_playhead (
        TRANSPORT, nframes);
#ifdef HAVE_JACK
      if (self->audio_backend ==
            AUDIO_BACKEND_JACK &&
          self->transport_type ==
            AUDIO_ENGINE_JACK_TIMEBASE_MASTER)
        {
          jack_transport_locate (
            self->client,
            (jack_nframes_t) PLAYHEAD->frames);
        }
#endif
    }

  /* update max time taken (for calculating DSP
   * %) */
  AUDIO_ENGINE->last_time_taken =
    g_get_monotonic_time () -
    AUDIO_ENGINE->last_time_taken;
  if (AUDIO_ENGINE->max_time_taken <
      AUDIO_ENGINE->last_time_taken)
    AUDIO_ENGINE->max_time_taken =
      AUDIO_ENGINE->last_time_taken;
}

/**
 * Called to fill in the external output buffers at
 * the end of the processing cycle.
 */
void
engine_fill_out_bufs (
  AudioEngine *   self,
  const nframes_t nframes)
{
  switch (self->audio_backend)
    {
    case AUDIO_BACKEND_DUMMY:
      break;
#ifdef HAVE_ALSA
    case AUDIO_BACKEND_ALSA:
      engine_alsa_fill_out_bufs (self, nframes);
      break;
#endif
#ifdef HAVE_JACK
    case AUDIO_BACKEND_JACK:
      engine_jack_fill_out_bufs (self, nframes);
      break;
#endif
#ifdef HAVE_PORT_AUDIO
    case AUDIO_BACKEND_PORT_AUDIO:
      engine_pa_fill_out_bufs (self, nframes);
      break;
#endif
#ifdef HAVE_SDL
    case AUDIO_BACKEND_SDL:
      /*engine_sdl_fill_out_bufs (self, nframes);*/
      break;
#endif
#ifdef HAVE_RTAUDIO
    case AUDIO_BACKEND_RTAUDIO:
      break;
#endif
    default:
      break;
    }
}

/**
 * Returns the int value correesponding to the
 * given AudioEngineBufferSize.
 */
int
engine_buffer_size_enum_to_int (
  AudioEngineBufferSize buffer_size)
{
  switch (buffer_size)
    {
    case AUDIO_ENGINE_BUFFER_SIZE_16:
      return 16;
    case AUDIO_ENGINE_BUFFER_SIZE_32:
      return 32;
    case AUDIO_ENGINE_BUFFER_SIZE_64:
      return 64;
    case AUDIO_ENGINE_BUFFER_SIZE_128:
      return 128;
    case AUDIO_ENGINE_BUFFER_SIZE_256:
      return 256;
    case AUDIO_ENGINE_BUFFER_SIZE_512:
      return 512;
    case AUDIO_ENGINE_BUFFER_SIZE_1024:
      return 1024;
    case AUDIO_ENGINE_BUFFER_SIZE_2048:
      return 2048;
    case AUDIO_ENGINE_BUFFER_SIZE_4096:
      return 4096;
    default:
      break;
    }
  g_return_val_if_reached (-1);
}

/**
 * Returns the int value correesponding to the
 * given AudioEngineSamplerate.
 */
int
engine_samplerate_enum_to_int (
  AudioEngineSamplerate samplerate)
{
  switch (samplerate)
    {
    case AUDIO_ENGINE_SAMPLERATE_22050:
      return 22050;
    case AUDIO_ENGINE_SAMPLERATE_32000:
      return 32000;
    case AUDIO_ENGINE_SAMPLERATE_44100:
      return 44100;
    case AUDIO_ENGINE_SAMPLERATE_48000:
      return 48000;
    case AUDIO_ENGINE_SAMPLERATE_88200:
      return 88200;
    case AUDIO_ENGINE_SAMPLERATE_96000:
      return 96000;
    case AUDIO_ENGINE_SAMPLERATE_192000:
      return 192000;
    default:
      break;
    }
  g_return_val_if_reached (-1);
}

/**
 * Closes any connections and free's data.
 */
void
engine_tear_down (
  AudioEngine * self)
{
  g_message ("tearing down audio engine...");

#ifdef HAVE_JACK
  if (self->audio_backend ==
        AUDIO_BACKEND_JACK)
    engine_jack_tear_down (self);
#endif
  if (self->audio_backend ==
        AUDIO_BACKEND_DUMMY)
    {
      engine_dummy_tear_down (self);
    }

  /* TODO free data */
}
