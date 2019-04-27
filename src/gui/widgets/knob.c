/*
 * gui/widgets/knob.c - knob
 *
 * Copyright (C) 2018 Alexandros Theodotou
 * Copyright (C) 2010 Paul Davis
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
#include <stdlib.h>
#include <math.h>

#include "gui/widgets/knob.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (KnobWidget, knob_widget, GTK_TYPE_DRAWING_AREA)

#define ARC_CUT_ANGLE 60

/**
 * Macro to get real value.
 */
#define GET_REAL_VAL ((*self->getter) (self->object))

/**
 * MAcro to get real value from knob value.
 */
#define REAL_VAL_FROM_KNOB(knob) (self->min + knob * (self->max - self-> min))

/**
 * Converts from real value to knob value
 */
#define KNOB_VAL_FROM_REAL(real) ((real - self->min) / (self->max - self->min))

/**
 * Sets real val
 */
#define SET_REAL_VAL(real) ((*self->setter)(self->object, real))

/**
 * Draws the knob.
 */
static int
draw_cb (GtkWidget * widget, cairo_t * cr, void* data)
{
  guint width, height;
  GtkStyleContext *context;
  KnobWidget * self = (KnobWidget *) data;

  context = gtk_widget_get_style_context (widget);

  width = gtk_widget_get_allocated_width (widget);
  height = gtk_widget_get_allocated_height (widget);

  gtk_render_background (context, cr, 0, 0, width, height);
  cairo_pattern_t* shade_pattern;

  const float scale = width;
  const float pointer_thickness = 3.0 * (scale/80);  //(if the knob is 80 pixels wide, we want a 3-pix line on it)

  const float start_angle = ((180 - ARC_CUT_ANGLE) * G_PI) / 180;
  const float end_angle = ((360 + ARC_CUT_ANGLE) * G_PI) / 180;

  const float value = KNOB_VAL_FROM_REAL (GET_REAL_VAL);
  const float value_angle = start_angle
    + value * (end_angle - start_angle);
  const float zero_angle = start_angle + (self->zero * (end_angle - start_angle));

  float value_x = cos (value_angle);
  float value_y = sin (value_angle);

  float xc =  0.5 + width/ 2.0;
  float yc = 0.5 + height/ 2.0;

  cairo_translate (cr, xc, yc);  //after this, everything is based on the center of the knob

  //get the knob color from the theme

  float center_radius = 0.48*scale;
  float border_width = 0.8;


  if (self->arc)
    {
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
      float intensity = fabsf (value - self->zero) / MAX(self->zero, (1.f - self->zero));
      const float intensity_inv = 1.0 - intensity;
      float r = intensity_inv * self->end_color.red   +
                intensity * self->start_color.red;
      float g = intensity_inv * self->end_color.green +
                intensity * self->start_color.green;
      float b = intensity_inv * self->end_color.blue  +
                intensity * self->start_color.blue;

      //draw the arc
      cairo_set_source_rgb (cr, r,g,b);
      cairo_set_line_width (cr, progress_width);
      if (zero_angle > value_angle)
        {
          cairo_arc (cr, 0, 0, progress_radius, value_angle, zero_angle);
        }
      else
        {
          cairo_arc (cr, 0, 0, progress_radius, zero_angle, value_angle);
        }
      cairo_stroke (cr);

      //shade the arc
      if (!self->flat)
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

  if (!self->flat)
    {
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
      if (self->bevel)
        {
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
        }
      else
        {
          //radial gradient
          shade_pattern = cairo_pattern_create_radial ( -center_radius, -center_radius, 1, -center_radius, -center_radius, center_radius*2.5  );  //note we have to offset the gradient from our centerpoint
          cairo_pattern_add_color_stop_rgba (shade_pattern, 0.0, 1,1,1, 0.2);
          cairo_pattern_add_color_stop_rgba (shade_pattern, 1.0, 0,0,0, 0.3);
          cairo_set_source (cr, shade_pattern);
          cairo_arc (cr, 0, 0, center_radius, 0, 2.0*G_PI);
          cairo_fill (cr);
          cairo_pattern_destroy (shade_pattern);
        }
    }
  else
    {
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
  if (!self->flat) {
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
  if (self->hover)
    {
      cairo_set_source_rgba (cr, 1,1,1, 0.12 );
      cairo_arc (cr, 0, 0, center_radius, 0, 2.0*G_PI);
      cairo_fill (cr);
    }

  cairo_identity_matrix(cr);

  return FALSE;
}

static void
on_crossing (GtkWidget * widget, GdkEvent *event, void * data)
{
  KnobWidget * self = (KnobWidget *) data;
  if (event->type == GDK_ENTER_NOTIFY)
    {
      self->hover = 1;
    }
  else if (event->type == GDK_LEAVE_NOTIFY)
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
  KnobWidget * self = (KnobWidget *) user_data;
  offset_y = - offset_y;
  int use_y =
    fabs (offset_y - self->last_y) >
    fabs (offset_x - self->last_x);
  SET_REAL_VAL (REAL_VAL_FROM_KNOB (clamp (KNOB_VAL_FROM_REAL (GET_REAL_VAL) + 0.004 * (use_y ? offset_y - self->last_y : offset_x - self->last_x),
               1.0f, 0.0f)));
  self->last_x = offset_x;
  self->last_y = offset_y;
  gtk_widget_queue_draw ((GtkWidget *)user_data);
}

static void
drag_end (GtkGestureDrag *gesture,
               gdouble         offset_x,
               gdouble         offset_y,
               gpointer        user_data)
{
  KnobWidget * self = (KnobWidget *) user_data;
  self->last_x = 0;
  self->last_y = 0;
}


/**
 * Creates a knob widget with the given options and binds it to the given value.
 */
KnobWidget *
knob_widget_new (float (*get_val)(void *),    ///< getter function
                  void (*set_val)(void *, float),    ///< setter function
                  void * object,              ///< object to call get/set with
                  float min,       ///< min value
                  float max,       ///< max value
                    int         size,
                    float       zero)
{
  g_warn_if_fail (object);

  KnobWidget * self = g_object_new (KNOB_WIDGET_TYPE, NULL);
  /*self->value = value;*/
  self->getter = get_val;
  self->setter = set_val;
  self->object = object;
  self->min = min;
  self->max = max;
  /*self->cur = get_knob_val (self)*/
  self->size = size; /* default 30 */
  self->hover = 0;
  self->zero = zero; /* default 0.05f */
  self->arc = 1;
  self->bevel = 1;
  self->flat = 1;
  gdk_rgba_parse (&self->start_color, "rgb(78%,78%,78%)");
  gdk_rgba_parse (&self->end_color, "rgb(66%,66%,66%)");
  self->last_x = 0;
  self->last_y = 0;

  /* set size */
  gtk_widget_set_size_request (GTK_WIDGET (self), size, size);

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
  return self;
}

static void
knob_widget_init (KnobWidget * self)
{
  /* make it able to notify */
  gtk_widget_set_has_window (GTK_WIDGET (self), TRUE);
  int crossing_mask = GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK;
  gtk_widget_add_events (GTK_WIDGET (self), crossing_mask);
  self->drag = GTK_GESTURE_DRAG (gtk_gesture_drag_new (GTK_WIDGET (&self->parent_instance)));

  gtk_widget_set_visible (GTK_WIDGET (self),
                          1);
}

static void
knob_widget_class_init (KnobWidgetClass * klass)
{
}
