/*
 * Copyright (C) 2018-2022 Alexandros Theodotou <alex at zrythm dot org>
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
#include "audio/engine.h"
#include "audio/meter.h"
#include "gui/widgets/fader.h"
#include "gui/widgets/meter.h"
#include "project.h"
#include "utils/color.h"
#include "utils/gtk.h"
#include "utils/math.h"
#include "utils/objects.h"
#include "utils/ui.h"
#include "zrythm_app.h"

G_DEFINE_TYPE (
  MeterWidget,
  meter_widget,
  GTK_TYPE_WIDGET)

static void
meter_snapshot (
  GtkWidget *   widget,
  GtkSnapshot * snapshot)
{
  MeterWidget * self = Z_METER_WIDGET (widget);

  GtkStyleContext * context =
    gtk_widget_get_style_context (widget);

  int width =
    gtk_widget_get_allocated_width (widget);
  int height =
    gtk_widget_get_allocated_height (widget);

  gtk_snapshot_render_background (
    snapshot, context, 0, 0, width, height);

  /* get values */
  float peak = self->meter_peak;
  float meter_val = self->meter_val;

  float value_px = (float) height * meter_val;
  if (value_px < 0)
    value_px = 0;

  /* draw filled in bar */
  float width_without_padding =
    (float) (width - self->padding * 2);

  GdkRGBA     bar_color = { 0, 0, 0, 1 };
  float       intensity = meter_val;
  const float intensity_inv = 1.f - intensity;
  bar_color.red =
    intensity_inv * self->end_color.red
    + intensity * self->start_color.red;
  bar_color.green =
    intensity_inv * self->end_color.green
    + intensity * self->start_color.green;
  bar_color.blue =
    intensity_inv * self->end_color.blue
    + intensity * self->start_color.blue;

  graphene_rect_t graphene_rect =
    GRAPHENE_RECT_INIT (0, 0, width, height);

  GdkRGBA color4;
  color_morph (
    &UI_COLORS->bright_green, &bar_color, 0.5f,
    &color4);
  color4.alpha = 1.f;

  /* use gradient */
  GskColorStop stop1, stop2, stop3, stop4, stop5;
  stop1.offset = 0;
  stop1.color = UI_COLORS->z_purple;
  stop2.offset = 0.2f;
  stop2.color = self->start_color;
  stop3.offset = 0.5f;
  stop3.color = bar_color;
  stop4.offset = 0.8f;
  stop4.color = color4;
  stop5.offset = 1;
  stop5.color = UI_COLORS->darkish_green;
  GskColorStop stops[] = {
    stop1, stop2, stop3, stop4, stop5
  };

  /* used to stretch the gradient a little bit to
   * make it look alive */
  float value_px_for_gradient =
    (1 - value_px) * 0.02f;

  float x = self->padding;
  gtk_snapshot_append_linear_gradient (
    snapshot,
    &GRAPHENE_RECT_INIT (
      x, (float) height - value_px,
      x + width_without_padding, value_px),
    &GRAPHENE_POINT_INIT (0, width),
    &GRAPHENE_POINT_INIT (
      0, (float) height - value_px_for_gradient),
    stops, G_N_ELEMENTS (stops));

  /* draw meter line */
  gtk_snapshot_append_color (
    snapshot,
    &Z_GDK_RGBA_INIT (0.4f, 0.1f, 0.05f, 1),
    &GRAPHENE_RECT_INIT (
      x, (float) height - value_px,
      width_without_padding, 1));

  /* draw peak */
  float peak_amp =
    math_get_amp_val_from_fader (peak);
  GdkRGBA color;
  if (peak_amp > 1.f)
    {
      /* make higher peak brighter */
      color.red = 0.6f + 0.4f * (float) peak;
      color.green = 0.1f;
      color.blue = 0.05f;
      color.alpha = 1;
    }
  else
    {
      /* make higher peak brighter */
      color.red = 0.4f + 0.4f * (float) peak;
      color.green = 0.4f + 0.4f * (float) peak;
      color.blue = 0.4f + 0.4f * (float) peak;
      color.alpha = 1;
    }
  float peak_px = (float) peak * height;
  gtk_snapshot_append_color (
    snapshot, &color,
    &GRAPHENE_RECT_INIT (
      x, (float) height - peak_px,
      width_without_padding, 2));

  self->last_meter_val = self->meter_val;
  self->last_meter_peak = self->meter_peak;

  /* draw border line */
  const float border_width = 1.f;
  GdkRGBA border_color = { 0.1f, 0.1f, 0.1f, 0.8f };
  float   border_widths[] = {
    border_width, border_width, border_width,
    border_width
  };
  GdkRGBA border_colors[] = {
    border_color, border_color, border_color,
    border_color
  };
  GskRoundedRect rounded_rect;
  gsk_rounded_rect_init_from_rect (
    &rounded_rect, &graphene_rect, 0);
  gtk_snapshot_append_border (
    snapshot, &rounded_rect, border_widths,
    border_colors);
}

