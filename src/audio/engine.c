/*
 * Copyright (C) 2018-2022 Alexandros Theodotou <alex at zrythm dot org>
 * Copyright (C) 2020 Ryan Gonzalez <rymg19 at gmail dot com>
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
 * the Free Software Foundation, either version 2 of the License, or
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
#include "audio/engine_pulse.h"
#include "audio/engine_rtaudio.h"
#include "audio/engine_rtmidi.h"
#include "audio/engine_sdl.h"
#include "audio/engine_windows_mme.h"
#include "audio/graph.h"
#include "audio/graph_node.h"
#include "audio/hardware_processor.h"
#include "audio/metronome.h"
#include "audio/midi_event.h"
#include "audio/midi_mapping.h"
#include "audio/pool.h"
#include "audio/recording_manager.h"
#include "audio/router.h"
#include "audio/sample_playback.h"
#include "audio/sample_processor.h"
#include "audio/tempo_track.h"
#include "audio/transport.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/main_window.h"
#include "plugins/carla_native_plugin.h"
#include "plugins/plugin.h"
#include "plugins/plugin_manager.h"
#include "plugins/lv2_plugin.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/arrays.h"
#include "utils/dsp.h"
#include "utils/flags.h"
#include "utils/mpmc_queue.h"
#include "utils/object_pool.h"
#include "utils/objects.h"
#include "utils/string.h"
#include "utils/ui.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#ifdef HAVE_JACK
#include "weak_libjack.h"
#endif

/**
 * Returns the audio backend as a string.
 */
const char *
engine_audio_backend_to_string (
  AudioBackend backend)
{
  return audio_backend_str[backend];
}

/**
 * Request the backend to set the buffer size.
 *
 * The backend is expected to call the buffer size
 * change callbacks.
 *
 * @seealso jack_set_buffer_size().
 */
void
engine_set_buffer_size (
  AudioEngine * self,
  uint32_t      buf_size)
{
  g_return_if_fail (
    g_thread_self () == zrythm_app->gtk_thread);

  g_message (
    "request to set engine buffer size to %u",
    buf_size);

#ifdef HAVE_JACK
  if (self->audio_backend == AUDIO_BACKEND_JACK)
    {
      jack_set_buffer_size (
        self->client, buf_size);
      g_debug ("called jack_set_buffer_size");
    }
#endif
}

/**
 * Updates frames per tick based on the time sig,
 * the BPM, and the sample rate
 *
 * @param thread_check Whether to throw a warning
 *   if not called from GTK thread.
 * @param update_from_ticks Whether to update the
 *   positions based on ticks (true) or frames
 *   (false).
 * @param bpm_change Whether this is a BPM change.
 */
void
engine_update_frames_per_tick (
  AudioEngine *       self,
  const int           beats_per_bar,
  const bpm_t         bpm,
  const sample_rate_t sample_rate,
  bool                thread_check,
  bool                update_from_ticks,
  bool                bpm_change)
{
  if (g_thread_self () == zrythm_app->gtk_thread)
    {
      g_message (
        "updating frames per tick: "
        "beats per bar %d, "
        "bpm %f, sample rate %u",
        beats_per_bar, (double) bpm, sample_rate);
    }
  else if (thread_check)
    {
      g_critical (
        "Called %s from non-GTK thread",
        __func__);
      return;
    }

  /* process all recording events */
  recording_manager_process_events (
    RECORDING_MANAGER);

  g_return_if_fail (
    beats_per_bar > 0 && bpm > 0 &&
    sample_rate > 0 &&
    self->transport->ticks_per_bar > 0);

  g_message (
    "frames per tick before: %f | "
    "ticks per frame before: %f",
    self->frames_per_tick,
    self->ticks_per_frame);

  self->frames_per_tick =
    (((double) sample_rate * 60.0 *
       (double) beats_per_bar) /
    ((double) bpm *
       (double) self->transport->ticks_per_bar));
  self->ticks_per_frame =
    1.0 / self->frames_per_tick;

  g_message (
    "frames per tick after: %f | "
    "ticks per frame after: %f",
    self->frames_per_tick,
    self->ticks_per_frame);

  /* update positions */
  transport_update_positions (
    self->transport, update_from_ticks);

  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      track_update_positions (
        TRACKLIST->tracks[i], update_from_ticks,
        bpm_change);
    }
}

/**
 * Cleans duplicate events and copies the events
 * to the given array.
 */
static inline void
clean_duplicates_and_copy (
  AudioEngine *       self,
  AudioEngineEvent ** events,
  int *               num_events)
{
  MPMCQueue * q = self->ev_queue;
  g_return_if_fail (q);

  /* only add events once to new array while
   * popping */
  *num_events = 0;
  AudioEngineEvent * event;
  while (event_queue_dequeue_event (q, &event))
    {
      bool already_exists = false;

      for (int i = 0; i < *num_events; i++)
        {
          if (event->type == events[i]->type
              && event->arg == events[i]->arg
              && event->uint_arg == events[i]->uint_arg)
            {
              already_exists = true;
            }
        }

      if (already_exists)
        {
          object_pool_return (
            self->ev_pool, event);
        }
      else
        {
          array_append (
            events, (*num_events), event);
        }
    }
}

/**
 * GSourceFunc to be added using idle add.
 *
 * This will loop indefinintely.
 */
