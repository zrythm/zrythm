/*
 * Copyright (C) 2018-2020 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/channel.h"
#include "gui/widgets/meter.h"
#include "gui/widgets/fader.h"
#include "utils/math.h"

G_DEFINE_TYPE (
  MeterWidget, meter_widget, GTK_TYPE_DRAWING_AREA)

static int
meter_draw_cb (
  GtkWidget * widget,
  cairo_t * cr,
  MeterWidget * self)
{
  GtkStyleContext *context =
  gtk_widget_get_style_context (widget);

  int width =
    gtk_widget_get_allocated_width (widget);
  int height =
    gtk_widget_get_allocated_height (widget);

  gtk_render_background (
    context, cr, 0, 0, width, height);

  /* get values */
  float peak = self->meter_peak;
  float meter_val = self->meter_val;

  float value_px = (float) height * meter_val;
  if (value_px < 0)
    value_px = 0;

  /* draw filled in bar */
  float width_without_padding =
    (float) (width - self->padding * 2);

  GdkRGBA bar_color = { 0, 0, 0, 1 };
  double intensity = (double) meter_val;
  const double intensity_inv = 1.0 - intensity;
  bar_color.red =
    intensity_inv * self->end_color.red +
    intensity * self->start_color.red;
  bar_color.green =
    intensity_inv * self->end_color.green +
    intensity * self->start_color.green;
  bar_color.blue =
    intensity_inv * self->end_color.blue  +
    intensity * self->start_color.blue;

  gdk_cairo_set_source_rgba (cr, &bar_color);

  /* use gradient */
  cairo_pattern_t * pat =
    cairo_pattern_create_linear (
      0.0, 0.0, 0.0, height);
  cairo_pattern_add_color_stop_rgba (
    pat, 0,
    bar_color.red, bar_color.green,
    bar_color.blue, 1);
  cairo_pattern_add_color_stop_rgba (
    pat, 0.5,
    bar_color.red, bar_color.green,
    bar_color.blue, 1);
  cairo_pattern_add_color_stop_rgba (
    pat, 0.75, 0, 1, 0, 1);
  cairo_pattern_add_color_stop_rgba (
    pat, 1, 0, 0.2, 1, 1);
  cairo_set_source (
    cr, pat);

  float x = self->padding;
  cairo_rectangle (
    cr, x,
    (float) height - value_px,
    x + width_without_padding,
    value_px);
  cairo_fill (cr);

  cairo_pattern_destroy (pat);

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
    cr, x * 2 + width_without_padding,
    (float) height - value_px);
  cairo_stroke (cr);

  /* draw peak */
  float peak_amp =
    math_get_amp_val_from_fader (peak);
  if (peak_amp > 1.f)
    {
      cairo_set_source_rgba (
        /* make higher peak brighter */
        cr, 0.6 + 0.4 * (double) peak, 0.1, 0.05, 1);
    }
  else
    {
      cairo_set_source_rgba (
        /* make higher peak brighter */
        cr,
        0.4 + 0.4 * (double) peak,
        0.4 + 0.4 * (double) peak,
        0.4 + 0.4 * (double) peak,
        1);
    }
  cairo_set_line_width (cr, 2.0);
  double peak_px = (double) peak * height;
  cairo_move_to (
    cr, x, height - peak_px);
  cairo_line_to (
    cr, x * 2 + width_without_padding,
    height - peak_px);
  cairo_stroke (cr);

  return FALSE;
}


static void
on_crossing (GtkWidget * widget, GdkEvent *event)
{
  MeterWidget * self = Z_METER_WIDGET (widget);
  GdkEventType type =
    gdk_event_get_event_type (event);
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

static gboolean
tick_cb (
  GtkWidget * widget,
  GdkFrameClock * frame_clock,
  MeterWidget * self)
{
  gtk_widget_queue_draw (widget);

  return G_SOURCE_CONTINUE;
}

/*
 * Timeout to "run" the meter.
 */
static gboolean
meter_timeout (
  MeterWidget * self)
{
  if (GTK_IS_WIDGET (self))
    {
      if (self->meter &&
          gtk_widget_get_mapped (
            GTK_WIDGET (self)))
        {
          meter_get_value (
            self->meter, AUDIO_VALUE_FADER,
            &self->meter_val,
            &self->meter_peak);
        }
      else
        {
          /* not mapped, skipping dsp */
        }

      return G_SOURCE_CONTINUE;
    }
  else
    return G_SOURCE_REMOVE;
}

/**
 * Creates a new Meter widget and binds it to the
 * given value.
 *
 * @param port Port this meter is for.
 */
void
meter_widget_setup (
  MeterWidget *      self,
  Port *             port,
  int                width)
{
  if (self->meter)
    {
      meter_free (self->meter);
    }
  self->meter = meter_new_for_port (port);
  self->padding = 2;

  /* set size */
  gtk_widget_set_size_request (
    GTK_WIDGET (self), width, -1);
}

static void
finalize (
  MeterWidget * self)
{
  if (self->meter)
    meter_free (self->meter);

  g_source_remove (self->source_id);

  G_OBJECT_CLASS (
    meter_widget_parent_class)->
      finalize (G_OBJECT (self));
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

  gtk_widget_add_tick_callback (
    GTK_WIDGET (self), (GtkTickCallback) tick_cb,
    self, NULL);
  self->source_id =
    g_timeout_add (
      20, (GSourceFunc) meter_timeout, self);
}

static void
meter_widget_class_init (MeterWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (
    klass, "meter");

  GObjectClass * oklass = G_OBJECT_CLASS (_klass);
  oklass->finalize = (GObjectFinalizeFunc) finalize;
}
