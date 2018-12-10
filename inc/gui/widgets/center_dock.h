/*
 * gui/widgets/center_dock.h - Main window widget
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

#ifndef __GUI_WIDGETS_CENTER_DOCK_H__
#define __GUI_WIDGETS_CENTER_DOCK_H__

#include <gtk/gtk.h>
#include <dazzle.h>

#define CENTER_DOCK_WIDGET_TYPE                  (center_dock_widget_get_type ())
G_DECLARE_FINAL_TYPE (CenterDockWidget,
                      center_dock_widget,
                      CENTER_DOCK,
                      WIDGET,
                      DzlDockBin)
#define MW_CENTER_DOCK MAIN_WINDOW->center_dock

typedef struct _RulerWidget RulerWidget;
typedef struct _TracklistWidget TracklistWidget;
typedef struct SnapGridWidget SnapGridWidget;
typedef struct _TimelineArrangerWidget TimelineArrangerWidget;
typedef struct _LeftDockEdgeWidget LeftDockEdgeWidget;
typedef struct _RightDockEdgeWidget RightDockEdgeWidget;
typedef struct _BotDockEdgeWidget BotDockEdgeWidget;

typedef struct _CenterDockWidget
{
  DzlDockBin               parent_instance;
  GtkBox                   * editor_top;
  GtkPaned                 * tracklist_timeline;
  GtkBox                   * tracklist_top;
  GtkScrolledWindow        * tracklist_scroll;
  GtkViewport              * tracklist_viewport;
  TracklistWidget          * tracklist;
  GtkGrid                  * tracklist_header;
  GtkBox                   * timeline_ruler;
  GtkScrolledWindow        * ruler_scroll;
  GtkViewport              * ruler_viewport;
  RulerWidget              * ruler;
  GtkScrolledWindow        * timeline_scroll;
  GtkViewport              * timeline_viewport;
  TimelineArrangerWidget * timeline;
  GtkToolbar               * instruments_toolbar;
  GtkToolButton            * instrument_add;
  GtkBox                   * snap_grid_midi_box;
  SnapGridWidget           * snap_grid_midi;
  LeftDockEdgeWidget *     left_dock_edge;
  RightDockEdgeWidget *    right_dock_edge;
  BotDockEdgeWidget *      bot_dock_edge;
} CenterDockWidget;

#endif

