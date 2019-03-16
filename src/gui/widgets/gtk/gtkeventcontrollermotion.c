/* GTK - The GIMP Toolkit
 * Copyright (C) 2017, Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Author(s): Matthias Clasen <mclasen@redhat.com>
 */

/**
 * SECTION:gtkeventcontrollermotion
 * @Short_description: Event controller for motion events
 * @Title: GtkEventControllerMotion
 * @See_also: #GtkEventController
 *
 * #GtkEventControllerMotion is an event controller meant for situations
 * where you need to track the position of the pointer.
 *
 * This object was added in 3.24.
 **/

/*#include "gtkintl.h"*/
/*#include "gtkwidget.h"*/
#include "gui/widgets/gtk/gtkeventcontrollerprivate.h"
#include "gui/widgets/gtk/gtkeventcontrollermotion.h"
/*#include "gtktypebuiltins.h"*/
/*#include "gtkmarshalers.h"*/
#include <gtk/gtk.h>

struct _GtkEventControllerMotion
{
  GtkEventController parent_instance;
};

struct _GtkEventControllerMotionClass
{
  GtkEventControllerClass parent_class;
};

enum {
  ENTER,
  LEAVE,
  MOTION,
  N_SIGNALS
};

static guint signals[N_SIGNALS] = { 0 };

G_DEFINE_TYPE (GtkEventControllerMotion, gtk_event_controller_motion, GTK_TYPE_EVENT_CONTROLLER)

static void
get_coords (GtkWidget      *widget,
            const GdkEvent *event,
            double         *x,
            double         *y)
{
  GdkWindow *window, *ancestor;
  GtkAllocation alloc;

  gtk_widget_get_allocation (widget, &alloc);
  gdk_event_get_coords (event, x, y);

  ancestor = gtk_widget_get_window (widget);
  window = gdk_event_get_window (event);

  while (window && ancestor && (window != ancestor))
    {
      gdk_window_coords_to_parent (window, *x, *y, x, y);
      window = gdk_window_get_parent (window);
    }

  if (!gtk_widget_get_has_window (widget))
    {
      *x -= alloc.x;
      *y -= alloc.y;
    }
}

static gboolean
gtk_event_controller_motion_handle_event (GtkEventController *controller,
                                          const GdkEvent     *event)
{
  GtkEventControllerClass *parent_class;
  GtkWidget *widget;
  GdkEventType type;

  widget = gtk_event_controller_get_widget (controller);

  type = gdk_event_get_event_type (event);
  if (type == GDK_ENTER_NOTIFY)
    {
      double x, y;
      get_coords (widget, event, &x, &y);
      g_signal_emit (controller, signals[ENTER], 0, x, y);
    }
  else if (type == GDK_LEAVE_NOTIFY)
    {
      g_signal_emit (controller, signals[LEAVE], 0);
    }
  else if (type == GDK_MOTION_NOTIFY)
    {
      double x, y;
      get_coords (widget, event, &x, &y);
      g_signal_emit (controller, signals[MOTION], 0, x, y);
    }

  parent_class = GTK_EVENT_CONTROLLER_CLASS (gtk_event_controller_motion_parent_class);

  return parent_class->handle_event (controller, event);
}

static void
gtk_event_controller_motion_class_init (GtkEventControllerMotionClass *klass)
{
  GtkEventControllerClass *controller_class = GTK_EVENT_CONTROLLER_CLASS (klass);

  controller_class->handle_event = gtk_event_controller_motion_handle_event;

  /**
   * GtkEventControllerMotion::enter:
   * @controller: The object that received the signal
   * @x: the x coordinate
   * @y: the y coordinate
   *
   * Signals that the pointer has entered the widget.
   */
  signals[ENTER] =
    g_signal_new ("enter",
                  GTK_TYPE_EVENT_CONTROLLER_MOTION,
                  G_SIGNAL_RUN_FIRST,
                  0, NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 2, G_TYPE_DOUBLE, G_TYPE_DOUBLE);

  /**
   * GtkEventControllerMotion::leave:
   * @controller: The object that received the signal
   *
   * Signals that pointer has left the widget.
   */
  signals[LEAVE] =
    g_signal_new ("leave",
                  GTK_TYPE_EVENT_CONTROLLER_MOTION,
                  G_SIGNAL_RUN_FIRST,
                  0, NULL, NULL,
        	  NULL,
                  G_TYPE_NONE, 0);

  /**
   * GtkEventControllerMotion::motion:
   * @controller: The object that received the signal
   * @x: the x coordinate
   * @y: the y coordinate
   *
   * Emitted when the pointer moves inside the widget.
   */
  signals[MOTION] =
    g_signal_new ("motion",
                  GTK_TYPE_EVENT_CONTROLLER_MOTION,
                  G_SIGNAL_RUN_FIRST,
                  0, NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 2, G_TYPE_DOUBLE, G_TYPE_DOUBLE);
}

static void
gtk_event_controller_motion_init (GtkEventControllerMotion *motion)
{
}

/**
 * gtk_event_controller_motion_new:
 * @widget: a #GtkWidget
 *
 * Creates a new event controller that will handle motion events
 * for the given @widget.
 *
 * Returns: a new #GtkEventControllerMotion
 *
 * Since: 3.24
 **/
GtkEventController *
gtk_event_controller_motion_new (GtkWidget *widget)
{
  g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);

  return g_object_new (GTK_TYPE_EVENT_CONTROLLER_MOTION,
                       "widget", widget,
                       NULL);
}