int
engine_process_events (
  AudioEngine * self)
{
  g_return_val_if_fail (
    g_thread_self () == zrythm_app->gtk_thread,
    G_SOURCE_REMOVE);

  if (self->exporting)
    {
      return G_SOURCE_CONTINUE;
    }

  self->last_events_process_started =
    g_get_monotonic_time ();

  /*g_debug ("PROCESS EVENTS");*/

  AudioEngineEvent * events[100];
  AudioEngineEvent * ev;
  int num_events = 0, i;
  clean_duplicates_and_copy (
    self, events, &num_events);

  /*g_debug ("%d EVENTS, waiting for pause", num_events);*/

  EngineState state;
  if (self->activated && num_events > 0)
    {
      /* pause engine */
      engine_wait_for_pause (self, &state, Z_F_FORCE);
    }

  /*g_debug ("waited");*/

  /*g_message ("starting processing");*/
  for (i = 0; i < num_events; i++)
    {
      if (i > 30)
        {
          g_message (
            "more than 30 engine events processed!");
        }
      g_message ("processing engine event %d", i);

      ev = events[i];

      /*g_message ("event type %d", ev->type);*/

      switch (ev->type)
        {
        case AUDIO_ENGINE_EVENT_BUFFER_SIZE_CHANGE:
#ifdef HAVE_JACK
          if (self->audio_backend ==
                AUDIO_BACKEND_JACK)
            {
              engine_jack_handle_buf_size_change (
                self, ev->uint_arg);
            }
#endif
           EVENTS_PUSH (
             ET_ENGINE_BUFFER_SIZE_CHANGED, NULL);
          break;
        case AUDIO_ENGINE_EVENT_SAMPLE_RATE_CHANGE:
#ifdef HAVE_JACK
          if (self->audio_backend ==
                AUDIO_BACKEND_JACK)
            {
              engine_jack_handle_sample_rate_change (
                self, ev->uint_arg);
            }
#endif
           EVENTS_PUSH (
             ET_ENGINE_SAMPLE_RATE_CHANGED, NULL);
          break;
        default:
          g_warning (
            "event %d not implemented yet",
            ev->type);
          break;
        }

/*return_to_pool:*/
      object_pool_return (
        self->ev_pool, ev);
    }
  /*g_message ("processed %d events", i);*/

  if (num_events > 6)
    g_message ("More than 6 events processed. "
               "Optimization needed.");

  /*g_usleep (8000);*/
  /*project_validate (PROJECT);*/

  if (self->activated && num_events > 0)
    {
      /* continue engine */
      engine_resume (self, &state);
    }

  self->last_events_processed =
    g_get_monotonic_time ();

  /*g_debug ("END PROCESS EVENTS");*/

  return G_SOURCE_CONTINUE;
}

void
engine_append_ports (
  AudioEngine * self,
  GPtrArray *   ports)
{
#define _ADD(port) \
  g_return_if_fail (port); \
  g_ptr_array_add (ports, port)

  _ADD (self->control_room->monitor_fader->amp);
  _ADD (self->control_room->monitor_fader->balance);
  _ADD (self->control_room->monitor_fader->mute);
  _ADD (self->control_room->monitor_fader->solo);
  _ADD (self->control_room->monitor_fader->listen);
  _ADD (self->control_room->monitor_fader->mono_compat_enabled);
  _ADD (self->control_room->monitor_fader->stereo_in->l);
  _ADD (self->control_room->monitor_fader->stereo_in->r);
  _ADD (self->control_room->monitor_fader->stereo_out->l);
  _ADD (self->control_room->monitor_fader->stereo_out->r);

  _ADD (self->monitor_out->l);
  _ADD (self->monitor_out->r);
  _ADD (self->midi_editor_manual_press);
  _ADD (self->midi_in);

  /* add fader ports */
  _ADD (self->sample_processor->fader->stereo_in->l);
  _ADD (self->sample_processor->fader->stereo_in->r);
  _ADD (self->sample_processor->fader->stereo_out->l);
  _ADD (self->sample_processor->fader->stereo_out->r);

  for (int i = 0;
       i < self->sample_processor->tracklist->num_tracks;
       i++)
    {
      Track * tr =
        self->sample_processor->tracklist->tracks[i];
      g_warn_if_fail (track_is_auditioner (tr));
      track_append_ports (
        tr, ports, F_INCLUDE_PLUGINS);
    }

  _ADD (self->transport->roll);
  _ADD (self->transport->stop);
  _ADD (self->transport->backward);
  _ADD (self->transport->forward);
  _ADD (self->transport->loop_toggle);
  _ADD (self->transport->rec_toggle);

  for (int i = 0;
       i < self->hw_in_processor->num_audio_ports; i++)
    {
      _ADD (self->hw_in_processor->audio_ports[i]);
    }
  for (int i = 0;
       i < self->hw_in_processor->num_midi_ports; i++)
    {
      _ADD (self->hw_in_processor->midi_ports[i]);
    }

  for (int i = 0;
       i < self->hw_out_processor->num_audio_ports; i++)
    {
      _ADD (self->hw_out_processor->audio_ports[i]);
    }
  for (int i = 0;
       i < self->hw_out_processor->num_midi_ports; i++)
    {
      _ADD (self->hw_out_processor->midi_ports[i]);
    }

#undef _ADD
}

/**
 * Sets up the audio engine before the project is
 * initialized/loaded.
 */
