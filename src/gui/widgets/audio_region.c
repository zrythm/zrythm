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

#include "audio/audio_bus_track.h"
#include "audio/channel.h"
#include "audio/clip.h"
#include "audio/engine.h"
#include "audio/instrument_track.h"
#include "audio/pool.h"
#include "audio/track.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/audio_region.h"
#include "gui/widgets/region.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline_arranger.h"
#include "project.h"
#include "utils/cairo.h"
#include "utils/ui.h"

G_DEFINE_TYPE (
  AudioRegionWidget,
  audio_region_widget,
  REGION_WIDGET_TYPE)

static gboolean
audio_region_draw_cb (
  GtkWidget *         widget,
  cairo_t *           cr,
  AudioRegionWidget * self)
{
  ARRANGER_OBJECT_WIDGET_GET_PRIVATE (self);

  /* not cached, redraw */
  if (ao_prv->redraw)
    {
      REGION_WIDGET_GET_PRIVATE (self);
      Region * r = rw_prv->region;
      Region * main_region =
        region_get_main (r);
      ArrangerObject * main_obj =
        (ArrangerObject *) main_region;

      GtkStyleContext * context =
        gtk_widget_get_style_context (widget);

      int width =
        gtk_widget_get_allocated_width (widget);
      int height =
        gtk_widget_get_allocated_height (widget);
      ao_prv->cached_surface =
        cairo_surface_create_similar (
          cairo_get_target (cr),
          CAIRO_CONTENT_COLOR_ALPHA,
          width,
          height);
      ao_prv->cached_cr =
        cairo_create (ao_prv->cached_surface);

      gtk_render_background (
        context, ao_prv->cached_cr,
        0, 0, width, height);

      /* draw background rectangle */
      region_widget_draw_background (
        (RegionWidget *) self, widget,
        ao_prv->cached_cr);

      /* draw loop dashes */
      region_widget_draw_loop_points (
        (RegionWidget *) self, widget,
        ao_prv->cached_cr);

      GdkRGBA * color =
        &main_region->lane->track->color;
      cairo_set_source_rgba (
        ao_prv->cached_cr, color->red + 0.4,
        color->green + 0.4,
        color->blue + 0.4, 1);
      cairo_set_line_width (ao_prv->cached_cr, 1);

      AudioClip * clip =
        AUDIO_POOL->clips[main_region->pool_id];

      long prev_frames = 0;
      long loop_end_frames =
        main_obj->loop_end_pos.frames;
      long loop_frames =
        arranger_object_get_loop_length_in_frames (
          main_obj);
      long clip_start_frames =
        main_obj->clip_start_pos.frames;
      for (double i = 0.0; i < (double) width; i += 0.6)
        {
          /* current single channel frames */
          long curr_frames =
            ui_px_to_frames_timeline (i, 0);
          curr_frames += clip_start_frames;
          while (curr_frames >= loop_end_frames)
            {
              curr_frames -= loop_frames;
            }
          float min = 0, max = 0;
          for (long j = prev_frames;
               j < curr_frames; j++)
            {
              for (unsigned int k = 0;
                   k < clip->channels; k++)
                {
                  long index =
                    j * clip->channels + k;
                  g_warn_if_fail (
                    index <
                      clip->num_frames * clip->channels);
                  float val =
                    clip->frames[index];
                  if (val > max)
                    max = val;
                  if (val < min)
                    min = val;
                }
            }
          /* normalize */
          min = (min + 1.f) / 2.f;
          max = (max + 1.f) / 2.f;
          z_cairo_draw_vertical_line (
            ao_prv->cached_cr,
            i,
            MAX (
              (double) min * (double) height, 0.0),
            MIN (
              (double) max * (double) height,
              (double) height));

          prev_frames = curr_frames;
        }

      region_widget_draw_name (
        Z_REGION_WIDGET (self),
        ao_prv->cached_cr);

      arranger_object_widget_draw_cut_line (
        Z_ARRANGER_OBJECT_WIDGET (self),
        ao_prv->cached_cr);

      ao_prv->redraw = 0;
    }

  cairo_set_source_surface (
    cr, ao_prv->cached_surface, 0, 0);
  cairo_paint (cr);

 return FALSE;
}

AudioRegionWidget *
audio_region_widget_new (
  AudioRegion * audio_region)
{
  AudioRegionWidget * self =
    g_object_new (
      AUDIO_REGION_WIDGET_TYPE, NULL);

  region_widget_setup (
    Z_REGION_WIDGET (self), audio_region);
  ARRANGER_OBJECT_WIDGET_GET_PRIVATE (self);

  /* connect signals */
  g_signal_connect (
    G_OBJECT (ao_prv->drawing_area), "draw",
    G_CALLBACK (audio_region_draw_cb), self);

  return self;
}

static void
audio_region_widget_class_init (
  AudioRegionWidgetClass * klass)
{
}

static void
audio_region_widget_init (
  AudioRegionWidget * self)
{
}

