/*
 * gui/widgets/control.h - A control widget
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

#ifndef __GUI_WIDGETS_CONTROL_H__
#define __GUI_WIDGETS_CONTROL_H__

#include "gui/widgets/control.h"

#include <gtk/gtk.h>

#define CONTROL_WIDGET_TYPE                  (control_widget_get_type ())
#define CONTROL_WIDGET(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CONTROL_WIDGET_TYPE, ControlWidget))
#define CONTROL_WIDGET_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST  ((klass), CONTROL_WIDGET, ControlWidgetClass))
#define IS_CONTROL_WIDGET(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CONTROL_WIDGET_TYPE))
#define IS_CONTROL_WIDGET_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE  ((klass), CONTROL_WIDGET_TYPE))
#define CONTROL_WIDGET_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS  ((obj), CONTROL_WIDGET_TYPE, ControlWidgetClass))

typedef struct ControlWidget ControlWidget;
typedef struct Port Port;
typedef struct KnobWidget KnobWidget;

typedef struct ControlWidget
{
  GtkBox                    parent_instance;
  GtkLabel *                name;
  GtkBox *                  knob_box;
  KnobWidget *              knob;
  GtkToggleButton *         toggle;
  Port *                    port; ///< the port it corresponds to
} ControlWidget;

typedef struct ControlWidgetClass
{
  GtkBoxClass       parent_class;
} ControlWidgetClass;

/**
 * Creates a control widget using the given control data.
 */
ControlWidget *
control_widget_new (Port * port);

#endif


