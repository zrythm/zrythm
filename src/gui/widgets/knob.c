/*
 * gui/widgets/knob.c - knob
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
#include <stdint.h>
#include <math.h>

#include "gui/widgets/knob.h"

#include <gtk/gtk.h>

int size = 60;
int hover = 0;
float zero = 0.5f; /* zero point 0.0-1.0 */
float val = 0.6f; /* value 0.0-1.0 */
int arc = 1; /* draw arc around the knob */
int bevel = 1; /* ?? */
int flat = 1; /* add 3D shade if 0 */
double red_start = 0.8, green_start = 0.8, blue_start = 0.8; /* away from zero */
double red_end = 0.7, green_end = 0.7, blue_end = 0.7; /* close to zero */

GtkGestureDrag * gdrag;
static double last_x, last_y;

/**
 * Draws the knob.
 */
int
draw_cb (GtkWidget * widget, cairo_t * cr, void* data)
{
  guint width, height;
  GdkRGBA color;
  GtkStyleContext *context;

  context = gtk_widget_get_style_context (widget);

  width = gtk_widget_get_allocated_width (widget);
  height = gtk_widget_get_allocated_height (widget);

  gtk_render_background (context, cr, 0, 0, width, height);
  cairo_pattern_t* shade_pattern;

  const float scale = width;
  const float pointer_thickness = 3.0 * (scale/80);  //(if the knob is 80 pixels wide, we want a 3-pix line on it)

  const float start_angle = ((180 - 65) * G_PI) / 180;
  const float end_angle = ((360 + 65) * G_PI) / 180;

  const float value_angle = start_angle + (val * (end_angle - start_angle));
  const float zero_angle = start_angle + (zero * (end_angle - start_angle));

  float value_x = cos (value_angle);
  float value_y = sin (value_angle);

  float xc =  0.5 + width/ 2.0;
  float yc = 0.5 + height/ 2.0;

  cairo_translate (cr, xc, yc);  //after this, everything is based on the center of the knob

  //get the knob color from the theme

  float center_radius = 0.48*scale;
  float border_width = 0.8;


  if ( arc ) {
          center_radius = scale*0.33;

          float inner_progress_radius = scale*0.38;
          float outer_progress_radius = scale*0.48;
          float progress_width = (outer_progress_radius-inner_progress_radius);
          float progress_radius = inner_progress_radius + progress_width/2.0;

          /* dark surrounding arc background */
          cairo_set_source_rgb (cr, 0.3, 0.3, 0.3 );
          cairo_set_line_width (cr, progress_width);
          cairo_arc (cr, 0, 0, progress_radius, start_angle, end_angle);
          cairo_stroke (cr);

          //look up the surrounding arc colors from the config
          // TODO

          //vary the arc color over the travel of the knob
          float intensity = fabsf (val - zero) / MAX(zero, (1.f - zero));
          const float intensity_inv = 1.0 - intensity;
          float r = intensity_inv * red_end   + intensity * red_start;
          float g = intensity_inv * green_end + intensity * green_start;
          float b = intensity_inv * blue_end  + intensity * blue_start;

          //draw the arc
          cairo_set_source_rgb (cr, r,g,b);
          cairo_set_line_width (cr, progress_width);
          if (zero_angle > value_angle) {
                  cairo_arc (cr, 0, 0, progress_radius, value_angle, zero_angle);
          } else {
                  cairo_arc (cr, 0, 0, progress_radius, zero_angle, value_angle);
            g_message ("value angle = %f, zero_angle = %f",
                       value_angle, zero_angle);
          }
          cairo_stroke (cr);

          //shade the arc
          if (!flat)
            {
              shade_pattern = cairo_pattern_create_linear (0.0, -yc, 0.0,  yc);  //note we have to offset the pattern from our centerpoint
              cairo_pattern_add_color_stop_rgba (shade_pattern, 0.0, 1,1,1, 0.15);
              cairo_pattern_add_color_stop_rgba (shade_pattern, 0.5, 1,1,1, 0.0);
              cairo_pattern_add_color_stop_rgba (shade_pattern, 1.0, 1,1,1, 0.0);
              cairo_set_source (cr, shade_pattern);
              cairo_arc (cr, 0, 0, outer_progress_radius-1, 0, 2.0*G_PI);
              cairo_fill (cr);
              cairo_pattern_destroy (shade_pattern);
            }

#if 0 //black border
          const float start_angle_x = cos (start_angle);
          const float start_angle_y = sin (start_angle);
          const float end_angle_x = cos (end_angle);
          const float end_angle_y = sin (end_angle);

          cairo_set_source_rgb (cr, 0, 0, 0 );
          cairo_set_line_width (cr, border_width);
          cairo_move_to (cr, (outer_progress_radius * start_angle_x), (outer_progress_radius * start_angle_y));
          cairo_line_to (cr, (inner_progress_radius * start_angle_x), (inner_progress_radius * start_angle_y));
          cairo_stroke (cr);
          cairo_move_to (cr, (outer_progress_radius * end_angle_x), (outer_progress_radius * end_angle_y));
          cairo_line_to (cr, (inner_progress_radius * end_angle_x), (inner_progress_radius * end_angle_y));
          cairo_stroke (cr);
          cairo_arc (cr, 0, 0, outer_progress_radius, start_angle, end_angle);
          cairo_stroke (cr);
#endif
  }

  if (!flat) {
          //knob shadow
          cairo_save(cr);
          cairo_translate(cr, pointer_thickness+1, pointer_thickness+1 );
          cairo_set_source_rgba (cr, 0, 0, 0, 0.1 );
          cairo_arc (cr, 0, 0, center_radius-1, 0, 2.0*G_PI);
          cairo_fill (cr);
          cairo_restore(cr);

          //inner circle
#define KNOB_COLOR 0, 90, 0, 0
          cairo_set_source_rgba(cr, KNOB_COLOR); /* knob color */
          cairo_arc (cr, 0, 0, center_radius, 0, 2.0*G_PI);
          cairo_fill (cr);

          //gradient
          if (bevel) {
                  //knob gradient
                  shade_pattern = cairo_pattern_create_linear (0.0, -yc, 0.0,  yc);  //note we have to offset the gradient from our centerpoint
                  cairo_pattern_add_color_stop_rgba (shade_pattern, 0.0, 1,1,1, 0.2);
                  cairo_pattern_add_color_stop_rgba (shade_pattern, 0.2, 1,1,1, 0.2);
                  cairo_pattern_add_color_stop_rgba (shade_pattern, 0.8, 0,0,0, 0.2);
                  cairo_pattern_add_color_stop_rgba (shade_pattern, 1.0, 0,0,0, 0.2);
                  cairo_set_source (cr, shade_pattern);
                  cairo_arc (cr, 0, 0, center_radius, 0, 2.0*G_PI);
                  cairo_fill (cr);
                  cairo_pattern_destroy (shade_pattern);

                  //flat top over beveled edge
                  cairo_set_source_rgba (cr, 90, 0, 0, 0.5 );
                  cairo_arc (cr, 0, 0, center_radius-pointer_thickness, 0, 2.0*G_PI);
                  cairo_fill (cr);
          } else {
                  //radial gradient
                  shade_pattern = cairo_pattern_create_radial ( -center_radius, -center_radius, 1, -center_radius, -center_radius, center_radius*2.5  );  //note we have to offset the gradient from our centerpoint
                  cairo_pattern_add_color_stop_rgba (shade_pattern, 0.0, 1,1,1, 0.2);
                  cairo_pattern_add_color_stop_rgba (shade_pattern, 1.0, 0,0,0, 0.3);
                  cairo_set_source (cr, shade_pattern);
                  cairo_arc (cr, 0, 0, center_radius, 0, 2.0*G_PI);
                  cairo_fill (cr);
                  cairo_pattern_destroy (shade_pattern);
          }

  } else {
          /* color inner circle */
          cairo_set_source_rgba(cr, 70, 70, 70, 0.2);
          cairo_arc (cr, 0, 0, center_radius, 0, 2.0*G_PI);
          cairo_fill (cr);
  }


  //black knob border
  cairo_set_line_width (cr, border_width);
  cairo_set_source_rgba (cr, 0,0,0, 1 );
  cairo_arc (cr, 0, 0, center_radius, 0, 2.0*G_PI);
  cairo_stroke (cr);

  //line shadow
  if (!flat) {
          cairo_save(cr);
          cairo_translate(cr, 1, 1 );
          cairo_set_source_rgba (cr, 0,0,0,0.3 );
          cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
          cairo_set_line_width (cr, pointer_thickness);
          cairo_move_to (cr, (center_radius * value_x), (center_radius * value_y));
          cairo_line_to (cr, ((center_radius*0.4) * value_x), ((center_radius*0.4) * value_y));
          cairo_stroke (cr);
          cairo_restore(cr);
  }

  //line
  cairo_set_source_rgba (cr, 1,1,1, 1 );
  cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
  cairo_set_line_width (cr, pointer_thickness);
  cairo_move_to (cr, (center_radius * value_x), (center_radius * value_y));
  cairo_line_to (cr, ((center_radius*0.4) * value_x), ((center_radius*0.4) * value_y));
  cairo_stroke (cr);

  //highlight if grabbed or if mouse is hovering over me
  if (hover)
    {
      cairo_set_source_rgba (cr, 1,1,1, 0.12 );
      cairo_arc (cr, 0, 0, center_radius, 0, 2.0*G_PI);
      cairo_fill (cr);
    }

  cairo_identity_matrix(cr);
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

double clamp(double x, double upper, double lower)
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
  val = clamp (val + 0.004 * (use_y ? offset_y - last_y : offset_x - last_x),
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
 * sets knob.
 */
void
knob_set (GtkWidget * da)
{
  gtk_widget_set_size_request (da, size, size);

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

