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

#include "audio/region.h"

#include <gtk/gtk.h>

struct Project;

typedef struct _Timeline
{
  /**
  * The total number of bars making up the song
  */
  int           total_bars;

  /**
   * The position of the playhead
   */
  Position      playhead_pos;

  /**
   * Loop start pos
   */
  Position      loop_start_pos;

  /**
   * Loop end pos
   */
  Position      loop_end_pos;

  /**
   * Start marker pos
   */
  Position      start_marker_pos;

  /**
   * End marker pos
   */
  Position      end_marker_pos;

  /**
   * An array containing all the regions in the timeline
   */
  GArray *      regions_array;

  /**
   * BPM
   */
  float           bpm;

  /**
   * Numerator of time signature (e.g. 4/4)
   */
  int             time_sig_numerator;

  /**
   * Denominator of time signature (e.g. 4/4)
   */
  int             time_sig_denominator;

  /**
   * Zoom level, used in ruler/timeline widget calculations
   */
  float           zoom_level;
} Timeline;

extern Timeline * timeline;

Timeline * timeline;

/**
 * Initialize timeline
 */
void
init_timeline ();

#endif
