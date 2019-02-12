/*
 * Copyright (C) 2018-2019 Alexandros Theodotou
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
G_DECLARE_FINAL_TYPE (TimelineRulerWidget,
                      timeline_ruler_widget,
                      Z,
                      TIMELINE_RULER_WIDGET,
                      RulerWidget)

#define MW_RULER MW_CENTER_DOCK->ruler

typedef struct _RulerRangeWidget RulerRangeWidget;
typedef struct _RulerMarkerWidget RulerMarkerWidget;

/**
 * The timeline ruler widget target acting upon.
 */
typedef enum TRWTarget
{
  TRW_TARGET_PLAYHEAD,
  TRW_TARGET_LOOP_START,
  TRW_TARGET_LOOP_END,
  TRW_TARGET_SONG_START,
  TRW_TARGET_SONG_END,
  TRW_TARGET_RANGE, ///< FIXME delete
} TRWTarget;

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
  RulerMarkerWidget *      song_start;
  RulerMarkerWidget *      song_end;
  RulerMarkerWidget *      loop_start;
  RulerMarkerWidget *      loop_end;
  RulerMarkerWidget *      cue_point;

  /**
   * Dragging playhead or creating range, etc.
   */
  UiOverlayAction          action;
  TRWTarget                target; ///< object working upon
  double                   start_x; ///< for dragging
  double                   last_offset_x;
  int                      range1_first; ///< range1 was before range2 at drag start
  GtkGestureDrag *         drag;
  GtkGestureMultiPress *   multipress;
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

#endif
