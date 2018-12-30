/*
 * gui/widgets/center_dock_bot_box.h - Main window widget
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

#ifndef __GUI_WIDGETS_CENTER_DOCK_BOT_BOX_H__
#define __GUI_WIDGETS_CENTER_DOCK_BOT_BOX_H__

#include <gtk/gtk.h>
#include <dazzle.h>

#define CENTER_DOCK_BOT_BOX_WIDGET_TYPE                  (center_dock_bot_box_widget_get_type ())
G_DECLARE_FINAL_TYPE (CenterDockBotBoxWidget,
                      center_dock_bot_box_widget,
                      CENTER_DOCK_BOT_BOX,
                      WIDGET,
                      GtkBox)

typedef struct _CenterDockBotBoxWidget
{
  GtkBox                   parent_instance;
  GtkToolbar               * left_tb;
  GtkToolButton            * instrument_add;
  GtkToolButton *          toggle_left_dock;
  GtkToolButton *          toggle_bot_dock;
  SnapGridWidget           * snap_grid_midi;
  GtkToolbar *             right_tb;
  GtkToolButton *          toggle_right_dock;
} CenterDockBotBoxWidget;

#endif
