// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/dsp/engine.h"
#include "common/dsp/master_track.h"
#include "common/dsp/track.h"
#include "common/dsp/tracklist.h"
#include "gui/cpp/backend/project.h"
#include "gui/cpp/backend/zrythm.h"
#include "gui/cpp/gtk_widgets/live_waveform.h"
#include "gui/cpp/gtk_widgets/zrythm_app.h"

#include <glib/gi18n.h>

constexpr size_t BUF_SIZE = 65000;

G_DEFINE_TYPE (LiveWaveformWidget, live_waveform_widget, GTK_TYPE_DRAWING_AREA)

static void
draw_lines (
  LiveWaveformWidget * self,
  cairo_t *            cr,
  const float *        lbuf,
  const float *        rbuf,
  size_t               lstart_index,
  size_t               rstart_index)
{
  gint width = gtk_widget_get_width (GTK_WIDGET (self));
  gint height = gtk_widget_get_height (GTK_WIDGET (self));

  /* draw */
  GdkRGBA color;
  gtk_widget_get_color (GTK_WIDGET (self), &color);
  gdk_cairo_set_source_rgba (cr, &color);
  float        half_height = (float) (height - 1) / 2.0f;
  uint32_t     nframes = AUDIO_ENGINE->block_length_;
  float        val;
  unsigned int step = MAX (1, (nframes / (unsigned int) width));

  for (unsigned int i = 0; i < nframes; i += step)
    {
      if (rbuf)
        {
          val = MAX (lbuf[lstart_index + i], rbuf[rstart_index + i]);
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
  LiveWaveformWidget * self = Z_LIVE_WAVEFORM_WIDGET (user_data);

  if (!PROJECT || !AUDIO_ENGINE)
    {
      return;
    }

  const auto block_size = (uint32_t) AUDIO_ENGINE->block_length_;

  AudioPort * port = nullptr;
  switch (self->type)
    {
    case LiveWaveformType::LIVE_WAVEFORM_ENGINE:
      z_return_if_fail (P_MASTER_TRACK);
      if (!P_MASTER_TRACK->channel_->stereo_out_->get_l ().write_ring_buffers_)
        {
          P_MASTER_TRACK->channel_->stereo_out_->set_write_ring_buffers (true);
          return;
        }
      port = &P_MASTER_TRACK->channel_->stereo_out_->get_l ();
      break;
    case LiveWaveformType::LIVE_WAVEFORM_PORT:
      if (!self->port->write_ring_buffers_)
        {
          self->port->write_ring_buffers_ = true;
          return;
        }
      port = self->port;
      break;
    }

  z_return_if_fail (IS_PORT_AND_NONNULL (port));

  /* if ring not ready yet skip draw */
  // if (!port->audio_ring_)
  // return;

  /* get the L buffer */
  uint32_t read_space_avail = port->audio_ring_->read_space ();
  uint32_t blocks_to_read = block_size == 0 ? 0 : read_space_avail / block_size;
  /* if buffer is not filled do not draw */
  if (blocks_to_read <= 0)
    return;

  while (read_space_avail > self->buffer->getNumSamples ())
    {
      self->buffer->setSize (
        2, self->buffer->getNumSamples () * 2, true, true, false);
    }
  uint32_t lblocks_read = port->audio_ring_->peek_multiple (
    self->buffer->getWritePointer (0), read_space_avail);
  lblocks_read /= block_size;
  uint32_t lstart_index = (lblocks_read - 1) * AUDIO_ENGINE->block_length_;
  if (lblocks_read == 0)
    {
      return;
      /*z_return_val_if_reached (FALSE);*/
    }

  if (self->type == LiveWaveformType::LIVE_WAVEFORM_ENGINE)
    {
      /* get the R buffer */
      port = &P_MASTER_TRACK->channel_->stereo_out_->get_r ();
      read_space_avail = port->audio_ring_->read_space ();
      blocks_to_read = read_space_avail / block_size;

      /* if buffer is not filled do not draw */
      if (blocks_to_read <= 0)
        return;

      while (read_space_avail > self->buffer->getNumSamples ())
        {
          self->buffer->setSize (
            2, self->buffer->getNumSamples () * 2, true, true, false);
        }
      size_t rblocks_read = port->audio_ring_->peek_multiple (
        self->buffer->getWritePointer (1), read_space_avail);
      rblocks_read /= block_size;
      size_t rstart_index = (rblocks_read - 1) * AUDIO_ENGINE->block_length_;
      if (rblocks_read == 0)
        {
          return;
          /*z_return_val_if_reached (FALSE);*/
        }

      draw_lines (
        self, cr, self->buffer->getReadPointer (0),
        self->buffer->getReadPointer (1), lstart_index, rstart_index);
    }
  else
    {
      draw_lines (
        self, cr, self->buffer->getReadPointer (0), nullptr, lstart_index, 0);
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

  self->buffer = std::make_unique<juce::AudioSampleBuffer> (2, BUF_SIZE);

  gtk_drawing_area_set_draw_func (
    GTK_DRAWING_AREA (self), live_waveform_draw_cb, self, nullptr);

  gtk_widget_add_tick_callback (
    GTK_WIDGET (self), (GtkTickCallback) update_activity, self, nullptr);
}

/**
 * Creates a LiveWaveformWidget for the
 * AudioEngine.
 */
void
live_waveform_widget_setup_engine (LiveWaveformWidget * self)
{
  init_common (self);
  self->type = LiveWaveformType::LIVE_WAVEFORM_ENGINE;
}

/**
 * Creates a LiveWaveformWidget for a port.
 */
LiveWaveformWidget *
live_waveform_widget_new_port (AudioPort * port)
{
  auto * self = static_cast<LiveWaveformWidget *> (
    g_object_new (LIVE_WAVEFORM_WIDGET_TYPE, nullptr));

  init_common (self);

  self->type = LiveWaveformType::LIVE_WAVEFORM_PORT;
  self->port = port;

  return self;
}

static void
finalize (GObject * gobj)
{
  LiveWaveformWidget * self = Z_LIVE_WAVEFORM_WIDGET (gobj);

  std::destroy_at (&self->buffer);

  G_OBJECT_CLASS (live_waveform_widget_parent_class)->finalize (G_OBJECT (self));
}

static void
live_waveform_widget_init (LiveWaveformWidget * self)
{
  std::construct_at (&self->buffer);

  gtk_widget_set_overflow (GTK_WIDGET (self), GTK_OVERFLOW_HIDDEN);
}

static void
live_waveform_widget_class_init (LiveWaveformWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (klass, "live-waveform");
  gtk_widget_class_set_accessible_role (klass, GTK_ACCESSIBLE_ROLE_PRESENTATION);

  GObjectClass * oklass = G_OBJECT_CLASS (klass);
  oklass->finalize = finalize;
}
