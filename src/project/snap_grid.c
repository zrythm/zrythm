/*
 * project/snap_grid.c - Snap Grid info
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

#include <math.h>

#include "project/snap_grid.h"

#include <gtk/gtk.h>

/**
 * Returns the grid intensity as a human-readable string.
 *
 * Must be free'd.
 */
char *
snap_grid_stringize (SnapGrid * snap_grid)
{
  int power = pow (2, snap_grid->grid_density);
  g_message ("power %d", power);
  return g_strdup_printf ("1/%d", power);
}
