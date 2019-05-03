/*
 * Copyright (C) 2018-2019 Alexandros Theodotou
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

#define CHANNEL_WIDGET_TYPE \
  (channel_widget_get_type ())
G_DECLARE_FINAL_TYPE (ChannelWidget,
                      channel_widget,
                      Z,
                      CHANNEL_WIDGET,
                      GtkGrid)

typedef struct _ColorAreaWidget ColorAreaWidget;
typedef struct _KnobWidget KnobWidget;
typedef struct _FaderWidget FaderWidget;
typedef struct _ChannelMeterWidget ChannelMeterWidget;
typedef struct Channel Channel;
typedef struct _ChannelSlotWidget ChannelSlotWidget;
typedef struct _RouteTargetSelectorWidget
  RouteTargetSelectorWidget;
typedef struct _PanWidget PanWidget;

typedef struct _ChannelWidget
{
  GtkGrid            parent_instance;
  RouteTargetSelectorWidget * output;
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
  GtkToggleButton *      solo;
  GtkToggleButton           * listen;
  GtkToggleButton           * mute;
  GtkToggleButton     * record;
  GtkBox              * meter_area;  ///< vertical including reading
  GtkBox *            pan_box;
  PanWidget *         pan;
  FaderWidget         * fader;
  MeterWidget         * meter_l;
  MeterWidget         * meter_r;
  GtkLabel            * meter_reading;
  GtkImage            * icon;
  double                meter_reading_val; ///< cache
  Channel             * channel;    ///< pointer to data

  /**
   * Signal handler IDs.
   */
  gulong              record_toggled_handler_id;
  gulong              solo_toggled_handler_id;
  gulong              mute_toggled_handler_id;
} ChannelWidget;

/**
 * Updates the slots.
 */
void
channel_update_slots (ChannelWidget * self);

/**
 * Blocks all signal handlers.
 */
void
channel_widget_block_all_signal_handlers (
  ChannelWidget * self);

/**
 * Unblocks all signal handlers.
 */
void
channel_widget_unblock_all_signal_handlers (
  ChannelWidget * self);

/**
 * Creates a channel widget using the given channel data.
 */
ChannelWidget *
channel_widget_new (Channel * channel);

/**
 * Updates the meter reading
 */
gboolean
channel_widget_update_meter_reading (
  ChannelWidget * widget,
  GdkFrameClock * frame_clock,
  gpointer        user_data);

/**
 * Updates everything on the widget.
 *
 * It is reduntant but keeps code organized. Should fix if it causes lags.
 */
void
channel_widget_refresh (ChannelWidget * self);

void
channel_widget_refresh_buttons (
  ChannelWidget * self);

/**
 * Displays the widget.
 */
void
channel_widget_show (ChannelWidget * self);


#endif
