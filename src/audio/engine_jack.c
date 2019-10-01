/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "config.h"

#ifdef HAVE_JACK

#include "audio/channel.h"
#include "audio/engine.h"
#include "audio/engine_jack.h"
#include "audio/ext_port.h"
#include "audio/midi.h"
#include "audio/mixer.h"
#include "audio/port.h"
#include "audio/routing.h"
#include "audio/transport.h"
#include "gui/widgets/main_window.h"
#include "plugins/plugin.h"
#include "plugins/lv2_plugin.h"
#include "project.h"
#include "utils/ui.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include <jack/metadata.h>
#include <jack/statistics.h>

/**
 * Adds a port to the array if it doesn't already
 * exist.
 */
/*static void*/
/*add_port_if_not_exists (*/
  /*Port **       ports,*/
  /*int *         num_ports,*/
  /*jack_port_t * jport)*/
/*{*/
  /*int found = 0;*/
  /*Port * port = NULL;*/
  /*const char * jport_name =*/
    /*jack_port_short_name (jport);*/
  /*for (int i = 0; i < * num_ports; i++)*/
    /*{*/
      /*port = ports[i];*/

      /*if (g_strstr_len (*/
            /*port->identifier.label,*/
            /*(gssize)*/
              /*strlen (port->identifier.label),*/
            /*jport_name))*/
        /*{*/
          /*found = 1;*/
        /*}*/
    /*}*/

  /*if (found)*/
    /*return;*/

  /*PortType type;*/
  /*PortFlow flow;*/

  /*const char * jtype =*/
    /*jack_port_type (jport);*/
  /*if (g_strcmp0 (*/
        /*jtype, JACK_DEFAULT_AUDIO_TYPE))*/
    /*type = TYPE_EVENT;*/
  /*else if (g_strcmp0 (*/
        /*jtype, JACK_DEFAULT_MIDI_TYPE))*/
    /*type = TYPE_AUDIO;*/
  /*else*/
    /*return;*/

  /*int jflags = jack_port_flags (jport);*/
  /*if (jflags & JackPortIsInput)*/
    /*flow = FLOW_INPUT;*/
  /*else if (jflags & JackPortIsOutput)*/
    /*flow = FLOW_OUTPUT;*/
  /*else*/
    /*return;*/

  /*port = port_new_with_data (*/
    /*INTERNAL_JACK_PORT,*/
    /*type,*/
    /*flow,*/
    /*jport_name,*/
    /*(void *) jport);*/

  /*ports[(*num_ports)++] = port;*/
/*}*/

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

  if(!ports) return;

  /* get selected MIDI devices */
  char ** devices =
    g_settings_get_strv (
      S_PREFERENCES,
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
              /*if (jack_connect (*/
                    /*self->client,*/
                    /*pname,*/
                    /*jack_port_name (*/
                      /*JACK_PORT_T (*/
                        /*self->midi_in->data))))*/
                /*g_warning (*/
                  /*"Failed connecting %s to %s",*/
                  /*pname,*/
                  /*self->midi_in->identifier.label);*/


              break;
            }
          j++;
        }
      i++;
    }
    jack_free(ports);
    g_strfreev(devices);
}

/** Jack sample rate callback. */
static int
sample_rate_cb (
  uint32_t nframes,
  AudioEngine * self)
{
  AUDIO_ENGINE->sample_rate = nframes;

  engine_update_frames_per_tick (
    self,
    TRANSPORT->beats_per_bar,
    TRANSPORT->bpm,
    AUDIO_ENGINE->sample_rate);

  g_message (
    "JACK: Sample rate changed to %d", nframes);
  return 0;
}

/** Jack buffer size callback. */
static int
buffer_size_cb (uint32_t nframes,
                     void *   data)
{
  engine_realloc_port_buffers (
    AUDIO_ENGINE, nframes);
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

  return 0;
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

      /* BBT */
      if (pos.valid & JackPositionBBT)
        {
          TRANSPORT->beats_per_bar =
            (int) pos.beats_per_bar;
          transport_set_bpm (
            TRANSPORT,
            (float) pos.beats_per_minute);
          transport_set_beat_unit (
            TRANSPORT, (int) pos.beat_type);
        }
    }

  /* clear output */
}

