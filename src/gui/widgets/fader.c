/*
 * gui/widgets/fader.c - Fader
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

/** \file
 */

#include <stdlib.h>

#include "audio/channel.h"
#include "gui/widgets/fader.h"
#include "utils/math.h"

G_DEFINE_TYPE (FaderWidget,
               fader_widget,
               GTK_TYPE_DRAWING_AREA)

#define GET_VAL ((*self->getter) (self->object))
#define SET_VAL(real) ((*self->setter)(self->object, real))

static double
fader_val_from_real (FaderWidget * self)
{
  double amp = GET_VAL;
  return math_get_fader_val_from_amp (amp);
}

static double
real_val_from_fader (FaderWidget * self, double fader)
{
  return math_get_amp_val_from_fader (fader);
}

static void
draw_cb (GtkWidget * widget, cairo_t * cr, void* data)
{
  guint width, height;
  GtkStyleContext *context;
  FaderWidget * self = (FaderWidget *) widget;
  context = gtk_widget_get_style_context (widget);

  width = gtk_widget_get_allocated_width (widget);
  height = gtk_widget_get_allocated_height (widget);

  gtk_render_background (context, cr, 0, 0, width, height);

  /* a custom shape that could be wrapped in a function */
  double x         = 0,        /* parameters like cairo_rectangle */
         y         = 0,
         aspect        = 1.0,     /* aspect ratio */
         corner_radius = height / 90.0;   /* and corner curvature radius */

  double radius = corner_radius / aspect;
  double degrees = G_PI / 180.0;
  /* value in pixels = total pixels * val
   * val is percentage */
  double fader_val = fader_val_from_real (self);
  double value_px = height * fader_val;

  /* draw background bar */
  cairo_new_sub_path (cr);
  cairo_arc (cr, x + width - radius, y + radius, radius, -90 * degrees, 0 * degrees);
  cairo_line_to (cr, x + width, y + height - value_px);
  /*cairo_arc (cr, x + width - radius, y + height - radius, radius, 0 * degrees, 90 * degrees);*/
  cairo_line_to (cr, x, y + height - value_px);
  /*cairo_arc (cr, x + radius, y + height - radius, radius, 90 * degrees, 180 * degrees);*/
  cairo_arc (cr, x + radius, y + radius, radius, 180 * degrees, 270 * degrees);
  cairo_close_path (cr);

  cairo_set_source_rgba (cr, 0.4, 0.4, 0.4, 0.2);
  cairo_fill(cr);

  /* draw filled in bar */
  float intensity = fader_val;
  const float intensity_inv = 1.0 - intensity;
  float r = intensity_inv * self->end_color.red   +
            intensity * self->start_color.red;
  float g = intensity_inv * self->end_color.green +
            intensity * self->start_color.green;
  float b = intensity_inv * self->end_color.blue  +
            intensity * self->start_color.blue;
  float a = intensity_inv * self->end_color.alpha  +
            intensity * self->start_color.alpha;

  cairo_set_source_rgba (cr, r,g,b,a);
  cairo_new_sub_path (cr);
  cairo_line_to (cr, x + width, y + (height - value_px));
  /*cairo_arc (cr, x + width - radius, y + radius, radius, -90 * degrees, 0 * degrees);*/
  cairo_arc (cr, x + width - radius, y + height - radius, radius, 0 * degrees, 90 * degrees);
  cairo_arc (cr, x + radius, y + height - radius, radius, 90 * degrees, 180 * degrees);
  cairo_line_to (cr, x, y + (height - value_px));
  /*cairo_arc (cr, x + radius, y + radius, radius, 180 * degrees, 270 * degrees);*/
  cairo_close_path (cr);

  cairo_fill(cr);


  /* draw border line */
  cairo_set_source_rgba (cr, 0.2, 0.2, 0.2, 1);
  cairo_new_sub_path (cr);
  cairo_arc (cr, x + width - radius, y + radius, radius, -90 * degrees, 0 * degrees);
  cairo_arc (cr, x + width - radius, y + height - radius, radius, 0 * degrees, 90 * degrees);
  cairo_arc (cr, x + radius, y + height - radius, radius, 90 * degrees, 180 * degrees);
  cairo_arc (cr, x + radius, y + radius, radius, 180 * degrees, 270 * degrees);
  cairo_close_path (cr);

  /*cairo_set_source_rgba (cr, 0.4, 0.2, 0.05, 0.2);*/
  /*cairo_fill_preserve (cr);*/
  cairo_set_line_width (cr, 1.7);
  cairo_stroke(cr);

  //highlight if grabbed or if mouse is hovering over me
  if (self->hover)
    {
      cairo_set_source_rgba (cr, 0.8, 0.8, 0.8, 0.12 );
      cairo_new_sub_path (cr);
      cairo_arc (cr, x + width - radius, y + radius, radius, -90 * degrees, 0 * degrees);
      cairo_arc (cr, x + width - radius, y + height - radius, radius, 0 * degrees, 90 * degrees);
      cairo_arc (cr, x + radius, y + height - radius, radius, 90 * degrees, 180 * degrees);
      cairo_arc (cr, x + radius, y + radius, radius, 180 * degrees, 270 * degrees);
      cairo_close_path (cr);
      cairo_fill (cr);
    }

  /* draw fader line */
  cairo_set_source_rgba (cr, 0, 0, 0, 1);
  cairo_set_line_width (cr, 16.0);
  cairo_set_line_cap  (cr, CAIRO_LINE_CAP_SQUARE);
  cairo_move_to (cr, x, y + (height - value_px));
  cairo_line_to (cr, x+ width, y + (height - value_px));
  cairo_stroke (cr);
  cairo_set_source_rgba (cr, 0.4, 0.1, 0.05, 1);
  cairo_set_line_width (cr, 3.0);
  cairo_move_to (cr, x, y + (height - value_px));
  cairo_line_to (cr, x+ width, y + (height - value_px));
  cairo_stroke (cr);

}


