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
#include "audio/chord_region.h"
#include "audio/instrument_track.h"
#include "audio/midi_note.h"
#include "audio/track.h"
#include "gui/backend/chord_selections.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/chord_region.h"
#include "gui/widgets/region.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline_arranger.h"
#include "utils/cairo.h"

G_DEFINE_TYPE (
  ChordRegionWidget,
  chord_region_widget,
  REGION_WIDGET_TYPE)

#define Y_PADDING 6.f
#define Y_HALF_PADDING 3.f

static gboolean
chord_region_draw_cb (
  GtkWidget * widget,
  cairo_t *cr,
  ChordRegionWidget * self)
{
  int i, j;
  REGION_WIDGET_GET_PRIVATE (self);

  GtkStyleContext *context =
    gtk_widget_get_style_context (widget);

  int width =
    gtk_widget_get_allocated_width (widget);
  int height =
    gtk_widget_get_allocated_height (widget);

  gtk_render_background (
    context, cr, 0, 0, width, height);

  Region * r = rw_prv->region;
  Region * main_region =
    region_get_main_region (r);
  int num_loops =
    region_get_num_loops (r, 1);
  long ticks_in_region =
    region_get_full_length_in_ticks (r);
  float x_start, x_end;

  /* draw midi notes */
  long loop_end_ticks =
    position_to_ticks (&r->loop_end_pos);
  long loop_ticks =
    region_get_loop_length_in_ticks (r);
  long clip_start_ticks =
    position_to_ticks (&r->clip_start_pos);
  ChordObject * co;
  ChordObject * next_co = NULL;
  for (i = 0; i < main_region->num_chord_objects;
       i++)
    {
      co = main_region->chord_objects[i];

      co =
        (ChordObject *)
        arranger_object_info_get_visible_counterpart (
          &co->obj_info);
      const ChordDescriptor * descr =
        chord_object_get_chord_descriptor (co);

      /* get ratio (0.0 - 1.0) on x where chord
       * starts & ends */
      long co_start_ticks =
        position_to_ticks (&co->pos);
      long co_end_ticks;
      if (i < main_region->num_chord_objects - 1)
        {
          next_co =
            main_region->chord_objects[i + 1];
          co_end_ticks =
            position_to_ticks (&next_co->pos);
        }
      else
        co_end_ticks =
          position_to_ticks (&main_region->end_pos);
      long tmp_start_ticks, tmp_end_ticks;

      /* adjust for clip start */
      /*int adjusted_mn_start_ticks =*/
        /*mn_start_ticks - clip_start_ticks;*/
      /*int adjusted_mn_end_ticks =*/
        /*mn_end_ticks - clip_end_ticks;*/

      /* if before loop start & after clip start */
      /*if (position_compare (*/
            /*&mn->start_pos, &r->loop_start_pos) < 0 &&*/
          /*position_compare (*/
            /*&mn->start_pos, &r->clip_start_pos) >= 0)*/
        /*{*/

        /*}*/
      /* if before loop end */
      if (position_compare (
            &co->pos, &r->loop_end_pos) < 0)
        {
          for (j = 0; j < num_loops; j++)
            {
              /* if note started before loop start
               * only draw it once */
              if (position_compare (
                    &co->pos,
                    &r->loop_start_pos) < 0 &&
                  j != 0)
                break;

              /* calculate draw endpoints */
              tmp_start_ticks =
                co_start_ticks + loop_ticks * j;
              /* if should be clipped */
              if (next_co &&
                  position_is_after_or_equal (
                    &next_co->pos,
                    &r->loop_end_pos))
                tmp_end_ticks =
                  loop_end_ticks + loop_ticks * j;
              else
                tmp_end_ticks =
                  co_end_ticks + loop_ticks * j;

              /* adjust for clip start */
              tmp_start_ticks -= clip_start_ticks;
              tmp_end_ticks -= clip_start_ticks;

              x_start =
                (float) tmp_start_ticks /
                (float) ticks_in_region;
              x_end =
                (float) tmp_end_ticks /
                (float) ticks_in_region;

              cairo_set_source_rgba (
                cr, 1, 1, 1, 0.3);

              /* draw */
              cairo_rectangle (
                cr,
                (double) x_start * (double) width,
                0,
                (double)
                  (x_end - x_start) *
                  (double) width,
                height);
              cairo_fill (cr);

              cairo_set_source_rgba (
                cr, 0, 0, 0, 1);

              /* draw name */
              PangoLayout * layout =
                z_cairo_create_default_pango_layout (
                  widget);
              char descr_str[100];
              chord_descriptor_to_string (
                descr, descr_str);
              z_cairo_draw_text_full (
                cr, widget,
                layout,
                descr_str,
                (int)
                ((double) x_start *
                  (double) width + 2.0),
                2);
              g_object_unref (layout);
            }
        }
    }

  region_widget_draw_name (
    Z_REGION_WIDGET (self), cr);

  return FALSE;
}

ChordRegionWidget *
chord_region_widget_new (
  Region * region)
{
  ChordRegionWidget * self =
    g_object_new (
      CHORD_REGION_WIDGET_TYPE,
      NULL);

  region_widget_setup (
    Z_REGION_WIDGET (self), region);
  REGION_WIDGET_GET_PRIVATE (self);

  /* connect signals */
  g_signal_connect (
    G_OBJECT (rw_prv->drawing_area), "draw",
    G_CALLBACK (chord_region_draw_cb), self);

  return self;
}

static void
chord_region_widget_class_init (
  ChordRegionWidgetClass * klass)
{
}

static void
chord_region_widget_init (
  ChordRegionWidget * self)
{
}
