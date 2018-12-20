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

typedef enum SnapGridType
{
  SNAP_GRID_TIMELINE,
  SNAP_GRID_MIDI
} SnapGridType;

typedef struct SnapGrid
{
  int grid_auto; ///< adaptive grid (grid automatically fits current time sig
                  ///< and zoom level)
  int grid_density; ///< power of 2 (e.g. if 0 -> 1
                                    ///< 1 -> 2
                                    ///< 2 -> 4
                                    ///< 3 -> 8, etc.
  int snap_to_grid; ///< snap to grid (either auto or grid density above)
  int snap_keep_offset; ///< snap to grid (either auto or grid intensity above)
                        ///< while keeping the offset
  int snap_to_edges; ///< snap to region edges
  SnapGridType   type;
} SnapGrid;

/**
 * Returns the grid intensity as a human-readable string.
 *
 * Must be free'd.
 */
char *
snap_grid_stringize (SnapGrid * snap_grid);

#endif
