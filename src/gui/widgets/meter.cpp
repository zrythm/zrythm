// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/channel.h"
#include "dsp/engine.h"
#include "dsp/meter.h"
#include "gui/widgets/fader.h"
#include "gui/widgets/meter.h"
#include "project.h"
#include "utils/gtk.h"
#include "utils/math.h"
#include "utils/objects.h"
#include "utils/ui.h"
#include "zrythm.h"
#include "zrythm_app.h"

G_DEFINE_TYPE (MeterWidget, meter_widget, GTK_TYPE_WIDGET)

static void
meter_snapshot (GtkWidget * widget, GtkSnapshot * snapshot)
{
  MeterWidget * self = Z_METER_WIDGET (widget);

  float width = (float) gtk_widget_get_width (widget);
  float height = (float) gtk_widget_get_height (widget);

  /* get values */
  float peak = self->meter_peak;
  float meter_val = self->meter_val;

  float value_px = height * meter_val;
  if (value_px < 0)
    value_px = 0;

#if 0
  GdkRGBA     bar_color = { 0, 0, 0, 1 };
  float       intensity = meter_val;
  const float intensity_inv = 1.f - intensity;
  bar_color.red =
    intensity_inv * self->end_color.red + intensity * self->start_color.red;
  bar_color.green =
    intensity_inv * self->end_color.green + intensity * self->start_color.green;
  bar_color.blue =
    intensity_inv * self->end_color.blue + intensity * self->start_color.blue;

  GdkRGBA color4;
  color_morph (&UI_COLORS->bright_green, &bar_color, 0.5f, &color4);
  color4.alpha = 1.f;
#endif

  /* gradient */
  /* Red 2 Yellow 2  Green 2
   * https://developer.gnome.org/hig/reference/palette.html */
  GskColorStop stop1, stop2, stop3;
  stop1.offset = 0;
  gdk_rgba_parse (&stop1.color, "#ED333B");
  stop2.offset = 0.5f;
  gdk_rgba_parse (&stop2.color, "#F8E45C");
  stop3.offset = 1.f;
  gdk_rgba_parse (&stop3.color, "#57E389");
  GskColorStop stops[] = {
    stop1,
    stop2,
    stop3,
  };

  /* used to stretch the gradient a little bit to make it look alive */
  float           value_px_for_gradient = (1 - value_px) * 0.02f;
  graphene_rect_t tmp_r =
    Z_GRAPHENE_RECT_INIT (0.f, height - value_px, width, value_px);
  graphene_point_t tmp_pt1 = Z_GRAPHENE_POINT_INIT (0, width);
  graphene_point_t tmp_pt2 =
    Z_GRAPHENE_POINT_INIT (0, height - value_px_for_gradient);
  gtk_snapshot_append_linear_gradient (
    snapshot, &tmp_r, &tmp_pt1, &tmp_pt2, stops, G_N_ELEMENTS (stops));

  /* draw meter line */
  GdkRGBA tmp_color = Z_GDK_RGBA_INIT (0.4f, 0.1f, 0.05f, 1);
  {
    graphene_rect_t tmp_r =
      Z_GRAPHENE_RECT_INIT (0.f, height - value_px, width, 1);
    gtk_snapshot_append_color (snapshot, &tmp_color, &tmp_r);
  }

  /* draw peak */
  float   peak_amp = math_get_amp_val_from_fader (peak);
  GdkRGBA color;
  if (peak_amp > 1.f && self->meter->port_->id_.type_ == PortType::Audio)
    {
      /* make higher peak brighter */
      color.red = 0.6f + 0.4f * peak;
      color.green = 0.1f;
      color.blue = 0.05f;
      color.alpha = 1;
    }
  else
    {
      /* make higher peak brighter */
      color.red = 0.4f + 0.4f * peak;
      color.green = 0.4f + 0.4f * peak;
      color.blue = 0.4f + 0.4f * peak;
      color.alpha = 1;
    }
  float peak_px = peak * height;
  {
    graphene_rect_t tmp_r =
      Z_GRAPHENE_RECT_INIT (0.f, height - peak_px, width, 2);
    gtk_snapshot_append_color (snapshot, &color, &tmp_r);
  }

  self->last_meter_val = self->meter_val;
  self->last_meter_peak = self->meter_peak;
}

static void
on_crossing (GtkWidget * widget, GdkEvent * event)
{
  MeterWidget * self = Z_METER_WIDGET (widget);
  GdkEventType  type = gdk_event_get_event_type (event);
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
tick_cb (GtkWidget * widget, GdkFrameClock * frame_clock, MeterWidget * self)
{
  if (
    gtk_widget_get_mapped (GTK_WIDGET (self)) && AUDIO_ENGINE->activated_
    && AUDIO_ENGINE->run_.load ())
    {
      self->meter->get_value (
        AudioValueFormat::Fader, &self->meter_val, &self->meter_peak);
    }
  else
    {
      /* not mapped, skipping dsp */
    }

  if (
    !math_floats_equal (self->meter_val, self->last_meter_val)
    || !math_floats_equal (self->meter_peak, self->last_meter_peak))
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

void
meter_widget_setup (MeterWidget * self, Port * port, bool small)
{
  object_delete_and_null (self->meter);
  self->meter = new Meter (*port);
  z_return_if_fail (self->meter);

  /* set size */
  int width = small ? 4 : 8;
  if (port->id_.type_ == PortType::Event)
    {
      width = small ? 6 : 10;
    }
  gtk_widget_set_size_request (GTK_WIDGET (self), width, -1);

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
    GTK_WIDGET (self), (GtkTickCallback) tick_cb, self, nullptr);

  z_trace ("meter widget set up for {}", port->get_full_designation ());
}

/**
 * Creates a new MeterWidget and binds it to the given port.
 *
 * @param port Port this meter is for.
 */
MeterWidget *
meter_widget_new (Port * port, int width)
{
  MeterWidget * self =
    static_cast<MeterWidget *> (g_object_new (METER_WIDGET_TYPE, nullptr));

  meter_widget_setup (self, port, true);

  return self;
}

static void
finalize (MeterWidget * self)
{
  object_delete_and_null (self->meter);

  G_OBJECT_CLASS (meter_widget_parent_class)->finalize (G_OBJECT (self));
}

static void
meter_widget_init (MeterWidget * self)
{
  self->start_color = UI_COLORS->z_yellow;
  self->end_color = UI_COLORS->bright_green;
  gtk_accessible_update_property (
    GTK_ACCESSIBLE (self), GTK_ACCESSIBLE_PROPERTY_VALUE_MAX, 2.0,
    GTK_ACCESSIBLE_PROPERTY_VALUE_MIN, 0.0, GTK_ACCESSIBLE_PROPERTY_VALUE_NOW,
    1.0, -1);
}

static void
meter_widget_class_init (MeterWidgetClass * _klass)
{
  GtkWidgetClass * wklass = GTK_WIDGET_CLASS (_klass);
  wklass->snapshot = meter_snapshot;
  gtk_widget_class_set_css_name (wklass, "meter");
  gtk_widget_class_set_accessible_role (wklass, GTK_ACCESSIBLE_ROLE_METER);

  GObjectClass * oklass = G_OBJECT_CLASS (_klass);
  oklass->finalize = (GObjectFinalizeFunc) finalize;
}
