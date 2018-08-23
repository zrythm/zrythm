/*
 * audio/timeline.h - The timeline struct containing all objects in
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

#ifndef __AUDIO_TIMELINE_H__
#define __AUDIO_TIMELINE_H__

#include <stdint.h>

#include "project.h"
#include "audio/region.h"

#include <gtk/gtk.h>

#define AUDIO_TIMELINE PROJECT->timeline

struct Project;

typedef enum {
	PLAYSTATE_RUNNING,
	PLAYSTATE_PAUSE_REQUESTED,
	PLAYSTATE_PAUSED
} Play_State;

typedef struct Timeline
{
  int           total_bars;             ///< total bars in the song
  Position      playhead_pos;           ///< playhead position
  Position      loop_start_pos;         ///< loop marker start position
  Position      loop_end_pos;           ///< loop marker end position
  Position      start_marker_pos;       ///< start marker position
  Position      end_marker_pos;         ///< end marker position
  GArray *      regions_array;          ///< array containing all regions
  int           time_sig_numerator;     ///< time signature numerator (eg 4)
  int           time_sig_denominator;   ///< time singature denominator (eg 4)
  float         zoom_level;             ///< zoom level used in ruler/timeline widget calculations FIXME move to gui
  uint32_t           position;       ///< Transport position in frames
	float              bpm;            ///< Transport tempo in beats per minute
	int               rolling;        ///< Transport speed (0=stop, 1=play)
  Play_State         play_state;     ///< play state
} Timeline;

extern Timeline * timeline;

Timeline * timeline;

/**
 * Initialize timeline
 */
void
init_timeline ();

#endif
