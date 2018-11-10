/*
 * gui/widgets/rack.h - Rack widget
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

#ifndef __GUI_WIDGETS_RACK_H__
#define __GUI_WIDGETS_RACK_H__

#include <gtk/gtk.h>

#define RACK_WIDGET_TYPE                  (rack_widget_get_type ())
#define RACK_WIDGET(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), RACK_WIDGET_TYPE, RackWidget))
#define RACK_WIDGET_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST  ((klass), RACK_WIDGET, RackWidgetClass))
#define IS_RACK_WIDGET(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), RACK_WIDGET_TYPE))
#define IS_RACK_WIDGET_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE  ((klass), RACK_WIDGET_TYPE))
#define RACK_WIDGET_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS  ((obj), RACK_WIDGET_TYPE, RackWidgetClass))

typedef struct RackRowWidget RackRowWidget;

typedef struct RackWidget
{
  GtkScrolledWindow        parent_instance;
  GtkBox *                 main_box;
  RackRowWidget *          rack_rows[120];
  int                      num_rack_rows;
  GtkBox *                 dummy_box; ///< box at the end
} RackWidget;

typedef struct RackWidgetClass
{
  GtkScrolledWindowClass     parent_class;
} RackWidgetClass;

RackWidget *
rack_widget_new ();

#endif

