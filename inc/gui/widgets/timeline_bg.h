/*
 * inc/gui/widgets/timeline_bg.h - Timeline background
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __GUI_WIDGETS_TIMELINE_BG_H__
#define __GUI_WIDGETS_TIMELINE_BG_H__

#include <gtk/gtk.h>

#define TIMELINE_BG_WIDGET_TYPE                  (timeline_bg_widget_get_type ())
#define TIMELINE_BG_WIDGET(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), TIMELINE_BG_WIDGET_TYPE, TimelineBg))
#define TIMELINE_BG_WIDGET_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST  ((klass), TIMELINE_BG_WIDGET, TimelineBgWidgetClass))
#define IS_TIMELINE_BG_WIDGET(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TIMELINE_BG_WIDGET_TYPE))
#define IS_TIMELINE_BG_WIDGET_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE  ((klass), TIMELINE_BG_WIDGET_TYPE))
#define TIMELINE_BG_WIDGET_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS  ((obj), TIMELINE_BG_WIDGET_TYPE, TimelineBgWidgetClass))

typedef struct TimelineBgWidget
{
  GtkDrawingArea           parent_instance;
  int                      total_px;
  double                   start_x; ///< for dragging
  GtkGestureDrag           * drag;
  GtkGestureMultiPress     * multipress;
} TimelineBgWidget;

typedef struct TimelineBgWidgetClass
{
  GtkDrawingAreaClass       parent_class;
} TimelineBgWidgetClass;

/**
 * Creates a timeline widget using the given timeline data.
 */
TimelineBgWidget *
timeline_bg_widget_new ();

#endif


