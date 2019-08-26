/*
 * Copyright (C) 2018-2019 Alexandros Theodotou
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

#include <math.h>
#include <stdlib.h>

#include "gui/widgets/bot_bar.h"
#include "gui/widgets/pan.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (PanWidget,
               pan_widget,
               GTK_TYPE_DRAWING_AREA)

#define GET_VAL ((*self->getter) (self->object))
#define SET_VAL(real) ((*self->setter)(self->object, real))

static int
draw_cb (
  GtkWidget * widget, cairo_t * cr, void* data)
{
  guint width, height;
  GtkStyleContext *context;
  PanWidget * self = (PanWidget *) widget;
  context = gtk_widget_get_style_context (widget);

  width = gtk_widget_get_allocated_width (widget);
  height = gtk_widget_get_allocated_height (widget);

  gtk_render_background (context, cr, 0, 0, width, height);

  /* draw filled in bar */
  float pan_val = GET_VAL;
  /*float intensity = pan_val;*/
  float value_px = pan_val * width;
  /*const float intensity_inv = 1.0 - intensity;*/
  /*float r = intensity_inv * self->end_color.red   +*/
            /*intensity * self->start_color.red;*/
  /*float g = intensity_inv * self->end_color.green +*/
            /*intensity * self->start_color.green;*/
  /*float b = intensity_inv * self->end_color.blue  +*/
            /*intensity * self->start_color.blue;*/
  /*float a = intensity_inv * self->end_color.alpha  +*/
            /*intensity * self->start_color.alpha;*/

  /* draw pan line */
  cairo_move_to (cr, value_px, 0);
  cairo_line_to (cr, value_px, height);
  cairo_stroke (cr);

  return FALSE;
}

/**
 * Returns the pan string.
 *
 * Must be free'd.
 */
static char *
get_pan_string (
  PanWidget * self)
{
  /* make it from -0.5 to 0.5 */
  float pan_val = GET_VAL - 0.5f;

  /* get as percentage */
  pan_val = (fabs (pan_val) / 0.5f) * 100.f;

  return
    g_strdup_printf ("%s%.0f%%",
                     GET_VAL < 0.5f ? "-" : "",
                     pan_val);
}

static gboolean
on_motion (GtkWidget * widget, GdkEvent *event,
           PanWidget * self)
{
  if (gdk_event_get_event_type (event) ==
      GDK_ENTER_NOTIFY)
    {
      gtk_widget_set_state_flags (
        GTK_WIDGET (self),
        GTK_STATE_FLAG_PRELIGHT, 0);
      bot_bar_change_status (
        "Pan - Click and drag to change value");
    }
  else if (gdk_event_get_event_type (event) ==
           GDK_LEAVE_NOTIFY)
    {
      gtk_widget_unset_state_flags (
        GTK_WIDGET (self),
        GTK_STATE_FLAG_PRELIGHT);
      bot_bar_change_status ("");
    }

  return FALSE;
}

static double clamp
(double x, double upper, double lower)
{
    return MIN(upper, MAX(x, lower));
}

static void
drag_update (GtkGestureDrag * gesture,
               gdouble         offset_x,
               gdouble         offset_y,
               gpointer        user_data)
{
  PanWidget * self = (PanWidget *) user_data;
  offset_y = - offset_y;
  int use_y =
    fabs (offset_y - self->last_y) >
    fabs (offset_x - self->last_x);
  SET_VAL (clamp (GET_VAL + 0.005 * (use_y ? offset_y - self->last_y : offset_x - self->last_x),
               1.0f, 0.0f));
  self->last_x = offset_x;
  self->last_y = offset_y;
  gtk_widget_queue_draw (GTK_WIDGET (self));

  char * str =
    get_pan_string (self);
  gtk_label_set_text (self->tooltip_label,
                      str);
  gtk_widget_set_tooltip_text (
    GTK_WIDGET (self), str);
  g_free (str);
  gtk_window_present (self->tooltip_win);
}

static void
drag_end (GtkGestureDrag *gesture,
               gdouble         offset_x,
               gdouble         offset_y,
               gpointer        user_data)
{
  PanWidget * self = (PanWidget *) user_data;
  self->last_x = 0;
  self->last_y = 0;
  gtk_widget_hide (GTK_WIDGET (self->tooltip_win));
}

/**
 * Creates a new Pan widget and binds it to the given value.
 */
PanWidget *
pan_widget_new (float (*get_val)(void *),    ///< getter function
                  void (*set_val)(void *, float),    ///< setter function
                  void * object,              ///< object to call get/set with
                  int height)
{
  PanWidget * self =
    g_object_new (PAN_WIDGET_TYPE,
                  "visible", 1,
                  "height-request", height,
                  NULL);
  self->getter = get_val;
  self->setter = set_val;
  self->object = object;

  /* connect signals */
  g_signal_connect (G_OBJECT (self), "draw",
                    G_CALLBACK (draw_cb), self);
  g_signal_connect (
    G_OBJECT (self), "enter-notify-event",
    G_CALLBACK (on_motion),  self);
  g_signal_connect (
    G_OBJECT(self), "leave-notify-event",
    G_CALLBACK (on_motion),  self);
  g_signal_connect (
    G_OBJECT(self->drag), "drag-update",
    G_CALLBACK (drag_update),  self);
  g_signal_connect (
    G_OBJECT(self->drag), "drag-end",
    G_CALLBACK (drag_end),  self);

  return self;
}

static void
pan_widget_init (PanWidget * self)
{
  gdk_rgba_parse (
    &self->start_color, "rgba(0%,100%,0%,1.0)");
  gdk_rgba_parse (
    &self->end_color, "rgba(0%,50%,50%,1.0)");

  /* make it able to notify */
  gtk_widget_set_has_window (
    GTK_WIDGET (self), TRUE);
  int crossing_mask =
    GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK;
  gtk_widget_add_events (
    GTK_WIDGET (self), crossing_mask);

  self->tooltip_win =
    GTK_WINDOW (gtk_window_new (GTK_WINDOW_POPUP));
  gtk_window_set_type_hint (self->tooltip_win,
                            GDK_WINDOW_TYPE_HINT_TOOLTIP);
  self->tooltip_label =
    GTK_LABEL (gtk_label_new ("label"));
  gtk_widget_set_visible (GTK_WIDGET (self->tooltip_label),
                          1);
  gtk_container_add (GTK_CONTAINER (self->tooltip_win),
                     GTK_WIDGET (self->tooltip_label));
  gtk_window_set_position (self->tooltip_win,
                           GTK_WIN_POS_MOUSE);

  self->drag = GTK_GESTURE_DRAG (gtk_gesture_drag_new (GTK_WIDGET (self)));
}

static void
pan_widget_class_init (PanWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (klass,
                                 "pan");
}
