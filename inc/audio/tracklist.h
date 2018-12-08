/*
 * audio/tracklist.h - Tracklist backend
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

/** \file
 */

#ifndef __AUDIO_TRACKLIST_H__
#define __AUDIO_TRACKLIST_H__

typedef struct Track Track;
typedef struct _TracklistWidget TracklistWidget;

#define GET_SELECTED_TRACKS \
  Track * selected_tracks[200]; \
  int num_selected = 0; \
  tracklist_get_selected_tracks (PROJECT->tracklist,\
                                 selected_tracks,\
                                 &num_selected);

/**
 * Internal Tracklist.
 */
typedef struct Tracklist
{
  /**
   * All tracks that exist.
   */
  Track *             tracks[200];
  int                 num_tracks;

  TracklistWidget *   widget;

} Tracklist;

Tracklist *
tracklist_new ();

/**
 * Finds selected tracks and puts them in given array.
 */
void
tracklist_get_selected_tracks (Tracklist * tracklist,
                               Track **    selected_tracks,
                               int *       num_selected);

/**
 * Finds visible tracks and puts them in given array.
 */
void
tracklist_get_visible_tracks (Tracklist * tracklist,
                              Track **    visible_tracks,
                              int *       num_visible);

int
tracklist_contains_master_track (Tracklist * tracklist);

int
tracklist_contains_chord_track (Tracklist * tracklist);

/**
 * Adds given track to given spot in tracklist.
 */
void
tracklist_add_track (Tracklist * tracklist,
                     Track *     track,
                     int         pos);

int
tracklist_get_track_pos (Tracklist * tracklist,
                         Track *     track);

#endif
