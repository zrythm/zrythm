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

#include "audio/engine.h"

typedef struct Track Track;
typedef struct _TracklistWidget TracklistWidget;

#define TRACKLIST (&PROJECT->tracklist)
#define GET_SELECTED_TRACKS \
  Track * selected_tracks[200]; \
  int num_selected = 0; \
  tracklist_get_selected_tracks (selected_tracks,\
                                 &num_selected);
#define MAX_TRACKS 300

typedef struct ChordTrack ChordTrack;

/**
 * Internal Tracklist.
 */
typedef struct Tracklist
{
  /**
   * All tracks that exist.
   *
   * These should always be sorted in the same way they
   * are visible in the GUI.
   */
  Track *             tracks[MAX_TRACKS];

  /**
   * The track IDs used to identify tracks.
   */
  int                 track_ids[MAX_TRACKS];
  int                 num_tracks;

  TracklistWidget *   widget;
} Tracklist;

/**
 * Initializes the tracklist.
 *
 * Note: mixer and master channel/track, chord track and
 * each channel must be initialized at this point.
 */
void
tracklist_init (Tracklist * self);

/**
 * Finds selected tracks and puts them in given array.
 */
void
tracklist_get_selected_tracks (Track **    selected_tracks,
                               int *       num_selected);

/**
 * Finds visible tracks and puts them in given array.
 */
void
tracklist_get_visible_tracks (Track **    visible_tracks,
                              int *       num_visible);

int
tracklist_contains_master_track ();

int
tracklist_contains_chord_track ();

/**
 * Adds given track to given spot in tracklist.
 */
void
tracklist_insert_track (Track *     track,
                        int         pos);

void
tracklist_remove_track (Track *     track);

void
tracklist_append_track (Track *     track);

int
tracklist_get_track_pos (Track *     track);

ChordTrack *
tracklist_get_chord_track ();

int
tracklist_get_last_visible_pos ();

Track*
tracklist_get_last_visible_track ();

Track *
tracklist_get_first_visible_track ();

Track *
tracklist_get_prev_visible_track (Track * track);

Track *
tracklist_get_next_visible_track (Track * track);

#endif
