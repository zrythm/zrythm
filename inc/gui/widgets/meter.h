/*
 * gui/widgets/meter.h - Meter widget
 *
 * Copyright (C) 2019 Alexandros Theodotou
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

#define METER_WIDGET_TYPE \
  (meter_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  MeterWidget, meter_widget,
  Z, METER_WIDGET, GtkDrawingArea)

typedef enum MeterType
{
  METER_TYPE_MIDI,
  METER_TYPE_DB
} MeterType;

typedef struct _MeterWidget
{
  GtkDrawingArea         parent_instance;

  /** Getter for current value. */
  GenericFloatGetter     getter;

  /**
   * Getter for max value (amp).
   *
   * Will be ignored if NULL or returning negative.
   */
  GenericFloatGetter     max_getter;

  /** Last meter value, used to show a falloff and
   * avoid sudden dips. */
  float                  last_val;

  /** Time the last val was taken at (last draw
   * time). */
  gint64                 last_draw_time;

  void *                 object;
  MeterType              type;

  /** Hovered or not. */
  int                    hover;

  /** Padding size for the border. */
  int                    padding;

  GdkRGBA                start_color;
  GdkRGBA                end_color;
} MeterWidget;

/**
 * Creates a new Meter widget and binds it to the
 * given value.
 *
 * @param getter Getter func.
 * @param max_getter Getter func for max (pass
 *   NULL for midi).
 * @param object Objet to call get with.
 * @param type Meter type.
 */
void
meter_widget_setup (
  MeterWidget *      self,
  GenericFloatGetter getter,
  GenericFloatGetter max_getter,
  void        *      object,
  MeterType          type,
  int                width);

#endif
