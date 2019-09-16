/*
 * gui/widgets/meter.c - meter widget
 *
 * Copyright (C) 2018 Alexandros Theodotou
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

/** \file
 */

#include "audio/channel.h"
#include "gui/widgets/meter.h"
#include "gui/widgets/fader.h"
#include "utils/math.h"

G_DEFINE_TYPE (MeterWidget, meter_widget, GTK_TYPE_DRAWING_AREA)

#define GET_REAL_VAL ((*self->getter) (self->object))

static float
meter_val_from_real (MeterWidget * self)
{
  float amp = math_dbfs_to_amp (GET_REAL_VAL);
  return math_get_fader_val_from_amp (amp);
}

static int
meter_draw_cb (
  GtkWidget * widget,
  cairo_t * cr, void* data)
{
  MeterWidget * self = (MeterWidget *) widget;

  GtkStyleContext *context =
  gtk_widget_get_style_context (widget);

  int width =
    gtk_widget_get_allocated_width (widget);
  int height =
    gtk_widget_get_allocated_height (widget);

  gtk_render_background (
    context, cr, 0, 0, width, height);

  float meter_val = -1.0;
  switch (self->type)
    {
    case METER_TYPE_DB:
      meter_val = meter_val_from_real (self);
      break;
    case METER_TYPE_MIDI:
      /* TODO */
      g_warn_if_reached ();
      break;
    }
  float value_px = (float) height * meter_val;
  if (value_px < 0)
    value_px = 0;

  /* draw filled in bar */
  float width_without_padding =
    (float) (width - 4);

  double intensity = (double) meter_val;
  const double intensity_inv = 1.0 - intensity;
  double r = intensity_inv * self->end_color.red   +
            intensity * self->start_color.red;
  double g = intensity_inv * self->end_color.green +
            intensity * self->start_color.green;
  double b = intensity_inv * self->end_color.blue  +
            intensity * self->start_color.blue;
  /*float a = intensity_inv * self->end_color.alpha  +*/
            /*intensity * self->start_color.alpha;*/
  cairo_set_source_rgba (cr, r,g,b, 1.0);
  float x = 2;
  cairo_rectangle (
    cr, x,
    (float) height - value_px,
    x + width_without_padding,
    value_px);
  cairo_fill(cr);

  /* draw border line */
  cairo_set_source_rgba (
    cr, 0.1, 0.1, 0.1, 1.0);
  cairo_set_line_width (cr, 1.7);
  cairo_rectangle (
    cr, x, 0,
    x + width_without_padding, height);
  cairo_stroke(cr);

  /* draw meter line */
  cairo_set_source_rgba (cr, 0.4, 0.1, 0.05, 1);
  cairo_set_line_width (cr, 1.0);
  cairo_move_to (
    cr, x, (float) height - value_px);
  cairo_line_to (
    cr, x + width_without_padding,
    (float) height - value_px);
  cairo_stroke (cr);

  return FALSE;
}


static void
on_crossing (GtkWidget * widget, GdkEvent *event)
{
  MeterWidget * self = Z_METER_WIDGET (widget);
  GdkEventType type = gdk_event_get_event_type (event);
  if (type == GDK_ENTER_NOTIFY)
    {
      self->hover = 1;
    }
  else if (type == GDK_LEAVE_NOTIFY)
    {
        self->hover = 0;
    }
  gtk_widget_queue_draw(widget);
}

/**
 * Creates a new Meter widget and binds it to the given value.
 */
void
meter_widget_setup (
  MeterWidget * self,
  float       (*getter)(void *),    ///< getter function
  void        * object,      ///< object to call get on
  MeterType   type,    ///< meter type
  int         width)
{
  self->getter = getter;
  self->object = object;
  self->type = type;

  /* set size */
  gtk_widget_set_size_request (GTK_WIDGET (self), width, -1);
}

static void
meter_widget_init (MeterWidget * self)
{
  /*gdk_rgba_parse (&self->start_color, "#00FF66");*/
  /*gdk_rgba_parse (&self->end_color, "#00FFCC");*/
  gdk_rgba_parse (&self->start_color, "#F9CA1B");
  gdk_rgba_parse (&self->end_color, "#1DDD6A");

  /* connect signals */
  g_signal_connect (
    G_OBJECT (self), "draw",
    G_CALLBACK (meter_draw_cb), self);
  g_signal_connect (
    G_OBJECT (self), "enter-notify-event",
    G_CALLBACK (on_crossing),  self);
  g_signal_connect (
    G_OBJECT(self), "leave-notify-event",
    G_CALLBACK (on_crossing),  self);
}

static void
meter_widget_class_init (MeterWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (klass,
                                 "meter");
}
