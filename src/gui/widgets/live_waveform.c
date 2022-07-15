/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
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
#include "audio/track.h"
#include "gui/widgets/live_waveform.h"
#include "gui/widgets/track.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/cairo.h"
#include "utils/midi.h"
#include "utils/objects.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "ext/zix/zix/ring.h"

#define BUF_SIZE 65000

G_DEFINE_TYPE (
  LiveWaveformWidget,
  live_waveform_widget,
  GTK_TYPE_DRAWING_AREA)

static void
draw_lines (
  LiveWaveformWidget * self,
  cairo_t *            cr,
  float *              lbuf,
  float *              rbuf,
  nframes_t            lstart_index,
  nframes_t            rstart_index)
{
  gint width =
    gtk_widget_get_allocated_width (GTK_WIDGET (self));
  gint height =
    gtk_widget_get_allocated_height (GTK_WIDGET (self));

  /* draw */
  gdk_cairo_set_source_rgba (cr, &self->color_green);
  float        half_height = (float) height / 2.0f;
  uint32_t     nframes = AUDIO_ENGINE->block_length;
  float        val;
  unsigned int step =
    MAX (1, (nframes / (unsigned int) width));

  for (unsigned int i = 0; i < nframes; i += step)
    {
      if (rbuf)
        {
          val = MAX (
            lbuf[lstart_index + i], rbuf[rstart_index + i]);
        }
      else
        {
          val = lbuf[lstart_index + i];
        }

      val = -val;

      double x = width * ((double) i / nframes);
      double y = half_height + val * half_height;

      if (i == 0)
        {
          cairo_move_to (cr, x, y);
        }

      cairo_line_to (cr, x, y);
    }
  cairo_stroke (cr);
}

static void
live_waveform_draw_cb (
  GtkDrawingArea * drawing_area,
  cairo_t *        cr,
  int              width,
  int              height,
  gpointer         user_data)
{
  LiveWaveformWidget * self =
    Z_LIVE_WAVEFORM_WIDGET (user_data);
  GtkWidget * widget = GTK_WIDGET (drawing_area);

  if (!PROJECT || !AUDIO_ENGINE)
    {
      return;
    }

  GtkStyleContext * context =
    gtk_widget_get_style_context (widget);

  gtk_render_background (context, cr, 0, 0, width, height);

  /* draw border */
  if (self->draw_border)
    {
      self->color_white.alpha = 0.2f;
      gdk_cairo_set_source_rgba (cr, &self->color_white);
      z_cairo_rounded_rectangle (
        cr, 0, 0, width, height, 1.0, 4.0);
      cairo_stroke (cr);
    }

  size_t block_size_in_bytes =
    sizeof (float) * (size_t) AUDIO_ENGINE->block_length;

  Port * port = NULL;
  switch (self->type)
    {
    case LIVE_WAVEFORM_ENGINE:
      g_return_if_fail (IS_TRACK_AND_NONNULL (P_MASTER_TRACK));
      if (!P_MASTER_TRACK->channel->stereo_out->l
             ->write_ring_buffers)
        {
          P_MASTER_TRACK->channel->stereo_out->l
            ->write_ring_buffers = true;
          P_MASTER_TRACK->channel->stereo_out->r
            ->write_ring_buffers = true;
          return;
        }
      port = P_MASTER_TRACK->channel->stereo_out->l;
      break;
    case LIVE_WAVEFORM_PORT:
      if (!self->port->write_ring_buffers)
        {
          self->port->write_ring_buffers = true;
          return;
        }
      port = self->port;
      break;
    }

  g_return_if_fail (IS_PORT_AND_NONNULL (port));

  /* if ring not ready yet skip draw */
  if (!port->audio_ring)
    return;

  /* get the L buffer */
  size_t read_space_avail =
    zix_ring_read_space (port->audio_ring);
  size_t blocks_to_read =
    block_size_in_bytes == 0
      ? 0
      : read_space_avail / block_size_in_bytes;
  /* if buffer is not filled do not draw */
  if (blocks_to_read <= 0)
    return;

  while (read_space_avail > self->buf_sz[0])
    {
      array_double_size_if_full (
        self->bufs[0], self->buf_sz[0], self->buf_sz[0],
        float);
    }
  size_t lblocks_read = zix_ring_peek (
    port->audio_ring, &(self->bufs[0][0]), read_space_avail);
  lblocks_read /= block_size_in_bytes;
  size_t lstart_index =
    (lblocks_read - 1) * AUDIO_ENGINE->block_length;
  if (lblocks_read == 0)
    {
      return;
      /*g_return_val_if_reached (FALSE);*/
    }

  if (self->type == LIVE_WAVEFORM_ENGINE)
    {
      /* get the R buffer */
      port = P_MASTER_TRACK->channel->stereo_out->r;
      read_space_avail =
        zix_ring_read_space (port->audio_ring);
      blocks_to_read = read_space_avail / block_size_in_bytes;

      /* if buffer is not filled do not draw */
      if (blocks_to_read <= 0)
        return;

      while (read_space_avail > self->buf_sz[1])
        {
          array_double_size_if_full (
            self->bufs[1], self->buf_sz[1], self->buf_sz[1],
            float);
        }
      size_t rblocks_read = zix_ring_peek (
        port->audio_ring, &(self->bufs[1][0]),
        read_space_avail);
      rblocks_read /= block_size_in_bytes;
      size_t rstart_index =
        (rblocks_read - 1) * AUDIO_ENGINE->block_length;
      if (rblocks_read == 0)
        {
          return;
          /*g_return_val_if_reached (FALSE);*/
        }

      draw_lines (
        self, cr, self->bufs[0], self->bufs[1], lstart_index,
        rstart_index);
    }
  else
    {
      draw_lines (
        self, cr, self->bufs[0], NULL, lstart_index, 0);
    }
}

