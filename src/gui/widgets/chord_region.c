// SPDX-FileCopyrightText: © 2018-2019 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/audio_bus_track.h"
#include "dsp/channel.h"
#include "dsp/chord_region.h"
#include "dsp/instrument_track.h"
#include "dsp/midi_note.h"
#include "dsp/track.h"
#include "gui/backend/chord_selections.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/chord_region.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/region.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline_arranger.h"
#include "utils/cairo.h"

/**
 * Recreates the pango layout for drawing chord
 * names inside the region.
 */
void
chord_region_recreate_pango_layouts (ZRegion * self)
{
  ArrangerObject * obj = (ArrangerObject *) self;

  if (!PANGO_IS_LAYOUT (self->chords_layout))
    {
      PangoFontDescription * desc;
      self->chords_layout = gtk_widget_create_pango_layout (
        GTK_WIDGET (arranger_object_get_arranger (obj)), NULL);
      desc = pango_font_description_from_string (
        REGION_NAME_FONT_NO_SIZE " 6");
      pango_layout_set_font_description (
        self->chords_layout, desc);
      pango_font_description_free (desc);
    }
  pango_layout_get_pixel_size (
    self->chords_layout, &obj->textw, &obj->texth);
}

#if 0
#  define Y_PADDING 6.f
#  define Y_HALF_PADDING 3.f

static gboolean
chord_region_draw_cb (
  GtkWidget * widget,
  cairo_t *cr,
  ChordRegionWidget * self)
{
  int i, j;
  REGION_WIDGET_GET_PRIVATE (self);

  GdkRectangle rect;
  gdk_cairo_get_clip_rectangle (
    cr, &rect);

  GtkStyleContext *context =
    gtk_widget_get_style_context (widget);

  int width =
    gtk_widget_get_width (widget);
  int height =
    gtk_widget_get_height (widget);

  gtk_render_background (
    context, cr, 0, 0, width, height);

  ZRegion * r = rw_prv->region;
  ArrangerObject * r_obj =
    (ArrangerObject *) r;
  ZRegion * main_region =
    region_get_main (r);
  ArrangerObject * main_region_obj =
    (ArrangerObject *) main_region;
  int num_loops =
    arranger_object_get_num_loops (r_obj, 1);
  long ticks_in_region =
    arranger_object_get_length_in_ticks (r_obj);
  float x_start, x_end;

  /* draw midi notes */
  long loop_end_ticks =
    r_obj->loop_end_pos.total_ticks;
  long loop_ticks =
    arranger_object_get_loop_length_in_ticks (r_obj);
  long clip_start_ticks =
    r_obj->clip_start_pos.total_ticks;
  ChordObject * co;
  ChordObject * next_co = NULL;
  ArrangerObject * next_co_obj = NULL;
  for (i = 0; i < main_region->num_chord_objects;
       i++)
    {
      co =
        chord_object_get_visible_counterpart (
          main_region->chord_objects[i]);
      ArrangerObject * co_obj =
        (ArrangerObject *) co;
      const ChordDescriptor * descr =
        chord_object_get_chord_descriptor (co);

      /* get ratio (0.0 - 1.0) on x where chord
       * starts & ends */
      long co_start_ticks =
        co_obj->pos.total_ticks;
      long co_end_ticks;
      if (i < main_region->num_chord_objects - 1)
        {
          next_co =
            main_region->chord_objects[i + 1];
          next_co_obj =
            (ArrangerObject *) next_co;
          co_end_ticks =
            next_co_obj->pos.total_ticks;
        }
      else
        co_end_ticks =
          main_region_obj->end_pos.total_ticks;
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
      if (position_is_before (
            &co_obj->pos, &r_obj->loop_end_pos))
        {
          for (j = 0; j < num_loops; j++)
            {
              /* if note started before loop start
               * only draw it once */
              if (position_is_before (
                    &co_obj->pos,
                    &r_obj->loop_start_pos) &&
                  j != 0)
                break;

              /* calculate draw endpoints */
              tmp_start_ticks =
                co_start_ticks + loop_ticks * j;
              /* if should be clipped */
              if (next_co &&
                  position_is_after_or_equal (
                    &next_co_obj->pos,
                    &r_obj->loop_end_pos))
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
    Z_REGION_WIDGET (self), cr, &rect);

  return FALSE;
}

ChordRegionWidget *
chord_region_widget_new (
  ZRegion * region)
{
  ChordRegionWidget * self =
    g_object_new (
      CHORD_REGION_WIDGET_TYPE,
      NULL);

  region_widget_setup (
    Z_REGION_WIDGET (self), region);
  ARRANGER_OBJECT_WIDGET_GET_PRIVATE (self);

  /* connect signals */
  g_signal_connect (
    G_OBJECT (ao_prv->drawing_area), "draw",
    G_CALLBACK (chord_region_draw_cb), self);

  return self;
}
#endif