void
engine_pre_setup (
  AudioEngine * self)
{
  /* init semaphores */
  zix_sem_init (&self->port_operation_lock, 1);

  /* start events */
  if (self->process_source_id)
    {
      g_message (
        "engine already processing events");
      return;
    }
  g_message (
    "%s: starting event timeout", __func__);
  self->process_source_id =
    g_timeout_add (
      12,
      (GSourceFunc) engine_process_events, self);

  g_return_if_fail (
    self && !self->setup && !self->pre_setup);

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
#if 0
      ret =
        engine_alsa_setup(self);
#endif
      break;
#endif
#ifdef HAVE_JACK
    case AUDIO_BACKEND_JACK:
      ret =
        engine_jack_setup (self);
      break;
#endif
#ifdef HAVE_PULSEAUDIO
    case AUDIO_BACKEND_PULSEAUDIO:
      ret =
        engine_pulse_setup (self);
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
    case AUDIO_BACKEND_ALSA_RTAUDIO:
    case AUDIO_BACKEND_JACK_RTAUDIO:
    case AUDIO_BACKEND_PULSEAUDIO_RTAUDIO:
    case AUDIO_BACKEND_COREAUDIO_RTAUDIO:
    case AUDIO_BACKEND_WASAPI_RTAUDIO:
    case AUDIO_BACKEND_ASIO_RTAUDIO:
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
      if (ZRYTHM_HAVE_UI && !ZRYTHM_TESTING)
        {
          ui_show_message_printf (
            MAIN_WINDOW ?
              GTK_WINDOW (MAIN_WINDOW) : NULL,
            GTK_MESSAGE_WARNING, true,
            _("Failed to initialize the %s audio "
              "backend. Will use the dummy backend "
              "instead. Please check your backend "
              "settings in the Preferences."),
            engine_audio_backend_to_string (
              self->audio_backend));
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
#ifdef HAVE_JACK
setup_dummy_midi:
#endif
      mret =
        engine_dummy_midi_setup (self);
      break;
#ifdef HAVE_ALSA
    case MIDI_BACKEND_ALSA:
#if 0
      mret =
        engine_alsa_midi_setup (self);
#endif
      break;
#endif
#ifdef HAVE_JACK
    case MIDI_BACKEND_JACK:
      if (self->client)
        {
          mret =
            engine_jack_midi_setup (self);
        }
      else
        {
          ui_show_message_printf (
            NULL, GTK_MESSAGE_ERROR, true,
            _("The JACK MIDI backend can only be "
            "used with the JACK audio backend "
            "(your current audio backend is %s). "
            "Will use the dummy MIDI backend "
            "instead."),
            engine_audio_backend_to_string (
              self->audio_backend));
          self->midi_backend =
            MIDI_BACKEND_DUMMY;
          goto setup_dummy_midi;
        }
      break;
#endif
#ifdef _WOE32
    case MIDI_BACKEND_WINDOWS_MME:
      mret =
        engine_windows_mme_setup (self);
      break;
#endif
#ifdef HAVE_RTMIDI
    case MIDI_BACKEND_ALSA_RTMIDI:
    case MIDI_BACKEND_JACK_RTMIDI:
    case MIDI_BACKEND_WINDOWS_MME_RTMIDI:
    case MIDI_BACKEND_COREMIDI_RTMIDI:
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
          ui_show_message_printf (
            GTK_WINDOW (MAIN_WINDOW),
            GTK_MESSAGE_WARNING, true,
            _("Failed to initialize the %s MIDI "
              "backend. Will use the dummy backend "
              "instead. Please check your backend "
              "settings in the Preferences."),
            engine_midi_backend_to_string (
              self->midi_backend));
        }

      self->midi_backend = MIDI_BACKEND_DUMMY;
      engine_dummy_midi_setup (self);
    }

  /* process any events now */
  g_message (
    "%s: processing engine events", __func__);
  engine_process_events (self);

  self->pre_setup = true;
}

/**
 * Sets up the audio engine after the project
 * is initialized/loaded.
 */
void
engine_setup (
  AudioEngine * self)
{
  g_message ("Setting up...");

  /* process any events now  */
  g_message (
    "%s: processing engine events", __func__);
  engine_process_events (self);

  hardware_processor_setup (
    self->hw_in_processor);
  hardware_processor_setup (
    self->hw_out_processor);

  if ((self->audio_backend == AUDIO_BACKEND_JACK
       && self->midi_backend != MIDI_BACKEND_JACK)
      ||
      (self->audio_backend != AUDIO_BACKEND_JACK
       && self->midi_backend == MIDI_BACKEND_JACK))
    {
      ui_show_message_printf (
        GTK_WINDOW (MAIN_WINDOW),
        GTK_MESSAGE_WARNING, true, "%s",
        _("Your selected combination of backends "
        "may not work properly. If you want to "
        "use JACK, please select JACK as both "
        "your audio and MIDI backend."));
    }

  self->buf_size_set = false;

  /* connect the sample processor to the engine
   * output */
  stereo_ports_connect (
    self->sample_processor->fader->stereo_out,
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

  /* process any events now */
  g_message ("processing engine events");
  engine_process_events (self);

  g_message ("done");
}

static AudioEngineEvent *
engine_event_new (void)
{
  AudioEngineEvent * self =
    object_new (AudioEngineEvent);

  return self;
}

static void
engine_event_free (
  AudioEngineEvent * ev)
{
  g_free_and_null (ev->backtrace);
  object_zero_and_free (ev);
}

static void
init_common (
  AudioEngine * self)
{
  self->schema_version =
    AUDIO_ENGINE_SCHEMA_VERSION;
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
#if 0
      self->audio_backend = AUDIO_BACKEND_ALSA;
#endif
      break;
#endif
#ifdef HAVE_PULSEAUDIO
    case AUDIO_BACKEND_PULSEAUDIO:
      self->audio_backend =
        AUDIO_BACKEND_PULSEAUDIO;
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
    case AUDIO_BACKEND_ALSA_RTAUDIO:
    case AUDIO_BACKEND_JACK_RTAUDIO:
    case AUDIO_BACKEND_PULSEAUDIO_RTAUDIO:
    case AUDIO_BACKEND_COREAUDIO_RTAUDIO:
    case AUDIO_BACKEND_WASAPI_RTAUDIO:
    case AUDIO_BACKEND_ASIO_RTAUDIO:
      self->audio_backend = ab_code;
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
#if 0
      self->midi_backend = MIDI_BACKEND_ALSA;
#endif
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
    case MIDI_BACKEND_ALSA_RTMIDI:
    case MIDI_BACKEND_JACK_RTMIDI:
    case MIDI_BACKEND_WINDOWS_MME_RTMIDI:
    case MIDI_BACKEND_COREMIDI_RTMIDI:
      self->midi_backend = mb_code;
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
      ui_show_message_printf (
        GTK_WINDOW (MAIN_WINDOW),
        GTK_MESSAGE_WARNING, true,
        _("The selected MIDI/audio backend was not "
          "found in the version of %s you have "
          "installed. The audio and MIDI backends "
          "were set to \"Dummy\". Please set your "
          "preferred backend from the "
          "preferences."),
        PROGRAM_NAME);
    }

  self->pan_law =
    ZRYTHM_TESTING
    ? PAN_LAW_MINUS_3DB
    :
    (PanLaw)
    g_settings_get_enum (S_P_DSP_PAN, "pan-law");
  self->pan_algo =
    ZRYTHM_TESTING
    ? PAN_ALGORITHM_SINE_LAW
    :
    (PanAlgorithm)
    g_settings_get_enum (
      S_P_DSP_PAN, "pan-algorithm");

  /* set a temporary buffer sizes */
  if (self->block_length == 0)
    {
      self->block_length = 8192;
    }
  if (self->midi_buf_size == 0)
    {
      self->midi_buf_size = 8192;
    }

  self->ev_pool =
    object_pool_new (
      (ObjectCreatorFunc) engine_event_new,
      (ObjectFreeFunc) engine_event_free,
      ENGINE_MAX_EVENTS);
  self->ev_queue = mpmc_queue_new ();
  mpmc_queue_reserve (
    self->ev_queue,
    (size_t)
    ENGINE_MAX_EVENTS *
      sizeof (AudioEngineEvent *));
}

