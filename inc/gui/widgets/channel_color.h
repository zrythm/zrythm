/*
 * gui/widgets/channel_color.h - channel color picker
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

#ifndef __GUI_WIDGETS_CHANNEL_COLOR_H__
#define __GUI_WIDGETS_CHANNEL_COLOR_H__

/**
 * \file
 *
 * Color picker for a channel strip.
 */

#include <gtk/gtk.h>

#define CHANNEL_COLOR_WIDGET_TYPE          (channel_color_widget_get_type ())
#define CHANNEL_COLOR_WIDGET(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CHANNEL_COLOR_WIDGET_TYPE, ChannelColor))
#define CHANNEL_COLOR_WIDGET_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST  ((klass), CHANNEL_COLOR_WIDGET, ChannelColorWidgetClass))
#define IS_CHANNEL_COLOR_WIDGET(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CHANNEL_COLOR_WIDGET_TYPE))
#define IS_CHANNEL_COLOR_WIDGET_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE  ((klass), CHANNEL_COLOR_WIDGET_TYPE))
#define CHANNEL_COLOR_WIDGET_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS  ((obj), CHANNEL_COLOR_WIDGET_TYPE, ChannelColorWidgetClass))

typedef struct ChannelColorWidget
{
  GtkDrawingArea         parent_instance;
  GdkRGBA                * color;          ///< color pointer to set/read value
} ChannelColorWidget;

typedef struct ChannelColorWidgetClass
{
  GtkDrawingAreaClass    parent_class;
} ChannelColorWidgetClass;

/**
 * Creates a channel color widget using the given color pointer.
 */
ChannelColorWidget *
channel_color_widget_new (GdkRGBA * color);


#endif
