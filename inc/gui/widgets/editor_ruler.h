/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

/**
 * \file
 *
 * MIDI ruler.
 */

#ifndef __GUI_WIDGETS_EDITOR_RULER_H__
#define __GUI_WIDGETS_EDITOR_RULER_H__

#include "gui/widgets/ruler.h"
#include "utils/ui.h"

#include <gtk/gtk.h>

/**
 * @addtogroup widgets
 *
 * @{
 */

#define EDITOR_RULER MW_CLIP_EDITOR_INNER->ruler

/**
 * Called from RulerWidget to draw the markers
 * specific to the editor, such as region loop
 * points.
 */
void
editor_ruler_widget_draw_markers (
  RulerWidget * self);

/**
 * Called from ruler drag begin.
 */
void
editor_ruler_on_drag_begin_no_marker_hit (
  RulerWidget * self,
  gdouble       start_x,
  gdouble       start_y);

void
editor_ruler_on_drag_update (
  RulerWidget * self,
  gdouble       offset_x,
  gdouble       offset_y);

void
editor_ruler_on_drag_end (RulerWidget * self);

int
editor_ruler_get_regions_in_range (
  RulerWidget * self,
  double        x_start,
  double        x_end,
  ZRegion **    regions);

/**
 * @}
 */

#endif
