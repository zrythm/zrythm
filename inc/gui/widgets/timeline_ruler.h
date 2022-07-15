/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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
 * Timeline ruler derived from base ruler.
 */

#ifndef __GUI_WIDGETS_TIMELINE_RULER_H__
#define __GUI_WIDGETS_TIMELINE_RULER_H__

#include "gui/widgets/ruler.h"
#include "utils/ui.h"

#include <gtk/gtk.h>

typedef struct _RulerRangeWidget  RulerRangeWidget;
typedef struct _RulerMarkerWidget RulerMarkerWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_RULER MW_TIMELINE_PANEL->ruler

void
timeline_ruler_widget_draw_markers (RulerWidget * self);

/**
 * Called from ruler drag begin.
 */
void
timeline_ruler_on_drag_begin_no_marker_hit (
  RulerWidget * self,
  gdouble       start_x,
  gdouble       start_y,
  int           height);

/**
 * Called from ruler drag end.
 */
void
timeline_ruler_on_drag_end (RulerWidget * self);

/**
 * Called from ruler drag update.
 */
void
timeline_ruler_on_drag_update (
  RulerWidget * self,
  gdouble       offset_x,
  gdouble       offset_y);

/**
 * @}
 */

#endif
