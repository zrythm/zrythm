/*
 * Copyright (C) 2018-2022 Alexandros Theodotou <alex at zrythm dot org>
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

#include "zrythm-config.h"

#ifdef HAVE_JACK

#include <math.h>

#include "audio/channel.h"
#include "audio/engine.h"
#include "audio/engine_jack.h"
#include "audio/ext_port.h"
#include "utils/midi.h"
#include "audio/router.h"
#include "audio/port.h"
#include "audio/tempo_track.h"
#include "audio/transport.h"
#include "gui/widgets/main_window.h"
#include "plugins/plugin.h"
#include "plugins/lv2_plugin.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/backtrace.h"
#include "utils/mpmc_queue.h"
#include "utils/object_pool.h"
#include "utils/string.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include <jack/thread.h>

/**
 * Refreshes the list of external ports.
 */
void
engine_jack_rescan_ports (
  AudioEngine * self)
{
  /* get all input ports */
  const char ** ports =
    jack_get_ports (
      self->client,
      NULL, NULL,
      JackPortIsPhysical);

  int i = 0;
  /*jack_port_t * jport;*/
  while (ports[i] != NULL)
    {
      /*jport =*/
        /*jack_port_by_name (*/
          /*self->client,*/
          /*ports[i]);*/

      /*add_port_if_not_exists (*/
        /*self->physical_ins,*/
        /*&self->num_physical_ins,*/
        /*jport);*/

      i++;
    }

  /* TODO clear unconnected remembered ports */
}

#if 0
static void
autoconnect_midi_controllers (
  AudioEngine * self)
{
  /* get all output MIDI ports */
  const char ** ports =
    jack_get_ports (
      self->client,
      NULL, JACK_DEFAULT_MIDI_TYPE,
      JackPortIsPhysical |
        JackPortIsOutput);

  if (!ports) return;

  /* get selected MIDI devices */
  char ** devices =
    g_settings_get_strv (
      S_P_GENERAL_ENGINE,
      "midi-controllers");

  if(!devices) return;

  int i = 0;
  int j;
  char * device;
  /*jack_port_t * port;*/
  while (ports[i] != NULL)
    {
      /* if port matches one of the selected
       * MIDI devices, connect it to Zrythm
       * MIDI In */
      j = 0;
      while ((device = devices[j]) != NULL)
        {
          if (g_str_match_string (
                device, ports[i], 1))
            {
              int ret =
                jack_connect (
                  self->client,
                  ports[i],
                  jack_port_name (
                    JACK_PORT_T (
                      self->midi_in->data)));
              if (ret)
                {
                  char msg[600];
                  engine_jack_get_error_message (
                    ret, msg);
                  g_warning (
                    "Failed connecting %s to %s:\n"
                    "%s",
                    ports[i],
                    self->midi_in->id.label,
                    msg);
                }
              break;
            }
          j++;
        }
      i++;
    }
    jack_free(ports);
    g_strfreev(devices);
}
#endif

void
engine_jack_handle_sample_rate_change (
  AudioEngine * self,
  uint32_t      samplerate)
{
  AUDIO_ENGINE->sample_rate = samplerate;

  if (P_TEMPO_TRACK)
    {
      int beats_per_bar =
        tempo_track_get_beats_per_bar (
          P_TEMPO_TRACK);
      engine_update_frames_per_tick (
        self, beats_per_bar,
        tempo_track_get_current_bpm (P_TEMPO_TRACK),
        self->sample_rate, true, true, false);
    }

  g_message (
    "JACK: Sample rate changed to %d", samplerate);
}

static void
process_change_request (
  AudioEngine * self)
{
  /* process immediately if gtk thread */
  if (g_thread_self () == zrythm_app->gtk_thread)
    {
      engine_process_events (self);
    }
  /* otherwise if activated wait for gtk thread
   * to process all events */
  else if (self->activated)
    {
      gint64 cur_time = g_get_monotonic_time ();
      while (
        cur_time >
          self->last_events_process_started ||
        cur_time > self->last_events_processed)
        {
          g_message (
            "-------- waiting for change");
          g_usleep (1000);
        }
    }
}

