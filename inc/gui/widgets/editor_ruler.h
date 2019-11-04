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

#define EDITOR_RULER_WIDGET_TYPE \
  (editor_ruler_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  EditorRulerWidget,
  editor_ruler_widget,
  Z, EDITOR_RULER_WIDGET,
  RulerWidget)

/**
 * @addtogroup widgets
 *
 * @{
 */

#define EDITOR_RULER MW_CLIP_EDITOR_INNER->ruler

typedef struct _RulerMarkerWidget RulerMarkerWidget;

typedef struct _EditorRulerWidget
{
  RulerWidget         parent_instance;

  /**
   * Markers.
   */
  RulerMarkerWidget *      loop_start;
  RulerMarkerWidget *      loop_end;
  RulerMarkerWidget *      clip_start;

  int                      range1_first; ///< range1 was before range2 at drag start
} EditorRulerWidget;

/**
 * Called when allocating the children of the
 * RulerWidget to allocate the RulerMarkerWidget.
 */
void
editor_ruler_widget_set_ruler_marker_position (
  EditorRulerWidget *   self,
  RulerMarkerWidget * rm,
  GtkAllocation *     allocation);

/**
 * Called from ruler drag begin.
 */
void
editor_ruler_on_drag_begin_no_marker_hit (
  EditorRulerWidget * self,
  gdouble             start_x,
  gdouble             start_y);

void
editor_ruler_on_drag_update (
  EditorRulerWidget * self,
  gdouble             offset_x,
  gdouble             offset_y);

void
editor_ruler_on_drag_end (
  EditorRulerWidget * self);

/**
 * @}
 */

#endif
