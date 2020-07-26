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

#include "zrythm-config.h"

#include <math.h>
#include <stdlib.h>
#include <signal.h>

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
#include "audio/graph.h"
#include "audio/graph_node.h"
#include "audio/metronome.h"
#include "audio/midi.h"
#include "audio/midi_mapping.h"
#include "audio/pool.h"
#include "audio/router.h"
#include "audio/sample_playback.h"
#include "audio/sample_processor.h"
#include "audio/tempo_track.h"
#include "audio/transport.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/main_window.h"
#include "plugins/plugin.h"
#include "plugins/plugin_manager.h"
#include "plugins/lv2_plugin.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/objects.h"
#include "utils/ui.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#ifdef HAVE_JACK
#include "weak_libjack.h"
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
       (double) self->transport->ticks_per_bar));

  /* update positions */
  transport_update_position_frames (
    self->transport);

  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      track_update_frames (TRACKLIST->tracks[i]);
    }
}

void
engine_setup (
  AudioEngine * self)
{
  g_message ("Setting up...");

  /* init semaphores */
  zix_sem_init (&self->port_operation_lock, 1);

  g_return_if_fail (self && !self->setup);

#ifdef TRIAL_VER
  self->zrythm_start_time =
    g_get_monotonic_time ();
#endif

  int ret = 0;
  switch (self->audio_backend)
    {
    case AUDIO_BACKEND_DUMMY:
      ret =
        engine_dummy_setup (self);
      break;
#ifdef HAVE_ALSA
    case AUDIO_BACKEND_ALSA:
      ret =
        engine_alsa_setup(self);
      break;
#endif
#ifdef HAVE_JACK
    case AUDIO_BACKEND_JACK:
      ret =
        engine_jack_setup (self);
      break;
#endif
#ifdef HAVE_PORT_AUDIO
    case AUDIO_BACKEND_PORT_AUDIO:
      ret =
        engine_pa_setup (self);
      break;
#endif
#ifdef HAVE_SDL
    case AUDIO_BACKEND_SDL:
      ret =
        engine_sdl_setup (self);
      break;
#endif
#ifdef HAVE_RTAUDIO
    case AUDIO_BACKEND_RTAUDIO:
      ret =
        engine_rtaudio_setup (self);
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
      engine_dummy_setup (self);
    }

  /* set up midi */
  int mret = 0;
  switch (self->midi_backend)
    {
    case MIDI_BACKEND_DUMMY:
      mret =
        engine_dummy_midi_setup (self);
      break;
#ifdef HAVE_ALSA
    case MIDI_BACKEND_ALSA:
      mret =
        engine_alsa_midi_setup (self);
      break;
#endif
#ifdef HAVE_JACK
    case MIDI_BACKEND_JACK:
      mret =
        engine_jack_midi_setup (self);
      break;
#endif
#ifdef _WOE32
    case MIDI_BACKEND_WINDOWS_MME:
      mret =
        engine_windows_mme_setup (self);
      break;
#endif
#ifdef HAVE_RTMIDI
    case MIDI_BACKEND_RTMIDI:
      mret =
        engine_rtmidi_setup (self);
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

      self->midi_backend = MIDI_BACKEND_DUMMY;
      engine_dummy_midi_setup (self);
    }

  self->buf_size_set = false;

  /* connect the sample processor to the engine
   * output */
  stereo_ports_connect (
    self->sample_processor->stereo_out,
    self->control_room->monitor_fader->stereo_in,
    true);

  /* connect fader to monitor out */
  stereo_ports_connect (
    self->control_room->monitor_fader->stereo_out,
    self->monitor_out, true);

  self->setup = true;

  /* Expose ports */
  port_set_expose_to_backend (
    self->midi_in, true);
  port_set_expose_to_backend (
    self->monitor_out->l, true);
  port_set_expose_to_backend (
    self->monitor_out->r, true);

  g_message ("done");
}

