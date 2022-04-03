/*
 * Copyright (C) 2019, 2021-2022 Alexandros Theodotou <alex at zrythm dot org>
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
 * Timeline minimap selection.
 */

#ifndef __GUI_WIDGETS_TIMELINE_MINIMAP_SELECTION_H__
#define __GUI_WIDGETS_TIMELINE_MINIMAP_SELECTION_H__

#include "utils/ui.h"

#include <gtk/gtk.h>

/**
 * @addtogroup widgets
 *
 * @{
 */

#define TIMELINE_MINIMAP_SELECTION_WIDGET_TYPE \
  (timeline_minimap_selection_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  TimelineMinimapSelectionWidget,
  timeline_minimap_selection_widget,
  Z,
  TIMELINE_MINIMAP_SELECTION_WIDGET,
  GtkWidget)

typedef struct _TimelineMinimapSelectionWidget
{
  GtkWidget parent_instance;

  UiCursorState cursor;

  /** Pointer back to parent. */
  TimelineMinimapWidget * parent;
} TimelineMinimapSelectionWidget;

TimelineMinimapSelectionWidget *
timeline_minimap_selection_widget_new (
  TimelineMinimapWidget * parent);

#if 0
void
timeline_minimap_selection_widget_on_motion (
  GtkWidget *      widget,
  GdkMotionEvent * event,
  gpointer         user_data);
#endif

/**
 * @}
 */

#endif
