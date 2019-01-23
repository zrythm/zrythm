/*
 * actions/timeline_selections.c - timeline
 *
 * Copyright (C) 2019 Alexandros Theodotou
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

#include "gui/backend/timeline_selections.h"

#include <gtk/gtk.h>

/**
 * Serializes to XML.
 *
 * MUST be free'd.
 */
char *
timeline_selections_serialize (
  TimelineSelections * ts) ///< TS to serialize
{
  return g_strdup_printf ("hello");
}
