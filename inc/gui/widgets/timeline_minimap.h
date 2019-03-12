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

/** \file */

#ifndef __GUI_WIDGETS_TIMELINE_MINIMAP_H__
#define __GUI_WIDGETS_TIMELINE_MINIMAP_H__

#include "audio/position.h"

#include <gtk/gtk.h>

#define TIMELINE_MINIMAP_WIDGET_TYPE \
  (timeline_minimap_widget_get_type ())
G_DECLARE_FINAL_TYPE (TimelineMinimapWidget,
                      timeline_minimap_widget,
                      Z,
                      TIMELINE_MINIMAP_WIDGET,
                      GtkOverlay)

#define MW_TIMELINE_MINIMAP \
  MW_CENTER_DOCK_BOT_BOX->timeline_minimap

typedef struct _TimelineMinimapBgWidget TimelineMinimapBgWidget;
typedef struct _TimelineMinimapSelectionWidget TimelineMinimapSelectionWidget;
typedef struct TimelineMinimap TimelineMinimap;

typedef enum TimelineMinimapAction
{
  TIMELINE_MINIMAP_ACTION_NONE,
  TIMELINE_MINIMAP_ACTION_RESIZING_L,
  TIMELINE_MINIMAP_ACTION_RESIZING_R,
  TIMELINE_MINIMAP_ACTION_STARTING_MOVING, ///< in drag_start
  TIMELINE_MINIMAP_ACTION_MOVING, ///< in drag start,
    ///< also for dragging up/down bitwig style
} TimelineMinimapAction;

typedef struct _TimelineMinimapWidget
{
  GtkOverlay                        parent_instance;
  TimelineMinimapBgWidget *         bg;
  TimelineMinimapSelectionWidget  * selection;
  TimelineMinimapAction             action;
  GtkGestureDrag *         drag;
  GtkGestureMultiPress *   multipress;
  GtkGestureMultiPress *   right_mouse_mp;
  double                   last_offset_x;  ///< for dragging regions, selections
  double                   last_offset_y;  ///< for selections
  double                   selection_start_pos; ///< to be set in drag_begin
  double                   selection_end_pos; ///< to be set in drag_begin
  double                   start_x; ///< for dragging
  double                   start_y; ///< for dragging
  double                   start_zoom_level; //< to be set in drag_begin
  int                      n_press; ///< for multipress
} TimelineMinimapWidget;

/**
 * Taken from arranger.c
 */
void
timeline_minimap_widget_px_to_pos (
  TimelineMinimapWidget * self,
  Position *              pos,
  int                     px);

/**
 * Causes reallocation.
 */
void
timeline_minimap_widget_refresh (
  TimelineMinimapWidget * self);

#endif