/**
 * Jack sample rate callback.
 *
 * This is called in a non-RT thread by JACK in
 * between its processing cycles, and will block
 * until this function returns so it is safe to
 * change the buffers here.
 */
static int
sample_rate_cb (
  uint32_t nframes,
  AudioEngine * self)
{
#if 0
  g_atomic_int_set (
    &AUDIO_ENGINE->changing_sample_rate, 1);
#endif

  ENGINE_EVENTS_PUSH (
    AUDIO_ENGINE_EVENT_SAMPLE_RATE_CHANGE,
    NULL, nframes, 0.f);

  process_change_request (self);

#if 0
  g_atomic_int_set (
    &AUDIO_ENGINE->changing_sample_rate, 0);
#endif

  return 0;
}

void
engine_jack_handle_buf_size_change (
  AudioEngine * self,
  uint32_t      frames)
{
  engine_realloc_port_buffers (
    AUDIO_ENGINE, frames);
#ifdef HAVE_JACK_PORT_TYPE_GET_BUFFER_SIZE
  AUDIO_ENGINE->midi_buf_size =
    jack_port_type_get_buffer_size(
      AUDIO_ENGINE->client, JACK_DEFAULT_MIDI_TYPE);
#endif
  g_message (
    "JACK: Block length changed to %d, "
    "midi buf size to %zu",
    AUDIO_ENGINE->block_length,
    AUDIO_ENGINE->midi_buf_size);
}

/** Jack buffer size callback. */
static int
buffer_size_cb (
  uint32_t      nframes,
  AudioEngine * self)
{
  ENGINE_EVENTS_PUSH (
    AUDIO_ENGINE_EVENT_BUFFER_SIZE_CHANGE,
    NULL, nframes, 0.f);

  process_change_request (self);

  return 0;
}

void
engine_jack_handle_position_change (
  AudioEngine * self)
{
  if (self->transport_type ==
        AUDIO_ENGINE_JACK_TIMEBASE_MASTER)
    {
      jack_transport_locate (
        self->client,
        (jack_nframes_t) PLAYHEAD->frames);
    }
}

void
engine_jack_handle_start (
  AudioEngine * self)
{
  if (self->transport_type ==
        AUDIO_ENGINE_JACK_TIMEBASE_MASTER)
    {
      jack_transport_start (self->client);
    }
}

void
engine_jack_handle_stop (
  AudioEngine * self)
{
  if (self->transport_type ==
        AUDIO_ENGINE_JACK_TIMEBASE_MASTER)
    {
      jack_transport_stop (self->client);
    }
}

/**
 * Prepares for processing.
 *
 * Called at the start of each process cycle.
 */
void
engine_jack_prepare_process (
  AudioEngine * self)
{
  /* if client, get transport position info */
  if (self->transport_type ==
        AUDIO_ENGINE_JACK_TRANSPORT_CLIENT)
    {
      jack_position_t pos;
      jack_transport_state_t state =
        jack_transport_query (
          self->client, &pos);

      if (state == JackTransportRolling)
        {
          TRANSPORT->play_state =
            PLAYSTATE_ROLLING;
        }
      else if (state == JackTransportStopped)
        {
          TRANSPORT->play_state =
            PLAYSTATE_PAUSED;
        }

      /* get position from timebase master */
      Position tmp;
      position_from_frames (
        &tmp, pos.frame);
      transport_set_playhead_pos (
        TRANSPORT, &tmp);

      /* BBT and BPM changes */
      if (pos.valid & JackPositionBBT)
        {
          tempo_track_set_beats_per_bar (
            P_TEMPO_TRACK,
            (int) pos.beats_per_bar);
          tempo_track_set_bpm (
            P_TEMPO_TRACK,
            (float) pos.beats_per_minute,
            (float) pos.beats_per_minute,
            true, true);
          tempo_track_set_beat_unit (
            P_TEMPO_TRACK,
            (int) pos.beat_type);
        }
    }

  /* clear output */
}

