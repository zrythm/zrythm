/*
 * gui/widgets/pan.h - Pan widget
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

/** \file
 */

#ifndef __GUI_WIDGETS_PAN_H__
#define __GUI_WIDGETS_PAN_H__

#include <gtk/gtk.h>

#define PAN_WIDGET_TYPE                  (pan_widget_get_type ())
#define PAN_WIDGET(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), PAN_WIDGET_TYPE, PanWidget))
#define PAN_WIDGET_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST  ((klass), PAN_WIDGET, PanWidgetClass))
#define IS_PAN_WIDGET(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PAN_WIDGET_TYPE))
#define IS_PAN_WIDGET_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE  ((klass), PAN_WIDGET_TYPE))
#define PAN_WIDGET_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS  ((obj), PAN_WIDGET_TYPE, PanWidgetClass))

typedef struct PanWidget
{
  GtkDrawingArea         parent_instance;
  GtkGestureDrag         * drag;
  float                  (*getter)(void*); ///< getter function for value
  void (*setter)(void*, float);       ///< getter
  void *                 object;
  double                 last_x;
  double                 last_y;
  int                    hover; ///< hovered or not
  GdkRGBA                start_color;
  GdkRGBA                end_color;
} PanWidget;

typedef struct PanWidgetClass
{
  GtkDrawingAreaClass    parent_class;
} PanWidgetClass;

/**
 * Creates a new Pan widget and binds it to the given value.
 */
PanWidget *
pan_widget_new (float       (*getter)(void *),    ///< getter function
                  void (*set_val)(void *, float),    ///< setter function
                  void        * object,      ///< object to call get on
                  int         height);

#endif


