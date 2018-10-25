/*
 * gui/widgets/track.h - Track view
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

#ifndef __GUI_WIDGETS_TRACK_H__
#define __GUI_WIDGETS_TRACK_H__

#include "audio/channel.h"

#include <gtk/gtk.h>

#define TRACK_WIDGET_TYPE                  (track_widget_get_type ())
#define TRACK_WIDGET(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), TRACK_WIDGET_TYPE, TrackWidget))
#define TRACK_WIDGET_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST  ((klass), TRACK_WIDGET, TrackWidgetClass))
#define IS_TRACK_WIDGET(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TRACK_WIDGET_TYPE))
#define IS_TRACK_WIDGET_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE  ((klass), TRACK_WIDGET_TYPE))
#define TRACK_WIDGET_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS  ((obj), TRACK_WIDGET_TYPE, TrackWidgetClass))

typedef struct ColorAreaWidget ColorAreaWidget;

typedef struct TrackWidget
{
  GtkGrid                 parent_instance;
  GtkBox                  * color_box;
  ColorAreaWidget         * color;
  GtkLabel                * track_name;
  GtkButton               * record;
  GtkButton               * solo;
  GtkButton               * mute;
  GtkButton               * show_automation;
  GtkImage                * icon;
  GtkBox *                automation_box;
  GtkPaned *              automation_paned;
  Track                   * track;
} TrackWidget;

typedef struct TrackWidgetClass
{
  GtkGridClass    parent_class;
} TrackWidgetClass;

/**
 * Creates a new track widget from the given track.
 */
TrackWidget *
track_widget_new (Track * track);

/**
 * Updates changes in the backend to the ui
 */
void
track_widget_update_all (TrackWidget * self);

#endif

