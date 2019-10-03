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

/** \file */

#ifndef __GUI_WIDGETS_AUTOMATION_CURVE_H__
#define __GUI_WIDGETS_AUTOMATION_CURVE_H__

#include <gtk/gtk.h>

#define AUTOMATION_CURVE_WIDGET_TYPE \
  (automation_curve_widget_get_type ())
G_DECLARE_FINAL_TYPE (AutomationCurveWidget,
                      automation_curve_widget,
                      Z,
                      AUTOMATION_CURVE_WIDGET,
                      GtkDrawingArea)

/*
 * extra space on top and bottom to make room for width 2
 * lines when they are close to the top & bottom */
#define AC_Y_PADDING 2.0
#define AC_Y_HALF_PADDING (AC_Y_PADDING / 2.0)

typedef enum ACW_CursorState
{
  ACW_STATE_NONE,
} ACW_CursorState;

typedef struct AutomationCurve AutomationCurve;

typedef struct _AutomationCurveWidget
{
  GtkDrawingArea              parent_instance;
  AutomationCurve *           ac;   ///< the automation curve associated with this
  //int                         hover;
  //int                         selected;

  /* for dragging */
  GtkGestureDrag *            drag;
  double                      last_x;
  double                      last_y;

  ACW_CursorState             cursor_state;

  /* Draw caching (set to 0 to redraw). */
  int                      cache;

  cairo_t *                cached_cr;

  cairo_surface_t *        cached_surface;
} AutomationCurveWidget;

/**
 * Creates a automation_curve.
 */
AutomationCurveWidget *
automation_curve_widget_new (AutomationCurve * ac);

#endif
