/*
 * gui/widgets/channel_meter.h - The small channel meter next to the channel_meter
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

#ifndef __GUI_WIDGETS_CHANNEL_METER_H__
#define __GUI_WIDGETS_CHANNEL_METER_H__

#include <gtk/gtk.h>

#define CHANNEL_METER_WIDGET_TYPE                  (channel_meter_widget_get_type ())
#define CHANNEL_METER_WIDGET(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CHANNEL_METER_WIDGET_TYPE, ChannelMeterWidget))
#define CHANNEL_METER_WIDGET_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST  ((klass), CHANNEL_METER_WIDGET, ChannelMeterWidgetClass))
#define IS_CHANNEL_METER_WIDGET(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CHANNEL_METER_WIDGET_TYPE))
#define IS_CHANNEL_METER_WIDGET_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE  ((klass), CHANNEL_METER_WIDGET_TYPE))
#define CHANNEL_METER_WIDGET_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS  ((obj), CHANNEL_METER_WIDGET_TYPE, ChannelMeterWidgetClass))

typedef struct ChannelMeterWidget
{
  GtkDrawingArea         parent_instance;
  float (*getter_l)(void*);       ///< getter
  float (*getter_r)(void*);       ///< getter
  void *                object;
  int                    hover;
  GdkRGBA                start_color;
  GdkRGBA                end_color;
} ChannelMeterWidget;

typedef struct ChannelMeterWidgetClass
{
  GtkDrawingAreaClass    parent_class;
} ChannelMeterWidgetClass;

/**
 * Creates a new ChannelMeter widget and binds it to the given value.
 */
ChannelMeterWidget *
channel_meter_widget_new (float (*getter_l)(void *),    ///< getter function
                          float (*getter_r)(void *),
                  void * object,              ///< object to call get/set with
int width);

#endif

