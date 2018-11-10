/*
 * gui/widgets/automator.h - A automator widget
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

#ifndef __GUI_WIDGETS_AUTOMATOR_H__
#define __GUI_WIDGETS_AUTOMATOR_H__

#include "gui/widgets/automator.h"

#include <gtk/gtk.h>

#define AUTOMATOR_WIDGET_TYPE                  (automator_widget_get_type ())
#define AUTOMATOR_WIDGET(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), AUTOMATOR_WIDGET_TYPE, AutomatorWidget))
#define AUTOMATOR_WIDGET_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST  ((klass), AUTOMATOR_WIDGET, AutomatorWidgetClass))
#define IS_AUTOMATOR_WIDGET(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AUTOMATOR_WIDGET_TYPE))
#define IS_AUTOMATOR_WIDGET_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE  ((klass), AUTOMATOR_WIDGET_TYPE))
#define AUTOMATOR_WIDGET_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS  ((obj), AUTOMATOR_WIDGET_TYPE, AutomatorWidgetClass))

typedef struct ControlWidget ControlWidget;

typedef struct AutomatorWidget
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

typedef struct AutomatorWidgetClass
{
  GtkGridClass       parent_class;
} AutomatorWidgetClass;


/**
 * Creates a automator widget using the given automator data.
 */
AutomatorWidget *
automator_widget_new ();

#endif

