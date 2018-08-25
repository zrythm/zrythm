/*
 * audio/timeline.c - The timeline struct containing all objects in
 *  the timeline and their metadata
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

#include "zrythm_app.h"
#include "project.h"
#include "audio/timeline.h"

#define DEFAULT_TOTAL_BARS 128
#define DEFAULT_BPM 140
#define DEFAULT_TIME_SIG_NUM 4
#define DEFAULT_TIME_SIG_DEN 4
#define DEFAULT_ZOOM_LEVEL 1.0f

void
init_timeline ()
{
  Timeline * timeline = malloc (sizeof (Timeline));

  // set inital total number of beats
  // this is applied to the ruler
  timeline->total_bars = DEFAULT_TOTAL_BARS;

  // init regions array
  timeline->regions_array =
    g_array_new (TRUE,
                 TRUE,
                 sizeof (Region));

  // set positions of playhead, start/end markers
  set_position_to_bar (&timeline->playhead_pos, 3);
  init_position_to_zero (&timeline->start_marker_pos);
  set_position_to_bar (&timeline->end_marker_pos,
                        32);
  init_position_to_zero (&timeline->loop_start_pos);
  set_position_to_bar (&timeline->loop_end_pos,
                        8);

  /* set BPM related defaults */
  timeline->bpm = DEFAULT_BPM;
  timeline->time_sig_numerator = DEFAULT_TIME_SIG_NUM;
  timeline->time_sig_denominator = DEFAULT_TIME_SIG_DEN;

  /* set playstate */
  timeline->play_state = PLAYSTATE_PAUSED;


  /* set zoom level */
  timeline->zoom_level = DEFAULT_ZOOM_LEVEL;

  PROJECT->timeline = timeline;
}
