/*
 * Copyright (C) 2019 Alexandros Theodotou
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
#include "audio/engine_jack.h"
#include "audio/engine_pa.h"
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

void
engine_setup (AudioEngine * self)
{
  transport_setup (self->transport,
                   self);

  self->type = ENGINE_TYPE_PORT_AUDIO;

  /* init semaphore */
  zix_sem_init (&self->port_operation_lock, 1);

  switch (self->type)
    {
    case ENGINE_TYPE_JACK:
      jack_setup (self);
      break;
    case ENGINE_TYPE_PORT_AUDIO:
      pa_setup (self);
      break;
    }
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
close_audio_engine (AudioEngine * self)
{
  g_message ("closing audio engine...");

  switch (self->type)
    {
    case ENGINE_TYPE_JACK:
      jack_client_close (self->client);
      break;
    case ENGINE_TYPE_PORT_AUDIO:
      pa_terminate (self);
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
engine_process_prepare (AudioEngine * self,
                        uint32_t      nframes)
{
  self->nframes = nframes;

  if (TRANSPORT->play_state == PLAYSTATE_PAUSE_REQUESTED)
    {
      TRANSPORT->play_state = PLAYSTATE_PAUSED;
      zix_sem_post (&TRANSPORT->paused);
    }
  else if (TRANSPORT->play_state == PLAYSTATE_ROLL_REQUESTED)
    {
      TRANSPORT->play_state = PLAYSTATE_ROLLING;
    }

  zix_sem_wait (&self->port_operation_lock);

  /* reset all buffers */
  port_clear_buffer (self->midi_in);
  port_clear_buffer (self->midi_editor_manual_press);

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

  /* loop position back if about to exit loop */
  if (self->transport->loop && /* if looping */
      IS_TRANSPORT_ROLLING && /* if rolling */
      (self->transport->playhead_pos.frames <=  /* if current pos is inside loop */
          self->transport->loop_end_pos.frames) &&
      ((self->transport->playhead_pos.frames + self->nframes) > /* if next pos will be outside loop */
          self->transport->loop_end_pos.frames))
    {
      transport_move_playhead (
        self->transport,
        &self->transport->loop_start_pos,
        1);
    }
  else if (IS_TRANSPORT_ROLLING)
    {
      /* move playhead as many samples as processed */
      transport_add_to_playhead (self->transport,
                                 self->nframes);
    }
}
