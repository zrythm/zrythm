/*
 * gui/widgets/channel_meter.c - Channel meter widget
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

#include "gui/widgets/channel_meter.h"
#include "gui/widgets/fader.h"

G_DEFINE_TYPE (ChannelMeterWidget, channel_meter_widget, GTK_TYPE_DRAWING_AREA)

#define GET_REAL_VAL_L ((*self->getter_l) (self->object))

#define GET_REAL_VAL_R ((*self->getter_r) (self->object))

static float
channel_meter_val_from_real (ChannelMeterWidget * self, int left)
{
  float real = left ? GET_REAL_VAL_L : GET_REAL_VAL_R;
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
real_val_from_channel_meter (ChannelMeterWidget * self, float channel_meter)
{
  if (channel_meter > FADER_STEP_1)
    return REAL_STEP_1 + ((channel_meter - FADER_STEP_1) / FADER_DIFF_1) * REAL_DIFF_1;
  else if (channel_meter > FADER_STEP_2)
    return REAL_STEP_2 + ((channel_meter - FADER_STEP_2) / FADER_DIFF_2) * REAL_DIFF_2;
  else if (channel_meter > FADER_STEP_3)
    return REAL_STEP_3 + ((channel_meter - FADER_STEP_3) / FADER_DIFF_3) * REAL_DIFF_3;
  else if (channel_meter > FADER_STEP_4)
    return REAL_STEP_4 + ((channel_meter - FADER_STEP_4) / FADER_DIFF_4) * REAL_DIFF_4;
  else
    return REAL_STEP_5 + ((channel_meter - FADER_STEP_5) / FADER_DIFF_5) * REAL_DIFF_5;
}

static int
draw_cb (GtkWidget * widget, cairo_t * cr, void* data)
{
  guint width, height;
  GtkStyleContext *context;
  ChannelMeterWidget * self = (ChannelMeterWidget *) widget;
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
  float l_channel_meter_val = channel_meter_val_from_real (self, 1);
  float r_channel_meter_val = channel_meter_val_from_real (self, 0);
  double l_value_px = height * (double) l_channel_meter_val;
  double r_value_px = height * (double) r_channel_meter_val;

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
  float half_width = width / 2 - 4;

  /* left */
  float l_intensity = l_channel_meter_val;
  const float l_intensity_inv = 1.0 - l_intensity;
  float l_r = l_intensity_inv * self->end_color.red   +
            l_intensity * self->start_color.red;
  float l_g = l_intensity_inv * self->end_color.green +
            l_intensity * self->start_color.green;
  float l_b = l_intensity_inv * self->end_color.blue  +
            l_intensity * self->start_color.blue;
  float l_a = l_intensity_inv * self->end_color.alpha  +
            l_intensity * self->start_color.alpha;
  cairo_set_source_rgba (cr, l_r,l_g,l_b, 1.0);
  float l_x = 2;
  cairo_new_sub_path (cr);
  cairo_line_to (cr, l_x + half_width, y + (height - l_value_px));
  cairo_arc (cr, l_x + half_width - radius, y + height - radius, radius, 0 * degrees, 90 * degrees);
  cairo_arc (cr, l_x + radius, y + height - radius, radius, 90 * degrees, 180 * degrees);
  cairo_line_to (cr, l_x, y + (height - l_value_px));
  cairo_close_path (cr);
  cairo_fill(cr);

  /* right */
  float r_intensity = r_channel_meter_val;
  const float r_intensity_inv = 1.0 - r_intensity;
  float r_r = r_intensity_inv * self->end_color.red   +
            r_intensity * self->start_color.red;
  float r_g = r_intensity_inv * self->end_color.green +
            r_intensity * self->start_color.green;
  float r_b = r_intensity_inv * self->end_color.blue  +
            r_intensity * self->start_color.blue;
  float r_a = r_intensity_inv * self->end_color.alpha  +
            r_intensity * self->start_color.alpha;
  cairo_set_source_rgba (cr, r_r,r_g,r_b, 1.0);
  float r_x = half_width + 6;
  cairo_new_sub_path (cr);
  cairo_line_to (cr, r_x + half_width, y + (height - r_value_px));
  cairo_arc (cr, r_x + half_width - radius, y + height - radius, radius, 0 * degrees, 90 * degrees);
  cairo_arc (cr, r_x + radius, y + height - radius, radius, 90 * degrees, 180 * degrees);
  cairo_line_to (cr, r_x, y + (height - r_value_px));
  cairo_close_path (cr);
  cairo_fill(cr);


  /* draw border line */
  /* left */
  cairo_set_source_rgba (cr, 0.2, 0.2, 0.2, 0.3);
  cairo_set_line_width (cr, 1.7);
  cairo_new_sub_path (cr);
  cairo_arc (cr, l_x + half_width - radius, y + radius, radius, -90 * degrees, 0 * degrees);
  cairo_arc (cr, l_x + half_width - radius, y + height - radius, radius, 0 * degrees, 90 * degrees);
  cairo_arc (cr, l_x + radius, y + height - radius, radius, 90 * degrees, 180 * degrees);
  cairo_arc (cr, l_x + radius, y + radius, radius, 180 * degrees, 270 * degrees);
  cairo_close_path (cr);
  cairo_stroke(cr);

  /* right */
  cairo_new_sub_path (cr);
  cairo_arc (cr, r_x + half_width - radius, y + radius, radius, -90 * degrees, 0 * degrees);
  cairo_arc (cr, r_x + half_width - radius, y + height - radius, radius, 0 * degrees, 90 * degrees);
  cairo_arc (cr, r_x + radius, y + height - radius, radius, 90 * degrees, 180 * degrees);
  cairo_arc (cr, r_x + radius, y + radius, radius, 180 * degrees, 270 * degrees);
  cairo_close_path (cr);
  cairo_stroke(cr);


  /* draw channel_meter line */
  /* left */
  cairo_set_source_rgba (cr, 0.4, 0.1, 0.05, 1);
  cairo_set_line_width (cr, 1.0);
  cairo_move_to (cr, l_x, y + (height - l_value_px));
  cairo_line_to (cr, l_x + half_width, y + (height - l_value_px));
  cairo_stroke (cr);

  /* right */
  cairo_set_source_rgba (cr, 0.4, 0.1, 0.05, 1);
  cairo_set_line_width (cr, 1.0);
  cairo_move_to (cr, r_x, y + (height - r_value_px));
  cairo_line_to (cr, r_x + half_width, y + (height - r_value_px));
  cairo_stroke (cr);

}


static void
on_crossing (GtkWidget * widget, GdkEvent *event)
{
  ChannelMeterWidget * self = CHANNEL_METER_WIDGET (widget);
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
 * Creates a new ChannelMeter widget and binds it to the given value.
 */
ChannelMeterWidget *
channel_meter_widget_new (float (*getter_l)(void *),    ///< getter function
                          float (*getter_r)(void *),
                  void * object,              ///< object to call get/set with
                  int width)
{
  ChannelMeterWidget * self = g_object_new (CHANNEL_METER_WIDGET_TYPE, NULL);
  self->getter_l = getter_l;
  self->getter_r = getter_r;
  self->object = object;

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
channel_meter_widget_init (ChannelMeterWidget * self)
{
  gdk_rgba_parse (&self->start_color, "#00FF66");
  gdk_rgba_parse (&self->end_color, "#00FFCC");

  /* make it able to notify */
  gtk_widget_set_has_window (GTK_WIDGET (self), TRUE);
  int crossing_mask = GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK;
  gtk_widget_add_events (GTK_WIDGET (self), crossing_mask);
}

static void
channel_meter_widget_class_init (ChannelMeterWidgetClass * klass)
{
}

