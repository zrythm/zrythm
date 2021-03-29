/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

/**
 * \file
 *
 * The control rooom.
 */

#ifndef __GUI_WIDGETS_CONTROL_ROOM_H__
#define __GUI_WIDGETS_CONTROL_ROOM_H__

#include <gtk/gtk.h>

#define CONTROL_ROOM_WIDGET_TYPE \
  (control_room_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ControlRoomWidget,
  control_room_widget,
  Z, CONTROL_ROOM_WIDGET,
  GtkGrid)

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_CONTROL_ROOM \
  MW_RIGHT_DOCK_EDGE->control_room

typedef struct _KnobWithNameWidget
  KnobWithNameWidget;
typedef struct ControlRoom ControlRoom;
typedef struct _SliderBarWidget SliderBarWidget;
typedef struct _MeterWidget MeterWidget;

typedef struct _ControlRoomWidget
{
  GtkGrid         parent_instance;

  /** Toolbar on the left. */
  GtkToolbar *    left_of_main_knob_toolbar;

  /** Output knob. */
  GtkBox *        main_knob_placeholder;
  KnobWithNameWidget * volume;

  /** For temporarily dimming the output. */
  GtkToggleToolButton * dim_output;

  /** Listen dim slider. */
  GtkBox *        listen_dim_slider_placeholder;
  SliderBarWidget * listen_dim_slider;

  /** Meter of output. */
  GtkBox *        main_meter_placeholder;
  MeterWidget *   main_meter;

  /** Pointer to backend. */
  ControlRoom *   control_room;

} ControlRoomWidget;

void
control_room_widget_setup (
  ControlRoomWidget * self,
  ControlRoom *       control_room);

/**
 * Creates a ControlRoomWidget.
 */
ControlRoomWidget *
control_room_widget_new (void);

/**
 * @}
 */

#endif
