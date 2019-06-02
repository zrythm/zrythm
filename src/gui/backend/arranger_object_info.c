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
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "gui/backend/arranger_object_info.h"

#include <gtk/gtk.h>

/**
 * Returns whether the object is transient or not.
 *
 * Transient objects are objects that are used
 * during moving operations.
 */
int
arranger_object_info_is_transient (
  ArrangerObjectInfo * self)
{
  return (self->type == AOI_TYPE_MAIN_TRANSIENT ||
          self->type == AOI_TYPE_LANE_TRANSIENT);
}

/**
 * Returns whether the object is a lane object or not
 * (only applies to TimelineArrangerWidget objects.
 */
int
arranger_object_info_is_lane (
  ArrangerObjectInfo * self)
{
  return (self->type == AOI_TYPE_LANE ||
          self->type == AOI_TYPE_LANE_TRANSIENT);
}