void
engine_init_loaded (
  AudioEngine * self,
  Project *     project)
{
  g_message ("Initializing...");

  self->project = project;

  audio_pool_init_loaded (self->pool);

  Track * tempo_track = NULL;
  if (project)
    {
      g_return_if_fail (project->tracklist);
      tempo_track =
        project->tracklist->tempo_track;
      if (!tempo_track)
        tempo_track =
          tracklist_get_track_by_type (
            project->tracklist,
            TRACK_TYPE_TEMPO);
      g_return_if_fail (tempo_track);
    }
  transport_init_loaded (
    self->transport, self, tempo_track);

  control_room_init_loaded (
    self->control_room, self);
  sample_processor_init_loaded (
    self->sample_processor, self);
  hardware_processor_init_loaded (
    self->hw_in_processor, self);
  hardware_processor_init_loaded (
    self->hw_out_processor, self);

  init_common (self);

  GPtrArray * ports = g_ptr_array_new ();
  engine_append_ports (self, ports);
  for (size_t i = 0; i < ports->len; i++)
    {
      Port * port = g_ptr_array_index (ports, i);
      PortIdentifier * id = &port->id;
      if (id->owner_type ==
            PORT_OWNER_TYPE_AUDIO_ENGINE)
        port_init_loaded (port, self);
      else if (
        id->owner_type == PORT_OWNER_TYPE_HW)
        {
          if (id->flow == FLOW_OUTPUT)
            port_init_loaded (
              port, self->hw_in_processor);
          else if (id->flow == FLOW_INPUT)
            port_init_loaded (
              port, self->hw_out_processor);
        }
      else if (
        id->owner_type == PORT_OWNER_TYPE_FADER)
        {
          if (id->flags2 &
                PORT_FLAG2_SAMPLE_PROCESSOR_FADER)
            port_init_loaded (
              port, self->sample_processor->fader);
          else if (id->flags2 &
                PORT_FLAG2_MONITOR_FADER)
            port_init_loaded (
              port,
              self->control_room->monitor_fader);
        }
    }

  g_message ("done");
}

/**
 * Create a new audio engine.
 *
 * This only initializes the engine and does not
 * connect to the backend.
 */
AudioEngine *
engine_new (
  Project * project)
{
  g_message ("Creating audio engine...");

  AudioEngine * self = object_new (AudioEngine);
  self->project = project;
  self->schema_version =
    AUDIO_ENGINE_SCHEMA_VERSION;

  if (project)
    {
      project->audio_engine = self;
    }

  self->sample_rate = 44000;
  self->transport = transport_new (self);
  self->pool = audio_pool_new ();
  self->control_room = control_room_new (self);
  self->sample_processor =
    sample_processor_new (self);

  /* init midi editor manual press */
  self->midi_editor_manual_press =
    port_new_with_type (
      TYPE_EVENT, FLOW_INPUT,
      "MIDI Editor Manual Press");
  self->midi_editor_manual_press->id.sym =
    g_strdup ("midi_editor_manual_press");
  self->midi_editor_manual_press->id.flags |=
    PORT_FLAG_MANUAL_PRESS;

  /* init midi in */
  self->midi_in =
    port_new_with_type (
      TYPE_EVENT, FLOW_INPUT, "MIDI in");
  self->midi_in->id.sym = g_strdup ("midi_in");

  /* init MIDI queues */
  self->midi_editor_manual_press->midi_events =
    midi_events_new ();
  self->midi_in->midi_events = midi_events_new ();

  /* create monitor out ports */
  Port * monitor_out_l, * monitor_out_r;
  monitor_out_l =
    port_new_with_type (
      TYPE_AUDIO, FLOW_OUTPUT, "Monitor Out L");
  monitor_out_l->id.sym = g_strdup ("monitor_out_l");
  monitor_out_r =
    port_new_with_type (
      TYPE_AUDIO, FLOW_OUTPUT, "Monitor Out R");
  monitor_out_r->id.sym = g_strdup ("monitor_out_r");
  self->monitor_out =
    stereo_ports_new_from_existing (
      monitor_out_l, monitor_out_r);
  stereo_ports_set_owner (
    self->monitor_out,
    PORT_OWNER_TYPE_AUDIO_ENGINE, self);

  self->hw_in_processor =
    hardware_processor_new (true, self);
  self->hw_out_processor =
    hardware_processor_new (false, self);

  init_common (self);

  return self;
}

