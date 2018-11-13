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

#ifndef __GUI_WIDGETS_AUTOMATION_CURVE_H__
#define __GUI_WIDGETS_AUTOMATION_CURVE_H__

#include <gtk/gtk.h>

#define AUTOMATION_CURVE_WIDGET_TYPE                  (automation_curve_widget_get_type ())
#define AUTOMATION_CURVE_WIDGET(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), AUTOMATION_CURVE_WIDGET_TYPE, AutomationCurveWidget))
#define AUTOMATION_CURVE_WIDGET_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST  ((klass), AUTOMATION_CURVE_WIDGET, AutomationCurveWidgetClass))
#define IS_AUTOMATION_CURVE_WIDGET(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AUTOMATION_CURVE_WIDGET_TYPE))
#define IS_AUTOMATION_CURVE_WIDGET_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE  ((klass), AUTOMATION_CURVE_WIDGET_TYPE))
#define AUTOMATION_CURVE_WIDGET_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS  ((obj), AUTOMATION_CURVE_WIDGET_TYPE, AutomationCurveWidgetClass))

typedef enum ACW_CursorState
{
  ACW_STATE_NONE,
} ACW_CursorState;

typedef struct AutomationCurve AutomationCurve;

typedef struct AutomationCurveWidget
{
  GtkDrawingArea              parent_instance;
  AutomationCurve *           ac;   ///< the automation curve associated with this
  int                         hover;
  int                         selected;

  /* for dragging */
  GtkGestureDrag *            drag;
  double                      last_x;
  double                      last_y;

  ACW_CursorState             cursor_state;
} AutomationCurveWidget;

typedef struct AutomationCurveWidgetClass
{
  GtkDrawingAreaClass       parent_class;
} AutomationCurveWidgetClass;

/**
 * Creates a automation_curve.
 */
AutomationCurveWidget *
automation_curve_widget_new (AutomationCurve * ac);

GType automation_curve_widget_get_type(void);

#endif

