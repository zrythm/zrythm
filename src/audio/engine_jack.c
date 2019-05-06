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

#include "config.h"

#ifdef HAVE_JACK

#include "audio/channel.h"
#include "audio/engine.h"
#include "audio/engine_jack.h"
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

#include <jack/statistics.h>

void
engine_jack_autoconnect_midi_controllers (
  AudioEngine * self)
{
  /* get all output ports */
  const char ** ports =
    jack_get_ports (
      self->client,
      NULL, NULL,
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
  char * pname;
  char * device;
  /*jack_port_t * port;*/
  while ((pname = (char *) ports[i]) != NULL)
    {
      /* if port matches one of the selected
       * MIDI devices, connect it to Zrythm
       * MIDI In */
      j = 0;
      while ((device = devices[j]) != NULL)
        {
          if (g_str_match_string (
                device, pname, 1))
            {
              if (jack_connect (
                    self->client,
                    pname,
                    jack_port_name (
                      JACK_PORT_T (
                        self->midi_in->data))))
                g_warning ("Failed connecting \"%s\" to \"%s\"",
                           pname,
                           self->midi_in->label);


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
int
jack_sample_rate_cb(uint32_t nframes, void * data)
{
  AUDIO_ENGINE->sample_rate = nframes;

  engine_update_frames_per_tick (
    TRANSPORT->beats_per_bar,
    TRANSPORT->bpm,
    AUDIO_ENGINE->sample_rate);

  g_message ("JACK: Sample rate changed to %d", nframes);
  return 0;
}

/** Jack buffer size callback. */
int
jack_buffer_size_cb (uint32_t nframes,
                     void *   data)
{
  int i, j;

  AUDIO_ENGINE->block_length = nframes;
  AUDIO_ENGINE->buf_size_set = true;
#ifdef HAVE_JACK_PORT_TYPE_GET_BUFFER_SIZE
  AUDIO_ENGINE->midi_buf_size =
    jack_port_type_get_buffer_size(
      AUDIO_ENGINE->client, JACK_DEFAULT_MIDI_TYPE);
#endif
  g_message (
    "JACK: Block length changed to %d, midi buf size to %zu",
    AUDIO_ENGINE->block_length,
    AUDIO_ENGINE->midi_buf_size);

  /** reallocate port buffers to new size */
  g_message ("Reallocating port buffers to %d", nframes);
  Port * port;
  Channel * ch;
  Plugin * pl;
  for (i = 0; i < PROJECT->num_ports; i++)
    {
      port = project_get_port (i);
      if (port)
        port->buf =
          realloc (port->buf,
                   nframes * sizeof (float));
      /* TODO memset */
    }
  for (i = 0; i < TRACKLIST->num_tracks; i++)
    {
      ch= TRACKLIST->tracks[i]->channel;

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
  /* FIXME this is the same as block_length */
  AUDIO_ENGINE->nframes = nframes;
  return 0;
}

/**
 * Zero's out the output buffers.
 */
void
engine_jack_clear_output_buffers (
  AudioEngine * self)
{
  int nframes = AUDIO_ENGINE->nframes;
  float * out_l =
    (float *)
    jack_port_get_buffer (
      JACK_PORT_T (AUDIO_ENGINE->stereo_out->l->data),
      nframes);
  float * out_r =
    (float *)
    jack_port_get_buffer (
      JACK_PORT_T (AUDIO_ENGINE->stereo_out->r->data),
      nframes);

  for (int i = 0; i < nframes; i++)
    {
      out_l[i] = 0;
      out_r[i] = 0;
    }

  /* avoid unused warnings */
  (void) out_l;
  (void) out_r;
}

/**
 * Receives MIDI events from JACK MIDI and puts them
 * in the JACK MIDI in port.
 */
void
engine_jack_receive_midi_events (
  AudioEngine * self,
  nframes_t     nframes,
  int           print)
{
  self->port_buf =
    jack_port_get_buffer (
      JACK_PORT_T (self->midi_in->data), nframes);
  MIDI_IN_NUM_EVENTS =
    jack_midi_get_event_count (self->port_buf);

  if (!print)
    return;

  /* print */
  if(MIDI_IN_NUM_EVENTS > 0)
    {
      g_message ("JACK: have %d events", MIDI_IN_NUM_EVENTS);
      for(int i=0; i < MIDI_IN_NUM_EVENTS; i++)
        {
          jack_midi_event_t * event = &MIDI_IN_EVENT(i);
          jack_midi_event_get(event, self->port_buf, i);
          uint8_t type = event->buffer[0] & 0xf0;
          uint8_t channel = event->buffer[0] & 0xf;
          switch (type)
            {
              case MIDI_CH1_NOTE_ON:
                assert (event->size == 3);
                g_message (" note on  (channel %2d): pitch %3d, velocity %3d", channel, event->buffer[1], event->buffer[2]);
                break;
              case MIDI_CH1_NOTE_OFF:
                assert (event->size == 3);
                g_message (" note off (channel %2d): pitch %3d, velocity %3d", channel, event->buffer[1], event->buffer[2]);
                break;
              case MIDI_CH1_CTRL_CHANGE:
                assert (event->size == 3);
                g_message (" control change (channel %2d): controller %3d, value %3d", channel, event->buffer[1], event->buffer[2]);
                break;
              default:
                      break;
            }
          /*g_message ("    event %d time is %d. 1st byte is 0x%x", i,*/
                     /*MIDI_IN_EVENT(i).time, *(MIDI_IN_EVENT(i).buffer));*/
        }
    }
}
/**
 * The process callback for this JACK application is
 * called in a special realtime thread once for each audio
 * cycle.
 */
int
jack_process_cb (
  nframes_t nframes, ///< the number of frames to fill
  void *    data) ///< user data
{
  return
    engine_process (
      (AudioEngine *) data, nframes);
}

int
jack_xrun_cb (void *arg)
{
  ui_show_notification_idle ("XRUN occurred");

  return 0;
}

/**
 * JACK calls this shutdown_callback if the server ever
 * shuts down or decides to disconnect the client.
 */
void
jack_shutdown_cb (void *arg)
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

  if (loading)
    {
      self->midi_in->data =
        (void *) jack_port_register (
          self->client, "MIDI_in",
          JACK_DEFAULT_MIDI_TYPE,
          JackPortIsInput, 0);
    }
  else
    {
      self->midi_in =
        port_new_with_data (
          INTERNAL_JACK_PORT,
          TYPE_EVENT,
          FLOW_INPUT,
          "JACK MIDI In",
          (void *) jack_port_register (
            self->client, "MIDI_in",
            JACK_DEFAULT_MIDI_TYPE,
            JackPortIsInput, 0));
      self->midi_in->owner_backend = 1;
      self->midi_in_id = self->midi_in->id;
    }

  /* init queue */
  self->midi_in->midi_events =
    midi_events_new (1);

  if (!self->midi_in->data)
    {
      g_warning ("no more JACK ports available");
    }

  /* autoconnect MIDI controllers */
  engine_jack_autoconnect_midi_controllers (
    AUDIO_ENGINE);

  return 0;
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

  const char **ports;
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
  self->sample_rate   = jack_get_sample_rate (self->client);
  self->block_length  = jack_get_buffer_size (self->client);


  /* set jack callbacks */
  jack_set_process_callback (self->client,
                             &jack_process_cb,
                             self);
  jack_set_buffer_size_callback (self->client,
                                 &jack_buffer_size_cb,
                                 self);
  jack_set_sample_rate_callback (self->client,
                                 &jack_sample_rate_cb,
                                 self);
  jack_set_xrun_callback (self->client,
                          &jack_xrun_cb,
                          self->client);
  jack_on_shutdown (self->client, &jack_shutdown_cb, self);
  /*jack_set_latency_callback(client, &jack_latency_cb, arg);*/
#ifdef JALV_JACK_SESSION
  /*jack_set_session_callback(client, &jack_session_cb, arg);*/
#endif

  /* create ports */
  Port * stereo_out_l, * stereo_out_r,
       * stereo_in_l, * stereo_in_r;

  if (loading)
    {
      self->stereo_out->l->data =
        (void *) jack_port_register (
          self->client, "Stereo_out_L",
          JACK_DEFAULT_AUDIO_TYPE,
          JackPortIsOutput, 0);
      self->stereo_out->r->data =
        (void *) jack_port_register (
          self->client, "Stereo_out_R",
          JACK_DEFAULT_AUDIO_TYPE,
          JackPortIsOutput, 0);
      self->stereo_in->l->data =
        (void *) jack_port_register (
          self->client, "Stereo_in_L",
          JACK_DEFAULT_AUDIO_TYPE,
          JackPortIsInput, 0);
      self->stereo_in->r->data =
        (void *) jack_port_register (
          self->client, "Stereo_in_R",
          JACK_DEFAULT_AUDIO_TYPE,
          JackPortIsInput, 0);
    }
  else
    {
      stereo_out_l = port_new_with_data (
        INTERNAL_JACK_PORT,
        TYPE_AUDIO,
        FLOW_OUTPUT,
        "JACK Stereo Out / L",
        (void *) jack_port_register (
          self->client, "Stereo_out_L",
          JACK_DEFAULT_AUDIO_TYPE,
          JackPortIsOutput, 0));
      stereo_out_r = port_new_with_data (
        INTERNAL_JACK_PORT,
        TYPE_AUDIO,
        FLOW_OUTPUT,
        "JACK Stereo Out / R",
        (void *) jack_port_register (
          self->client, "Stereo_out_R",
          JACK_DEFAULT_AUDIO_TYPE,
          JackPortIsOutput, 0));
      stereo_in_l = port_new_with_data (
        INTERNAL_JACK_PORT,
        TYPE_AUDIO,
        FLOW_INPUT,
        "JACK Stereo In / L",
        (void *) jack_port_register (
          self->client, "Stereo_in_L",
          JACK_DEFAULT_AUDIO_TYPE,
          JackPortIsInput, 0));
      stereo_in_r = port_new_with_data (
        INTERNAL_JACK_PORT,
        TYPE_AUDIO,
        FLOW_INPUT,
        "JACK Stereo In / R",
        (void *) jack_port_register (
          self->client, "Stereo_in_R",
          JACK_DEFAULT_AUDIO_TYPE,
          JackPortIsInput, 0));

      stereo_in_l->owner_backend = 1;
      stereo_in_r->owner_backend = 1;
      stereo_out_l->owner_backend = 1;
      stereo_out_r->owner_backend = 1;

      self->stereo_in =
        stereo_ports_new (stereo_in_l,
                          stereo_in_r);
      self->stereo_out =
        stereo_ports_new (stereo_out_l,
                          stereo_out_r);
    }

  if (!self->stereo_in->l->data ||
      !self->stereo_in->r->data ||
      !self->stereo_out->l->data ||
      !self->stereo_out->r->data)
    {
      g_error ("no more JACK ports available");
    }

  /* Tell the JACK server that we are ready to roll.  Our
   * process() callback will start running now. */
  if (jack_activate (self->client))
    {
      g_error ("cannot activate client");
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

  ports =
    jack_get_ports (
      self->client, NULL, NULL,
      JackPortIsPhysical|JackPortIsInput);
  if (ports == NULL) {
          g_error ("no physical playback ports\n");
          exit (1);
  }

  if (jack_connect (
        self->client,
        jack_port_name (
          JACK_PORT_T (
            self->stereo_out->l->data)),
        ports[0]))
    g_error ("cannot connect output ports\n");

  if (jack_connect (
        self->client,
        jack_port_name (
          JACK_PORT_T (self->stereo_out->r->data)),
        ports[1]))
    g_error ("cannot connect output ports\n");

  jack_free (ports);

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
jack_tear_down ()
{
  jack_client_close (AUDIO_ENGINE->client);

  /* init semaphore */
  zix_sem_init (&AUDIO_ENGINE->port_operation_lock,
                1);
}

#endif /* HAVE_JACK */
