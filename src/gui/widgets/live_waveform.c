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
#include "zrythm_app.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

G_DEFINE_TYPE (
  LiveWaveformWidget, live_waveform_widget,
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
  if (!PROJECT || !AUDIO_ENGINE)
    {
      return false;
    }

  GtkStyleContext * context =
    gtk_widget_get_style_context (widget);

  gint width =
    gtk_widget_get_allocated_width (widget);
  gint height =
    gtk_widget_get_allocated_height (widget);

  gtk_render_background (
    context, cr, 0, 0, width, height);

  /* draw border */
  if (self->draw_border)
    {
      self->color_white.alpha = 0.2;
      gdk_cairo_set_source_rgba (
        cr, &self->color_white);
      z_cairo_rounded_rectangle (
        cr, 0, 0, width, height, 1.0, 4.0);
      cairo_stroke (cr);
    }

  /* draw */
  gdk_cairo_set_source_rgba (
    cr, &self->color_green);
  float half_height = (float) height / 2.0f;
  uint32_t nframes = AUDIO_ENGINE->block_length;
  float val;
  cairo_move_to (cr, 0, half_height);
  unsigned int step =
    MAX (1, (nframes / (unsigned int) width));

  size_t size =
    sizeof (float) *
    (size_t) AUDIO_ENGINE->block_length;

  if (!P_MASTER_TRACK)
    return FALSE;

  if (!P_MASTER_TRACK->channel->stereo_out->l->
        write_ring_buffers)
    {
      P_MASTER_TRACK->channel->stereo_out->l->
        write_ring_buffers = 1;
      P_MASTER_TRACK->channel->stereo_out->r->
        write_ring_buffers = 1;
      return FALSE;
    }

  /* get the L buffer */
  Port * port =
    P_MASTER_TRACK->channel->stereo_out->l;
  size_t read_space_avail =
    zix_ring_read_space (port->audio_ring);
  size_t blocks_to_read =
    size == 0 ?
      0 : read_space_avail / size;
  /* if buffer is not filled do not draw */
  if (blocks_to_read <= 0)
    return FALSE;

  float lbuf[
    AUDIO_ENGINE->block_length *
      blocks_to_read];
  size_t lblocks_read =
    zix_ring_peek (
      port->audio_ring, &lbuf[0],
      read_space_avail);
  lblocks_read /= size;
  size_t lstart_index =
    (lblocks_read - 1) *
      AUDIO_ENGINE->block_length;
  if (lblocks_read == 0)
    {
      return FALSE;
      /*g_return_val_if_reached (FALSE);*/
    }

  /* get the R buffer */
  port =
    P_MASTER_TRACK->channel->stereo_out->r;
  read_space_avail =
    zix_ring_read_space (port->audio_ring);
  blocks_to_read =
    read_space_avail / size;
  g_return_val_if_fail (
    blocks_to_read > 0, FALSE);
  float rbuf[
    AUDIO_ENGINE->block_length *
      blocks_to_read];
  size_t rblocks_read =
    zix_ring_peek (
      port->audio_ring, &rbuf[0],
      read_space_avail);
  rblocks_read /= size;
  size_t rstart_index =
    (rblocks_read - 1) *
      AUDIO_ENGINE->block_length;
  if (rblocks_read == 0)
    {
      return FALSE;
      /*g_return_val_if_reached (FALSE);*/
    }

  for (unsigned int i = 0; i < nframes; i += step)
    {
      val =
        MAX (
          lbuf[lstart_index + i],
          rbuf[rstart_index + i]);

      val = - val;

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
  gdk_rgba_parse (&self->color_white, "white");
  gdk_rgba_parse (&self->color_green, "#11FF44");
}

static void
live_waveform_widget_class_init (
  LiveWaveformWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (
    klass, "live-waveform");
}