/**
 * @param force_pause Whether to force transport
 *   pause, otherwise for engine to process and
 *   handle the pause request.
 */
void
engine_wait_for_pause (
  AudioEngine * self,
  EngineState * state,
  bool          force_pause)
{
  state->running =
    g_atomic_int_get (&self->run);
  state->playing = TRANSPORT_IS_ROLLING;
  state->looping = TRANSPORT->loop;

  /* send panic */
  midi_events_panic_all (F_QUEUED);

  if (state->playing)
    {
      transport_request_pause (TRANSPORT);

      if (force_pause)
        {
          TRANSPORT->play_state = PLAYSTATE_PAUSED;
        }
      else
        {
          while (TRANSPORT->play_state ==
                   PLAYSTATE_PAUSE_REQUESTED &&
                 !self->stop_dummy_audio_thread)
            {
              g_usleep (100);
            }
        }
    }

  g_message (
    "setting run to 0 and waiting for cycle to "
    "finish...");

  g_atomic_int_set (&self->run, 0);
  while (g_atomic_int_get (
           &self->cycle_running))
    {
      g_usleep (100);
    }

  g_message ("cycle finished");

  if (PROJECT->loaded)
    {
#if 0
      /* process all recording events */
      zix_sem_wait (
        &RECORDING_MANAGER->processing_sem);
      recording_manager_process_events (
        RECORDING_MANAGER);
      zix_sem_post (
        &RECORDING_MANAGER->processing_sem);
#endif

      /* run one more time to flush panic
       * messages */
      engine_process_prepare (self, 1);
      EngineProcessTimeInfo time_nfo = {
        .g_start_frame =
          (unsigned_frame_t) PLAYHEAD->frames,
        .local_offset = 0,
        .nframes = 1, };
      router_start_cycle (ROUTER, time_nfo);
      engine_post_process (self, 0, 1);
    }
}

void
engine_resume (
  AudioEngine * self,
  EngineState * state)
{
  g_message ("resuming engine...");

  TRANSPORT->loop = state->looping;

  if (state->playing)
    {
      position_update_frames_from_ticks (
        &TRANSPORT->playhead_before_pause);
      transport_move_playhead (
        TRANSPORT, &TRANSPORT->playhead_before_pause,
        F_NO_PANIC, F_NO_SET_CUE_POINT,
        F_NO_PUBLISH_EVENTS);
      transport_request_roll (TRANSPORT);
    }
  else
    {
      transport_request_pause (TRANSPORT);
    }

  g_atomic_int_set (
    &self->run, (guint) state->running);
}

/**
 * Waits for n processing cycles to finish.
 *
 * Used during tests.
 */
void
engine_wait_n_cycles (
  AudioEngine * self,
  int           n)
{
  unsigned long expected_cycle =
    self->cycle + (unsigned long) n;
  while (self->cycle < expected_cycle)
    {
      g_usleep (12);
    }
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

      /* process any events now  */
      g_message (
        "%s: processing engine events", __func__);
      engine_process_events (self);

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
      EngineState state;
      engine_wait_for_pause (self, &state, true);

      self->activated = false;
    }

  if (!activate)
    {
      hardware_processor_activate (
        HW_IN_PROCESSOR, false);
    }

#ifdef HAVE_JACK
  if (self->audio_backend == AUDIO_BACKEND_JACK)
    {
      engine_jack_activate (self, activate);
    }
#endif
#ifdef HAVE_PULSEAUDIO
  if (self->audio_backend
        == AUDIO_BACKEND_PULSEAUDIO)
    {
      engine_pulse_activate (self, activate);
    }
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
  if (midi_backend_is_rtmidi (self->midi_backend))
    {
      engine_rtmidi_activate (self, activate);
    }
#endif
#ifdef HAVE_SDL
  if (self->audio_backend == AUDIO_BACKEND_SDL)
    engine_sdl_activate (self, activate);
#endif
#ifdef HAVE_RTAUDIO
  if (audio_backend_is_rtaudio (
        self->audio_backend))
    {
      engine_rtaudio_activate (self, activate);
    }
#endif

  if (activate)
    {
      hardware_processor_activate (
        HW_IN_PROCESSOR, true);
    }

  /* process any events now */
  g_message ("processing engine events");
  engine_process_events (self);

  self->activated = activate;

  if (ZRYTHM_HAVE_UI && PROJECT->loaded)
    {
      EVENTS_PUSH (
        ET_ENGINE_ACTIVATE_CHANGED, NULL);
    }

  g_message ("done");
}

void
engine_realloc_port_buffers (
  AudioEngine * self,
  nframes_t     nframes)
{
  AUDIO_ENGINE->block_length = nframes;
  AUDIO_ENGINE->buf_size_set = true;
  g_message (
    "Block length changed to %d. "
    "reallocating buffers...",
    AUDIO_ENGINE->block_length);

  /* not needed anymore, buffers are allocated
   * during graph recalc */
#if 0
  /** reallocate port buffers to new size */
  GPtrArray * ports = g_ptr_array_new ();
  port_get_all (ports);
  for (size_t i = 0; i < ports->len; i++)
    {
      Port * port = g_ptr_array_index (ports, i);
      g_warn_if_fail (port);

      size_t new_sz =
        MAX (
          port->min_buf_size,
          nframes * sizeof (float));
      port->buf = g_realloc (port->buf, new_sz);
      memset (port->buf, 0, new_sz);
    }
  object_free_w_func_and_null (
    g_ptr_array_unref, ports);
#endif

  /* TODO make function that fetches all plugins
   * in the project */
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      Channel * ch = TRACKLIST->tracks[i]->channel;

      if (!ch)
        continue;

      for (int j = 0; j < STRIP_SIZE * 2 + 1; j++)
        {
          Plugin * pl;
          if (j < STRIP_SIZE)
            pl = ch->midi_fx[j];
          else if (j == STRIP_SIZE)
            pl = ch->instrument;
          else
            pl = ch->inserts[j - (STRIP_SIZE + 1)];

          if (pl && !pl->instantiation_failed)
            {
              if (pl->setting->open_with_carla)
                {
                  carla_native_plugin_update_buffer_size_and_sample_rate (
                    pl->carla);
                }
              else if (
                pl->setting->descr->protocol ==
                  PROT_LV2)
                {
                  lv2_plugin_allocate_port_buffers (
                    pl->lv2);
                }
            }
        }
    }
  AUDIO_ENGINE->nframes = nframes;

  router_recalc_graph (ROUTER, false);

  g_message ("done");
}

