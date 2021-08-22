/*
 * Copyright (C) 2020-2021 Alexandros Theodotou <alex at zrythm dot org>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "audio/position.h"
#include "audio/snap_grid.h"
#include "gui/backend/audio_selections.h"
#include "gui/widgets/audio_arranger.h"
#include "project.h"

void
audio_arranger_widget_snap_range_r (
  ArrangerWidget * self,
  Position *       pos)
{
  if (self->resizing_range_start)
    {
      /* set range 1 at current point */
      ui_px_to_pos_editor (
        self->start_x, &AUDIO_SELECTIONS->sel_start,
        true);
      if (SNAP_GRID_ANY_SNAP (
            self->snap_grid) &&
          !self->shift_held)
        {
          position_snap_simple (
            &AUDIO_SELECTIONS->sel_start,
            SNAP_GRID_EDITOR);
        }
      position_set_to_pos (
        &AUDIO_SELECTIONS->sel_end,
        &AUDIO_SELECTIONS->sel_start);

      self->resizing_range_start = false;
    }

  /* set range */
  if (SNAP_GRID_ANY_SNAP (self->snap_grid) &&
      !self->shift_held)
    {
      position_snap_simple (pos, SNAP_GRID_EDITOR);
    }
  position_set_to_pos (
    &AUDIO_SELECTIONS->sel_end, pos);
  audio_selections_set_has_range (
    AUDIO_SELECTIONS, true);
}
