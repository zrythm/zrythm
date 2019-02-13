/*
 * Copyright (C) 2018-2019 Alexandros Theodotou
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

/**
 * \file
 *
 * Snap/grid information.
 */

#ifndef __AUDIO_SNAP_GRID_H__
#define __AUDIO_SNAP_GRID_H__

#include "audio/position.h"

#define SNAP_GRID_IS_MIDI(sg) \
  (&PROJECT->snap_grid_midi == sg)
#define SNAP_GRID_IS_TIMELINE(sg) \
  (&PROJECT->snap_grid_timeline == sg)
/* if any snapping is enabled */
#define SNAP_GRID_ANY_SNAP(sg) \
  (sg->snap_to_grid || sg->snap_to_grid_keep_offset || \
   sg->snap_to_events)
#define SNAP_GRID_TIMELINE \
  (&PROJECT->snap_grid_timeline)
#define SNAP_GRID_MIDI \
  (&PROJECT->snap_grid_timeline)

#define MAX_SNAP_POINTS 120096

typedef enum NoteLength
{
  NOTE_LENGTH_2_1,
  NOTE_LENGTH_1_1,
  NOTE_LENGTH_1_2,
  NOTE_LENGTH_1_4,
  NOTE_LENGTH_1_8,
  NOTE_LENGTH_1_16,
  NOTE_LENGTH_1_32,
  NOTE_LENGTH_1_64,
  NOTE_LENGTH_1_128
} NoteLength;

typedef enum NoteType
{
  NOTE_TYPE_NORMAL,
  NOTE_TYPE_DOTTED, ///< 2/3 of its original size
  NOTE_TYPE_TRIPLET ///< 3/2 of its original size
} NoteType;

typedef struct SnapGrid
{
  /**
   * If this is on, the grid will chose the note length
   * automatically.
   *
   * The note type still applies.
   */
  int              grid_auto;

  /**
   * These two determine the divisions of the grid.
   */
  NoteLength       note_length;
  NoteType         note_type;

  int              snap_to_grid;
  int              snap_to_grid_keep_offset;
  int              snap_to_events;

  /**
   * Snap points to be used by the grid and by position
   * to calculate previous/next snap point.
   *
   * Cache.
   */
  Position         snap_points[MAX_SNAP_POINTS];
  int              num_snap_points;
} SnapGrid;

static const cyaml_strval_t
note_length_strings[] =
{
	{ "2/1",          NOTE_LENGTH_2_1    },
	{ "1/1",          NOTE_LENGTH_1_1   },
	{ "1/2",          NOTE_LENGTH_1_2   },
	{ "1/4",          NOTE_LENGTH_1_4   },
	{ "1/8",          NOTE_LENGTH_1_8   },
	{ "1/16",         NOTE_LENGTH_1_16   },
	{ "1/32",         NOTE_LENGTH_1_32   },
	{ "1/64",         NOTE_LENGTH_1_64   },
	{ "1/128",        NOTE_LENGTH_1_128   },
};

static const cyaml_strval_t
note_type_strings[] =
{
	{ "normal",       NOTE_TYPE_NORMAL    },
	{ "dotted",       NOTE_TYPE_DOTTED   },
	{ "triplet",      NOTE_TYPE_TRIPLET   },
};

static const cyaml_schema_field_t
  snap_grid_fields_schema[] =
{
  CYAML_FIELD_INT (
    "grid_auto", CYAML_FLAG_DEFAULT,
    SnapGrid, grid_auto),
  CYAML_FIELD_ENUM (
    "note_length", CYAML_FLAG_DEFAULT,
    SnapGrid, note_length, note_length_strings,
    CYAML_ARRAY_LEN (note_length_strings)),
  CYAML_FIELD_ENUM (
    "note_type", CYAML_FLAG_DEFAULT,
    SnapGrid, note_type, note_type_strings,
    CYAML_ARRAY_LEN (note_type_strings)),
  CYAML_FIELD_INT (
    "snap_to_grid", CYAML_FLAG_DEFAULT,
    SnapGrid, snap_to_grid),
  CYAML_FIELD_INT (
    "snap_to_grid_keep_offset", CYAML_FLAG_DEFAULT,
    SnapGrid, snap_to_grid_keep_offset),
  CYAML_FIELD_INT (
    "snap_to_events", CYAML_FLAG_DEFAULT,
    SnapGrid, snap_to_events),

	CYAML_FIELD_END
};

static const cyaml_schema_value_t
snap_grid_schema = {
	CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
	  SnapGrid, snap_grid_fields_schema),
};

void
snap_grid_init (SnapGrid *   self,
                NoteLength   note_length);

/**
 * Updates snap points.
 */
void
snap_grid_update_snap_points (SnapGrid * self);

/**
 * Sets note length and re-calculates snap points.
 */
//void
//snap_grid_set_note_length (SnapGrid * self,
                           //NoteLength note_length);

/**
 * Gets given note length and type in ticks.
 */
int
snap_grid_get_note_ticks (NoteLength note_length,
                          NoteType   note_type);

/**
 * Sets note type and re-calculates snap points.
 */
//void
//snap_grid_set_note_type (SnapGrid *self,
                         //NoteType   note_type);

/**
 * Returns the grid intensity as a human-readable string.
 *
 * Must be free'd.
 */
char *
snap_grid_stringize (NoteLength note_length,
                     NoteType   note_type);

#endif