/**
 * Clears the underlying backend's output buffers.
 *
 * Used when returning early.
 */
static void
clear_output_buffers (
  AudioEngine * self,
  nframes_t     nframes)
{
  /* clear the monitor output (used by rtaudio) */
  port_clear_buffer (self->monitor_out->l);
  port_clear_buffer (self->monitor_out->r);

  /* clear outputs exposed to jack */
#ifdef HAVE_JACK
  for (size_t i = 0;
       i < ROUTER->graph->external_out_ports->len;
       i++)
    {
      Port * port =
        g_ptr_array_index (
          ROUTER->graph->external_out_ports, i);

      if (port->internal_type == INTERNAL_JACK_PORT
          && port->id.type == TYPE_AUDIO
          &&
          self->audio_backend == AUDIO_BACKEND_JACK)
        {
          jack_port_t * jport =
            JACK_PORT_T (port->data);
          float * out =
            (float *)
            jack_port_get_buffer (jport, nframes);
          dsp_fill (
            &out[0], DENORMAL_PREVENTION_VAL,
            nframes);
        }
      else if (
        port->internal_type == INTERNAL_JACK_PORT
        && port->id.type == TYPE_EVENT
        && self->midi_backend == MIDI_BACKEND_JACK)
        {
          jack_port_t * jport =
            JACK_PORT_T (port->data);
          void * buf =
            jack_port_get_buffer (jport, nframes);
          jack_midi_clear_buffer (buf);
        }
    }
#endif
}

/**
 * To be called by each implementation to prepare
 * the structures before processing.
 *
 * Clears buffers, marks all as unprocessed, etc.
 *
 * @return Whether the cycle should be skipped.
 */
bool
engine_process_prepare (
  AudioEngine * self,
  nframes_t     nframes)
{
  g_atomic_int_set (
    &self->preparing_for_process, 1);

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
      if (ZRYTHM_TESTING)
        g_message ("pause requested handled");
      self->transport->play_state = PLAYSTATE_PAUSED;
      /*zix_sem_post (&TRANSPORT->paused);*/
#ifdef HAVE_JACK
      if (self->audio_backend == AUDIO_BACKEND_JACK)
        {
          engine_jack_handle_stop (self);
        }
#endif
    }
  else if (self->transport->play_state ==
             PLAYSTATE_ROLL_REQUESTED &&
           self->transport->
             countin_frames_remaining == 0)
    {
      self->transport->play_state =
        PLAYSTATE_ROLLING;
      self->remaining_latency_preroll =
        router_get_max_route_playback_latency (
          self->router);
#if 0
      g_message (
        "starting playback, remaining latency "
        "preroll: %u",
        self->remaining_latency_preroll);
#endif
#ifdef HAVE_JACK
      if (self->audio_backend == AUDIO_BACKEND_JACK)
        {
          engine_jack_handle_start (self);
        }
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
#if 0
      engine_alsa_prepare_process (self);
#endif
      break;
#endif
    default:
      break;
    }

  /* clear monitor out before in case we need to
   * return early */
  port_clear_buffer (self->monitor_out->l);
  port_clear_buffer (self->monitor_out->r);

  bool lock_acquired =
    zix_sem_try_wait (&self->port_operation_lock);

  if (!lock_acquired && !self->exporting)
    {
      if (ZRYTHM_TESTING)
        g_message (
          "port operation lock is busy, skipping "
          "cycle...");
      return true;
    }

  /* reset all buffers */
  fader_clear_buffers (MONITOR_FADER);
  port_clear_buffer (self->midi_in);
  port_clear_buffer (
    self->midi_editor_manual_press);

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

  g_atomic_int_set (
    &self->preparing_for_process, 0);

  return false;
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
}

/**
 * Processes current cycle.
 *
 * To be called by each implementation in its
 * callback.
 */
