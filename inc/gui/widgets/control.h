/*
 * gui/widgets/control.h - A control widget
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

#ifndef __GUI_WIDGETS_CONTROL_H__
#define __GUI_WIDGETS_CONTROL_H__

#include "gui/widgets/control.h"

#include <gtk/gtk.h>

#define CONTROL_WIDGET_TYPE                  (control_widget_get_type ())
G_DECLARE_FINAL_TYPE (ControlWidget,
                      control_widget,
                      Z,
                      CONTROL_WIDGET,
                      GtkBox)

typedef struct _ControlWidget ControlWidget;
typedef struct Port Port;
typedef struct _KnobWidget KnobWidget;

typedef struct _ControlWidget
{
  GtkBox                    parent_instance;
  GtkLabel *                name;
  GtkBox *                  knob_box;
  KnobWidget *              knob;
  GtkToggleButton *         toggle;
  Port *                    port; ///< the port it corresponds to
} ControlWidget;

/**
 * Creates a control widget using the given control data.
 */
ControlWidget *
control_widget_new (Port * port);

#endif
