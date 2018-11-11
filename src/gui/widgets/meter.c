/*
 * gui/widgets/meter.c - meter widget
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

#include "audio/channel.h"
#include "gui/widgets/meter.h"
#include "gui/widgets/fader.h"

G_DEFINE_TYPE (MeterWidget, meter_widget, GTK_TYPE_DRAWING_AREA)

#define GET_REAL_VAL ((*self->getter) (self->object))

static float
meter_val_from_real (MeterWidget * self)
{
  float real = GET_REAL_VAL;
  if (real > REAL_STEP_1)
      return FADER_STEP_1 + (((real - REAL_STEP_1) / REAL_DIFF_1) * FADER_DIFF_1);
  else if (real > REAL_STEP_2)
      return FADER_STEP_2 + (((real - REAL_STEP_2) / REAL_DIFF_2) * FADER_DIFF_2);
  else if (real > REAL_STEP_3)
      return FADER_STEP_3 + (((real - REAL_STEP_3) / REAL_DIFF_3) * FADER_DIFF_3);
  else if (real > REAL_STEP_4)
      return FADER_STEP_4 + (((real - REAL_STEP_4) / REAL_DIFF_4) * FADER_DIFF_4);
  else
      return FADER_STEP_5 + (((real - REAL_STEP_5) / REAL_DIFF_5) * FADER_DIFF_5);
}

static float
real_val_from_meter (MeterWidget * self, float meter)
{
  if (meter > FADER_STEP_1)
    return REAL_STEP_1 + ((meter - FADER_STEP_1) / FADER_DIFF_1) * REAL_DIFF_1;
  else if (meter > FADER_STEP_2)
    return REAL_STEP_2 + ((meter - FADER_STEP_2) / FADER_DIFF_2) * REAL_DIFF_2;
  else if (meter > FADER_STEP_3)
    return REAL_STEP_3 + ((meter - FADER_STEP_3) / FADER_DIFF_3) * REAL_DIFF_3;
  else if (meter > FADER_STEP_4)
    return REAL_STEP_4 + ((meter - FADER_STEP_4) / FADER_DIFF_4) * REAL_DIFF_4;
  else
    return REAL_STEP_5 + ((meter - FADER_STEP_5) / FADER_DIFF_5) * REAL_DIFF_5;
}

static int
draw_cb (GtkWidget * widget, cairo_t * cr, void* data)
{
  guint width, height;
  GtkStyleContext *context;
  MeterWidget * self = (MeterWidget *) widget;
  context = gtk_widget_get_style_context (widget);

  width = gtk_widget_get_allocated_width (widget);
  height = gtk_widget_get_allocated_height (widget);

  gtk_render_background (context, cr, 0, 0, width, height);

  /* a custom shape that could be wrapped in a function */
  double arc_x         = 0,        /* parameters like cairo_rectangle */
         arc_y         = 0,
         aspect        = 1.0,     /* aspect ratio */
         corner_radius = height / 90.0;   /* and corner curvature radius */

  double radius = corner_radius / aspect;
  double degrees = G_PI / 180.0;
  /* value in pixels = total pixels * val
   * val is percentage */
  float meter_val;
  switch (self->type)
    {
    case METER_TYPE_DB:
      meter_val = meter_val_from_real (self);
      break;
    case METER_TYPE_MIDI:
      break;
    }
  double value_px = height * (double) meter_val;
  if (value_px < 0)
    value_px = 0;

  /* draw background bar */
  /*cairo_new_sub_path (cr);*/
  /*cairo_arc (cr, x + width - radius, y + radius, radius, -90 * degrees, 0 * degrees);*/
  /*cairo_line_to (cr, x + width, y + height - value_px);*/
  /*[>cairo_arc (cr, x + width - radius, y + height - radius, radius, 0 * degrees, 90 * degrees);<]*/
  /*cairo_line_to (cr, x, y + height - value_px);*/
  /*[>cairo_arc (cr, x + radius, y + height - radius, radius, 90 * degrees, 180 * degrees);<]*/
  /*cairo_arc (cr, x + radius, y + radius, radius, 180 * degrees, 270 * degrees);*/
  /*cairo_close_path (cr);*/

  /*cairo_set_source_rgba (cr, 0.4, 0.4, 0.4, 0.2);*/
  /*cairo_fill(cr);*/

  /* draw filled in bar */
  float width_without_padding = width - 4;

  float intensity = meter_val;
  const float intensity_inv = 1.0 - intensity;
  float r = intensity_inv * self->end_color.red   +
            intensity * self->start_color.red;
  float g = intensity_inv * self->end_color.green +
            intensity * self->start_color.green;
  float b = intensity_inv * self->end_color.blue  +
            intensity * self->start_color.blue;
  /*float a = intensity_inv * self->end_color.alpha  +*/
            /*intensity * self->start_color.alpha;*/
  cairo_set_source_rgba (cr, r,g,b, 1.0);
  float x = 2;
  cairo_new_sub_path (cr);
  cairo_move_to (cr, x, arc_y + (height - value_px));
  cairo_line_to (cr, x + width_without_padding, arc_y + (height - value_px));
  cairo_arc (cr, x + width_without_padding - radius, arc_y + height - radius, radius, 0 * degrees, 90 * degrees);
  cairo_arc (cr, x + radius, arc_y + height - radius, radius, 90 * degrees, 180 * degrees);
  cairo_line_to (cr, x, arc_y + (height - value_px));
  cairo_close_path (cr);
  cairo_fill(cr);


  /* draw border line */
  cairo_set_source_rgba (cr, 0.1, 0.1, 0.1, 1.0);
  cairo_set_line_width (cr, 1.7);
  cairo_new_sub_path (cr);
  cairo_move_to (cr, x + width_without_padding - radius, arc_y + radius);
  cairo_arc (cr, x + width_without_padding - radius, arc_y + radius, radius, -90 * degrees, 0 * degrees);
  cairo_arc (cr, x + width_without_padding - radius, arc_y + height - radius, radius, 0 * degrees, 90 * degrees);
  cairo_arc (cr, x + radius, arc_y + height - radius, radius, 90 * degrees, 180 * degrees);
  cairo_arc (cr, x + radius, arc_y + radius, radius, 180 * degrees, 270 * degrees);
  cairo_close_path (cr);
  cairo_stroke(cr);

  /* draw meter line */
  cairo_set_source_rgba (cr, 0.4, 0.1, 0.05, 1);
  cairo_set_line_width (cr, 1.0);
  cairo_move_to (cr, x, arc_y + (height - value_px));
  cairo_line_to (cr, x + width_without_padding, arc_y + (height - value_px));
  cairo_stroke (cr);
}