int
engine_process (
  AudioEngine *   self,
  const nframes_t total_frames_to_process)
{
  if (ZRYTHM_TESTING)
    {
      /*g_debug (*/
        /*"engine process started. total frames to "*/
        /*"process: %u", total_frames_to_process);*/
    }

  g_return_val_if_fail (
    total_frames_to_process > 0, -1);

  /*g_message ("processing...");*/
  g_atomic_int_set (&self->cycle_running, 1);

  /* calculate timestamps (used for synchronizing
   * external events like Windows MME MIDI) */
  self->timestamp_start =
    g_get_monotonic_time ();
  self->timestamp_end =
    self->timestamp_start +
    (total_frames_to_process * 1000000) /
      self->sample_rate;

  if (G_UNLIKELY (!engine_get_run (self)))
    {
      /*g_message ("ENGINE NOT RUNNING");*/
      /*g_message ("skipping processing...");*/
      clear_output_buffers (
        self, total_frames_to_process);
      g_atomic_int_set (&self->cycle_running, 0);
      return 0;
    }

  /*count++;*/
  /*self->cycle = count;*/

  /* run pre-process code */
  bool skip_cycle =
    engine_process_prepare (
      self, total_frames_to_process);

  if (G_UNLIKELY (skip_cycle))
    {
      clear_output_buffers (
        self, total_frames_to_process);
      g_atomic_int_set (&self->cycle_running, 0);
      return 0;
    }

  /* puts MIDI in events in the MIDI in port */
  receive_midi_events (
    self, total_frames_to_process, 1);

  /* process HW processor to get audio/MIDI data
   * from hardware */
  hardware_processor_process (
    HW_IN_PROCESSOR, total_frames_to_process);

  nframes_t total_frames_remaining =
    total_frames_to_process;

  /* --- handle preroll --- */

  EngineProcessTimeInfo split_time_nfo = {
    .g_start_frame =
      (unsigned_frame_t) PLAYHEAD->frames,
    .local_offset = 0,
    .nframes = 0, };

  while (self->remaining_latency_preroll > 0)
    {
      nframes_t num_preroll_frames =
        MIN (
          total_frames_remaining,
          self->remaining_latency_preroll);
      if (ZRYTHM_TESTING)
        {
          if (num_preroll_frames > 0)
            {
              g_message ("prerolling for %u frames",
                num_preroll_frames);
            }
        }

      /* loop through each route */
      for (size_t i = 0;
           i < self->router->graph->n_init_triggers;
           i++)
        {
          GraphNode * start_node =
            self->router->graph->
              init_trigger_list[i];

#define route_latency \
  (start_node->route_playback_latency)

          if (self->remaining_latency_preroll >
                route_latency + num_preroll_frames)
            {
              /* this route will no-roll for the
               * complete pre-roll cycle */
            }
          else if (self->remaining_latency_preroll >
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

              /* this route will do a partial roll
               * from num_preroll_frames */
            }
          else
            {
              /* this route will do a normal roll
               * for the complete pre-roll cycle */
            }

#undef route_latency

        } /* foreach route */

      /* offset to start processing at in this
       * cycle */
      nframes_t preroll_offset =
        total_frames_to_process -
          total_frames_remaining;
      g_warn_if_fail (
        preroll_offset + num_preroll_frames <=
          self->nframes);

      split_time_nfo.local_offset = preroll_offset;
      split_time_nfo.nframes = num_preroll_frames;
      router_start_cycle (
        self->router, split_time_nfo);

      self->remaining_latency_preroll -=
        num_preroll_frames;
      total_frames_remaining -= num_preroll_frames;

      if (total_frames_remaining == 0)
        break;

    } /* while latency preroll frames remaining */

  /* if we still have frames to process (i.e., if
   * preroll finished completely and can start
   * processing normally) */
  if (total_frames_remaining > 0)
    {
      nframes_t cur_offset =
        total_frames_to_process -
          total_frames_remaining;

      /* queue metronome if met within this cycle */
      if (self->transport->metronome_enabled &&
            TRANSPORT_IS_ROLLING)
        {
          metronome_queue_events (
            self, cur_offset,
            total_frames_remaining);
        }

      /* split at countin */
      if (self->transport->
            countin_frames_remaining > 0)
        {
          nframes_t countin_frames =
            MIN (
              total_frames_remaining,
              self->transport->
                countin_frames_remaining);

          /* process for countin frames */
          split_time_nfo.local_offset = cur_offset;
          split_time_nfo.nframes = countin_frames;
          router_start_cycle (
            self->router, split_time_nfo);
          self->transport->
            countin_frames_remaining -=
              countin_frames;

          /* adjust total frames remaining to
           * process and current offset */
          total_frames_remaining -= countin_frames;
          if (total_frames_remaining == 0)
            goto finalize_processing;
          cur_offset += countin_frames;
        }

      /* split at preroll */
      if (self->transport->
            countin_frames_remaining == 0 &&
          self->transport->
            preroll_frames_remaining > 0)
        {
          nframes_t preroll_frames =
            MIN (
              total_frames_remaining,
              self->transport->
                preroll_frames_remaining);

          /* process for preroll frames */
          split_time_nfo.local_offset = cur_offset;
          split_time_nfo.nframes = preroll_frames;
          router_start_cycle (
            self->router, split_time_nfo);
          self->transport->
            preroll_frames_remaining -=
              preroll_frames;

          /* process for remaining frames */
          cur_offset += preroll_frames;
          nframes_t remaining_frames =
            total_frames_remaining - preroll_frames;
          if (remaining_frames > 0)
            {
              split_time_nfo.local_offset =
                cur_offset;
              split_time_nfo.nframes =
                remaining_frames;
              router_start_cycle (
                self->router, split_time_nfo);
            }
        }
      else
        {
          /* run the cycle for the remaining
           * frames - this will also play the
           * queued metronome events (if any) */
          split_time_nfo.local_offset = cur_offset;
          split_time_nfo.nframes =
            total_frames_remaining;
          router_start_cycle (
            self->router, split_time_nfo);
        }
    }

finalize_processing:

  /* run post-process code for the number of frames
   * remaining after handling preroll (if any) */
  engine_post_process (
    self, total_frames_remaining,
    total_frames_to_process);

  self->cycle++;

  g_atomic_int_set (&self->cycle_running, 0);

  if (ZRYTHM_TESTING)
    {
      /*g_debug ("engine process ended...");*/
    }

  self->last_timestamp_start =
    self->timestamp_start;
  self->last_timestamp_end =
    g_get_monotonic_time ();

  /*
   * processing finished, return 0 (OK)
   */
  return 0;
}

/**
 * To be called after processing for common logic.
 *
 * @param roll_nframes Frames to roll (add to the
 *   playhead - if transport rolling).
 * @param nframes Total frames for this processing
 *   cycle.
 */
