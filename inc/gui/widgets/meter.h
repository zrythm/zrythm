/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/** \file
 */

#ifndef __GUI_WIDGETS_METER_H__
#define __GUI_WIDGETS_METER_H__

#include "utils/general.h"

#include <gtk/gtk.h>

#define METER_WIDGET_TYPE (meter_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  MeterWidget,
  meter_widget,
  Z,
  METER_WIDGET,
  GtkWidget)

typedef struct Meter Meter;

typedef struct _MeterWidget
{
  GtkWidget parent_instance;

  /** Associated meter. */
  Meter * meter;

  /** Hovered or not. */
  int hover;

  /** Padding size for the border. */
  int padding;

  GdkRGBA start_color;
  GdkRGBA end_color;

  float meter_val;
  float meter_peak;

  /** Caches of last drawn values so that meters
   * are redrawn only when there are changes. */
  float last_meter_val;
  float last_meter_peak;

  /** ID of the source function. */
  guint     source_id;
  GSource * timeout_source;
} MeterWidget;

/**
 * Creates a new Meter widget and binds it to the
 * given value.
 *
 * @param port Port this meter is for.
 */
void
meter_widget_setup (MeterWidget * self, Port * port, int width);

#endif
