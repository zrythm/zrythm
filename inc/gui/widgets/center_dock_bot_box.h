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
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __GUI_WIDGETS_CENTER_DOCK_BOT_BOX_H__
#define __GUI_WIDGETS_CENTER_DOCK_BOT_BOX_H__

#include <gtk/gtk.h>

#define CENTER_DOCK_BOT_BOX_WIDGET_TYPE \
  (center_dock_bot_box_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  CenterDockBotBoxWidget,
  center_dock_bot_box_widget,
  Z, CENTER_DOCK_BOT_BOX_WIDGET,
  GtkBox)

#define MW_CENTER_DOCK_BOT_BOX \
  MW_CENTER_DOCK->bot_box

typedef struct _TimelineMinimapWidget TimelineMinimapWidget;

typedef struct _CenterDockBotBoxWidget
{
  GtkBox                   parent_instance;
  GtkToolbar               * left_tb;
  GtkToolButton            * instrument_add;
  GtkToolButton *          toggle_left_dock;
  GtkToolButton *          toggle_bot_dock;
  GtkToolbar *             right_tb;
  GtkToolButton *          toggle_right_dock;
  TimelineMinimapWidget *  timeline_minimap;
} CenterDockBotBoxWidget;

#endif
