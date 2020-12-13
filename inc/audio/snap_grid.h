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
 * Snap/grid information.
 */

#ifndef __AUDIO_SNAP_GRID_H__
#define __AUDIO_SNAP_GRID_H__

#include <stdbool.h>

#include "audio/position.h"

/**
 * @addtogroup audio
 *
 * @{
 */

#define SNAP_GRID_IS_MIDI(sg) \
  (&PROJECT->snap_grid_midi == sg)
#define SNAP_GRID_IS_TIMELINE(sg) \
  (&PROJECT->snap_grid_timeline == sg)
/* if any snapping is enabled */
#define SNAP_GRID_ANY_SNAP(sg) \
  (sg->snap_to_grid || sg->snap_to_events)
#define SNAP_GRID_TIMELINE \
  (&PROJECT->snap_grid_timeline)
/* FIXME rename to snap grid editor */
#define SNAP_GRID_MIDI \
  (&PROJECT->snap_grid_midi)

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
   * If this is on, the snap note length will be
   * determined automatically based on the current
   * zoom level.
   *
   * The snap note type still applies.
   *
   * TODO this will be done after v1.
   */
  bool             snap_adaptive;

  /** Snap note length. */
  NoteLength       snap_note_length;

  /** Snap note type. */
  NoteType         snap_note_type;

  /** Whether to snap to the grid. */
  bool             snap_to_grid;

  /**
   * Whether to keep the offset when moving items.
   *
   * This requires @ref SnapGrid.snap_to_grid to be
   * enabled.
   */
  bool             snap_to_grid_keep_offset;

  /** Whether to snap to events. */
  bool             snap_to_events;

  /** Default note length. */
  NoteLength       default_note_length;
  /** Default note type. */
  NoteType         default_note_type;

  /**
   * If this is on, the default note length will be
   * determined automatically based on the current
   * zoom level.
   *
   * The default note type still applies.
   *
   * TODO this will be done after v1.
   */
  bool             default_adaptive;

  /** Whether to use the "snap" options as default
   * length or not. */
  bool             link;

  /**
   * Snap points to be used by the grid and by
   * position
   * to calculate previous/next snap point.
   *
   * Cache.
   */
  Position *       snap_points;
  int              num_snap_points;
  size_t           snap_points_size;
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

/**
 * These are not meant to be serialized, they are
 * only used for convenience.
 */
static const cyaml_strval_t
note_type_short_strings[] =
{
  { "",       NOTE_TYPE_NORMAL    },
  { ".",       NOTE_TYPE_DOTTED   },
  { "t",      NOTE_TYPE_TRIPLET   },
};

static const cyaml_schema_field_t
  snap_grid_fields_schema[] =
{
  YAML_FIELD_ENUM (
    SnapGrid, snap_note_length, note_length_strings),
  YAML_FIELD_ENUM (
    SnapGrid, snap_note_type, note_type_strings),
  YAML_FIELD_INT (
    SnapGrid, snap_adaptive),
  YAML_FIELD_ENUM (
    SnapGrid, default_note_length,
    note_length_strings),
  YAML_FIELD_ENUM (
    SnapGrid, default_note_type, note_type_strings),
  YAML_FIELD_INT (
    SnapGrid, default_adaptive),
  YAML_FIELD_INT (
    SnapGrid, link),
  YAML_FIELD_INT (
    SnapGrid, snap_to_grid),
  YAML_FIELD_INT (
    SnapGrid, snap_to_grid_keep_offset),
  YAML_FIELD_INT (
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
snap_grid_init (
  SnapGrid *   self,
  NoteLength   note_length);

int
snap_grid_get_ticks_from_length_and_type (
  NoteLength length,
  NoteType   type);

/**
 * Updates snap points.
 */
void
snap_grid_update_snap_points (
  SnapGrid * self);

/**
 * Gets a snap point's length in ticks.
 */
int
snap_grid_get_snap_ticks (
  SnapGrid * self);

/**
 * Gets a the default length in ticks.
 */
int
snap_grid_get_default_ticks (
  SnapGrid * self);

/**
 * Returns the grid intensity as a human-readable
 * string.
 *
 * Must be free'd.
 */
char *
snap_grid_stringize (
  NoteLength note_length,
  NoteType   note_type);

/**
 * Returns the next or previous SnapGrid point.
 *
 * Must not be free'd.
 *
 * @param self Snap grid to search in.
 * @param pos Position to search for.
 * @param return_prev 1 to return the previous
 * element or 0 to return the next.
 */
Position *
snap_grid_get_nearby_snap_point (
  SnapGrid *       self,
  const Position * pos,
  const int        return_prev);

/**
 * @}
 */

#endif
