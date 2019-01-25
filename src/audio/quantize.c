/*
 * project/quantize.c - Snap Grid info
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

#include "audio/engine.h"
#include "audio/quantize.h"
#include "audio/transport.h"
#include "project.h"

#include <gtk/gtk.h>

void
quantize_update_snap_points (Quantize * self)
{
  Position tmp, end_pos;
  position_init (&tmp);
  position_set_to_bar (&end_pos,
                       TRANSPORT->total_bars + 1);
  self->num_snap_points = 0;
  position_set_to_pos (
    &self->snap_points[self->num_snap_points++],
    &tmp);
  while (position_compare (&tmp, &end_pos)
           < 0)
    {
      position_add_ticks (
        &tmp,
        snap_grid_get_note_ticks (self->note_length,
                                  self->note_type));
      /*position_print (&tmp);*/
      position_set_to_pos (
        &self->snap_points[self->num_snap_points++],
        &tmp);
    }
}

/**
 * Initializes the quantize struct.
 */
void
quantize_init (Quantize *   self,
               NoteLength   note_length)
{
  self->use_grid = 1;
  self->note_length = note_length;
  self->num_snap_points = 0;
  self->note_type = NOTE_TYPE_NORMAL;
}