static void
on_crossing (GtkWidget * widget, GdkEvent *event)
{
  FaderWidget * self = FADER_WIDGET (widget);
  if (gdk_event_get_event_type (event) == GDK_ENTER_NOTIFY)
    self->hover = 1;
  else if (gdk_event_get_event_type (event) == GDK_LEAVE_NOTIFY)
    {
      if (!gtk_gesture_drag_get_offset (self->drag,
                                       NULL,
                                       NULL))
        self->hover = 0;
    }
  gtk_widget_queue_draw(widget);
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
  FaderWidget * self = (FaderWidget *) user_data;
  offset_y = - offset_y;
  int use_y = abs(offset_y - self->last_y) > abs(offset_x - self->last_x);
  /*double multiplier = 0.005;*/
  double diff = use_y ? offset_y - self->last_y : offset_x - self->last_x;
  double height = gtk_widget_get_allocated_height (GTK_WIDGET (self));
  double adjusted_diff = diff / height;
  double new_fader_val = clamp (fader_val_from_real (self) + adjusted_diff,
                                1.0,
                                0.0);
  SET_VAL (real_val_from_fader (self, new_fader_val));
  self->last_x = offset_x;
  self->last_y = offset_y;
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

static void
drag_end (GtkGestureDrag *gesture,
               gdouble         offset_x,
               gdouble         offset_y,
               gpointer        user_data)
{
  FaderWidget * self = (FaderWidget *) user_data;
  self->last_x = 0;
  self->last_y = 0;
}

/**
 * Creates a new Fader widget and binds it to the given value.
 */
void
fader_widget_setup (
  FaderWidget * self,
  float         (*get_val)(void *),    ///< getter function
  void          (*set_val)(void *, float),    ///< setter function
  void *        object,              ///< object to call get/set with
  int width)
{
  self->getter = get_val;
  self->setter = set_val;
  self->object = object;

  /* set size */
  gtk_widget_set_size_request (GTK_WIDGET (self), width, -1);
}

static void
fader_widget_init (FaderWidget * self)
{
  gdk_rgba_parse (&self->start_color, "rgba(40%,20%,5%,1.0)");
  gdk_rgba_parse (&self->end_color, "rgba(30%,17%,3%,0.8)");

  /* make it able to notify */
  gtk_widget_set_has_window (GTK_WIDGET (self), TRUE);
  int crossing_mask = GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK;
  gtk_widget_add_events (GTK_WIDGET (self), crossing_mask);

  self->drag = GTK_GESTURE_DRAG (gtk_gesture_drag_new (GTK_WIDGET (self)));

  /* connect signals */
  g_signal_connect (G_OBJECT (self), "draw",
                    G_CALLBACK (draw_cb), self);
  g_signal_connect (G_OBJECT (self), "enter-notify-event",
                    G_CALLBACK (on_crossing),  self);
  g_signal_connect (G_OBJECT(self), "leave-notify-event",
                    G_CALLBACK (on_crossing),  self);
  g_signal_connect (G_OBJECT(self->drag), "drag-update",
                    G_CALLBACK (drag_update),  self);
  g_signal_connect (G_OBJECT(self->drag), "drag-end",
                    G_CALLBACK (drag_end),  self);

}

static void
fader_widget_class_init (FaderWidgetClass * klass)
{
}
