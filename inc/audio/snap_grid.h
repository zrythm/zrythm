/*
 * project/snap_grid.h - Snap Grid info
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

#ifndef __PROJECT_SNAP_GRID_H__
#define __PROJECT_SNAP_GRID_H__

#include "audio/position.h"

#define SNAP_GRID_IS_MIDI(sg) \
  (&ZRYTHM->snap_grid_midi == sg)
#define SNAP_GRID_IS_TIMELINE(sg) \
  (&ZRYTHM->snap_grid_timeline == sg)
/* if any snapping is enabled */
#define SNAP_GRID_ANY_SNAP(sg) \
  (sg->snap_to_grid || sg->snap_to_grid_keep_offset || \
   sg->snap_to_events)

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
   */
  Position         snap_points[12096];
  int              num_snap_points;
} SnapGrid;

SnapGrid *
snap_grid_new (NoteLength   note_length);

void
snap_grid_setup (SnapGrid * self);

/**
 * Sets note length and re-calculates snap points.
 */
void
snap_grid_set_note_length (SnapGrid * self,
                           NoteLength note_length);

/**
 * Gets currently selected note length in ticks.
 */
int
snap_grid_get_note_ticks (SnapGrid * self);

/**
 * Sets note type and re-calculates snap points.
 */
void
snap_grid_set_note_type (SnapGrid *self,
                         NoteType   note_type);

/**
 * Returns the grid intensity as a human-readable string.
 *
 * Must be free'd.
 */
char *
snap_grid_stringize (SnapGrid * snap_grid);

#endif
