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

#include "audio/channel.h"
#include "gui/widgets/meter.h"

#include <gtk/gtk.h>

#define CHANNEL_WIDGET_TYPE                  (channel_widget_get_type ())
G_DECLARE_FINAL_TYPE (ChannelWidget,
                      channel_widget,
                      CHANNEL,
                      WIDGET,
                      GtkGrid)
#define IS_CHANNEL_WIDGET(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CHANNEL_WIDGET_TYPE))

typedef struct _ColorAreaWidget ColorAreaWidget;
typedef struct KnobWidget KnobWidget;
typedef struct FaderWidget FaderWidget;
typedef struct ChannelMeterWidget ChannelMeterWidget;
typedef struct Channel Channel;
typedef struct _ChannelSlotWidget ChannelSlotWidget;
typedef struct PanWidget PanWidget;

typedef struct _ChannelWidget
{
  GtkGrid            parent_instance;
  GtkLabel            * output;
  ColorAreaWidget     * color;
  GtkLabel            * name;
  GtkBox              * phase_controls;
  GtkButton           * phase_invert;
  GtkLabel            * phase_reading;
  KnobWidget          * phase_knob;
  GtkBox              * slots_box;
  GtkBox              * slot_boxes[STRIP_SIZE];      ///< array of slot boxes (1 per plugin)
  ChannelSlotWidget   * slots[STRIP_SIZE];
  GtkToggleButton     * toggles[STRIP_SIZE];   ///< toggle buttons (per slot)
  GtkLabel            * labels[STRIP_SIZE];     ///< labels (per slot)
  //int                 num_slots;        ///< counter
  //GtkBox              * dummy_slot_box;    ///< for dnd
  GtkToggleButton     * slot1b;
  GtkToggleButton     * slot2b;
  //GtkButton           * add_slot;
  GtkButton           * e;
  GtkButton           * solo;
  GtkButton           * listen;
  GtkButton           * mute;
  GtkToggleButton     * record;
  GtkBox              * fader_area;
  GtkBox              * meter_area;  ///< vertical including reading
  GtkBox              * meters_box;  ///< box for l/r meters
  GtkBox *            pan_box;
  PanWidget *         pan;
  FaderWidget         * fader;
  MeterWidget         * meters[2];    ///< meter widgets (l/r)
  GtkLabel            * meter_reading;
  GtkImage            * icon;
  GtkImage *          output_img;
  Channel             * channel;    ///< pointer to data
} ChannelWidget;

/**
 * Updates the slots.
 */
void
channel_update_slots (ChannelWidget * self);

/**
 * Creates a channel widget using the given channel data.
 */
ChannelWidget *
channel_widget_new (Channel * channel);

/**
 * Updates the meter reading
 */
void
channel_widget_update_meter_reading (ChannelWidget * widget);

/**
 * Updates everything on the widget.
 *
 * It is reduntant but keeps code organized. Should fix if it causes lags.
 */
void
channel_widget_refresh (ChannelWidget * self);

/**
 * Displays the widget.
 */
void
channel_widget_show (ChannelWidget * self);


#endif