static void
on_crossing (GtkWidget * widget, GdkEvent * event)
{
  MeterWidget * self = Z_METER_WIDGET (widget);
  GdkEventType  type =
    gdk_event_get_event_type (event);
  if (type == GDK_ENTER_NOTIFY)
    {
      self->hover = 1;
    }
  else if (type == GDK_LEAVE_NOTIFY)
    {
      self->hover = 0;
    }
  gtk_widget_queue_draw (widget);
}

static gboolean
tick_cb (
  GtkWidget *     widget,
  GdkFrameClock * frame_clock,
  MeterWidget *   self)
{
  if (
    gtk_widget_get_mapped (GTK_WIDGET (self))
    && AUDIO_ENGINE->activated
    && engine_get_run (AUDIO_ENGINE))
    {
      meter_get_value (
        self->meter, AUDIO_VALUE_FADER,
        &self->meter_val, &self->meter_peak);
    }
  else
    {
      /* not mapped, skipping dsp */
    }

  if (
    !math_floats_equal (
      self->meter_val, self->last_meter_val)
    || !math_floats_equal (
      self->meter_peak, self->last_meter_peak))
    {
      gtk_widget_queue_draw (widget);
    }

  return G_SOURCE_CONTINUE;
}

#if 0
/*
 * Timeout to "run" the meter.
 */
static int
meter_timeout (
  void * data)
{
  MeterWidget * self = (MeterWidget *) data;

  if (GTK_IS_WIDGET (self))
    {
      if (self->meter
          &&
          gtk_widget_get_mapped (GTK_WIDGET (self))
          && AUDIO_ENGINE->activated
          && engine_get_run (AUDIO_ENGINE))
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
#endif

/**
 * Creates a new Meter widget and binds it to the
 * given value.
 *
 * @param port Port this meter is for.
 */
void
meter_widget_setup (
  MeterWidget * self,
  Port *        port,
  int           width)
{
  if (self->meter)
    {
      meter_free (self->meter);
    }
  self->meter = meter_new_for_port (port);
  g_return_if_fail (self->meter);
  self->padding = 2;

  /* set size */
  gtk_widget_set_size_request (
    GTK_WIDGET (self), width, -1);

  /* connect signals */
#if 0
  g_signal_connect (
    G_OBJECT (self), "enter-notify-event",
    G_CALLBACK (on_crossing),  self);
  g_signal_connect (
    G_OBJECT(self), "leave-notify-event",
    G_CALLBACK (on_crossing),  self);
#endif
  (void) on_crossing;

  gtk_widget_add_tick_callback (
    GTK_WIDGET (self), (GtkTickCallback) tick_cb,
    self, NULL);
#if 0
  self->timeout_source = g_timeout_source_new (20);
  g_source_set_callback (
    self->timeout_source, meter_timeout,
    self, NULL);
  self->source_id =
    g_source_attach (self->timeout_source, NULL);
#endif

  char buf[1200];
  port_get_full_designation (port, buf);
  g_message ("meter widget set up for %s", buf);
}

static void
finalize (MeterWidget * self)
{
  object_free_w_func_and_null (
    meter_free, self->meter);

  G_OBJECT_CLASS (meter_widget_parent_class)
    ->finalize (G_OBJECT (self));
}

static void
meter_widget_init (MeterWidget * self)
{
  self->start_color = UI_COLORS->z_yellow;
  self->end_color = UI_COLORS->bright_green;
}

static void
meter_widget_class_init (MeterWidgetClass * _klass)
{
  GtkWidgetClass * wklass =
    GTK_WIDGET_CLASS (_klass);
  wklass->snapshot = meter_snapshot;
  gtk_widget_class_set_css_name (wklass, "meter");

  GObjectClass * oklass = G_OBJECT_CLASS (_klass);
  oklass->finalize = (GObjectFinalizeFunc) finalize;
}
