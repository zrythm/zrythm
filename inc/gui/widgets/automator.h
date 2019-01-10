/*
 * gui/widgets/automator.h - A automator widget
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

#ifndef __GUI_WIDGETS_AUTOMATOR_H__
#define __GUI_WIDGETS_AUTOMATOR_H__

#include "gui/widgets/automator.h"

#include <gtk/gtk.h>

#define AUTOMATOR_WIDGET_TYPE \
  (automator_widget_get_type ())
G_DECLARE_FINAL_TYPE (AutomatorWidget,
                      automator_widget,
                      Z,
                      AUTOMATOR_WIDGET,
                      GtkGrid)

typedef struct _ControlWidget ControlWidget;

typedef struct _AutomatorWidget
{
  GtkGrid                   parent_instance;
  GtkLabel *                name;
  GtkToggleButton *         power;
  GtkViewport *             controls_viewport;
  GtkBox *                  controls_box;
  ControlWidget *           controls[800];
  int                       num_controls;
  GtkToggleButton *         automate;
} AutomatorWidget;

/**
 * Creates a automator widget using the given automator data.
 */
AutomatorWidget *
automator_widget_new ();

#endif
