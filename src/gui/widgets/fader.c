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

#include "gui/widgets/fader.h"

static GtkGestureDrag * gdrag;
static double last_x, last_y;
static float val = 0.6f;
static int hover = 0;

static int
draw_cb (GtkWidget * widget, cairo_t * cr, void* data)
{
  guint width, height;
  GtkStyleContext *context;
  context = gtk_widget_get_style_context (widget);

  width = gtk_widget_get_allocated_width (widget);
  height = gtk_widget_get_allocated_height (widget);

  gtk_render_background (context, cr, 0, 0, width, height);

  /* a custom shape that could be wrapped in a function */
  double x         = 0,        /* parameters like cairo_rectangle */
         y         = 0,
         aspect        = 1.0,     /* aspect ratio */
         corner_radius = height / 20.0;   /* and corner curvature radius */

  double radius = corner_radius / aspect;
  double degrees = G_PI / 180.0;
  /* value in pixels = total pixels * val
   * val is percentage */
  double value_px = height * val;

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
  cairo_set_source_rgba (cr, 0.4, 0.2, 0.05, 0.9);
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
  if (hover)
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
   switch (gdk_event_get_event_type (event))
    {
    case GDK_ENTER_NOTIFY:
      hover = 1;
      break;

    case GDK_LEAVE_NOTIFY:
      if (!gtk_gesture_drag_get_offset (gdrag,
                                       NULL,
                                       NULL))
        hover = 0;
      break;
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
  offset_y = - offset_y;
  int use_y = abs(offset_y - last_y) > abs(offset_x - last_x);
  val = clamp (val + 0.005 * (use_y ? offset_y - last_y : offset_x - last_x),
               1.0f, 0.0f);
  last_x = offset_x;
  last_y = offset_y;
  gtk_widget_queue_draw ((GtkWidget *)user_data);
}

static void
drag_end (GtkGestureDrag *gesture,
               gdouble         offset_x,
               gdouble         offset_y,
               gpointer        user_data)
{
  last_x = 0;
  last_y = 0;
}
/**
 * sets up the fader.
 */
void
fader_set (GtkWidget * da)
{
  /* make it able to notify */
  gtk_widget_set_has_window (GTK_WIDGET (da), TRUE);
  int crossing_mask = GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK;
  gtk_widget_add_events (GTK_WIDGET (da), crossing_mask);

  gdrag = GTK_GESTURE_DRAG (gtk_gesture_drag_new (GTK_WIDGET (da)));

  g_signal_connect (G_OBJECT (da), "draw",
                    G_CALLBACK (draw_cb), NULL);
  g_signal_connect (G_OBJECT (da), "enter-notify-event",
                    G_CALLBACK (on_crossing),  NULL);
  g_signal_connect (G_OBJECT(da), "leave-notify-event",
                    G_CALLBACK (on_crossing),  NULL);
  /*g_signal_connect (G_OBJECT(gdrag), "drag-begin",*/
                    /*G_CALLBACK (drag_begin),  NULL);*/
  g_signal_connect (G_OBJECT(gdrag), "drag-update",
                    G_CALLBACK (drag_update),  da);
  g_signal_connect (G_OBJECT(gdrag), "drag-end",
                    G_CALLBACK (drag_end),  da);
}