static void
init_common (
  AudioEngine * self)
{
  self->metronome = metronome_new ();
  self->router = router_new ();

  /* get audio backend */
  AudioBackend ab_code = AUDIO_BACKEND_DUMMY;
  if (ZRYTHM_TESTING)
    {
      ab_code = AUDIO_BACKEND_DUMMY;
    }
  else if (zrythm_app->audio_backend)
    {
      ab_code =
        engine_audio_backend_from_string (
          zrythm_app->audio_backend);
    }
  else
    {
      ab_code =
        (AudioBackend)
        g_settings_get_enum (
          S_P_GENERAL_ENGINE,
          "audio-backend");
    }

  int backend_reset_to_dummy = 0;

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
      g_warning (
        "selected audio backend not found. "
        "switching to dummy");
      g_settings_set_enum (
        S_P_GENERAL_ENGINE, "audio-backend",
        AUDIO_BACKEND_DUMMY);
      backend_reset_to_dummy = 1;
      break;
    }

  /* get midi backend */
  MidiBackend mb_code = MIDI_BACKEND_DUMMY;
  if (ZRYTHM_TESTING)
    {
      mb_code = MIDI_BACKEND_DUMMY;
    }
  else if (zrythm_app->midi_backend)
    {
      mb_code =
        engine_midi_backend_from_string (
          zrythm_app->midi_backend);
    }
  else
    {
      mb_code =
        (MidiBackend)
        g_settings_get_enum (
          S_P_GENERAL_ENGINE,
          "midi-backend");
    }

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
      g_warning (
        "selected midi backend not found. "
        "switching to dummy");
      g_settings_set_enum (
        S_P_GENERAL_ENGINE, "midi-backend",
        MIDI_BACKEND_DUMMY);
      backend_reset_to_dummy = 1;
      break;
    }

  if (backend_reset_to_dummy && !ZRYTHM_TESTING)
    {
      char str[800];
      sprintf (
        str,
        _("The selected MIDI/audio backend was not "
          "found in the version of %s you have "
          "installed. The audio and MIDI backends "
          "were set to \"Dummy\". Please set your "
          "preferred backend from the "
          "preferences."),
        PROGRAM_NAME);
      ui_show_message_full (
        GTK_WINDOW (MAIN_WINDOW),
        GTK_MESSAGE_WARNING, str);
    }

  self->pan_law =
    ZRYTHM_TESTING ?
      PAN_LAW_MINUS_3DB :
      (PanLaw)
      g_settings_get_enum (
        S_P_DSP_PAN,
        "pan-law");
  self->pan_algo =
    ZRYTHM_TESTING ?
      PAN_ALGORITHM_SINE_LAW :
      (PanAlgorithm)
      g_settings_get_enum (
        S_P_DSP_PAN,
        "pan-algorithm");

  /* set a temporary buffer sizes */
  if (self->block_length == 0)
    {
      self->block_length = 8192;
    }
  if (self->midi_buf_size == 0)
    {
      self->midi_buf_size = 8192;
    }
}

void
engine_init_loaded (
  AudioEngine * self)
{
  g_message ("Initializing...");

  audio_pool_init_loaded (self->pool);
  transport_init_loaded (self->transport);
  control_room_init_loaded (self->control_room);
  sample_processor_init_loaded (
    self->sample_processor);

  init_common (self);

  g_message ("done");
}

/**
 * Create a new audio engine.
 *
 * This only initializes the engine and doe snot
 * connect to the backend.
 */