static int
update_activity (
  GtkWidget *          widget,
  GdkFrameClock *      frame_clock,
  LiveWaveformWidget * self)
{
  gtk_widget_queue_draw (widget);

  return G_SOURCE_CONTINUE;
}

static void
init_common (LiveWaveformWidget * self)
{
  self->draw_border = 1;

  self->bufs[0] = object_new_n (BUF_SIZE, float);
  self->bufs[1] = object_new_n (BUF_SIZE, float);
  self->buf_sz[0] = BUF_SIZE;
  self->buf_sz[1] = BUF_SIZE;

  gtk_drawing_area_set_draw_func (
    GTK_DRAWING_AREA (self), live_waveform_draw_cb, self,
    NULL);

  gtk_widget_add_tick_callback (
    GTK_WIDGET (self), (GtkTickCallback) update_activity,
    self, NULL);
}

/**
 * Creates a LiveWaveformWidget for the
 * AudioEngine.
 */
void
live_waveform_widget_setup_engine (LiveWaveformWidget * self)
{
  init_common (self);
  self->type = LIVE_WAVEFORM_ENGINE;
}

/**
 * Creates a LiveWaveformWidget for a port.
 */
LiveWaveformWidget *
live_waveform_widget_new_port (Port * port)
{
  LiveWaveformWidget * self =
    g_object_new (LIVE_WAVEFORM_WIDGET_TYPE, NULL);

  init_common (self);

  self->type = LIVE_WAVEFORM_PORT;
  self->port = port;

  return self;
}

static void
finalize (LiveWaveformWidget * self)
{
  object_zero_and_free_if_nonnull (self->bufs[0]);
  object_zero_and_free_if_nonnull (self->bufs[1]);

  G_OBJECT_CLASS (live_waveform_widget_parent_class)
    ->finalize (G_OBJECT (self));
}

static void
live_waveform_widget_init (LiveWaveformWidget * self)
{
  gtk_widget_set_tooltip_text (
    GTK_WIDGET (self), _ ("Live waveform indicator"));
  gdk_rgba_parse (&self->color_white, "white");
  gdk_rgba_parse (&self->color_green, "#11FF44");
}

static void
live_waveform_widget_class_init (
  LiveWaveformWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (klass, "live-waveform");

  GObjectClass * oklass = G_OBJECT_CLASS (klass);
  oklass->finalize = (GObjectFinalizeFunc) finalize;
}
