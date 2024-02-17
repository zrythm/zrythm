// SPDX-FileCopyrightText: © 2018-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#ifdef HAVE_JACK

#  include <math.h>

#  include "dsp/channel.h"
#  include "dsp/engine.h"
#  include "dsp/engine_jack.h"
#  include "dsp/ext_port.h"
#  include "dsp/port.h"
#  include "dsp/router.h"
#  include "dsp/tempo_track.h"
#  include "dsp/transport.h"
#  include "gui/widgets/main_window.h"
#  include "plugins/lv2_plugin.h"
#  include "plugins/plugin.h"
#  include "project.h"
#  include "settings/settings.h"
#  include "utils/backtrace.h"
#  include "utils/error.h"
#  include "utils/midi.h"
#  include "utils/mpmc_queue.h"
#  include "utils/object_pool.h"
#  include "utils/string.h"
#  include "utils/ui.h"
#  include "zrythm_app.h"

#  include <glib/gi18n.h>
#  include <gtk/gtk.h>

#  include <jack/thread.h>

typedef enum
{
  Z_AUDIO_ENGINE_JACK_ERROR_FAILED,
  Z_AUDIO_ENGINE_JACK_ERROR_NO_PHYSICAL_PORTS,
  Z_AUDIO_ENGINE_JACK_ERROR_CONNECTION_CHANGE_FAILED,
} ZAudioEngineJackError;

#  define Z_AUDIO_ENGINE_JACK_ERROR z_audio_engine_jack_error_quark ()
GQuark
z_audio_engine_jack_error_quark (void);
G_DEFINE_QUARK (
  z - audio - engine - jack - error - quark,
  z_audio_engine_jack_error)

/**
 * Refreshes the list of external ports.
 */