static void
on_crossing (GtkWidget * widget, GdkEvent *event)
{
  MeterWidget * self = METER_WIDGET (widget);
   switch (gdk_event_get_event_type (event))
    {
    case GDK_ENTER_NOTIFY:
      self->hover = 1;
      break;

    case GDK_LEAVE_NOTIFY:
        self->hover = 0;
      break;
    }
  gtk_widget_queue_draw(widget);
}

/**
 * Creates a new Meter widget and binds it to the given value.
 */
MeterWidget *
meter_widget_new (float       (*getter)(void *),    ///< getter function
                  void        * object,      ///< object to call get on
                  MeterType   type,    ///< meter type
                  int         width)
{
  MeterWidget * self = g_object_new (METER_WIDGET_TYPE, NULL);
  self->getter = getter;
  self->object = object;
  self->type = type;

  /* set size */
  gtk_widget_set_size_request (GTK_WIDGET (self), width, -1);

  /* connect signals */
  g_signal_connect (G_OBJECT (self), "draw",
                    G_CALLBACK (draw_cb), self);
  g_signal_connect (G_OBJECT (self), "enter-notify-event",
                    G_CALLBACK (on_crossing),  self);
  g_signal_connect (G_OBJECT(self), "leave-notify-event",
                    G_CALLBACK (on_crossing),  self);

  return self;
}

static void
meter_widget_init (MeterWidget * self)
{
  gdk_rgba_parse (&self->start_color, "#00FF66");
  gdk_rgba_parse (&self->end_color, "#00FFCC");

  /* make it able to notify */
  /*gtk_widget_set_has_window (GTK_WIDGET (self), TRUE);*/
  /*int crossing_mask = GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK;*/
  /*gtk_widget_add_events (GTK_WIDGET (self), crossing_mask);*/
}

static void
meter_widget_class_init (MeterWidgetClass * klass)
{
}


