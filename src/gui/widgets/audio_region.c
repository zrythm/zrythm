/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/bus_track.h"
#include "audio/channel.h"
#include "audio/instrument_track.h"
#include "audio/track.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/audio_region.h"
#include "gui/widgets/region.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline_arranger.h"
#include "utils/cairo.h"
#include "utils/ui.h"

G_DEFINE_TYPE (AudioRegionWidget,
               audio_region_widget,
               REGION_WIDGET_TYPE)

static gboolean
audio_region_draw_cb (AudioRegionWidget * self,
         cairo_t *cr, gpointer data)
{
  REGION_WIDGET_GET_PRIVATE (data);
  Region * r = rw_prv->region;
  AudioRegion * ar = (AudioRegion *) r;

  guint width, height;
  GtkStyleContext *context;

  context =
    gtk_widget_get_style_context (GTK_WIDGET (self));

  width =
    gtk_widget_get_allocated_width (
      GTK_WIDGET (self));
  height =
    gtk_widget_get_allocated_height (
      GTK_WIDGET (self));

  gtk_render_background (context, cr,
                         0, 0, width, height);

  GdkRGBA * color = &r->track->color;
  cairo_set_source_rgba (cr,
                         color->red + 0.3,
                         color->green + 0.3,
                         color->blue + 0.3,
                         0.9);
  cairo_set_line_width (cr, 1);

  int num_loops =
    region_get_num_loops (r, 1);
  long ticks_in_region =
    region_get_full_length_in_ticks (r);
  float x_start, y_start, x_end;

  long prev_frames = 0;
  long loop_end_frames =
    position_to_frames (&r->loop_end_pos) *
    ar->channels;
  long loop_start_frames =
    position_to_frames (&r->loop_end_pos) *
    ar->channels;
  long loop_frames =
    region_get_loop_length_in_frames (r) *
    ar->channels;
  long clip_start_frames =
    position_to_frames (&r->clip_start_pos) *
    ar->channels;
  int px =
    ui_pos_to_px_timeline (&r->loop_start_pos, 0);
  Position tmp;
  long frame_interval =
    ui_px_to_frames_timeline (ar->channels, 0);
  for (double i = 0.0; i < (double) width; i += 0.6)
    {
      long curr_frames =
        i * frame_interval;
      /*if (curr_frames < loop_start_frames)*/
        curr_frames += clip_start_frames;
      /*else*/
        while (curr_frames >= loop_end_frames)
          curr_frames -= loop_frames;
      float min = 0, max = 0;
      for (int j = prev_frames; j < curr_frames; j++)
        {
          float val = ar->buff[j];
          if (val > max)
            max = val;
          if (val < min)
            min = val;
        }
      min = (min + 1.0) / 2.0; /* normallize */
      max = (max + 1.0) / 2.0; /* normalize */
      z_cairo_draw_vertical_line (
        cr,
        i,
        MAX (min * height, 0),
        MIN (max * height, height));

      prev_frames = curr_frames;
    }

 return FALSE;
}

/**
 * Sets the appropriate cursor.
 */
static void
on_motion (GtkWidget * widget, GdkEventMotion *event)
{
  AudioRegionWidget * self = Z_AUDIO_REGION_WIDGET (widget);
  GtkAllocation allocation;
  gtk_widget_get_allocation (widget,
                             &allocation);
  ARRANGER_WIDGET_GET_PRIVATE (MW_TIMELINE);
  REGION_WIDGET_GET_PRIVATE (self);
}

AudioRegionWidget *
audio_region_widget_new (AudioRegion * audio_region)
{
  g_message ("Creating audio region widget...");
  AudioRegionWidget * self =
    g_object_new (AUDIO_REGION_WIDGET_TYPE, NULL);

  region_widget_setup (Z_REGION_WIDGET (self),
                       (Region *) audio_region);

  /* connect signals */
  g_signal_connect (
    G_OBJECT (self), "draw",
    G_CALLBACK (audio_region_draw_cb), self);
  g_signal_connect (
    G_OBJECT (self), "enter-notify-event",
    G_CALLBACK (on_motion),  self);
  g_signal_connect (
    G_OBJECT(self), "leave-notify-event",
    G_CALLBACK (on_motion),  self);
  g_signal_connect (
    G_OBJECT(self), "motion-notify-event",
    G_CALLBACK (on_motion),  self);

  return self;
}

static void
audio_region_widget_class_init (AudioRegionWidgetClass * klass)
{
}

static void
audio_region_widget_init (AudioRegionWidget * self)
{
}