AudioEngine *
engine_new (
  Project * project)
{
  g_message ("Creating audio engine...");

  AudioEngine * self = object_new (AudioEngine);

  if (project)
    {
      project->audio_engine = self;
    }

  self->transport = transport_new (self);
  self->pool = audio_pool_new ();
  self->control_room = control_room_new ();
  self->sample_processor = sample_processor_new ();

  /* init midi editor manual press */
  self->midi_editor_manual_press =
    port_new_with_type (
      TYPE_EVENT, FLOW_INPUT,
      "MIDI Editor Manual Press");
  self->midi_editor_manual_press->id.flags |=
    PORT_FLAG_MANUAL_PRESS;

  /* init midi in */
  self->midi_in =
    port_new_with_type (
      TYPE_EVENT, FLOW_INPUT, "MIDI in");

  /* init MIDI queues */
  self->midi_editor_manual_press->midi_events =
    midi_events_new (
      self->midi_editor_manual_press);
  self->midi_in->midi_events =
    midi_events_new (self->midi_in);

  /* create monitor out ports */
  Port * monitor_out_l, * monitor_out_r;
  monitor_out_l =
    port_new_with_type (
      TYPE_AUDIO, FLOW_OUTPUT, "Monitor Out L");
  monitor_out_r =
    port_new_with_type (
      TYPE_AUDIO, FLOW_OUTPUT, "Monitor Out R");
  monitor_out_l->id.owner_type =
    PORT_OWNER_TYPE_BACKEND;
  monitor_out_r->id.owner_type =
    PORT_OWNER_TYPE_BACKEND;
  self->monitor_out =
    stereo_ports_new_from_existing (
      monitor_out_l, monitor_out_r);

  init_common (self);

  return self;
}

/**
 * Activates the audio engine to start processing
 * and receiving events.
 *
 * @param activate Activate or deactivate.
 */
