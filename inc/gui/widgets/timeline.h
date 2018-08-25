/*
 * inc/gui/widgets/timeline.h - Timeline
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

#ifndef __GUI_WIDGETS_TIMELINE_H__
#define __GUI_WIDGETS_TIMELINE_H__

#include <gtk/gtk.h>

#define TIMELINE_WIDGET_TYPE                  (timeline_widget_get_type ())
#define TIMELINE_WIDGET(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), TIMELINE_WIDGET_TYPE, Timeline))
#define TIMELINE_WIDGET_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST  ((klass), TIMELINE_WIDGET, TimelineWidgetClass))
#define IS_TIMELINE_WIDGET(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TIMELINE_WIDGET_TYPE))
#define IS_TIMELINE_WIDGET_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE  ((klass), TIMELINE_WIDGET_TYPE))
#define TIMELINE_WIDGET_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS  ((obj), TIMELINE_WIDGET_TYPE, TimelineWidgetClass))

typedef struct TimelineWidget
{
  GtkDrawingArea           parent_instance;
} TimelineWidget;

typedef struct TimelineWidgetClass
{
  GtkDrawingAreaClass       parent_class;
} TimelineWidgetClass;

/**
 * Creates a timeline widget using the given timeline data.
 */
TimelineWidget *
timeline_widget_new (GtkWidget * _multi_paned,
              GtkWidget * overlay);

#endif

