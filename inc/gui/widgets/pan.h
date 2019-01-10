/*
 * gui/widgets/pan.h - Pan widget
 *
 * Copyright (C) 2019 Alexandros Theodotou
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

#define PAN_WIDGET_TYPE \
  (pan_widget_get_type ())
G_DECLARE_FINAL_TYPE (PanWidget,
                      pan_widget,
                      Z,
                      PAN_WIDGET,
                      GtkDrawingArea)

typedef struct _PanWidget
{
  GtkDrawingArea         parent_instance;
  GtkGestureDrag         * drag;
  float                  (*getter)(void*); ///< getter function for value
  void (*setter)(void*, float);       ///< getter
  void *                 object;
  double                 last_x;
  double                 last_y;
  GdkRGBA                start_color;
  GdkRGBA                end_color;
} PanWidget;

/**
 * Creates a new Pan widget and binds it to the given value.
 */
PanWidget *
pan_widget_new (float       (*getter)(void *),    ///< getter function
                  void (*set_val)(void *, float),    ///< setter function
                  void        * object,      ///< object to call get on
                  int         height);

#endif