void
engine_activate (
  AudioEngine * self,
  bool          activate)
{
  if (activate)
    {
      g_message ("Activating...");
    }
  else
    {
      g_message ("Deactivating...");
    }

  if (activate)
    {
      if (self->activated)
        {
          g_message ("already activated");
          return;
        }

      engine_realloc_port_buffers (
        self, self->block_length);
    }
  else
    {
      if (!self->activated)
        {
          g_message ("already deactivated");
          return;
        }

      /* wait to finish */
      g_atomic_int_set (&self->run, false);
      g_usleep (100000);

      self->activated = false;
    }

#ifdef HAVE_JACK
  if (self->audio_backend == AUDIO_BACKEND_JACK &&
      self->midi_backend == MIDI_BACKEND_JACK)
    engine_jack_activate (self, activate);
#endif
  if (self->audio_backend == AUDIO_BACKEND_DUMMY)
    {
      engine_dummy_activate (self, activate);
    }
#ifdef _WOE32
  if (self->midi_backend ==
        MIDI_BACKEND_WINDOWS_MME)
    {
      engine_windows_mme_activate (self, activate);
    }
#endif
#ifdef HAVE_RTMIDI
  if (self->midi_backend ==
        MIDI_BACKEND_RTMIDI)
    {
      engine_rtmidi_activate (self, activate);
    }
#endif
#ifdef HAVE_SDL
  if (self->audio_backend == AUDIO_BACKEND_SDL)
    engine_sdl_activate (self, activate);
#endif
#ifdef HAVE_RTAUDIO
  if (self->audio_backend == AUDIO_BACKEND_RTAUDIO)
    engine_rtaudio_activate (self, activate);
#endif

  self->activated = activate;

  g_message ("done");
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
    "Block length changed to %d. "
    "reallocating buffers...",
    AUDIO_ENGINE->block_length);

  /** reallocate port buffers to new size */
  Port * port;
  Channel * ch;
  Plugin * pl;
  int max_size = 20;
  Port ** ports =
    calloc (
      (size_t) max_size, sizeof (Port *));
  int num_ports = 0;
  port_get_all (
    &ports, &max_size, true, &num_ports);
  for (i = 0; i < num_ports; i++)
    {
      port = ports[i];
      g_warn_if_fail (port);

      port->buf =
        realloc (
          port->buf,
          nframes * sizeof (float));
      memset (
        port->buf, 0, nframes * sizeof (float));
    }
  free (ports);
  for (i = 0; i < TRACKLIST->num_tracks; i++)
    {
      ch = TRACKLIST->tracks[i]->channel;

      if (!ch)
        continue;

      for (j = 0; j < STRIP_SIZE * 2 + 1; j++)
        {
          if (j < STRIP_SIZE)
            pl = ch->midi_fx[j];
          else if (j == STRIP_SIZE)
            pl = ch->instrument;
          else
            pl = ch->inserts[j - (STRIP_SIZE + 1)];

          if (pl)
            {
              if (pl->descr->protocol == PROT_LV2 &&
                  !pl->descr->open_with_carla)
                {
                  lv2_plugin_allocate_port_buffers (
                    pl->lv2);
                }
            }
        }
    }
  AUDIO_ENGINE->nframes = nframes;

  g_message ("done");
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
  if (self->denormal_prevention_val_positive)
    {
      self->denormal_prevention_val = - 1e-20f;
    }
  else
    {
      self->denormal_prevention_val = 1e-20f;
    }
  self->denormal_prevention_val_positive =
    !self->denormal_prevention_val_positive;

  self->last_time_taken = g_get_monotonic_time ();
  self->nframes = nframes;

  if (self->transport->play_state ==
        PLAYSTATE_PAUSE_REQUESTED)
    {
      g_message ("pause requested handled");
      self->transport->play_state = PLAYSTATE_PAUSED;
      /*zix_sem_post (&TRANSPORT->paused);*/
#ifdef HAVE_JACK
      if (self->audio_backend == AUDIO_BACKEND_JACK)
        jack_transport_stop (
          self->client);
#endif
    }
  else if (self->transport->play_state ==
           PLAYSTATE_ROLL_REQUESTED)
    {
      self->transport->play_state = PLAYSTATE_ROLLING;
      self->remaining_latency_preroll =
        router_get_max_playback_latency (
          self->router);
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
    zix_sem_try_wait (&self->port_operation_lock);

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
    self->sample_processor, nframes);

  /* prepare channels for this cycle */
  Channel * ch;
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      ch = TRACKLIST->tracks[i]->channel;

      if (ch)
        channel_prepare_process (ch);
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
  nframes_t     total_frames_to_process)
{
  /* calculate timestamps (used for synchronizing
   * external events like Windows MME MIDI) */
  self->timestamp_start =
    g_get_monotonic_time ();
  self->timestamp_end =
    self->timestamp_start +
    (total_frames_to_process * 1000000) /
      self->sample_rate;

  /* Clear output buffers just in case we have to
   * return early */
  clear_output_buffers (
    self, total_frames_to_process);

  if (!g_atomic_int_get (&self->run))
    {
      /*g_message ("ENGINE NOT RUNNING");*/
      return 0;
    }

  /*count++;*/
  /*self->cycle = count;*/

  /* run pre-process code */
  engine_process_prepare (
    self, total_frames_to_process);

  if (AUDIO_ENGINE->skip_cycle)
    {
      AUDIO_ENGINE->skip_cycle = 0;
      return 0;
    }

  /* puts MIDI in events in the MIDI in port */
  receive_midi_events (
    self, total_frames_to_process, 1);

  nframes_t total_frames_remaining =
    total_frames_to_process;
  while (self->remaining_latency_preroll > 0)
    {
      nframes_t num_preroll_frames =
        MIN (
          total_frames_to_process,
          self->remaining_latency_preroll);
      for (size_t i = 0;
           i < self->router->graph->n_init_triggers;
           i++)
        {
          GraphNode * start_node =
            self->router->graph->
              init_trigger_list[i];
          nframes_t route_latency =
            start_node->playback_latency;

          if (self->remaining_latency_preroll >
                route_latency + num_preroll_frames)
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
               * num_preroll_frames.
               * shorten and split the process
               * cycle */
              num_preroll_frames =
                MIN (
                  num_preroll_frames,
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
      /*g_message (*/
        /*"======== processing at %d for %d samples "*/
        /*"(preroll: %ld)",*/
        /*total_frames_to_process - total_frames_remaining,*/
        /*num_preroll_frames,*/
        /*self->remaining_latency_preroll);*/
      router_start_cycle (
        self->router, num_preroll_frames,
        total_frames_to_process -
          total_frames_remaining,
        PLAYHEAD);

      self->remaining_latency_preroll -=
        num_preroll_frames;
      total_frames_remaining -= num_preroll_frames;

      if (total_frames_remaining == 0)
        break;
    }

  if (total_frames_remaining > 0)
    {
      /* queue metronome if met within this cycle */
      if (self->transport->metronome_enabled &&
          TRANSPORT_IS_ROLLING)
        {
          metronome_queue_events (
            self,
            total_frames_to_process -
              total_frames_remaining,
            total_frames_remaining);
        }

      /*g_message (*/
        /*"======== processing at %d for %d samples "*/
        /*"(preroll: %ld)",*/
        /*total_frames_to_process - total_frames_remaining,*/
        /*total_frames_remaining,*/
        /*self->remaining_latency_preroll);*/
      router_start_cycle (
        self->router, total_frames_remaining,
        total_frames_to_process -
          total_frames_remaining,
        PLAYHEAD);
    }
  /*g_message ("end====================");*/

  /* run post-process code */
  engine_post_process (
    self, total_frames_remaining);

#ifdef TRIAL_VER
  /* go silent if limit reached */
  if (self->timestamp_start -
        self->zrythm_start_time >
          (TRIAL_LIMIT_MINS * (gint64) 60000000))
    {
      if (!self->limit_reached)
        {
          EVENTS_PUSH (
            ET_TRIAL_LIMIT_REACHED, NULL);
          self->limit_reached = 1;
        }
    }
#endif

  /*self->cycle++;*/

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
  if (!self->exporting)
    {
      /* fill in the external buffers */
      engine_fill_out_bufs (self, nframes);
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
        self->transport, nframes);
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

  zix_sem_post (&self->port_operation_lock);
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
 * Reset the bounce mode on the engine, all tracks
 * and regions to OFF.
 */
void
engine_reset_bounce_mode (
  AudioEngine * self)
{
  self->bounce_mode = BOUNCE_OFF;

  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      Track * track = TRACKLIST->tracks[i];
      if (track->out_signal_type != TYPE_AUDIO)
        continue;

      for (int j = 0; j < track->num_lanes; j++)
        {
          TrackLane * lane = track->lanes[j];

          for (int k = 0; k < lane->num_regions; k++)
            {
              ZRegion * r = lane->regions[k];
              if (r->id.type != REGION_TYPE_MIDI)
                continue;

              r->bounce = 0;
            }
        }

      track->bounce = 0;
    }
}

void
engine_free (
  AudioEngine * self)
{
  g_message ("freeing...");

  if (self->activated)
    {
      engine_activate (self, false);
    }

  router_free (self->router);

  switch (self->audio_backend)
    {
#ifdef HAVE_JACK
    case AUDIO_BACKEND_JACK:
      engine_jack_tear_down (self);
      break;
#endif
#ifdef HAVE_RTAUDIO
    case AUDIO_BACKEND_RTAUDIO:
      engine_rtaudio_tear_down (self);
      break;
#endif
    case AUDIO_BACKEND_DUMMY:
      engine_dummy_tear_down (self);
      break;
    default:
      break;
    }

  stereo_ports_disconnect (self->monitor_out);
  stereo_ports_free (self->monitor_out);

  port_disconnect_all (self->midi_in);
  object_free_w_func_and_null (
    port_free, self->midi_in);
  port_disconnect_all (
    self->midi_editor_manual_press);
  object_free_w_func_and_null (
    port_free, self->midi_editor_manual_press);

  object_free_w_func_and_null (
    sample_processor_free, self->sample_processor);
  object_free_w_func_and_null (
    metronome_free, self->metronome);
  object_free_w_func_and_null (
    audio_pool_free, self->pool);
  object_free_w_func_and_null (
    control_room_free, self->control_room);
  object_free_w_func_and_null (
    transport_free, self->transport);

  object_zero_and_free (self);

  g_message ("done");
}