/**
 * The process callback for this JACK application is
 * called in a special realtime thread once for
 * each audio cycle.
 *
 * @param nframes The number of frames to process.
 * @param data User data.
 */
static int
process_cb (
  nframes_t nframes,
  void *    data)
{
  AudioEngine * engine = (AudioEngine *) data;
  return engine_process (engine, nframes);
}

/**
 * Client-supplied function that is called whenever
 * an xrun has occurred.
 *
 * @see jack_get_xrun_delayed_usecs()
 *
 * @return zero on success, non-zero on error
 */
static int
xrun_cb (
  AudioEngine * self)
{
  gint64 cur_time = g_get_monotonic_time ();
  if (cur_time - self->last_xrun_notification >
        6000000)
    {
      /* TODO make a notification message queue */
#if 0
      g_message (
        "%s",
        _("XRUN occurred - check your JACK "
        "configuration"));
#endif
      self->last_xrun_notification = cur_time;
    }

  return 0;
}

static void
timebase_cb (
  jack_transport_state_t state,
  jack_nframes_t nframes,
  jack_position_t *pos,
  int new_pos,
  void *arg)
{
  AudioEngine * self = (AudioEngine *) arg;
  if (!engine_get_run (self))
    return;

  /* Mandatory fields */
  pos->valid = JackPositionBBT;
  pos->frame = (jack_nframes_t) PLAYHEAD->frames;

  /* BBT */
  pos->bar = position_get_bars (PLAYHEAD, true);
  pos->beat = position_get_beats (PLAYHEAD, true);
  pos->tick =
    position_get_sixteenths (PLAYHEAD, true) *
    TICKS_PER_SIXTEENTH_NOTE +
    (int) floor (position_get_ticks (PLAYHEAD));
  Position bar_start;
  position_set_to_bar (
    &bar_start, position_get_bars (PLAYHEAD, true));
  pos->bar_start_tick =
    (double)
    (PLAYHEAD->ticks -
     bar_start.ticks);
  pos->beats_per_bar =
    (float)
    tempo_track_get_beats_per_bar (P_TEMPO_TRACK);
  pos->beat_type =
    (float)
    tempo_track_get_beat_unit (P_TEMPO_TRACK);
  pos->ticks_per_beat =
    TRANSPORT->ticks_per_beat;
  pos->beats_per_minute =
    (double)
    tempo_track_get_current_bpm (P_TEMPO_TRACK);
}

/**
 * JACK calls this shutdown_callback if the server
 * ever shuts down or decides to disconnect the
 * client.
 */
static void
shutdown_cb (void *arg)
{
  g_warning ("Jack shutting down...");

  if (ZRYTHM_HAVE_UI && MAIN_WINDOW)
    {
      char * msg = _("JACK has shut down");
      ui_show_error_message (
        MAIN_WINDOW, false, msg);
    }
}

/**
 * Sets up the MIDI engine to use jack.
 */
int
engine_jack_midi_setup (
  AudioEngine * self)
{
  g_message ("%s: Setting up JACK MIDI", __func__);

  /* TODO: case 1 - no jack client (using another
   * backend)
   *
   * create a client and attach midi. */

  /* case 2 - jack client exists, just attach to
   * it */
  self->midi_buf_size = 4096;
#ifdef HAVE_JACK_PORT_TYPE_GET_BUFFER_SIZE
  self->midi_buf_size =
    jack_port_type_get_buffer_size (
      self->client, JACK_DEFAULT_MIDI_TYPE);
#endif

  return 0;
}

/**
 * Updates the JACK Transport type.
 */