/**
 * The process callback for this JACK application is
 * called in a special realtime thread once for each audio
 * cycle.
 */
static int
process_cb (
  nframes_t nframes, ///< the number of frames to fill
  void *    data) ///< user data
{
  return
    engine_process (
      (AudioEngine *) data, nframes);
}

static int
xrun_cb (void *arg)
{
  ui_show_notification_idle ("XRUN occurred");

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
  /*AudioEngine * self = (AudioEngine *) arg;*/

  /* Mandatory fields */
  pos->valid = JackPositionBBT;
  pos->frame = (jack_nframes_t) PLAYHEAD->frames;

  /* BBT */
  pos->bar = PLAYHEAD->bars;
  pos->beat = PLAYHEAD->beats;
  pos->tick =
    PLAYHEAD->sixteenths * TICKS_PER_SIXTEENTH_NOTE +
    PLAYHEAD->ticks;
  Position bar_start;
  position_set_to_bar (
    &bar_start, PLAYHEAD->bars);
  pos->bar_start_tick =
    (double)
    (PLAYHEAD->total_ticks -
     bar_start.total_ticks);
  pos->beats_per_bar =
    (float) TRANSPORT->beats_per_bar;
  pos->beat_type =
    (float) TRANSPORT->beat_unit;
  pos->ticks_per_beat =
    TRANSPORT->ticks_per_beat;
  pos->beats_per_minute =
    (double) TRANSPORT->bpm;
}

/**
 * JACK calls this shutdown_callback if the server ever
 * shuts down or decides to disconnect the client.
 */
static void
shutdown_cb (void *arg)
{
  // TODO
  g_warning ("Jack shutting down...");
}

/**
 * Sets up the MIDI engine to use jack.
 */
int
jack_midi_setup (
  AudioEngine * self,
  int           loading)
{
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

  /*if (loading)*/
    /*{*/
      /*self->midi_in->data =*/
        /*(void *) jack_port_register (*/
          /*self->client, "MIDI_in",*/
          /*JACK_DEFAULT_MIDI_TYPE,*/
          /*JackPortIsInput, 0);*/
      /*self->midi_out->data =*/
        /*(void *) jack_port_register (*/
          /*self->client, "MIDI_out",*/
          /*JACK_DEFAULT_MIDI_TYPE,*/
          /*JackPortIsOutput, 0);*/
    /*}*/
  /*else*/
    /*{*/
      /*self->midi_in =*/
        /*port_new_with_data (*/
          /*INTERNAL_JACK_PORT,*/
          /*TYPE_EVENT,*/
          /*FLOW_INPUT,*/
          /*"JACK MIDI In",*/
          /*(void *) jack_port_register (*/
            /*self->client, "MIDI_in",*/
            /*JACK_DEFAULT_MIDI_TYPE,*/
            /*JackPortIsInput, 0));*/
      /*self->midi_in->identifier.owner_type =*/
        /*PORT_OWNER_TYPE_BACKEND;*/
      /*self->midi_out =*/
        /*port_new_with_data (*/
          /*INTERNAL_JACK_PORT,*/
          /*TYPE_EVENT,*/
          /*FLOW_OUTPUT,*/
          /*"JACK MIDI Out",*/
          /*(void *) jack_port_register (*/
            /*self->client, "MIDI_out",*/
            /*JACK_DEFAULT_MIDI_TYPE,*/
            /*JackPortIsOutput, 0));*/
      /*self->midi_out->identifier.owner_type =*/
        /*PORT_OWNER_TYPE_BACKEND;*/
    /*}*/

  /* init queue */
  /*self->midi_in->midi_events =*/
    /*midi_events_new (*/
      /*self->midi_in);*/
  /*self->midi_out->midi_events =*/
    /*midi_events_new (*/
      /*self->midi_out);*/

  /*if (!self->midi_in->data ||*/
      /*!self->midi_out->data)*/
    /*{*/
      /*g_warning ("no more JACK ports available");*/
    /*}*/

  /* autoconnect MIDI controllers */
  autoconnect_midi_controllers (
    AUDIO_ENGINE);

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
  /* release timebase master if held */
  if (self->transport_type ==
        AUDIO_ENGINE_JACK_TIMEBASE_MASTER &&
      type !=
        AUDIO_ENGINE_JACK_TIMEBASE_MASTER)
    {
      jack_release_timebase (self->client);
    }
  /* acquire timebase master */
  else if (self->transport_type !=
             AUDIO_ENGINE_JACK_TIMEBASE_MASTER &&
           type ==
             AUDIO_ENGINE_JACK_TIMEBASE_MASTER)
    {
      jack_set_timebase_callback (
        self->client, 0, timebase_cb, self);
    }

  g_message ("set to %d", type);
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
  const char *client_name = "Zrythm";
  const char *server_name = NULL;
  jack_options_t options = JackNoStartServer;
  jack_status_t status;
  char * msg;

  // open a client connection to the JACK server
  jack_client_t * client =
    jack_client_open (client_name,
                      options,
                      &status,
                      server_name);

  if (client)
    {
      jack_client_close (client);
      return 0;
    }
  else
    {
      msg =
        g_strdup_printf (
          _("JACK Error: %s"),
          engine_jack_get_error_message (
            status));
      ui_show_error_message (
        win, msg);
      g_free (msg);
      return 1;
    }

  return 0;
}

