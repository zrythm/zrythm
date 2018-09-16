/*
 * gui/widgets/instrument_track.h - Instrument track widget
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

#ifndef __GUI_WIDGETS_INSTRUMENT_TRACK_H__
#define __GUI_WIDGETS_INSTRUMENT_TRACK_H__

#include <gtk/gtk.h>

#define INSTRUMENT_TRACK_WIDGET_TYPE                  (instrument_track_widget_get_type ())
#define INSTRUMENT_TRACK_WIDGET(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), INSTRUMENT_TRACK_WIDGET_TYPE, InstrumentTrackWidget))
#define INSTRUMENT_TRACK_WIDGET_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST  ((klass), INSTRUMENT_TRACK_WIDGET, InstrumentTrackWidgetClass))
#define IS_INSTRUMENT_TRACK_WIDGET(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), INSTRUMENT_TRACK_WIDGET_TYPE))
#define IS_INSTRUMENT_TRACK_WIDGET_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE  ((klass), INSTRUMENT_TRACK_WIDGET_TYPE))
#define INSTRUMENT_TRACK_WIDGET_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS  ((obj), INSTRUMENT_TRACK_WIDGET_TYPE, InstrumentTrackWidgetClass))

typedef struct ColorAreaWidget ColorAreaWidget;
typedef struct KnobWidget KnobWidget;
typedef struct FaderWidget FaderWidget;
typedef struct ChannelMeterWidget ChannelMeterWidget;
typedef struct Channel Channel;
typedef struct ChannelSlotWidget ChannelSlotWidget;

typedef struct InstrumentTrackWidget
{
  GtkGrid            parent_instance;
  GtkLabel            * name;
  ColorAreaWidget     * color;
  GtkBox              * phase_controls;
  GtkButton           * phase_invert;
  GtkLabel            * phase_reading;
  KnobWidget          * phase_knob;
  GtkBox              * slots_box;
  GtkBox              * slot_boxes[MAX_PLUGINS];      ///< array of slot boxes (1 per plugin)
  ChannelSlotWidget   * slots[MAX_PLUGINS];
  GtkToggleButton     * toggles[MAX_PLUGINS];   ///< toggle buttons (per slot)
  GtkLabel            * labels[MAX_PLUGINS];     ///< labels (per slot)
  //int                 num_slots;        ///< counter
  //GtkBox              * dummy_slot_box;    ///< for dnd
  GtkToggleButton     * slot1b;
  GtkToggleButton     * slot2b;
  //GtkButton           * add_slot;
  GtkDrawingArea      * pan;
  GtkButton           * e;
  GtkButton           * solo;
  GtkButton           * listen;
  GtkButton           * mute;
  GtkToggleButton     * record;
  GtkBox              * fader_area;
  GtkBox              * meter_area;
  FaderWidget         * fader;
  ChannelMeterWidget  * cm;          ///< the little green channel meter
  GtkLabel            * meter_reading;
  GtkImage            * icon;
  int                 selected;  ///< selected or not

  Channel             * channel;    ///< pointer to data
} InstrumentTrackWidget;

typedef struct InstrumentTrackWidgetClass
{
  GtkGridClass       parent_class;
} InstrumentTrackWidgetClass;

/**
 * Creates an instrument track widget using the given channel data.
 */
InstrumentTrackWidget *
instrument_track_widget_new (Channel * channel);

/**
 * Updates the meter reading
 */
gboolean
instrument_track_widget_update_meter_reading (InstrumentTrackWidget * widget);

#endif

