/*
 * gui/instrument_timeline_view.h - The view of an instrument left of its
 *   timeline counterpart
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

#ifndef __GUI_WIDGETS_TRACKS_H__
#define __GUI_WIDGETS_TRACKS_H__

#include <gtk/gtk.h>

#define USE_WIDE_HANDLE 1

#define TRACKS_WIDGET_TYPE                  (tracks_widget_get_type ())
#define TRACKS_WIDGET(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), TRACKS_WIDGET_TYPE, TracksWidget))
#define TRACKS_WIDGET_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST  ((klass), TRACKS_WIDGET, TracksWidgetClass))
#define IS_TRACKS_WIDGET(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TRACKS_WIDGET_TYPE))
#define IS_TRACKS_WIDGET_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE  ((klass), TRACKS_WIDGET_TYPE))
#define TRACKS_WIDGET_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS  ((obj), TRACKS_WIDGET_TYPE, TracksWidgetClass))

typedef struct Channel Channel;

typedef struct TracksWidget
{
  GtkPaned               parent_instance;  ///< parent paned with master
  GtkPaned               * tracks[100]; ///< paned stacks
  GtkBox                 * dummy_box;
} TracksWidget;

typedef struct TracksWidgetClass
{
  GtkPanedClass    parent_class;
} TracksWidgetClass;

/**
 * Creates a new tracks widget and sets it to main window.
 */
void
tracks_widget_setup ();

void
tracks_widget_add_channel (TracksWidget * self, Channel * channel);

#endif
