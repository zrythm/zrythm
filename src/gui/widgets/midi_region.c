/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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
#include "audio/instrument_track.h"
#include "audio/midi_note.h"
#include "audio/track.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_region.h"
#include "gui/widgets/region.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline_arranger.h"
#include "utils/cairo.h"

G_DEFINE_TYPE (MidiRegionWidget,
               midi_region_widget,
               REGION_WIDGET_TYPE)


#define Y_PADDING 6.f
#define Y_HALF_PADDING 3.f

static gboolean
midi_region_draw_cb (
  GtkWidget *        widget,
  cairo_t *          cr,
  MidiRegionWidget * self)
{
  ARRANGER_OBJECT_WIDGET_GET_PRIVATE (self);

  GdkRectangle rect;
  gdk_cairo_get_clip_rectangle (
    cr, &rect);

  /* not cached, redraw */
  if (arranger_object_widget_should_redraw (
        (ArrangerObjectWidget *) self, &rect))
    {
      int i, j;
      REGION_WIDGET_GET_PRIVATE (self);

      GtkStyleContext *context =
        gtk_widget_get_style_context (widget);

      int width =
        gtk_widget_get_allocated_width (widget);
      int height =
        gtk_widget_get_allocated_height (widget);

      z_cairo_reset_caches (
        &ao_prv->cached_cr,
        &ao_prv->cached_surface, rect.width,
        rect.height, cr);

      gtk_render_background (
        context, ao_prv->cached_cr, 0, 0,
        rect.width, rect.height);

      /* draw background rectangle */
      region_widget_draw_background (
        (RegionWidget *) self, widget,
        ao_prv->cached_cr, &rect);

      /* draw loop dashes */
      region_widget_draw_loop_points (
        (RegionWidget *) self, widget,
        ao_prv->cached_cr, &rect);

      cairo_set_source_rgba (
        ao_prv->cached_cr, 1, 1, 1, 1);

      MidiRegion * r =
        (MidiRegion *) rw_prv->region;
      Region * main_region =
        region_get_main (r);
      ArrangerObject * r_obj =
        (ArrangerObject *) r;
      int num_loops =
        arranger_object_get_num_loops (r_obj, 1);
      long ticks_in_region =
        arranger_object_get_length_in_ticks (r_obj);
      float x_start, y_start, x_end;

      int min_val = 127, max_val = 0;
      for (i = 0; i < main_region->num_midi_notes; i++)
        {
          MidiNote * mn = main_region->midi_notes[i];

          if (mn->val < min_val)
            min_val = mn->val;
          if (mn->val > max_val)
            max_val = mn->val;
        }
      float y_interval =
        MAX (
          (float) (max_val - min_val) + 1.f, 7.f);
      float y_note_size = 1.f / y_interval;

      /* draw midi notes */
      long loop_end_ticks =
        r_obj->loop_end_pos.total_ticks;
      long loop_ticks =
        arranger_object_get_loop_length_in_ticks (
          r_obj);
      long clip_start_ticks =
        r_obj->clip_start_pos.total_ticks;

      for (i = 0; i < main_region->num_midi_notes; i++)
        {
          MidiNote * mn = main_region->midi_notes[i];

          mn =
            (MidiNote *)
            arranger_object_get_visible_counterpart (
              (ArrangerObject *) mn);
          ArrangerObject * mn_obj =
            (ArrangerObject *) mn;

          /* get ratio (0.0 - 1.0) on x where midi note
           * starts & ends */
          long mn_start_ticks =
            position_to_ticks (&mn_obj->pos);
          long mn_end_ticks =
            position_to_ticks (&mn_obj->end_pos);
          long tmp_start_ticks, tmp_end_ticks;

          /* if before loop end */
          if (position_is_before (
                &mn_obj->pos,
                &r_obj->loop_end_pos))
            {
              for (j = 0; j < num_loops; j++)
                {
                  /* if note started before loop start
                   * only draw it once */
                  if (position_is_before (
                        &mn_obj->pos,
                        &r_obj->loop_start_pos) &&
                      j != 0)
                    break;

                  /* calculate draw endpoints */
                  tmp_start_ticks =
                    mn_start_ticks + loop_ticks * j;
                  /* if should be clipped */
                  if (position_is_after_or_equal (
                        &mn_obj->end_pos,
                        &r_obj->loop_end_pos))
                    tmp_end_ticks =
                      loop_end_ticks + loop_ticks * j;
                  else
                    tmp_end_ticks =
                      mn_end_ticks + loop_ticks * j;

                  /* adjust for clip start */
                  tmp_start_ticks -= clip_start_ticks;
                  tmp_end_ticks -= clip_start_ticks;

                  /* get ratios (0.0 - 1.0) of
                   * where midi note is */
                  x_start =
                    (float) tmp_start_ticks /
                    (float) ticks_in_region;
                  x_end =
                    (float) tmp_end_ticks /
                    (float) ticks_in_region;
                  y_start =
                    ((float) max_val - (float) mn->val) /
                    y_interval;

                  /* get actual values using the
                   * ratios */
                  x_start *= (float) width;
                  x_end *= (float) width;
                  y_start *= (float) height;

                  /* skip if any part of the note is
                   * not visible in the rect */
                  if ((x_start >= rect.x &&
                       x_start < rect.x + rect.width) ||
                      (x_end >= rect.x &&
                       x_end < rect.x + rect.width) ||
                      (x_start < rect.x &&
                       x_end > rect.x))
                    {
                      /* draw */
                      cairo_rectangle (
                        ao_prv->cached_cr,
                        MAX (
                          x_start -
                            (float) rect.x, 0.f),
                        MAX (
                          y_start -
                            (float) rect.y, 0.f),
                        MIN (
                          x_end -
                            MAX (
                              x_start,
                              (float) rect.x),
                          (float) rect.width),
                        MIN (
                          y_note_size *
                            (float) height -
                            (float) rect.y,
                          (float) rect.height));
                      cairo_fill (ao_prv->cached_cr);
                    }
                }
            }
        }

      region_widget_draw_name (
        Z_REGION_WIDGET (self), ao_prv->cached_cr,
        &rect);

      arranger_object_widget_draw_cut_line (
        Z_ARRANGER_OBJECT_WIDGET (self),
        ao_prv->cached_cr, &rect);

      arranger_object_widget_post_redraw (
        (ArrangerObjectWidget *) self, &rect);
    }

  cairo_set_source_surface (
    cr, ao_prv->cached_surface, rect.x, rect.y);
  cairo_paint (cr);

 return FALSE;
}

MidiRegionWidget *
midi_region_widget_new (
  Region * midi_region)
{
  MidiRegionWidget * self =
    g_object_new (
      MIDI_REGION_WIDGET_TYPE,
      NULL);

  region_widget_setup (
    Z_REGION_WIDGET (self),
    midi_region);
  ARRANGER_OBJECT_WIDGET_GET_PRIVATE (self);

  /* connect signals */
  g_signal_connect (
    G_OBJECT (ao_prv->drawing_area), "draw",
    G_CALLBACK (midi_region_draw_cb), self);

  return self;
}

static void
midi_region_widget_class_init (
  MidiRegionWidgetClass * klass)
{
}

static void
midi_region_widget_init (
  MidiRegionWidget * self)
{
}
