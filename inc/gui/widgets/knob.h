/*
 * gui/widgets/knob.h - knob
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

#ifndef __GUI_WIDGETS_KNOB_H__
#define __GUI_WIDGETS_KNOB_H__

#include <gtk/gtk.h>

#define KNOB_WIDGET_TYPE          (knob_widget_get_type ())
#define KNOB_WIDGET(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), KNOB_WIDGET_TYPE, Knob))
#define KNOB_WIDGET_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST  ((klass), KNOB_WIDGET, KnobWidgetClass))
#define IS_KNOB_WIDGET(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), KNOB_WIDGET_TYPE))
#define IS_KNOB_WIDGET_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE  ((klass), KNOB_WIDGET_TYPE))
#define KNOB_WIDGET_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS  ((obj), KNOB_WIDGET_TYPE, KnobWidgetClass))

typedef struct KnobWidget
{
  GtkDrawingArea        parent_instance;
  float (*getter)(void*);       ///< getter
  void (*setter)(void*, float);       ///< getter
  void *                object;
  int                   size;  ///< size in px
  int                   hover;   ///< used to detect if hovering or not
  float                 zero;   ///<   zero point 0.0-1.0 */
  int                   arc;    ///< draw arc around the knob or not
  int                   bevel;  ///< bevel
  int                   flat;    ///< flat or 3D
  float                 min;    ///< min value (eg. 1)
  float                 max;    ///< max value (eg. 180)
  GdkRGBA              start_color;    ///< color away from zero point
  GdkRGBA              end_color;     ///< color close to zero point
  GtkGestureDrag        *drag;     ///< used for drag gesture
  double                last_x;    ///< used in gesture drag
  double                last_y;    ///< used in gesture drag
} KnobWidget;

typedef struct KnobWidgetClass
{
  GtkDrawingAreaClass   parent_class;
} KnobWidgetClass;

/**
 * Creates a knob widget with the given options and binds it to the given value.
 */
KnobWidget *
knob_widget_new (float (*get_val)(void *),    ///< getter function
                 void (*set_val)(void *, float),    ///< setter function
                 void * object,              ///< object to call get/set with
                 float  min,
                 float  max,
                 int    size,
                 float  zero);



#endif
