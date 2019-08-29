/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/engine.h"
#include "audio/master_track.h"
#include "audio/midi.h"
#include "audio/track.h"
#include "gui/widgets/live_waveform.h"
#include "gui/widgets/track.h"
#include "gui/widgets/track_top_grid.h"
#include "project.h"
#include "utils/cairo.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

G_DEFINE_TYPE (LiveWaveformWidget,
               live_waveform_widget,
               GTK_TYPE_DRAWING_AREA)

/**
 * Draws the color picker.
 */
static int
live_waveform_draw_cb (
  GtkWidget *       widget,
  cairo_t *         cr,
  LiveWaveformWidget * self)
{
  GdkRGBA color;

  GtkStyleContext * context =
    gtk_widget_get_style_context (widget);

  guint width =
    gtk_widget_get_allocated_width (widget);
  guint height =
    gtk_widget_get_allocated_height (widget);

  gtk_render_background (
    context, cr, 0, 0, width, height);

  /* draw border */
  if (self->draw_border)
    {
      gdk_rgba_parse (&color, "white");
      color.alpha = 0.2;
      gdk_cairo_set_source_rgba (cr, &color);
      z_cairo_rounded_rectangle (
        cr, 0, 0, width, height, 1.0, 4.0);
      cairo_stroke (cr);
    }

  /* draw */
  gdk_rgba_parse (&color, "#11FF44");
  gdk_cairo_set_source_rgba (cr, &color);
  double half_height = height / 2.0;
  int nframes = AUDIO_ENGINE->nframes;
  float val;
  cairo_move_to (cr, 0, half_height);
  for (int i = 0; i < nframes; i++)
    {
      val =
        MAX (
          AUDIO_ENGINE->monitor_out->l->buf[i],
          AUDIO_ENGINE->monitor_out->r->buf[i]);

      cairo_line_to (
        cr, width * ((double) i / nframes),
        half_height + val * half_height);
    }
  cairo_stroke (cr);

  return FALSE;
}

static int
update_activity (
  GtkWidget * widget,
  GdkFrameClock * frame_clock,
  LiveWaveformWidget * self)
{
  gtk_widget_queue_draw (widget);

  return G_SOURCE_CONTINUE;
}

/**
 * Creates a LiveWaveformWidget for the
 * AudioEngine.
 */
void
live_waveform_widget_setup_engine (
  LiveWaveformWidget * self)
{
  self->draw_border = 1;

  g_signal_connect (
    G_OBJECT (self), "draw",
    G_CALLBACK (live_waveform_draw_cb), self);

  gtk_widget_add_tick_callback (
    GTK_WIDGET (self),
    (GtkTickCallback) update_activity,
    self, NULL);
}

static void
live_waveform_widget_init (
  LiveWaveformWidget * self)
{
  gtk_widget_set_tooltip_text (
    GTK_WIDGET (self), _("Live waveform indicator"));
}

static void
live_waveform_widget_class_init (
  LiveWaveformWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (
    klass, "live-waveform");
}