void
engine_jack_set_transport_type (
  AudioEngine * self,
  AudioEngineJackTransportType type)
{
  if (!ZRYTHM_TESTING)
    {
      g_settings_set_enum (
        S_UI, "jack-transport-type", type);
    }

  /* release timebase master if held */
  if (self->transport_type ==
        AUDIO_ENGINE_JACK_TIMEBASE_MASTER &&
      type !=
        AUDIO_ENGINE_JACK_TIMEBASE_MASTER)
    {
      g_message ("releasing JACK timebase");
      jack_release_timebase (self->client);
    }
  /* acquire timebase master */
  else if (self->transport_type !=
             AUDIO_ENGINE_JACK_TIMEBASE_MASTER &&
           type ==
             AUDIO_ENGINE_JACK_TIMEBASE_MASTER)
    {
      g_message ("becoming JACK timebase master");
      jack_set_timebase_callback (
        self->client, 0, timebase_cb, self);
    }

  g_message (
    "set JACK transport type to %s",
    jack_transport_type_strings[type].str);
  self->transport_type = type;
}

/**
 * Tests if JACK is working properly.
 *
 * Returns 0 if ok, non-null if has errors.
 *
 * If win is not null, it displays error messages
 * to it.
 */
int
engine_jack_test (
  GtkWindow * win)
{
  const char *client_name = PROGRAM_NAME;
  const char *server_name = NULL;
  jack_options_t options = JackNoStartServer;
  jack_status_t status;

  /* open a client connection to the JACK server */
  jack_client_t * client =
    jack_client_open (
      client_name, options, &status, server_name);

  if (client)
    {
      jack_client_close (client);
      return 0;
    }
  else
    {
      char msg[500];
      engine_jack_get_error_message (
        status, msg);
      char full_msg[800];
      sprintf (
        full_msg, "JACK Error: %s", msg);
      ui_show_error_message (win, false, full_msg);
      return 1;
    }

  return 0;
}

/**
 * Sets up the audio engine to use jack.
 */
int
engine_jack_setup (
  AudioEngine * self)
{
  g_message ("Setting up JACK...");

  const char *client_name = PROGRAM_NAME;
  const char *server_name = NULL;
  jack_options_t options = JackNoStartServer;
  jack_status_t status;

  /* open a client connection to the JACK server */
  self->client =
    jack_client_open (
      client_name, options, &status, server_name);

  if (!self->client)
    {
      char msg[600];
      engine_jack_get_error_message (
        status, msg);
      char full_msg[800];
      sprintf (
        full_msg, "JACK Error: %s", msg);
      ui_show_error_message (
        NULL, false, full_msg);

      return -1;
    }

  /* Set audio engine properties */
  self->sample_rate =
    jack_get_sample_rate (self->client);
  self->block_length =
    jack_get_buffer_size (self->client);

  /* set jack callbacks */
  jack_set_process_callback (
    self->client, process_cb, self);
  jack_set_buffer_size_callback (
    self->client,
    (JackBufferSizeCallback) buffer_size_cb, self);
  jack_set_sample_rate_callback (
    self->client,
    (JackSampleRateCallback) sample_rate_cb,
    self);
  jack_set_xrun_callback (
    self->client, (JackXRunCallback) xrun_cb, self);
  jack_on_shutdown (
    self->client, shutdown_cb, self);
  /*jack_set_latency_callback(client, &jack_latency_cb, arg);*/
#ifdef JALV_JACK_SESSION
  /*jack_set_session_callback(client, &jack_session_cb, arg);*/
#endif

  engine_jack_set_transport_type (
    self,
    g_settings_get_enum (
      S_UI, "jack-transport-type"));

  g_message ("JACK set up");
  return 0;
}

/**
 * Copies the error message corresponding to \p
 * status in \p msg.
 */
