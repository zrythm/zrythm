/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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
  MidiRegionWidget * self,
  cairo_t *cr,
  gpointer data)
{
  REGION_WIDGET_GET_PRIVATE (data);
  guint width, height;
  GtkStyleContext *context;

  context =
    gtk_widget_get_style_context (GTK_WIDGET (self));

  width =
    gtk_widget_get_allocated_width (GTK_WIDGET (self));
  height =
    gtk_widget_get_allocated_height (GTK_WIDGET (self));

  gtk_render_background (context, cr, 0, 0, width, height);

  /*cairo_set_source_rgba (cr,*/
                         /*color->red - 0.3,*/
                         /*color->green - 0.3,*/
                         /*color->blue - 0.3,*/
                         /*0.9);*/
  cairo_set_source_rgba (
    cr, 1, 1, 1, 1);

  MidiRegion * mr =
    (MidiRegion *) rw_prv->region;
  Region * r = (Region *) mr;
  int num_loops =
    region_get_num_loops (r, 1);
  long ticks_in_region =
    region_get_full_length_in_ticks (
      (Region *) mr);
  float x_start, y_start, x_end;

  int min_val = 127, max_val = 0;
  for (int i = 0; i < mr->num_midi_notes; i++)
    {
      MidiNote * mn = mr->midi_notes[i];

      if (mn->val < min_val)
        min_val = mn->val;
      if (mn->val > max_val)
        max_val = mn->val;
    }
  float y_interval = MAX ((max_val - min_val) + 1.f,
                          12.f);
  float y_note_size = 1.f / y_interval;

  /* draw midi notes */
  long loop_end_ticks =
    position_to_ticks (&r->loop_end_pos);
  long loop_ticks =
    region_get_loop_length_in_ticks (r);
  long clip_start_ticks =
    position_to_ticks (&r->clip_start_pos);
  /*int px =*/
    /*ui_pos_to_px_timeline (&r->loop_start_pos, 0);*/
  for (int i = 0; i < mr->num_midi_notes; i++)
    {
      MidiNote * mn = mr->midi_notes[i];

      if (!midi_note_is_visible (mn))
        continue;

      /* get ratio (0.0 - 1.0) on x where midi note starts
       * & ends */
      int mn_start_ticks =
        position_to_ticks (&mn->start_pos);
      int mn_end_ticks =
        position_to_ticks (&mn->end_pos);
      int tmp_start_ticks, tmp_end_ticks;

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
            &mn->start_pos, &r->loop_end_pos) < 0)
        {
          for (int j = 0;
               j < num_loops;
               j++)
            {
              /* if note started before loop start
               * only draw it once */
              if (position_compare (
                    &mn->start_pos,
                    &r->loop_start_pos) < 0 &&
                  j != 0)
                break;

              /* calculate draw endpoints */
              tmp_start_ticks =
                mn_start_ticks + loop_ticks * j;
              /* if should be clipped */
              if (position_compare (
                    &mn->end_pos, &r->loop_end_pos) >= 0)
                tmp_end_ticks =
                  loop_end_ticks + loop_ticks * j;
              else
                tmp_end_ticks =
                  mn_end_ticks + loop_ticks * j;

              /* adjust for clip start */
              tmp_start_ticks -= clip_start_ticks;
              tmp_end_ticks -= clip_start_ticks;

              x_start =
                (float) tmp_start_ticks /
                ticks_in_region;
              x_end =
                (float) tmp_end_ticks /
                ticks_in_region;

              /* get ratio (0.0 - 1.0) on y where
               * midi note is */
              y_start =
                (max_val - mn->val) / y_interval;

              /* draw */
              cairo_rectangle (
                cr,
                x_start * width,
                y_start * height,
                (x_end - x_start) * width,
                y_note_size * height);
              cairo_fill (cr);
            }
        }
    }

  char * str =
    g_strdup (rw_prv->region->name);
  char * new_str = str;;
  Region * region = rw_prv->region;
  if (DEBUGGING)
    {
      if (region_is_transient (region))
        {
          new_str =
            g_strdup_printf (
              "%s [t]", str);
          g_free (str);
          str = new_str;
        }
      if (region_is_lane (region))
        {
          new_str =
            g_strdup_printf (
              "%s [l]", str);
          g_free (str);
          str = new_str;
        }
    }

#define FONT "Sans SemiBold 9"
#define PADDING_R 5
#define HEIGHT 19
#define CURVINESS 4.0

  /* draw dark bg behind text */
  PangoLayout *layout;
  PangoFontDescription *desc;
  /* Create a PangoLayout, set the font and text */
  layout = pango_cairo_create_layout (cr);
  pango_layout_set_text (layout, str, -1);
  desc = pango_font_description_from_string (FONT);
  pango_layout_set_font_description (layout, desc);
  pango_font_description_free (desc);
  pango_layout_set_ellipsize (
    layout, PANGO_ELLIPSIZE_END);
  pango_layout_set_width (
    layout,
    pango_units_from_double (width - PADDING_R));
  PangoRectangle pangorect;
  /* get extents */
  pango_layout_get_pixel_extents (
    layout, NULL, &pangorect);
  GdkRGBA c2;
  gdk_rgba_parse (&c2, "#323232");
  cairo_set_source_rgba (
    cr, c2.red, c2.green, c2.blue, 0.8);
  double radius = CURVINESS / 1.0;
  double degrees = G_PI / 180.0;
  cairo_new_sub_path (cr);
  cairo_move_to (cr, pangorect.width + PADDING_R, 0);
  cairo_arc (
    cr, (pangorect.width + PADDING_R) - radius,
    HEIGHT - radius, radius,
    0 * degrees, 90 * degrees);
  cairo_line_to (cr, 0, HEIGHT);
  cairo_line_to (cr, 0, 0);
  cairo_close_path (cr);
  cairo_fill (cr);
#undef PADDING_R
#undef HEIGHT
#undef CURVINESS
#undef FONT

  /* draw text */
  cairo_set_source_rgba (
    cr, 1, 1, 1, 1);
  cairo_translate (cr, 2, 2);
  pango_cairo_show_layout (cr, layout);
  /* free the layout object */
  g_object_unref (layout);
  g_free (str);

 return FALSE;
}

MidiRegionWidget *
midi_region_widget_new (MidiRegion * midi_region)
{
  MidiRegionWidget * self =
    g_object_new (MIDI_REGION_WIDGET_TYPE,
                  NULL);

  region_widget_setup (Z_REGION_WIDGET (self),
                       (Region *) midi_region);
  REGION_WIDGET_GET_PRIVATE (self);

  /* connect signals */
  g_signal_connect (
    G_OBJECT (rw_prv->drawing_area), "draw",
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
