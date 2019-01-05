/*
 * gui/widgets/meter.h - Meter widget
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

#ifndef __GUI_WIDGETS_METER_H__
#define __GUI_WIDGETS_METER_H__

#include <gtk/gtk.h>

#define METER_WIDGET_TYPE                  (meter_widget_get_type ())
G_DECLARE_FINAL_TYPE (MeterWidget,
                      meter_widget,
                      METER,
                      WIDGET,
                      GtkDrawingArea)
#define IS_METER_WIDGET(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), METER_WIDGET_TYPE))

typedef enum MeterType
{
  METER_TYPE_MIDI,
  METER_TYPE_DB
} MeterType;

typedef struct _MeterWidget
{
  GtkDrawingArea         parent_instance;
  float                  (*getter)(void*); ///< getter function for value
  void *                 object;
  MeterType              type;
  int                    hover; ///< hovered or not
  GdkRGBA                start_color;
  GdkRGBA                end_color;
} MeterWidget;

/**
 * Creates a new Meter widget and binds it to the given value.
 */
void
meter_widget_setup (
  MeterWidget * self,
  float       (*getter)(void *),    ///< getter function
  void        * object,      ///< object to call get on
  MeterType   type,    ///< meter type
  int         width);

#endif
