/*
 * inc/gui/widgets/ruler.h - DigitalMeter
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __GUI_WIDGETS_DIGITAL_METER_H__
#define __GUI_WIDGETS_DIGITAL_METER_H__

#include <gtk/gtk.h>

#define DIGITAL_METER_WIDGET_TYPE                  (digital_meter_widget_get_type ())
#define DIGITAL_METER_WIDGET(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), DIGITAL_METER_WIDGET_TYPE, DigitalMeter))
#define DIGITAL_METER_WIDGET_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST  ((klass), DIGITAL_METER_WIDGET, DigitalMeterWidgetClass))
#define IS_DIGITAL_METER_WIDGET(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DIGITAL_METER_WIDGET_TYPE))
#define IS_DIGITAL_METER_WIDGET_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE  ((klass), DIGITAL_METER_WIDGET_TYPE))
#define DIGITAL_METER_WIDGET_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS  ((obj), DIGITAL_METER_WIDGET_TYPE, DigitalMeterWidgetClass))

typedef enum DigitalMeterType
{
  DIGITAL_METER_TYPE_BPM,
  DIGITAL_METER_TYPE_POSITION,
  DIGITAL_METER_TYPE_TIMESIG
} DigitalMeterType;

typedef struct DigitalMeterWidget
{
  GtkDrawingArea           parent_instance;
  DigitalMeterType         type;
  GtkGestureDrag           * drag;
  double                   last_y;
  int height_start_pos;
  int height_end_pos;

  /* for BPM */
  int                      num_part_start_pos;
  int                      num_part_end_pos;
  int                      dec_part_start_pos;
  int                      dec_part_end_pos;
  int                      update_num; ///< flag to update bpm
  int                      update_dec; ///< flag to update bpm decimal

  /* for position */
  int                      bars_start_pos;
  int                      bars_end_pos;
  int                      beats_start_pos;
  int                      beats_end_pos;
  int                      quarter_beats_start_pos;
  int                      quarter_beats_end_pos;
  int                      ticks_start_pos;
  int                      ticks_end_pos;
  int                      update_bars; ///< flag to update bars
  int                      update_beats; ///< flag to update beats
  int                      update_quarter_beats; ///< flag to update beats
  int                      update_ticks; ///< flag to update beats

  /* for time sig */
  SnapGrid *               snap_grid;
  int                      update_dens; ///< flag to update density
} DigitalMeterWidget;

typedef struct DigitalMeterWidgetClass
{
  GtkDrawingAreaClass       parent_class;
} DigitalMeterWidgetClass;

/**
 * Creates a digital meter with the given type (bpm or position).
 */
DigitalMeterWidget *
digital_meter_widget_new (DigitalMeterType      type,
                          SnapGrid *            snap_grid); ///< for timesig

#endif

