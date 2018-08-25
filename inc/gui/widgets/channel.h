/*
 * gui/widgets/channel.h - A channel widget
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

#ifndef __GUI_WIDGETS_CHANNEL_H__
#define __GUI_WIDGETS_CHANNEL_H__

#include <gtk/gtk.h>

#define CHANNEL_WIDGET_TYPE                  (channel_widget_get_type ())
#define CHANNEL_WIDGET(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CHANNEL_WIDGET_TYPE, Channel))
#define CHANNEL_WIDGET_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST  ((klass), CHANNEL_WIDGET, ChannelWidgetClass))
#define IS_CHANNEL_WIDGET(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CHANNEL_WIDGET_TYPE))
#define IS_CHANNEL_WIDGET_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE  ((klass), CHANNEL_WIDGET_TYPE))
#define CHANNEL_WIDGET_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS  ((obj), CHANNEL_WIDGET_TYPE, ChannelWidgetClass))

typedef struct ChannelColorWidget ChannelColorWidget;
typedef struct KnobWidget KnobWidget;
typedef struct FaderWidget FaderWidget;
typedef struct ChannelMeterWidget ChannelMeterWidget;
typedef struct Channel Channel;

typedef struct ChannelWidget
{
  GtkGrid            parent_instance;
  GtkLabel            * output;
  ChannelColorWidget  * color;
  GtkLabel            * name;
  GtkBox              * phase_controls;
  GtkButton           * phase_invert;
  GtkLabel            * phase_reading;
  KnobWidget          * phase_knob;
  GtkBox              * slots;
  GtkToggleButton     * slot1b;
  GtkToggleButton     * slot2b;
  GtkButton           * add_slot;
  GtkDrawingArea      * pan;
  GtkButton           * e;
  GtkButton           * solo;
  GtkButton           * listen;
  GtkButton           * mute;
  GtkToggleButton     * record;
  GtkBox              * fader_area;
  GtkBox              * meter_area;
  FaderWidget         * fader;
  ChannelMeterWidget  * cm;
  GtkDrawingArea      * meter_reading;
  GtkImage            * icon;

  Channel             * channel;    ///< pointer to data
} ChannelWidget;

typedef struct ChannelWidgetClass
{
  GtkGridClass       parent_class;
} ChannelWidgetClass;

/**
 * Creates a channel widget using the given channel data.
 */
ChannelWidget *
channel_widget_new (Channel * channel);

#endif
