/*
 * Copyright (C) 2019, 2022 Alexandros Theodotou <alex at zrythm dot org>
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

/**
 * @file
 *
 * Timeline minimap bg.
 */

#ifndef __GUI_WIDGETS_TIMELINE_MINIMAP_BG_H__
#define __GUI_WIDGETS_TIMELINE_MINIMAP_BG_H__

#include <gtk/gtk.h>

#define TIMELINE_MINIMAP_BG_WIDGET_TYPE \
  (timeline_minimap_bg_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  TimelineMinimapBgWidget,
  timeline_minimap_bg_widget,
  Z,
  TIMELINE_MINIMAP_BG_WIDGET,
  GtkWidget)

typedef struct _TimelineMinimapBgWidget
{
  GtkWidget parent_instance;
} TimelineMinimapBgWidget;

TimelineMinimapBgWidget *
timeline_minimap_bg_widget_new (void);

#endif
