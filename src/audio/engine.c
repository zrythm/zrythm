/*
 * Copyright (C) 2018 Alexandros Theodotou
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

#include "zrythm_app.h"
#include "audio/engine.h"
#include "audio/midi.h"
#include "audio/mixer.h"
#include "plugins/plugin.h"
#include "plugins/plugin_manager.h"
#include "plugins/lv2_plugin.h"

#include <gtk/gtk.h>

#include <jack/jack.h>
#include <jack/midiport.h>

typedef jack_default_audio_sample_t   sample_t;
typedef jack_nframes_t                nframes_t;

jack_port_t *output_port1, *output_port2, *midi_in_port;
jack_client_t *client;

/* FIXME */
sample_t note_on;
unsigned char note = 0;

/** Jack sample rate callback. */
static int
jack_sample_rate_cb(nframes_t nframes, void* data)
{
  AUDIO_ENGINE->sample_rate = nframes;
  g_message ("JACK: Sample rate changed to %d", nframes);
  return 0;
}

/** Jack buffer size callback. */
static int
jack_buffer_size_cb(nframes_t nframes, void* data)
{
  AUDIO_ENGINE->block_length = nframes;
  AUDIO_ENGINE->buf_size_set = true;
#ifdef HAVE_JACK_PORT_TYPE_GET_BUFFER_SIZE
  AUDIO_ENGINE->midi_buf_size = jack_port_type_get_buffer_size(
          AUDIO_ENGINE->client, JACK_DEFAULT_MIDI_TYPE);
#endif
  for (int i = 0; i< PLUGIN_MANAGER->num_plugins; i++)
    {
      if (PLUGIN_MANAGER->plugins[i]->descr.protocol = PROT_LV2)
        {
          lv2_allocate_port_buffers ((LV2_Plugin *)PLUGIN_MANAGER->plugins[i]->original_plugin);
        }
    }
  g_message ("JACK: Block length changed to %d, midi buf size to %lu",
             AUDIO_ENGINE->block_length,
             AUDIO_ENGINE->midi_buf_size);
  return 0;
}

/**
 * The process callback for this JACK application is called in a
 * special realtime thread once for each audio cycle.
 *
 * This client follows a simple rule: when the JACK transport is
 * running, copy the input port to the output.  When it stops, exit.
 *
 */
static int
jack_process_cb (nframes_t    nframes,     ///< the number of frames to fill
         void              * data)       ///< user data
{
  sample_t *out1, *out2;
  int i;

  /**
   * get jack's buffers with nframes frames for left & right
   */
  out1 = (sample_t *)
    jack_port_get_buffer (output_port1, nframes);
  out2 = (sample_t *)
    jack_port_get_buffer (output_port2, nframes);

  /* set midi events from midi input */
  void* port_buf = jack_port_get_buffer(midi_in_port, nframes);
  jack_midi_event_t in_event;
  jack_nframes_t event_index = 0;
  jack_nframes_t event_count = jack_midi_get_event_count(port_buf);
  if(event_count > 1)
    {
      g_message ("JACK: have %d events", event_count);
      for(i=0; i<event_count; i++)
        {
          jack_midi_event_get(&in_event, port_buf, i);
          g_message ("    event %d time is %d. 1st byte is 0x%x", i, in_event.time, *(in_event.buffer));
        }
    }
  jack_midi_event_get(&in_event, port_buf, 0);
  for(i = 0; i < nframes; i++)
    {
      if ((in_event.time == i) && (event_index < event_count))
        {
          if (((*(in_event.buffer) & 0xf0)) == MIDI_CH1_NOTE_ON)
            {
              /* note on */
              note = *(in_event.buffer + 1);
              if (*(in_event.buffer + 2) == 0)
                {
                  note_on = 0.0;
                }
              else
                {
                  note_on = (float)(*(in_event.buffer + 2)) / 127.f;
                }
            }
          else if (((*(in_event.buffer)) & 0xf0) == MIDI_CH1_NOTE_OFF)
            {
              /* note off */
              note = *(in_event.buffer + 1);
              note_on = 0.0;
            }
          event_index++;
          if(event_index < event_count)
                  jack_midi_event_get(&in_event, port_buf, event_index);
        }
      /*ramp += note_frqs[note];*/
      /*ramp = (ramp > 1.0) ? ramp - 2.0 : ramp;*/
      /*out[i] = note_on*sin(2*M_PI*ramp);*/
    }

  /*
   * process
   */
  mixer_process (nframes, out1, out2);

  /*
   * processing finished, return 0
   */
  return 0;
}

