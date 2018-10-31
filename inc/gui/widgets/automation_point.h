/*
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

/** \file */

#ifndef __GUI_WIDGETS_AUTOMATION_POINT_H__
#define __GUI_WIDGETS_AUTOMATION_POINT_H__

#include <audio/automation_point.h>

#include <gtk/gtk.h>

#define AUTOMATION_POINT_WIDGET_TYPE                  (automation_point_widget_get_type ())
#define AUTOMATION_POINT_WIDGET(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), AUTOMATION_POINT_WIDGET_TYPE, AutomationPointWidget))
#define AUTOMATION_POINT_WIDGET_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST  ((klass), AUTOMATION_POINT_WIDGET, AutomationPointWidgetClass))
#define IS_AUTOMATION_POINT_WIDGET(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AUTOMATION_POINT_WIDGET_TYPE))
#define IS_AUTOMATION_POINT_WIDGET_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE  ((klass), AUTOMATION_POINT_WIDGET_TYPE))
#define AUTOMATION_POINT_WIDGET_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS  ((obj), AUTOMATION_POINT_WIDGET_TYPE, AutomationPointWidgetClass))

#define AP_WIDGET_POINT_SIZE 4
#define AP_WIDGET_CURVE_H 2
#define AP_WIDGET_CURVE_W 8

typedef enum AutomationPointHoverState
{
  AP_HOVER_STATE_NONE,
  AP_HOVER_STATE_MIDDLE
} AutomationPointHoverState;

typedef struct AutomationPointWidget
{
  GtkBox                    parent_instance;
  AutomationPoint *         ap;   ///< the automation_point associated with this
  AutomationPointHoverState hover_state;
} AutomationPointWidget;

typedef struct AutomationPointWidgetClass
{
  GtkBoxClass       parent_class;
} AutomationPointWidgetClass;

/**
 * Creates a automation_point.
 */
AutomationPointWidget *
automation_point_widget_new (AutomationPoint * automation_point);

GType automation_point_widget_get_type(void);

#endif