void
engine_jack_rescan_ports (AudioEngine * self)
{
  /* get all input ports */
  const char ** ports =
    jack_get_ports (self->client, NULL, NULL, JackPortIsPhysical);

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

void
engine_jack_handle_sample_rate_change (AudioEngine * self, uint32_t samplerate)
{
  AUDIO_ENGINE->sample_rate = samplerate;

  if (P_TEMPO_TRACK)
    {
      int beats_per_bar = tempo_track_get_beats_per_bar (P_TEMPO_TRACK);
      engine_update_frames_per_tick (
        self, beats_per_bar, tempo_track_get_current_bpm (P_TEMPO_TRACK),
        self->sample_rate, true, true, false);
    }

  g_message ("JACK: Sample rate changed to %d", samplerate);
}

static void
process_change_request (AudioEngine * self)
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
        cur_time > self->last_events_process_started
        || cur_time > self->last_events_processed)
        {
          g_message ("-------- waiting for change");
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
sample_rate_cb (uint32_t nframes, AudioEngine * self)
{
#  if 0
  g_atomic_int_set (
    &AUDIO_ENGINE->changing_sample_rate, 1);
#  endif

  ENGINE_EVENTS_PUSH (AUDIO_ENGINE_EVENT_SAMPLE_RATE_CHANGE, NULL, nframes, 0.f);

  process_change_request (self);

#  if 0
  g_atomic_int_set (
    &AUDIO_ENGINE->changing_sample_rate, 0);
#  endif

  return 0;
}

void
engine_jack_handle_buf_size_change (AudioEngine * self, uint32_t frames)
{
  engine_realloc_port_buffers (AUDIO_ENGINE, frames);
#  ifdef HAVE_JACK_PORT_TYPE_GET_BUFFER_SIZE
  AUDIO_ENGINE->midi_buf_size = jack_port_type_get_buffer_size (
    AUDIO_ENGINE->client, JACK_DEFAULT_MIDI_TYPE);
#  endif
  g_message (
    "JACK: Block length changed to %d, "
    "midi buf size to %zu",
    AUDIO_ENGINE->block_length, AUDIO_ENGINE->midi_buf_size);
  g_atomic_int_set (&AUDIO_ENGINE->handled_jack_buffer_size_change, 1);
}

/** Jack buffer size callback. */
int
engine_jack_buffer_size_cb (uint32_t nframes, AudioEngine * self)
{
  g_message ("JACK buffer size changed: %u", nframes);
  g_atomic_int_set (&AUDIO_ENGINE->handled_jack_buffer_size_change, 0);
  ENGINE_EVENTS_PUSH (AUDIO_ENGINE_EVENT_BUFFER_SIZE_CHANGE, NULL, nframes, 0.f);

  /* process immediately if gtk thread */
  if (g_thread_self () == zrythm_app->gtk_thread)
    {
      engine_process_events (self);
    }
  /* otherwise if activated wait for gtk thread
   * to process all events */
  else if (self->activated && engine_get_run (self))
    {
      while (
        g_atomic_int_get (&AUDIO_ENGINE->handled_jack_buffer_size_change) == 0)
        {
          g_message (
            "-- waiting for engine to handle JACK buffer size change on GUI thread... (engine_process_events)");
          g_usleep (1000000);
        }
    }

  return 0;
}

void
engine_jack_handle_position_change (AudioEngine * self)
{
  if (self->transport_type == AUDIO_ENGINE_JACK_TIMEBASE_MASTER)
    {
      jack_transport_locate (self->client, (jack_nframes_t) PLAYHEAD->frames);
    }
}

void
engine_jack_handle_start (AudioEngine * self)
{
  if (self->transport_type == AUDIO_ENGINE_JACK_TIMEBASE_MASTER)
    {
      jack_transport_start (self->client);
    }
}

void
engine_jack_handle_stop (AudioEngine * self)
{
  if (self->transport_type == AUDIO_ENGINE_JACK_TIMEBASE_MASTER)
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
engine_jack_prepare_process (AudioEngine * self)
{
  /* if client, get transport position info */
  if (self->transport_type == AUDIO_ENGINE_JACK_TRANSPORT_CLIENT)
    {
      jack_position_t        pos;
      jack_transport_state_t state = jack_transport_query (self->client, &pos);

      if (state == JackTransportRolling)
        {
          TRANSPORT->play_state = PLAYSTATE_ROLLING;
        }
      else if (state == JackTransportStopped)
        {
          TRANSPORT->play_state = PLAYSTATE_PAUSED;
        }

      /* get position from timebase master */
      Position tmp;
      position_from_frames (&tmp, pos.frame);
      transport_set_playhead_pos (TRANSPORT, &tmp);

      /* BBT and BPM changes */
      if (pos.valid & JackPositionBBT)
        {
          tempo_track_set_beats_per_bar (P_TEMPO_TRACK, (int) pos.beats_per_bar);
          tempo_track_set_bpm (
            P_TEMPO_TRACK, (float) pos.beats_per_minute,
            (float) pos.beats_per_minute, true, true);
          tempo_track_set_beat_unit (P_TEMPO_TRACK, (int) pos.beat_type);
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
process_cb (nframes_t nframes, void * data)
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
xrun_cb (AudioEngine * self)
{
  gint64 cur_time = g_get_monotonic_time ();
  if (cur_time - self->last_xrun_notification > 6000000)
    {
      /* TODO make a notification message queue */
#  if 0
      g_message (
        "%s",
        _("XRUN occurred - check your JACK "
        "configuration"));
#  endif
      self->last_xrun_notification = cur_time;
    }

  return 0;
}

static void
timebase_cb (
  jack_transport_state_t state,
  jack_nframes_t         nframes,
  jack_position_t *      pos,
  int                    new_pos,
  void *                 arg)
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
    position_get_sixteenths (PLAYHEAD, true) * TICKS_PER_SIXTEENTH_NOTE
    + (int) floor (position_get_ticks (PLAYHEAD));
  Position bar_start;
  position_set_to_bar (&bar_start, position_get_bars (PLAYHEAD, true));
  pos->bar_start_tick = (double) (PLAYHEAD->ticks - bar_start.ticks);
  pos->beats_per_bar = (float) tempo_track_get_beats_per_bar (P_TEMPO_TRACK);
  pos->beat_type = (float) tempo_track_get_beat_unit (P_TEMPO_TRACK);
  pos->ticks_per_beat = TRANSPORT->ticks_per_beat;
  pos->beats_per_minute = (double) tempo_track_get_current_bpm (P_TEMPO_TRACK);
}

/**
 * JACK calls this shutdown_callback if the server
 * ever shuts down or decides to disconnect the
 * client.
 */
static void
shutdown_cb (void * arg)
{
  g_warning ("Jack shutting down...");

  if (ZRYTHM_HAVE_UI && MAIN_WINDOW)
    {
      char * msg = _ ("JACK has shut down");
      ui_show_error_message (false, msg);
    }
}

static void
freewheel_cb (int starting, void * arg)
{
  if (starting)
    {
      g_message ("JACK: starting freewheel");
    }
  else
    {
      g_message ("JACK: stopping freewheel");
    }
}

static void
port_registration_cb (jack_port_id_t port_id, int registered, void * arg)
{
  AudioEngine * self = (AudioEngine *) arg;

  jack_port_t * jport = jack_port_by_id (self->client, port_id);
  g_message (
    "JACK: port '%s' %sregistered", jack_port_name (jport),
    registered ? "" : "un");
}

static void
port_connect_cb (jack_port_id_t a, jack_port_id_t b, int connect, void * arg)
{
  AudioEngine * self = (AudioEngine *) arg;

  jack_port_t * jport_a = jack_port_by_id (self->client, a);
  jack_port_t * jport_b = jack_port_by_id (self->client, b);
  g_message (
    "JACK: port '%s' %sconnected %s '%s'", jack_port_name (jport_a),
    connect ? "" : "dis", connect ? "to" : "from", jack_port_name (jport_b));

  /* if port was disconnected from one of Zrythm's
   * tracked hardware ports, set it as deactivated
   * so it can be force-activated next scan */
  if (!connect)
    {
      ExtPort * tmp = ext_port_new_from_jack_port (jport_a);
      char *    id = ext_port_get_id (tmp);
      ExtPort * ext_port =
        hardware_processor_find_ext_port (HW_IN_PROCESSOR, id);
      if (ext_port)
        {
          g_message ("setting '%s' to pending reconnect", id);
          ext_port->pending_reconnect = true;
        }
      g_free (id);
      ext_port_free (tmp);
    }
}

/**
 * Sets up the MIDI engine to use jack.
 */
int
engine_jack_midi_setup (AudioEngine * self)
{
  g_message ("%s: Setting up JACK MIDI", __func__);

  /* TODO: case 1 - no jack client (using another
   * backend)
   *
   * create a client and attach midi. */

  /* case 2 - jack client exists, just attach to
   * it */
  self->midi_buf_size = 4096;
#  ifdef HAVE_JACK_PORT_TYPE_GET_BUFFER_SIZE
  self->midi_buf_size =
    jack_port_type_get_buffer_size (self->client, JACK_DEFAULT_MIDI_TYPE);
#  endif

  return 0;
}

/**
 * Updates the JACK Transport type.
 */
void
engine_jack_set_transport_type (
  AudioEngine *                self,
  AudioEngineJackTransportType type)
{
  if (!ZRYTHM_TESTING)
    {
      g_settings_set_enum (S_UI, "jack-transport-type", type);
    }

  /* release timebase master if held */
  if (
    self->transport_type == AUDIO_ENGINE_JACK_TIMEBASE_MASTER
    && type != AUDIO_ENGINE_JACK_TIMEBASE_MASTER)
    {
      g_message ("releasing JACK timebase");
      jack_release_timebase (self->client);
    }
  /* acquire timebase master */
  else if (
    self->transport_type != AUDIO_ENGINE_JACK_TIMEBASE_MASTER
    && type == AUDIO_ENGINE_JACK_TIMEBASE_MASTER)
    {
      g_message ("becoming JACK timebase master");
      jack_set_timebase_callback (self->client, 0, timebase_cb, self);
    }

  g_message (
    "set JACK transport type to %s", jack_transport_type_strings[type].str);
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
engine_jack_test (GtkWindow * win)
{
  const char *   client_name = PROGRAM_NAME;
  const char *   server_name = NULL;
  jack_options_t options = JackNoStartServer;
  jack_status_t  status;

  /* open a client connection to the JACK server */
  jack_client_t * client =
    jack_client_open (client_name, options, &status, server_name);

  if (client)
    {
      jack_client_close (client);
      return 0;
    }
  else
    {
      char msg[500];
      engine_jack_get_error_message (status, msg);
      if (win)
        {
          ui_show_message_full (GTK_WIDGET (win), _ ("JACK Error"), "%s", msg);
        }
      else
        {
          g_warning ("JACK Error: %s", msg);
        }
      return 1;
    }

  return 0;
}

/**
 * Sets up the audio engine to use jack.
 */
int
engine_jack_setup (AudioEngine * self)
{
  g_message ("Setting up JACK...");

  const char *   client_name = PROGRAM_NAME;
  const char *   server_name = NULL;
  jack_options_t options = JackNoStartServer;
  if (ZRYTHM_TESTING)
    {
      server_name = "zrythm-pipewire-0";
      options = options | JackServerName;
    }
  jack_status_t status;

  /* open a client connection to the JACK server */
  self->client = jack_client_open (client_name, options, &status, server_name);

  if (!self->client)
    {
      char msg[600];
      engine_jack_get_error_message (status, msg);
      char full_msg[800];
      sprintf (full_msg, "JACK Error: %s", msg);
      ui_show_error_message (false, full_msg);

      return -1;
    }

  /* set jack callbacks */
  int ret;
  jack_set_process_callback (self->client, process_cb, self);
  g_atomic_int_set (&self->handled_jack_buffer_size_change, 1);
  jack_set_buffer_size_callback (
    self->client, (JackBufferSizeCallback) engine_jack_buffer_size_cb, self);
  jack_set_sample_rate_callback (
    self->client, (JackSampleRateCallback) sample_rate_cb, self);
  jack_set_xrun_callback (self->client, (JackXRunCallback) xrun_cb, self);
  jack_set_freewheel_callback (self->client, freewheel_cb, self);
  ret = jack_set_port_registration_callback (
    self->client, port_registration_cb, self);
  g_return_val_if_fail (ret == 0, -1);
  ret = jack_set_port_connect_callback (self->client, port_connect_cb, self);
  g_return_val_if_fail (ret == 0, -1);
  jack_on_shutdown (self->client, shutdown_cb, self);
  /*jack_set_latency_callback(client, &jack_latency_cb, arg);*/
#  ifdef JALV_JACK_SESSION
  /*jack_set_session_callback(client, &jack_session_cb, arg);*/
#  endif

  /* Set audio engine properties */
  self->sample_rate = jack_get_sample_rate (self->client);
  self->block_length = jack_get_buffer_size (self->client);
  g_message (
    "jack sample rate %u, block length %u", self->sample_rate,
    self->block_length);

  engine_jack_set_transport_type (
    self,
    ZRYTHM_TESTING
      ? AUDIO_ENGINE_JACK_TRANSPORT_CLIENT
      : g_settings_get_enum (S_UI, "jack-transport-type"));

  g_message ("JACK set up");
  return 0;
}

/**
 * Copies the error message corresponding to \p
 * status in \p msg.
 */
void
engine_jack_get_error_message (jack_status_t status, char * msg)
{
  if (status & JackFailure)
    {
      strcpy (
        msg,
        /* TRANSLATORS: JACK failure messages */
        _ ("Overall operation failed"));
    }
  else if (status & JackInvalidOption)
    {
      strcpy (
        msg,
        _ ("The operation contained an invalid or "
           "unsupported option"));
    }
  else if (status & JackNameNotUnique)
    {
      strcpy (msg, _ ("The desired client name was not unique"));
    }
  else if (status & JackServerFailed)
    {
      strcpy (msg, _ ("Unable to connect to the JACK server"));
    }
  else if (status & JackServerError)
    {
      strcpy (msg, _ ("Communication error with the JACK server"));
    }
  else if (status & JackNoSuchClient)
    {
      strcpy (msg, _ ("Requested client does not exist"));
    }
  else if (status & JackLoadFailure)
    {
      strcpy (msg, _ ("Unable to load internal client"));
    }
  else if (status & JackInitFailure)
    {
      strcpy (msg, _ ("Unable to initialize client"));
    }
  else if (status & JackShmFailure)
    {
      strcpy (msg, _ ("Unable to access shared memory"));
    }
  else if (status & JackVersionError)
    {
      strcpy (msg, _ ("Client's protocol version does not match"));
    }
  else if (status & JackBackendError)
    {
      strcpy (msg, _ ("Backend error"));
    }
  else if (status & JackClientZombie)
    {
      strcpy (msg, _ ("Client zombie"));
    }
  else
    g_warn_if_reached ();
}

void
engine_jack_tear_down (AudioEngine * self)
{
  jack_client_close (self->client);
  self->client = NULL;

  /* init semaphore */
  zix_sem_init (&self->port_operation_lock, 1);
}

/**
 * Disconnects and reconnects the monitor output
 * port to the selected devices.
 *
 * @return Whether successful.
 */
bool
engine_jack_reconnect_monitor (AudioEngine * self, bool left, GError ** error)
{
  if (ZRYTHM_TESTING)
    return true;

  gchar ** devices =
    g_settings_get_strv (S_MONITOR, left ? "l-devices" : "r-devices");

  Port * port = left ? self->monitor_out->l : self->monitor_out->r;

  /* disconnect port */
  int ret = jack_port_disconnect (self->client, JACK_PORT_T (port->data));
  if (ret)
    {
      char msg[600];
      engine_jack_get_error_message (ret, msg);
      g_set_error (
        error, Z_AUDIO_ENGINE_JACK_ERROR,
        Z_AUDIO_ENGINE_JACK_ERROR_CONNECTION_CHANGE_FAILED,
        _ ("JACK: Failed to disconnect monitor out: %s"), msg);
      return false;
    }

  int  i = 0;
  int  num_connected = 0;
  char jack_msg[600];
  while (devices[i])
    {
      char *    device = devices[i++];
      ExtPort * ext_port =
        hardware_processor_find_ext_port (self->hw_out_processor, device);
      if (ext_port)
        {
          /*g_return_val_if_reached (-1);*/
          ret = jack_connect (
            self->client, jack_port_name (JACK_PORT_T (port->data)),
            ext_port->full_name);
          if (ret)
            {
              engine_jack_get_error_message (ret, jack_msg);
              g_warning ("cannot connect monitor out: %s", jack_msg);
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
      const char ** ports = jack_get_ports (
        self->client, NULL, JACK_DEFAULT_AUDIO_TYPE,
        JackPortIsPhysical | JackPortIsInput);
      if (ports == NULL || ports[0] == NULL || ports[1] == NULL)
        {
          g_set_error (
            error, Z_AUDIO_ENGINE_JACK_ERROR,
            Z_AUDIO_ENGINE_JACK_ERROR_NO_PHYSICAL_PORTS, "%s",
            _ ("JACK: No physical playback ports found"));
          return false;
        }

      ret = jack_connect (
        self->client, jack_port_name (JACK_PORT_T (port->data)),
        left ? ports[0] : ports[1]);
      if (ret)
        {
          engine_jack_get_error_message (ret, jack_msg);
          g_set_error (
            error, Z_AUDIO_ENGINE_JACK_ERROR,
            Z_AUDIO_ENGINE_JACK_ERROR_CONNECTION_CHANGE_FAILED,
            _ ("JACK: Failed to connect monitor output [%s]: %s"),
            left ? _ ("left") : _ ("right"), jack_msg);
          return false;
        }
      else
        {
          num_connected++;
        }

      jack_free (ports);
    }

  g_return_val_if_fail (num_connected > 0, false);

  return true;
}

int
engine_jack_activate (AudioEngine * self, bool activate)
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
        self->monitor_out->l->data && self->monitor_out->r->data, -1);

      g_message ("connecting to system out ports...");

      GError * err = NULL;
      bool     ret = engine_jack_reconnect_monitor (self, true, &err);
      if (!ret)
        {
          HANDLE_ERROR (
            err, "%s", _ ("Failed to connect to left monitor output port"));
          return -1;
        }
      ret = engine_jack_reconnect_monitor (self, false, &err);
      if (!ret)
        {
          HANDLE_ERROR (
            err, "%s", _ ("Failed to connect to right monitor output port"));
          return -1;
        }
    }
  else
    {
      int ret = jack_deactivate (self->client);
      if (ret != 0)
        {
          char msg[600];
          engine_jack_get_error_message (ret, msg);
          g_critical ("Failed deactivating JACK client: %s", msg);
        }
    }

  return 0;
}

/**
 * Returns the JACK type string.
 */
const char *
engine_jack_get_jack_type (PortType type)
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
engine_jack_is_pipewire (AudioEngine * self)
{
#  if defined(_WOE32) || defined(__APPLE__)
  return false;
#  else
  const char * libname = "libjack.so.0";
  void *       lib_handle = dlopen (libname, RTLD_LAZY);
  g_return_val_if_fail (lib_handle, false);

  const char * func_name = "jack_get_version_string";

  const char * (*jack_get_version_string) (void);
  *(void **) (&jack_get_version_string) = dlsym (lib_handle, func_name);
  if (!jack_get_version_string)
    {
      g_message ("%s () not found in %s", func_name, libname);
      return false;
    }
  else
    {
      g_message ("%s () found in %s", func_name, libname);
      const char * ver = (*jack_get_version_string) ();
      g_message ("ver %s", ver);
      return string_contains_substr (ver, "PipeWire");
    }
#  endif
}

#endif /* HAVE_JACK */