/**
 * JACK calls this shutdown_callback if the server ever shuts down or
 * decides to disconnect the client.
 */
void
jack_shutdown_cb (void *arg)
{
  // TODO
  g_error ("Jack shutting down...");
}

/**
 * Init audio engine
 */
void
init_audio_engine()
{
    g_message ("Initializing audio engine...");
    AUDIO_ENGINE = malloc (sizeof (Audio_Engine));
    Audio_Engine * engine = AUDIO_ENGINE;

    const char **ports;
    const char *client_name = "zrythm";
    const char *server_name = NULL;
    jack_options_t options = JackNullOption;
    jack_status_t status;
    int i;

    engine->buf_size_set = false;
    /*engine->block_length  = 4096;  [> Should be set by backend <]*/
    /*engine->midi_buf_size = 1024;  [> Should be set by backend <]*/
    /*engine->sample_rate = 96000; [> FIXME <]*/

    // open a client connection to the JACK server
    client = jack_client_open (client_name, options, &status, server_name);
    if (client == NULL) {
            g_error ("jack_client_open() failed, "
                     "status = 0x%2.0x", status);
            if (status & JackServerFailed) {
                    g_error ("Unable to connect to JACK server");
            }
            exit (1);
    }
    if (status & JackServerStarted) {
    // FIXME g_info
            g_message ("JACK server started\n");
    }
    if (status & JackNameNotUnique) {
            client_name = jack_get_client_name(client);
            g_error ("unique name `%s' assigned\n", client_name);
    }

    /* Set audio engine properties */
    engine->sample_rate   = jack_get_sample_rate(client);
    engine->block_length  = jack_get_buffer_size(client);
    engine->midi_buf_size = 4096;
#ifdef HAVE_JACK_PORT_TYPE_GET_BUFFER_SIZE
    engine->midi_buf_size = jack_port_type_get_buffer_size(
          client, JACK_DEFAULT_MIDI_TYPE);
#endif

    /* set jack callbacks */
    jack_set_process_callback (client, &jack_process_cb, NULL);
    jack_set_buffer_size_callback(client, &jack_buffer_size_cb, NULL);
    jack_set_sample_rate_callback(client, &jack_sample_rate_cb, NULL);
    jack_on_shutdown(client, &jack_shutdown_cb, NULL);
    /*jack_set_latency_callback(client, &jack_latency_cb, arg);*/
#ifdef JALV_JACK_SESSION
    /*jack_set_session_callback(client, &jack_session_cb, arg);*/
#endif

    /* create ports */
    output_port1 = jack_port_register (client, "output1",
                                      JACK_DEFAULT_AUDIO_TYPE,
                                      JackPortIsOutput, 0);

    output_port2 = jack_port_register (client, "output2",
                                      JACK_DEFAULT_AUDIO_TYPE,
                                      JackPortIsOutput, 0);
    midi_in_port = jack_port_register (client, "midi_in",
                                       JACK_DEFAULT_MIDI_TYPE,
                                       JackPortIsInput, 0);

    if ((output_port1 == NULL) || (output_port2 == NULL)) {
            g_error ("no more JACK ports available\n");
    }

    /* initialize mixer, which handles the processing */
    mixer_init ();

    /* Tell the JACK server that we are ready to roll.  Our
     * process() callback will start running now. */
    if (jack_activate (client)) {
            g_error ("cannot activate client");
            return;
    }
    g_message ("Jack activated");

    /* Connect the ports.  You can't do this before the client is
     * activated, because we can't make connections to clients
     * that aren't running.  Note the confusing (but necessary)
     * orientation of the driver backend ports: playback ports are
     * "input" to the backend, and capture ports are "output" from
     * it.
     */

    ports = jack_get_ports (client, NULL, NULL,
                            JackPortIsPhysical|JackPortIsInput);
    if (ports == NULL) {
            g_error ("no physical playback ports\n");
            exit (1);
    }

    if (jack_connect (client, jack_port_name (output_port1), ports[0])) {
            g_error ("cannot connect output ports\n");
    }

    if (jack_connect (client, jack_port_name (output_port2), ports[1])) {
            g_error ("cannot connect output ports\n");
    }

    jack_free (ports);

}

void
close_audio_engine ()
{
    g_message ("closing audio engine...");

    jack_client_close (client);
}