/**
 * Sets up the audio engine to use jack.
 */
int
jack_setup (AudioEngine * self,
            int           loading)
{
  g_message ("Setting up JACK...");

  const char *client_name = "Zrythm";
  const char *server_name = NULL;
  jack_options_t options = JackNoStartServer;
  jack_status_t status;

  // open a client connection to the JACK server
  self->client =
    jack_client_open (client_name,
                      options,
                      &status,
                      server_name);

  if (!self->client)
    {
      char * msg =
        g_strdup_printf (
          "JACK Error: %s",
          engine_jack_get_error_message (status));
      ui_show_error_message (
        NULL, msg);
      g_free (msg);

      return -1;
    }

  /* Set audio engine properties */
  self->sample_rate =
    jack_get_sample_rate (self->client);
  self->block_length =
    jack_get_buffer_size (self->client);
  self->transport_type =
    AUDIO_ENGINE_JACK_TIMEBASE_MASTER;

  /* set jack callbacks */
  jack_set_process_callback (
    self->client, &process_cb, self);
  jack_set_buffer_size_callback (
    self->client, &buffer_size_cb, self);
  jack_set_sample_rate_callback (
    self->client,
    (JackSampleRateCallback) sample_rate_cb,
    self);
  jack_set_xrun_callback (
    self->client, &xrun_cb, self->client);
  jack_on_shutdown (
    self->client, &shutdown_cb, self);
  jack_set_timebase_callback (
    self->client, 0, timebase_cb, self);
  /*jack_set_latency_callback(client, &jack_latency_cb, arg);*/
#ifdef JALV_JACK_SESSION
  /*jack_set_session_callback(client, &jack_session_cb, arg);*/
#endif

  /* create ports */
  Port * monitor_out_l, * monitor_out_r;

  const char * monitor_out_l_str =
    "Monitor out L";
  const char * monitor_out_r_str =
    "Monitor out R";

  if (loading)
    {
    }
  else
    {
      monitor_out_l = port_new_with_type (
        TYPE_AUDIO,
        FLOW_OUTPUT,
        monitor_out_l_str);
      monitor_out_r = port_new_with_type (
        TYPE_AUDIO,
        FLOW_OUTPUT,
        monitor_out_r_str);

      monitor_out_l->identifier.owner_type =
        PORT_OWNER_TYPE_BACKEND;
      monitor_out_r->identifier.owner_type =
        PORT_OWNER_TYPE_BACKEND;

      self->monitor_out =
        stereo_ports_new_from_existing (
          monitor_out_l, monitor_out_r);

      /* expose to jack */
      port_set_expose_to_backend (
        self->monitor_out->l, 1);
      port_set_expose_to_backend (
        self->monitor_out->r, 1);

      if (!self->monitor_out->l->data ||
          !self->monitor_out->r->data)
        {
          g_error ("no more JACK ports available");
        }
    }

  /*engine_jack_rescan_ports (self);*/

  /* get all input physical ports */
  ext_ports_get (
    TYPE_AUDIO,
    FLOW_INPUT,
    1, self->hw_stereo_outs,
    &self->num_hw_stereo_outs);

  g_message ("JACK set up");
  return 0;
}


