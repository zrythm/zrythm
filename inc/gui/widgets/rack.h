/*
 * gui/widgets/rack.h - Rack widget
 *
 * Copyright (C) 2019 Alexandros Theodotou
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

#ifndef __GUI_WIDGETS_RACK_H__
#define __GUI_WIDGETS_RACK_H__

#include <gtk/gtk.h>

#define RACK_WIDGET_TYPE \
  (rack_widget_get_type ())
G_DECLARE_FINAL_TYPE (RackWidget,
                      rack_widget,
                      Z,
                      RACK_WIDGET,
                      GtkScrolledWindow)

typedef struct _RackRowWidget RackRowWidget;

typedef struct _RackWidget
{
  GtkScrolledWindow        parent_instance;
  GtkBox *                 main_box;
  RackRowWidget *          rack_rows[120];
  int                      num_rack_rows;
  GtkBox *                 dummy_box; ///< box at the end
} RackWidget;

void
rack_widget_setup (RackWidget * self);

#endif
