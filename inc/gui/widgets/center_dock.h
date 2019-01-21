/*
 * gui/widgets/center_dock.h - Main window widget
 *
 * Copyright (C) 2019 Alexandros Theodotou
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

#define CENTER_DOCK_WIDGET_TYPE \
  (center_dock_widget_get_type ())
G_DECLARE_FINAL_TYPE (CenterDockWidget,
                      center_dock_widget,
                      Z,
                      CENTER_DOCK_WIDGET,
                      DzlDockBin)
#define MW_CENTER_DOCK MAIN_WINDOW->center_dock

typedef struct _RulerWidget RulerWidget;
typedef struct _TracklistWidget TracklistWidget;
typedef struct _SnapGridWidget SnapGridWidget;
typedef struct _TimelineArrangerWidget TimelineArrangerWidget;
typedef struct _TimelineRulerWidget TimelineRulerWidget;
typedef struct _LeftDockEdgeWidget LeftDockEdgeWidget;
typedef struct _RightDockEdgeWidget RightDockEdgeWidget;
typedef struct _BotDockEdgeWidget BotDockEdgeWidget;
typedef struct _TopDockEdgeWidget TopDockEdgeWidget;
typedef struct _CenterDockBotBoxWidget CenterDockBotBoxWidget;
typedef struct _TracklistHeaderWidget TracklistHeaderWidget;

typedef struct _CenterDockWidget
{
  DzlDockBin               parent_instance;
  GtkBox                   * editor_top;
  GtkPaned                 * tracklist_timeline;
  GtkBox                   * tracklist_top;
  GtkScrolledWindow        * tracklist_scroll;
  GtkViewport              * tracklist_viewport;
  TracklistHeaderWidget *  tracklist_header;
  TracklistWidget          * tracklist;
  GtkScrolledWindow        * ruler_scroll;
  GtkViewport              * ruler_viewport;
  TimelineRulerWidget *    ruler;
  GtkScrolledWindow        * timeline_scroll;
  GtkViewport              * timeline_viewport;
  CenterDockBotBoxWidget * bot_box;
  TimelineArrangerWidget * timeline;
  LeftDockEdgeWidget *     left_dock_edge;
  RightDockEdgeWidget *    right_dock_edge;
  BotDockEdgeWidget *      bot_dock_edge;
  TopDockEdgeWidget *      top_dock_edge;
} CenterDockWidget;

void
center_dock_widget_setup (CenterDockWidget * self);

#endif
