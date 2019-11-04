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

#define TIMELINE_RULER_WIDGET_TYPE \
  (timeline_ruler_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  TimelineRulerWidget,
  timeline_ruler_widget,
  Z, TIMELINE_RULER_WIDGET,
  RulerWidget)

typedef struct _RulerRangeWidget RulerRangeWidget;
typedef struct _RulerMarkerWidget RulerMarkerWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_RULER MW_TIMELINE_PANEL->ruler

typedef struct _TimelineRulerWidget
{
  RulerWidget              parent_instance;
  RulerRangeWidget *       range;

  /**
   * Set on drag begin.
   *
   * Useful for moving range.
   */
  Position                 range1_start_pos;
  Position                 range2_start_pos;

  /**
   * Markers.
   */
  RulerMarkerWidget *      loop_start;
  RulerMarkerWidget *      loop_end;
  RulerMarkerWidget *      cue_point;

  /** range1 was before range2 at drag start. */
  int                      range1_first;
} TimelineRulerWidget;

void
timeline_ruler_widget_set_ruler_range_position (
  TimelineRulerWidget * self,
  RulerRangeWidget *    rr,
  GtkAllocation *       allocation);

void
timeline_ruler_widget_set_ruler_marker_position (
  TimelineRulerWidget * self,
  RulerMarkerWidget *    rr,
  GtkAllocation *       allocation);

/**
 * Called from ruler drag begin.
 */
void
timeline_ruler_on_drag_begin_no_marker_hit (
  TimelineRulerWidget * self,
  gdouble               start_x,
  gdouble               start_y,
  int                  height);

/**
 * Called from ruler drag end.
 */
void
timeline_ruler_on_drag_end (
  TimelineRulerWidget * self);

/**
 * Called from ruler drag update.
 */
void
timeline_ruler_on_drag_update (
  TimelineRulerWidget * self,
  gdouble               offset_x,
  gdouble               offset_y);

/**
 * @}
 */

#endif