void
engine_post_process (
  AudioEngine *   self,
  const nframes_t roll_nframes,
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
  if (TRANSPORT_IS_ROLLING
      && self->remaining_latency_preroll == 0)
    {
      transport_add_to_playhead (
        self->transport, roll_nframes);
#ifdef HAVE_JACK
      if (self->audio_backend == AUDIO_BACKEND_JACK)
        {
          engine_jack_handle_position_change (self);
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
#if 0
      engine_alsa_fill_out_bufs (self, nframes);
#endif
      break;
#endif
#ifdef HAVE_JACK
    case AUDIO_BACKEND_JACK:
      /*engine_jack_fill_out_bufs (self, nframes);*/
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
    /*case AUDIO_BACKEND_RTAUDIO:*/
      /*break;*/
#endif
    default:
      break;
    }
}

/**
 * Returns the int value corresponding to the
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
 * Returns the int value corresponding to the
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

AudioBackend
engine_audio_backend_from_string (
  char * str)
{
  for (int i = 0; i < NUM_AUDIO_BACKENDS; i++)
    {
      if (string_is_equal (
            audio_backend_str[i], str))
        {
          return (AudioBackend) i;
        }
    }

  if (string_is_equal (str, "none"))
    {
      return AUDIO_BACKEND_DUMMY;
    }

  g_message (
    "Audio backend '%s' not found. The available "
    "choices are:", str);
  for (int i = 0; i < NUM_AUDIO_BACKENDS; i++)
    {
      g_message ("%s", audio_backend_str[i]);
    }

  g_return_val_if_reached (AUDIO_BACKEND_DUMMY);
}

MidiBackend
engine_midi_backend_from_string (
  char * str)
{
  for (int i = 0; i < NUM_MIDI_BACKENDS; i++)
    {
      if (string_is_equal (
            midi_backend_str[i], str))
        {
          return (MidiBackend) i;
        }
    }

  if (string_is_equal (str, "none"))
    {
      return MIDI_BACKEND_DUMMY;
    }
  else if (string_is_equal (str, "jack"))
    {
      return MIDI_BACKEND_JACK;
    }

  g_message (
    "MIDI backend '%s' not found. The available "
    "choices are:", str);
  for (int i = 0; i < NUM_MIDI_BACKENDS; i++)
    {
      g_message ("%s", midi_backend_str[i]);
    }

  g_return_val_if_reached (MIDI_BACKEND_DUMMY);
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

  tracklist_mark_all_tracks_for_bounce (
    TRACKLIST, false);
}

/**
 * Stops events from getting fired.
 */
static void
engine_stop_events (
  AudioEngine * self)
{
  if (self->process_source_id)
    {
      /* remove the source func */
      g_source_remove_and_zero (
        self->process_source_id);
    }

  /* process any remaining events - clear the
   * queue. */
  engine_process_events (self);
}

/**
 * Clones the audio engine.
 *
 * To be used for serialization.
 */
AudioEngine *
engine_clone (
  const AudioEngine * src)
{
  AudioEngine * self = object_new (AudioEngine);
  self->schema_version =
    AUDIO_ENGINE_SCHEMA_VERSION;

  self->transport_type = src->transport_type;
  self->sample_rate = src->sample_rate;
  self->frames_per_tick = src->frames_per_tick;
  self->monitor_out =
    stereo_ports_clone (src->monitor_out);
  self->midi_editor_manual_press =
    port_clone (src->midi_editor_manual_press);
  self->midi_in = port_clone (src->midi_in);
  self->transport =
    transport_clone (src->transport);
  self->pool = audio_pool_clone (src->pool);
  self->control_room =
    control_room_clone (src->control_room);
  self->sample_processor =
    sample_processor_clone (src->sample_processor);
  self->hw_in_processor =
    hardware_processor_clone (
      src->hw_in_processor);
  self->hw_out_processor =
    hardware_processor_clone (
      src->hw_out_processor);

  return self;
}

void
engine_free (
  AudioEngine * self)
{
  g_debug ("freeing engine...");

  if (self->process_source_id)
    engine_stop_events (self);

  if (self->router)
    {
      /* terminate graph threads */
      graph_terminate (self->router->graph);
    }

  if (self->activated)
    {
      engine_activate (self, false);
    }

  object_free_w_func_and_null (
    router_free, self->router);

  switch (self->audio_backend)
    {
#ifdef HAVE_JACK
    case AUDIO_BACKEND_JACK:
      engine_jack_tear_down (self);
      break;
#endif
#ifdef HAVE_RTAUDIO
    case AUDIO_BACKEND_ALSA_RTAUDIO:
    case AUDIO_BACKEND_JACK_RTAUDIO:
    case AUDIO_BACKEND_PULSEAUDIO_RTAUDIO:
    case AUDIO_BACKEND_COREAUDIO_RTAUDIO:
    case AUDIO_BACKEND_WASAPI_RTAUDIO:
    case AUDIO_BACKEND_ASIO_RTAUDIO:
      engine_rtaudio_tear_down (self);
      break;
#endif
#ifdef HAVE_PULSEAUDIO
    case AUDIO_BACKEND_PULSEAUDIO:
      engine_pulse_tear_down (self);
      break;
#endif
    case AUDIO_BACKEND_DUMMY:
      engine_dummy_tear_down (self);
      break;
    default:
      break;
    }

  if (self == AUDIO_ENGINE)
    stereo_ports_disconnect (self->monitor_out);
  object_free_w_func_and_null (
    stereo_ports_free, self->monitor_out);

  if (self == AUDIO_ENGINE)
    port_disconnect_all (self->midi_in);
  object_free_w_func_and_null (
    port_free, self->midi_in);
  if (self == AUDIO_ENGINE)
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

  object_free_w_func_and_null (
    object_pool_free, self->ev_pool);
  object_free_w_func_and_null (
    mpmc_queue_free, self->ev_queue);

  object_free_w_func_and_null (
    hardware_processor_free,
    self->hw_in_processor);
  object_free_w_func_and_null (
    hardware_processor_free,
    self->hw_out_processor);

  object_zero_and_free (self);

  g_debug ("finished freeing engine");
}
