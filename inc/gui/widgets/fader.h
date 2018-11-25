/*
 * gui/widgets/fader.h - fader
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

#ifndef __GUI_WIDGETS_FADER_H__
#define __GUI_WIDGETS_FADER_H__

#include <gtk/gtk.h>

#define FADER_WIDGET_TYPE                  (fader_widget_get_type ())
#define FADER_WIDGET(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), FADER_WIDGET_TYPE, FaderWidget))
#define FADER_WIDGET_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST  ((klass), FADER_WIDGET, FaderWidgetClass))
#define IS_FADER_WIDGET(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FADER_WIDGET_TYPE))
#define IS_FADER_WIDGET_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE  ((klass), FADER_WIDGET_TYPE))
#define FADER_WIDGET_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS  ((obj), FADER_WIDGET_TYPE, FaderWidgetClass))

typedef struct FaderWidget
{
  GtkDrawingArea         parent_instance;
  GtkGestureDrag *       drag;
  float (*getter)(void*); ///< getter
  void (*setter)(void*, float); ///< setter
  void *                 object;
  double                 last_x;
  double                 last_y;
  int                    hover;
  GdkRGBA                start_color;
  GdkRGBA                end_color;
} FaderWidget;

typedef struct FaderWidgetClass
{
  GtkDrawingAreaClass    parent_class;
} FaderWidgetClass;

/**
 * Creates a new Fader widget and binds it to the given value.
 */
FaderWidget *
fader_widget_new (float (*get_val)(void *),    ///< getter function
                  void (*set_val)(void *, float),    ///< setter function
                  void * object,              ///< object to call get/set with
int width);

#endif
