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

#include "zrythm.h"
#include "audio/channel.h"
#include "audio/engine.h"
#include "audio/midi.h"
#include "audio/mixer.h"
#include "audio/transport.h"
#include "plugins/plugin.h"
#include "plugins/plugin_manager.h"
#include "plugins/lv2_plugin.h"
#include "project.h"

#include <gtk/gtk.h>

#include <jack/jack.h>
#include <jack/midiport.h>

#define JACK_PORT_T(exp) ((jack_port_t *) exp)
#define MIDI_IN_EVENT(i) (AUDIO_ENGINE->midi_in->midi_events.jack_midi_events[i])
#define MIDI_IN_NUM_EVENTS AUDIO_ENGINE->midi_in->midi_events.num_events

typedef jack_default_audio_sample_t   sample_t;
typedef jack_nframes_t                nframes_t;

 void
engine_update_frames_per_tick (AudioEngine * self,
                               int beats_per_bar,
                               int bpm,
                               int sample_rate)
{
  self->frames_per_tick =
    (sample_rate * 60.f * beats_per_bar) /
    (bpm * TICKS_PER_BAR);

  /* update positions */
  transport_update_position_frames (self->transport);
}

/** Jack sample rate callback. */
static int
jack_sample_rate_cb(nframes_t nframes, void* data)
{
  AudioEngine * engine = (AudioEngine *) data;
  engine->sample_rate = nframes;

  if (PROJECT)
    engine_update_frames_per_tick (
      engine,
      engine->transport->beats_per_bar,
      engine->transport->bpm,
      engine->sample_rate);
  else
    engine_update_frames_per_tick (engine, 4, 120, 44000);

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
  g_message ("JACK: Block length changed to %d, midi buf size to %lu",
             AUDIO_ENGINE->block_length,
             AUDIO_ENGINE->midi_buf_size);

  /** reallocate port buffers to new size */
  g_message ("Reallocating port buffers to %d", nframes);
  for (int i = 0; i < AUDIO_ENGINE->num_ports; i++)
    {
      Port * port = AUDIO_ENGINE->ports[i];
      port->nframes = nframes;
      port->buf = realloc (port->buf, nframes * sizeof (sample_t));
      /* TODO memset */
    }
  for (int i = 0; i < MIXER->num_channels; i++)
    {
      Channel * channel = MIXER->channels[i];
      for (int j = 0; j < STRIP_SIZE; j++)
        {
          if (channel->strip[j])
            {
              Plugin * plugin = channel->strip[j];
              if (plugin->descr->protocol == PROT_LV2)
                {
                  lv2_allocate_port_buffers (
                                (Lv2Plugin *)plugin->original_plugin);
                }
            }
        }
    }
  /* FIXME this is the same as block_length */
  AUDIO_ENGINE->nframes = nframes;
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
  AudioEngine * engine = (AudioEngine *) data;
  if (!engine->run)
    return 0;

  sample_t * stereo_out_l, * stereo_out_r;
  int i = 0;

  if (TRANSPORT->play_state == PLAYSTATE_PAUSE_REQUESTED)
    {
      TRANSPORT->play_state = PLAYSTATE_PAUSED;
      zix_sem_post (&TRANSPORT->paused);
    }
  else if (TRANSPORT->play_state == PLAYSTATE_ROLL_REQUESTED)
    {
      TRANSPORT->play_state = PLAYSTATE_ROLLING;
    }

  zix_sem_wait (&engine->port_operation_lock);

  /* reset all buffers */
  port_clear_buffer (engine->midi_in);
  port_clear_buffer (engine->midi_editor_manual_press);

  /*g_message ("jack start");*/

  /* get MIDI events from JACK and store to engine MIDI in port
   * FIXME referencing  AUDIO ENGINE all the time is expensive,
   * use local vars where possible */
  void* port_buf = jack_port_get_buffer (
        JACK_PORT_T (engine->midi_in->data), nframes);
  MIDI_IN_NUM_EVENTS = jack_midi_get_event_count(port_buf);
  if(MIDI_IN_NUM_EVENTS > 0)
    {
      g_message ("JACK: have %d events", MIDI_IN_NUM_EVENTS);
      for(int i=0; i < MIDI_IN_NUM_EVENTS; i++)
        {
          jack_midi_event_t * event = &MIDI_IN_EVENT(i);
          jack_midi_event_get(event, port_buf, i);
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
  /* get MIDI events from other sources */
  /*if (engine->panic)*/
    /*{*/
      /*midi_panic (&engine->midi_editor_manual_press->midi_events);*/
    /*}*/
  midi_events_dequeue (&engine->midi_editor_manual_press->midi_events);

  /* set all to unprocessed for this cycle */
  for (int i = 0; i < MIXER->num_channels; i++)
    {
      Channel * channel = MIXER->channels[i];
      channel->processed = 0;
      for (int j = 0; j < STRIP_SIZE; j++)
        {
          if (channel->strip[j])
            {
              channel->strip[j]->processed = 0;
            }
        }
    }

  /*
   * process
   */
  mixer_process (MIXER, nframes);

  /**
   * get jack's buffers with nframes frames for left & right
   */
  stereo_out_l = (sample_t *)
    jack_port_get_buffer (JACK_PORT_T (engine->stereo_out->l->data),
                          nframes);
  stereo_out_r = (sample_t *)
    jack_port_get_buffer (JACK_PORT_T (engine->stereo_out->r->data),
                           nframes);

  /* by this time, the Master channel should have its Stereo Out ports filled.
   * pass their buffers to jack's buffers */
  for (i = 0; i < nframes; i++)
    {
      stereo_out_l[i] = MIXER->master->stereo_out->l->buf[i];
      stereo_out_r[i] = MIXER->master->stereo_out->r->buf[i];
    }

  zix_sem_post (&engine->port_operation_lock);

  /* stop panicking */
  if (engine->panic)
    {
      engine->panic = 0;
    }

  /* loop position back if about to exit loop */
  if (engine->transport->loop && /* if looping */
      IS_TRANSPORT_ROLLING && /* if rolling */
      (engine->transport->playhead_pos.frames <=  /* if current pos is inside loop */
          engine->transport->loop_end_pos.frames) &&
      ((engine->transport->playhead_pos.frames + nframes) > /* if next pos will be outside loop */
          engine->transport->loop_end_pos.frames))
    {
      transport_move_playhead (
        engine->transport,
        &engine->transport->loop_start_pos,
        1);
    }
  else if (IS_TRANSPORT_ROLLING)
    {
      /* move playhead as many samples as processed */
      transport_add_to_playhead (engine->transport,
                                 nframes);
    }


  /*g_message ("jack end");*/
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

void
engine_setup (AudioEngine * self)
{
  const char **ports;
  const char *client_name = "Zrythm";
  const char *server_name = NULL;
  jack_options_t options = JackNullOption;
  jack_status_t status;

  transport_setup (self->transport,
                   self);

  // open a client connection to the JACK server
  self->client = jack_client_open (client_name, options, &status, server_name);
  if (self->client == NULL) {
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
          client_name = jack_get_client_name(self->client);
          g_error ("unique name `%s' assigned\n", client_name);
  }

  /* Set audio engine properties */
  self->sample_rate   = jack_get_sample_rate (self->client);
  self->block_length  = jack_get_buffer_size (self->client);
  self->midi_buf_size = 4096;
#ifdef HAVE_JACK_PORT_TYPE_GET_BUFFER_SIZE
  self->midi_buf_size = jack_port_type_get_buffer_size (
        self->client, JACK_DEFAULT_MIDI_TYPE);
#endif


  /* set jack callbacks */
  jack_set_process_callback (self->client, &jack_process_cb, self);
  jack_set_buffer_size_callback(self->client, &jack_buffer_size_cb, self);
  jack_set_sample_rate_callback(self->client, &jack_sample_rate_cb, self);
  jack_on_shutdown(self->client, &jack_shutdown_cb, self);
  /*jack_set_latency_callback(client, &jack_latency_cb, arg);*/
#ifdef JALV_JACK_SESSION
  /*jack_set_session_callback(client, &jack_session_cb, arg);*/
#endif

  /* create ports */
  Port * stereo_out_l = port_new_with_data (
        self->block_length,
        INTERNAL_JACK_PORT,
        TYPE_AUDIO,
        FLOW_OUTPUT,
        "JACK Stereo Out / L",
        (void *) jack_port_register (self->client, "Stereo_out_L",
                                    JACK_DEFAULT_AUDIO_TYPE,
                                    JackPortIsOutput, 0));
  Port * stereo_out_r = port_new_with_data (
        self->block_length,
        INTERNAL_JACK_PORT,
        TYPE_AUDIO,
        FLOW_OUTPUT,
        "JACK Stereo Out / R",
        (void *) jack_port_register (self->client, "Stereo_out_R",
                                    JACK_DEFAULT_AUDIO_TYPE,
                                    JackPortIsOutput, 0));
  Port * stereo_in_l = port_new_with_data (
        self->block_length,
        INTERNAL_JACK_PORT,
        TYPE_AUDIO,
        FLOW_INPUT,
        "JACK Stereo In / L",
        (void *) jack_port_register (self->client, "Stereo_in_L",
                                    JACK_DEFAULT_AUDIO_TYPE,
                                    JackPortIsInput, 0));
  Port * stereo_in_r = port_new_with_data (
        self->block_length,
        INTERNAL_JACK_PORT,
        TYPE_AUDIO,
        FLOW_INPUT,
        "JACK Stereo In / R",
        (void *) jack_port_register (self->client, "Stereo_in_R",
                                    JACK_DEFAULT_AUDIO_TYPE,
                                    JackPortIsInput, 0));
  Port * midi_in = port_new_with_data (
        self->block_length,
        INTERNAL_JACK_PORT,
        TYPE_EVENT,
        FLOW_INPUT,
        "JACK MIDI In",
        (void *) jack_port_register (self->client, "MIDI_in",
                                     JACK_DEFAULT_MIDI_TYPE,
                                     JackPortIsInput, 0));

  Port * midi_editor_manual_press = port_new_with_type (
        self->block_length,
        TYPE_EVENT,
        FLOW_INPUT,
        "MIDI Editor Manual Press");


  stereo_in_l->owner_jack = 1;
  stereo_in_r->owner_jack = 1;
  stereo_out_l->owner_jack = 1;
  stereo_out_r->owner_jack = 1;
  midi_in->owner_jack = 1;
  midi_editor_manual_press->owner_jack = 0;

  self->stereo_in  = stereo_ports_new (stereo_in_l, stereo_in_r);
  self->stereo_out = stereo_ports_new (stereo_out_l, stereo_out_r);
  self->midi_in    = midi_in;
  self->midi_editor_manual_press = midi_editor_manual_press;

  /* init MIDI queues for manual presse/piano roll */
  self->midi_editor_manual_press->midi_events.queue = calloc (1, sizeof (MidiEvents));

  if (!self->stereo_in->l->data || !self->stereo_in->r->data ||
      !self->stereo_out->l->data || !self->stereo_out->r->data ||
      !self->midi_in->data)
    {
      g_error ("no more JACK ports available");
    }

  /* init semaphore */
  zix_sem_init (&self->port_operation_lock, 1);

  /* Tell the JACK server that we are ready to roll.  Our
   * process() callback will start running now. */
  if (jack_activate (self->client))
    {
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

  ports = jack_get_ports (self->client, NULL, NULL,
                          JackPortIsPhysical|JackPortIsInput);
  if (ports == NULL) {
          g_error ("no physical playback ports\n");
          exit (1);
  }

  if (jack_connect (self->client, jack_port_name (
              JACK_PORT_T(self->stereo_out->l->data)), ports[0])) {
          g_error ("cannot connect output ports\n");
  }

  if (jack_connect (self->client, jack_port_name (
              JACK_PORT_T (self->stereo_out->r->data)), ports[1])) {
          g_error ("cannot connect output ports\n");
  }

  jack_free (ports);
}


/**
 * Init audio engine
 */
AudioEngine *
engine_new ()
{
    g_message ("Initializing audio engine...");
    AudioEngine * engine = calloc (1, sizeof (AudioEngine));


    engine->buf_size_set = false;

    engine->transport = transport_new ();
    engine->mixer = mixer_new ();

    return engine;
}

void
close_audio_engine ()
{
    g_message ("closing audio engine...");

    jack_client_close (AUDIO_ENGINE->client);
}
