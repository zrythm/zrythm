/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __ACTIONS_TRACKLIST_SELECTIONS_H__
#define __ACTIONS_TRACKLIST_SELECTIONS_H__

#include "audio/track.h"
#include "utils/yaml.h"

#define TRACKLIST_SELECTIONS \
  (&PROJECT->tracklist_selections)

/**
 * Selections to be used for the tracklist's current
 * selections, copying, undoing, etc.
 */
typedef struct TracklistSelections
{
  /** Tracks doing action upon */
  Track *              tracks[600];
  int                  track_ids[600];
  int                  num_tracks;
} TracklistSelections;

static const cyaml_schema_field_t
  tracklist_selections_fields_schema[] =
{
  CYAML_FIELD_SEQUENCE_COUNT (
    "track_ids", CYAML_FLAG_DEFAULT,
    TracklistSelections, track_ids,
    num_tracks,
    &int_schema, 0, CYAML_UNLIMITED),

	CYAML_FIELD_END
};

static const cyaml_schema_value_t
tracklist_selections_schema = {
  CYAML_VALUE_MAPPING (CYAML_FLAG_POINTER,
    TracklistSelections,
    tracklist_selections_fields_schema),
};

void
tracklist_selections_init_loaded (
  TracklistSelections * ts);

/**
 * Clone the struct for copying, undoing, etc.
 */
TracklistSelections *
tracklist_selections_clone ();

/**
 * Returns if there are any selections.
 */
int
tracklist_selections_has_any (
  TracklistSelections * self);

/**
 * Gets highest track in the selections.
 *
 * If transient is 1, transient objects rae checked
 * instead.
 */
Track *
tracklist_selections_get_highest_track (
  TracklistSelections * self);

/**
 * Gets lowest track in the selections.
 *
 * If transient is 1, transient objects rae checked
 * instead.
 */
Track *
tracklist_selections_get_lowest_track (
  TracklistSelections * self);

void
tracklist_selections_paste_at_pos (
  TracklistSelections * self,
  int                   pos);

void
tracklist_selections_add_track (
  TracklistSelections * self,
  Track *               track);

/**
 * Returns if the Track is selected or not.
 */
int
tracklist_selections_contains_track (
  TracklistSelections * self,
  Track *               track);

void
tracklist_selections_remove_track (
  TracklistSelections * self,
  Track *               track);

/**
 * Clears selections.
 */
void
tracklist_selections_clear (
  TracklistSelections * ts);

void
tracklist_selections_free (
  TracklistSelections * self);

SERIALIZE_INC (
  TracklistSelections, tracklist_selections)
DESERIALIZE_INC (
  TracklistSelections, tracklist_selections)
PRINT_YAML_INC (
  TracklistSelections, tracklist_selections)

#endif
