/*
 * audio/transport.c - transport
 *
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

#include "audio/engine.h"
#include "audio/transport.h"

/**
 * Sets BPM and does any necessary processing (like notifying interested
 * parties).
 */
void
transport_set_bpm (float bpm)
{
  if (bpm < MIN_BPM)
    bpm = MIN_BPM;
  else if (bpm > MAX_BPM)
    bpm = MAX_BPM;
  TRANSPORT->bpm = bpm;
  engine_update_frames_per_tick (TRANSPORT->beats_per_bar,
                                 bpm,
                                 AUDIO_ENGINE->sample_rate);
}

void
transport_init ()
{
  Transport * transport = calloc (1, sizeof (Transport));
  TRANSPORT = transport;

  // set inital total number of beats
  // this is applied to the ruler
  transport->total_bars = DEFAULT_TOTAL_BARS;

  // init regions array
  transport->regions_array =
    g_array_new (TRUE,
                 TRUE,
                 sizeof (Region));

  /* set BPM related defaults */
  transport->beats_per_bar = DEFAULT_BEATS_PER_BAR;
  transport->beat_unit = DEFAULT_BEAT_UNIT;
  transport_set_bpm (DEFAULT_BPM);

  // set positions of playhead, start/end markers
  position_set_to_bar (&transport->playhead_pos, 1);
  position_set_to_bar (&transport->q_pos, 1);
  position_set_to_bar (&transport->start_marker_pos, 1);
  position_set_to_bar (&transport->end_marker_pos, 128);
  position_set_to_bar (&transport->loop_start_pos, 1);
  position_set_to_bar (&transport->loop_end_pos, 8);

  /* set playstate */
  transport->play_state = PLAYSTATE_PAUSED;


  /* set zoom level */
  transport->zoom_level = DEFAULT_ZOOM_LEVEL;

  zix_sem_init(&transport->paused, 0);
}

void
transport_request_pause ()
{
  TRANSPORT->play_state = PLAYSTATE_PAUSE_REQUESTED;
}

void
transport_request_roll ()
{
  TRANSPORT->play_state = PLAYSTATE_ROLL_REQUESTED;
}

/**
 * Moves the playhead by the time corresponding to given samples.
 */
void
transport_update_playhead (int frames)
{
  if (TRANSPORT->play_state == PLAYSTATE_ROLLING)
    {
      position_add_frames (&TRANSPORT->playhead_pos,
                           frames);
    }
}
