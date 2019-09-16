/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Tracklist backend.
 */

#ifndef __AUDIO_TRACKLIST_H__
#define __AUDIO_TRACKLIST_H__

#include "audio/engine.h"

/**
 * @addtogroup audio
 *
 * @{
 */

typedef struct Track Track;
typedef struct _TracklistWidget TracklistWidget;
typedef struct _PinnedTracklistWidget
  PinnedTracklistWidget;

#define TRACKLIST (&PROJECT->tracklist)
#define MAX_TRACKS 3000

typedef struct Track ChordTrack;

/**
 * The Tracklist contains all the tracks in the
 * Project.
 *
 * There should be a clear separation between the
 * Tracklist and the Mixer. The Tracklist should be
 * concerned with Tracks in the arranger, and the
 * Mixer should be concerned with Channels, routing
 * and Port connections.
 */
typedef struct Tracklist
{
  /**
   * All tracks that exist.
   *
   * These should always be sorted in the same way
   * they should appear in the GUI and include
   * hidden tracks.
   */
  Track *             tracks[MAX_TRACKS];

  int                 num_tracks;

  /** The chord track, for convenience. */
  Track *             chord_track;

  /** The marker track, for convenience. */
  Track *             marker_track;

  /** The master track, for convenience. */
  Track *             master_track;

  /** Non-pinned TracklistWidget. */
  TracklistWidget *   widget;

  /** PinnedTracklistWidget. */
  PinnedTracklistWidget * pinned_widget;
} Tracklist;

static const cyaml_schema_field_t
  tracklist_fields_schema[] =
{
  CYAML_FIELD_SEQUENCE_COUNT (
    "tracks", CYAML_FLAG_DEFAULT,
    Tracklist, tracks, num_tracks,
    &track_schema, 0, CYAML_UNLIMITED),

	CYAML_FIELD_END
};

static const cyaml_schema_value_t
tracklist_schema = {
  CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
    Tracklist, tracklist_fields_schema),
};

/**
 * Initializes the tracklist when loading a project.
 */
void
tracklist_init_loaded (
  Tracklist * self);

/**
 * Finds visible tracks and puts them in given array.
 */
void
tracklist_get_visible_tracks (
  Tracklist * self,
  Track **    visible_tracks,
  int *       num_visible);

int
tracklist_contains_master_track (
  Tracklist * self);

int
tracklist_contains_chord_track (
  Tracklist * self);

/**
 * Prints the tracks (for debugging).
 */
void
tracklist_print_tracks (
  Tracklist * self);

/**
 * Adds given track to given spot in tracklist.
 *
 * @param publish_events Publish UI events.
 * @param recalc_graph Recalculate routing graph.
 */
void
tracklist_insert_track (
  Tracklist * self,
  Track *     track,
  int         pos,
  int         publish_events,
  int         recalc_graph);

/**
 * Removes a track from the Tracklist and the
 * TracklistSelections.
 *
 * Also disconnects the channel.
 *
 * @param rm_pl Remove plugins or not.
 * @param free Free the track or not (free later).
 * @param publish_events Push a track deleted event
 *   to the UI.
 * @param recalc_graph Recalculate the mixer graph.
 */
void
tracklist_remove_track (
  Tracklist * self,
  Track *     track,
  int         rm_pl,
  int         free,
  int         publish_events,
  int         recalc_graph);

/**
 * Moves a track from its current position to the
 * position given by \p pos.
 *
 * @param publish_events Push UI update events or
 *   not.
 * @param recalc_graph Recalculate routing graph.
 */
void
tracklist_move_track (
  Tracklist * self,
  Track *     track,
  int         pos,
  int         publish_events,
  int         recalc_graph);

/**
 * Calls tracklist_insert_track with the given
 * options.
 */
void
tracklist_append_track (
  Tracklist * self,
  Track *     track,
  int         publish_events,
  int         recalc_graph);

int
tracklist_get_track_pos (
  Tracklist * self,
  Track *     track);

ChordTrack *
tracklist_get_chord_track (
  Tracklist * self);

int
tracklist_get_last_visible_pos (
  Tracklist * self);

Track*
tracklist_get_last_visible_track (
  Tracklist * self);

Track *
tracklist_get_first_visible_track (
  Tracklist * self);

Track *
tracklist_get_prev_visible_track (
  Tracklist * self,
  Track * track);

Track *
tracklist_get_next_visible_track (
  Tracklist * self,
  Track * track);

/**
 * Returns 1 if the track name is not taken.
 */
static inline int
tracklist_track_name_is_unique (
  Tracklist * self,
  const char * name)
{
  for (int i = 0; i < self->num_tracks; i++)
    {
      if (!g_strcmp0 (name, self->tracks[i]->name))
        return 0;
    }
  return 1;
}

/**
 * Returns if the tracklist has soloed tracks.
 */
int
tracklist_has_soloed (
  const Tracklist * self);

/**
 * @}
 */

#endif
