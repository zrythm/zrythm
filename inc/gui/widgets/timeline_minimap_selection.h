/*
 * gui/widgets/timeline_minimap_selection.h - Minimap
 *   selection
 *
 * Copyright (C) 2019 Alexandros Theodotou
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/** \file */

#ifndef __GUI_WIDGETS_TIMELINE_MINIMAP_SELECTION_H__
#define __GUI_WIDGETS_TIMELINE_MINIMAP_SELECTION_H__

#include "utils/ui.h"

#include <gtk/gtk.h>

#define TIMELINE_MINIMAP_SELECTION_WIDGET_TYPE \
  (timeline_minimap_selection_widget_get_type ())
G_DECLARE_FINAL_TYPE (TimelineMinimapSelectionWidget,
                      timeline_minimap_selection_widget,
                      Z,
                      TIMELINE_MINIMAP_SELECTION_WIDGET,
                      GtkBox)


typedef struct _TimelineMinimapSelectionWidget
{
  GtkBox                 parent_instance;
  GtkDrawingArea *       drawing_area;
  UiCursorState          cursor;
  TimelineMinimapWidget * parent; ///< pointer back to parent
} TimelineMinimapSelectionWidget;

TimelineMinimapSelectionWidget *
timeline_minimap_selection_widget_new (
  TimelineMinimapWidget * parent);

void
timeline_minimap_selection_widget_on_motion (GtkWidget * widget,
           GdkEventMotion *event,
           gpointer user_data);

#endif
