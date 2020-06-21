/*
 * Copyright (C) 2018-2020 Alexandros Theodotou <alex at zrythm dot org>
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
#include "audio/track.h"

/**
 * @addtogroup audio
 *
 * @{
 */

typedef struct Track Track;
typedef struct _TracklistWidget TracklistWidget;
typedef struct _PinnedTracklistWidget
  PinnedTracklistWidget;

#define TRACKLIST (PROJECT->tracklist)
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
   *
   * Pinned tracks should have lower indices. Ie,
   * the sequence must be:
   * {
   *   pinned track,
   *   pinned track,
   *   pinned track,
   *   track,
   *   track,
   *   track,
   *   ...
   * }
   */
  Track *             tracks[MAX_TRACKS];

  int                 num_tracks;

  /** The chord track, for convenience. */
  Track *             chord_track;

  /** The marker track, for convenience. */
  Track *             marker_track;

  /** The tempo track, for convenience. */
  Track *             tempo_track;

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
  tracklist_schema =
{
  YAML_VALUE_PTR (
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

/**
 * Returns the Track matching the given name, if
 * any.
 */
Track *
tracklist_find_track_by_name (
  Tracklist *  self,
  const char * name);

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
  bool        rm_pl,
  bool        free,
  bool        publish_events,
  bool        recalc_graph);

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

/**
 * Pins or unpins the Track.
 */
void
tracklist_set_track_pinned (
  Tracklist * self,
  Track *     track,
  const int   pinned,
  int         publish_events,
  int         recalc_graph);

/**
 * Returns the index of the given Track.
 */
int
tracklist_get_track_pos (
  Tracklist * self,
  Track *     track);

ChordTrack *
tracklist_get_chord_track (
  const Tracklist * self);

/**
 * Returns the first visible Track.
 *
 * @param pinned 1 to check the pinned tracklist,
 *   0 to check the non-pinned tracklist.
 */
Track *
tracklist_get_first_visible_track (
  Tracklist * self,
  const int   pinned);

/**
 * Returns the previous visible Track in the same
 * Tracklist as the given one (ie, pinned or not).
 */
Track *
tracklist_get_prev_visible_track (
  Tracklist * self,
  Track *     track);

/**
 * Returns the index of the last Track.
 *
 * @param pinned_only Only consider pinned Track's.
 * @param visible_only Only consider visible
 *   Track's.
 */
int
tracklist_get_last_pos (
  Tracklist * self,
  const int   pinned_only,
  const int   visible_only);

/**
 * Returns the last Track.
 *
 * @param pinned_only Only consider pinned Track's.
 * @param visible_only Only consider visible
 *   Track's.
 */
Track*
tracklist_get_last_track (
  Tracklist * self,
  const int   pinned_only,
  const int   visible_only);

/**
 * Returns the next visible Track in the same
 * Tracklist as the given one (ie, pinned or not).
 */
Track *
tracklist_get_next_visible_track (
  Tracklist * self,
  Track *     track);

/**
 * Returns the Track after delta visible Track's.
 *
 * Negative delta searches backwards.
 *
 * This function searches tracks only in the same
 * Tracklist as the given one (ie, pinned or not).
 */
Track *
tracklist_get_visible_track_after_delta (
  Tracklist * self,
  Track *     track,
  int         delta);

/**
 * Returns the number of visible Tracks between
 * src and dest (negative if dest is before src).
 *
 * The caller is responsible for checking that
 * both tracks are in the same tracklist (ie,
 * pinned or not).
 */
int
tracklist_get_visible_track_diff (
  Tracklist *   self,
  const Track * src,
  const Track * dest);

/**
 * Returns 1 if the track name is not taken.
 *
 * @param track_to_skip Track to skip when searching.
 */
int
tracklist_track_name_is_unique (
  Tracklist *  self,
  const char * name,
  Track *      track_to_skip);

/**
 * Returns if the tracklist has soloed tracks.
 */
int
tracklist_has_soloed (
  const Tracklist * self);

/**
 * @param visible 1 for visible, 0 for invisible.
 */
int
tracklist_get_num_visible_tracks (
  Tracklist * self,
  int         visible);

/**
 * Activate or deactivate all plugins.
 *
 * This is useful for exporting: deactivating and
 * reactivating a plugin will reset its state.
 */
void
tracklist_activate_all_plugins (
  Tracklist * self,
  bool        activate);

/**
 * Exposes each track's ports that should be
 * exposed to the backend.
 *
 * This should be called after setting up the
 * engine.
 */
void
tracklist_expose_ports_to_backend (
  Tracklist * self);

Tracklist *
tracklist_new (Project * project);

void
tracklist_free (
  Tracklist * self);

/**
 * Define guile module.
 */
void
guile_tracklist_define_module (void);

/**
 * @}
 */

#endif