const char *
engine_jack_get_error_message (
  jack_status_t status)
{
  const char *
    jack_failure =
    /* TRANSLATORS: JACK failure messages */
      _("Overall operation failed");
  const char *
    jack_invalid_option =
      _("The operation contained an invalid or "
      "unsupported option");
  const char *
    jack_name_not_unique =
      _("The desired client name was not unique");
  const char *
    jack_server_failed =
      _("Unable to connect to the JACK server");
  const char *
    jack_server_error =
      _("Communication error with the JACK server");
  const char *
    jack_no_such_client =
      _("Requested client does not exist");
  const char *
    jack_load_failure =
      _("Unable to load internal client");
  const char *
    jack_init_failure =
      _("Unable to initialize client");
  const char *
    jack_shm_failure =
      _("Unable to access shared memory");
  const char *
    jack_version_error =
      _("Client's protocol version does not match");
  const char *
    jack_backend_error =
      _("Backend error");
  const char *
    jack_client_zombie =
      _("Client zombie");

  if (status & JackFailure)
    return jack_failure;
  if (status & JackInvalidOption)
    return jack_invalid_option;
  if (status & JackNameNotUnique)
    return jack_name_not_unique;
  if (status & JackServerFailed)
    return jack_server_failed;
  if (status & JackServerError)
    return jack_server_error;
  if (status & JackNoSuchClient)
    return jack_no_such_client;
  if (status & JackLoadFailure)
    return jack_load_failure;
  if (status & JackInitFailure)
    return jack_init_failure;
  if (status & JackShmFailure)
    return jack_shm_failure;
  if (status & JackVersionError)
    return jack_version_error;
  if (status & JackBackendError)
    return jack_backend_error;
  if (status & JackClientZombie)
    return jack_client_zombie;
  return NULL;
}

void
jack_tear_down (
  AudioEngine * self)
{
  jack_client_close (self->client);

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

int
engine_jack_activate (
  AudioEngine * self)
{
  /* Tell the JACK server that we are ready to roll.  Our
   * process() callback will start running now. */
  if (jack_activate (self->client))
    {
      g_warning ("cannot activate client");
      return -1;
    }
  g_message ("Jack activated");

  /* Connect the ports.  You can't do this before the client is
   * activated, because we can't make connections to clients
   * that aren't running.  Note the confusing (but necessary)
   * orientation of the driver backend ports: playback ports are
   * "input" to the backend, and capture ports are "output" from
   * it.
   */

  /* FIXME this just connects to the first ports
   * it finds. add a menu in the welcome screen to
   * set up default output */
  const char **ports =
    jack_get_ports (
      self->client, NULL, JACK_DEFAULT_AUDIO_TYPE,
      JackPortIsPhysical|JackPortIsInput);
  if (ports == NULL) {
          g_error ("no physical playback ports\n");
          exit (1);
  }

  if (jack_connect (
        self->client,
        jack_port_name (
          JACK_PORT_T (
            self->monitor_out->l->data)),
        ports[0]))
    g_critical ("cannot connect output ports\n");

  if (jack_connect (
        self->client,
        jack_port_name (
          JACK_PORT_T (self->monitor_out->r->data)),
        ports[1]))
    g_critical ("cannot connect output ports\n");

  jack_free (ports);

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

#endif /* HAVE_JACK */