void
engine_jack_get_error_message (
  jack_status_t status,
  char *        msg)
{
  if (status & JackFailure)
    {
      strcpy (
        msg,
      /* TRANSLATORS: JACK failure messages */
        _("Overall operation failed"));
    }
  else if (status & JackInvalidOption)
    {
      strcpy (
        msg,
        _("The operation contained an invalid or "
        "unsupported option"));
    }
  else if (status & JackNameNotUnique)
    {
      strcpy (
        msg,
        _("The desired client name was not unique"));
    }
  else if (status & JackServerFailed)
    {
      strcpy (
        msg,
        _("Unable to connect to the JACK server"));
    }
  else if (status & JackServerError)
    {
      strcpy (
        msg,
        _("Communication error with the JACK server"));
    }
  else if (status & JackNoSuchClient)
    {
      strcpy (
        msg,
        _("Requested client does not exist"));
    }
  else if (status & JackLoadFailure)
    {
      strcpy (
        msg,
        _("Unable to load internal client"));
    }
  else if (status & JackInitFailure)
    {
      strcpy (
        msg,
        _("Unable to initialize client"));
    }
  else if (status & JackShmFailure)
    {
      strcpy (
        msg,
        _("Unable to access shared memory"));
    }
  else if (status & JackVersionError)
    {
      strcpy (
        msg,
        _("Client's protocol version does not match"));
    }
  else if (status & JackBackendError)
    {
      strcpy (msg, _("Backend error"));
    }
  else if (status & JackClientZombie)
    {
      strcpy (msg, _("Client zombie"));
    }
  else
    g_warn_if_reached ();
}

void
engine_jack_tear_down (
  AudioEngine * self)
{
  jack_client_close (self->client);
  self->client = NULL;

  /* init semaphore */
  zix_sem_init (
    &self->port_operation_lock, 1);
}

/**
 * Fills the external out bufs.
 */
void
engine_jack_fill_out_bufs (
  AudioEngine *   self,
  const nframes_t nframes)
{
  /*g_message ("filling out bufs");*/
  /*g_message ("%p", self->hw_stereo_outs[0]);*/
  /*float * l_buf =*/
    /*ext_port_get_buffer (*/
      /*self->hw_stereo_outs[0], nframes);*/
  /*float * r_buf =*/
    /*ext_port_get_buffer (*/
      /*self->hw_stereo_outs[1], nframes);*/

  /*for (int i = 0; i < nframes; i++)*/
    /*{*/
      /*if (i == 50)*/
        /*g_message ("before lbuf %f", l_buf[i]);*/
      /*l_buf[i] = self->monitor_out->l->buf[i];*/
      /*if (i == 50)*/
        /*g_message ("after lbuf %f", l_buf[i]);*/
      /*r_buf[i] = self->monitor_out->r->buf[i];*/
    /*}*/
}

/**
 * Disconnects and reconnects the monitor output
 * port to the selected devices.
 */
