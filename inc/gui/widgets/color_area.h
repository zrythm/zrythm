/*
 * gui/widgets/color_area.h - channel color picker
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

#ifndef __GUI_WIDGETS_COLOR_AREA_H__
#define __GUI_WIDGETS_COLOR_AREA_H__

/**
 * \file
 *
 * Color picker for a channel strip.
 */

#include <gtk/gtk.h>

#define COLOR_AREA_WIDGET_TYPE          (color_area_widget_get_type ())
#define COLOR_AREA_WIDGET(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), COLOR_AREA_WIDGET_TYPE, ColorArea))
#define COLOR_AREA_WIDGET_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST  ((klass), COLOR_AREA_WIDGET, ColorAreaWidgetClass))
#define IS_COLOR_AREA_WIDGET(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), COLOR_AREA_WIDGET_TYPE))
#define IS_COLOR_AREA_WIDGET_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE  ((klass), COLOR_AREA_WIDGET_TYPE))
#define COLOR_AREA_WIDGET_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS  ((obj), COLOR_AREA_WIDGET_TYPE, ColorAreaWidgetClass))

typedef struct ColorAreaWidget
{
  GtkDrawingArea         parent_instance;
  GdkRGBA                * color;          ///< color pointer to set/read value
} ColorAreaWidget;

typedef struct ColorAreaWidgetClass
{
  GtkDrawingAreaClass    parent_class;
} ColorAreaWidgetClass;

/**
 * Creates a channel color widget using the given color pointer.
 */
ColorAreaWidget *
color_area_widget_new (GdkRGBA * color);


#endif