int
engine_jack_reconnect_monitor (
  AudioEngine * self,
  bool          left)
{
  gchar ** devices =
    g_settings_get_strv (
      S_MONITOR, left ? "l-devices" : "r-devices");

  Port * port =
    left ?
      self->monitor_out->l : self->monitor_out->r;

  /* disconnect port */
  int ret =
    jack_port_disconnect (
      self->client, JACK_PORT_T (port->data));
  if (ret)
    {
      char msg[600];
      engine_jack_get_error_message (ret, msg);
      g_critical (
        "failed to disconnect monitor out: %s",
        msg);
      return ret;
    }

  int i = 0;
  int num_connected = 0;
  while (devices[i])
    {
      char * device = devices[i++];
      ExtPort * ext_port =
        hardware_processor_find_ext_port (
          self->hw_out_processor, device);
      if (ext_port)
        {
          /*g_return_val_if_reached (-1);*/
          ret =
            jack_connect (
              self->client,
              jack_port_name (
                JACK_PORT_T (port->data)),
              ext_port->full_name);
          if (ret)
            {
              char msg[600];
              engine_jack_get_error_message (ret, msg);
              g_warning (
                "cannot connect monitor out: %s",
                msg);
            }
          else
            {
              num_connected++;
            }
        }
    }

  if (devices)
    {
      g_strfreev (devices);
    }

  /* if nothing connected, attempt to connect to
   * first port found */
  if (num_connected == 0)
    {
      const char **ports =
        jack_get_ports (
          self->client, NULL,
          JACK_DEFAULT_AUDIO_TYPE,
          JackPortIsPhysical | JackPortIsInput);
      if (ports == NULL ||
          ports[0] == NULL ||
          ports[1] == NULL)
        {
          g_critical (
            "no physical playback ports found");
          return -1;
        }

      ret =
        jack_connect (
          self->client,
          jack_port_name (
            JACK_PORT_T (port->data)),
          left ? ports[0] : ports[1]);
      if (ret)
        {
          char msg[600];
          engine_jack_get_error_message (ret, msg);
          g_warning (
            "cannot connect monitor out: %s",
            msg);
        }
      else
        {
          num_connected++;
        }

      jack_free (ports);
    }

  return num_connected == 0;
}

int
engine_jack_activate (
  AudioEngine * self,
  bool          activate)
{
  if (activate)
    {
      /* Tell the JACK server that we are ready to
       * roll.  Our process() callback will start
       * running now. */
      if (jack_activate (self->client))
        {
          g_warning ("cannot activate client");
          return -1;
        }
      g_message ("Jack activated");

      /* Connect the ports.  You can't do this before
       * the client is
       * activated, because we can't make connections
       * to clients
       * that aren't running.  Note the confusing (but
       * necessary)
       * orientation of the driver backend ports:
       * playback ports are
       * "input" to the backend, and capture ports are
       * "output" from
       * it.
       */

      g_return_val_if_fail (
        self->monitor_out->l->data &&
          self->monitor_out->r->data,
        -1);

      g_message (
        "connecting to system out ports...");

      int ret =
        engine_jack_reconnect_monitor (self, true);
      if (ret)
        {
          return -1;
        }
      ret =
        engine_jack_reconnect_monitor (
          self, false);
      if (ret)
        {
          return -1;
        }
    }
  else
    {
      int ret = jack_deactivate (self->client);
      if (ret != 0)
        {
          char msg[600];
          engine_jack_get_error_message (
            ret, msg);
          g_critical (
            "Failed deactivating JACK client: %s",
            msg);
        }
    }

  return 0;
}

/**
 * Returns the JACK type string.
 */
const char *
engine_jack_get_jack_type (
  PortType type)
{
  switch (type)
    {
    case TYPE_AUDIO:
      return JACK_DEFAULT_AUDIO_TYPE;
      break;
    case TYPE_EVENT:
      return JACK_DEFAULT_MIDI_TYPE;
      break;
    default:
      return NULL;
    }
}

/**
 * Returns if this is a pipewire session.
 */
bool
engine_jack_is_pipewire (
  AudioEngine * self)
{
#if defined (_WOE32) || defined (__APPLE__)
  return false;
#else
  const char * libname = "libjack.so.0";
  void * lib_handle =
    dlopen (libname, RTLD_LAZY);
  g_return_val_if_fail (lib_handle, false);

  const char * func_name = "jack_get_version_string";

  const char * (*jack_get_version_string)(void);
  * (void **) (&jack_get_version_string) =
    dlsym (lib_handle, func_name);
  if (!jack_get_version_string)
    {
      g_message (
        "%s () not found in %s", func_name, libname);
      return false;
    }
  else
    {
      g_message (
        "%s () found in %s", func_name,
        libname);
      const char * ver =
        (*jack_get_version_string) ();
      g_message ("ver %s", ver);
      return
        string_contains_substr (ver, "PipeWire");
    }
#endif
}

#endif /* HAVE_JACK */
