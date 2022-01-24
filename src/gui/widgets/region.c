/*
 * Copyright (C) 2018-2022 Alexandros Theodotou <alex at zrythm dot org>
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

#include "zrythm-config.h"

#include <math.h>

#include "audio/audio_bus_track.h"
#include "audio/audio_region.h"
#include "audio/automation_region.h"
#include "audio/channel.h"
#include "audio/fade.h"
#include "audio/instrument_track.h"
#include "audio/tempo_track.h"
#include "audio/track.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/arranger_object.h"
#include "gui/widgets/automation_point.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/cpu.h"
#include "gui/widgets/main_notebook.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/region.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_panel.h"
#include "gui/widgets/timeline_ruler.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/cairo.h"
#include "utils/color.h"
#include "utils/debug.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/math.h"
#include "utils/objects.h"
#include "utils/ui.h"
#include "zrythm_app.h"
#include "zrythm.h"

#include <glib/gi18n-lib.h>

static const GdkRGBA object_fill_color = {
  1, 1, 1, 1 };

/** Background color for the name. */
/*static GdkRGBA name_bg_color;*/

typedef enum RegionCounterpart
{
  REGION_COUNTERPART_MAIN,
  REGION_COUNTERPART_LANE,
} RegionCounterpart;

/**
 * Recreates the pango layouts for drawing.
 *
 * @param width Full width of the region.
 */
static void
recreate_pango_layouts (
  ZRegion * self,
  int      width)
{
  ArrangerObject * obj = (ArrangerObject *) self;

  if (G_UNLIKELY (!PANGO_IS_LAYOUT (self->layout)))
    {
      PangoFontDescription *desc;
      self->layout =
        gtk_widget_create_pango_layout (
          GTK_WIDGET (
            arranger_object_get_arranger (obj)),
          NULL);
      desc =
        pango_font_description_from_string (
          REGION_NAME_FONT);
      pango_layout_set_font_description (
        self->layout, desc);
      pango_font_description_free (desc);
      pango_layout_set_ellipsize (
        self->layout, PANGO_ELLIPSIZE_END);
    }
  pango_layout_set_width (
    self->layout,
    pango_units_from_double (
      MAX (width - REGION_NAME_PADDING_R, 0)));
}

/**
 * Returns the last_*_full_rect and
 * last_*_draw_rect.
 *
 * @param[out] full_rect
 * @param[out] draw_rect
 */
static void
get_last_rects (
  ZRegion *         self,
  RegionCounterpart counterpart,
  GdkRectangle *    full_rect,
  GdkRectangle *    draw_rect)
{
  if (counterpart == REGION_COUNTERPART_MAIN)
    {
      *draw_rect = self->last_main_draw_rect;
      *full_rect = self->last_main_full_rect;
    }
  else
    {
      *draw_rect = self->last_lane_draw_rect;
      *full_rect = self->last_lane_full_rect;
    }

  g_return_if_fail ((*draw_rect).width < 40000);
}

/**
 * @param rect Arranger rectangle.
 * @param full_rect Object full rectangle.
 */
static void
draw_loop_points (
  ZRegion *      self,
  GtkSnapshot *  snapshot,
  GdkRectangle * full_rect,
  GdkRectangle * draw_rect)
{
  double dashes[] = { 5 };

  ArrangerObject * obj =
    (ArrangerObject *) self;

  ArrangerWidget * arranger =
    arranger_object_get_arranger (obj);

  Position tmp;
  double loop_start_ticks =
    obj->loop_start_pos.ticks;
  double loop_end_ticks =
    obj->loop_end_pos.ticks;
  z_warn_if_fail_cmp (
    loop_end_ticks, >, loop_start_ticks);
  double loop_ticks =
    arranger_object_get_loop_length_in_ticks (
      obj);
  double clip_start_ticks =
    obj->clip_start_pos.ticks;

  /* get x px for loop point */
  position_from_ticks (
    &tmp, loop_start_ticks - clip_start_ticks);
  int x_px =
    ui_pos_to_px_timeline (&tmp, 0);

  int vis_offset_x =
    draw_rect->x - full_rect->x;
  int vis_width = draw_rect->width;
  int full_width = full_rect->width;
  int full_height = full_rect->height;

  const int padding = 1;
  const int line_width = 1;

  GskRenderNode * loop_line_node = NULL;
  if (!arranger->loop_line_node)
    {
      arranger->loop_line_node =
        gsk_cairo_node_new (
          &GRAPHENE_RECT_INIT (
            0, 0, 3, 800));
      cairo_t * cr =
        gsk_cairo_node_get_draw_context (
          arranger->loop_line_node);

      cairo_set_dash (
        cr, dashes, 1, 0);
      cairo_set_line_width (cr, line_width);
      cairo_set_source_rgba (
        cr, 0, 0, 0, 1.0);
      cairo_move_to (cr, padding, 0);
      cairo_line_to (cr, padding, 800);
      cairo_stroke (cr);
      cairo_destroy (cr);
    }
  loop_line_node = arranger->loop_line_node;

  GskRenderNode * clip_start_line_node = NULL;
  if (!arranger->clip_start_line_node)
    {
      arranger->clip_start_line_node =
        gsk_cairo_node_new (
          &GRAPHENE_RECT_INIT (
            0, 0, 3, 800));

      cairo_t * cr =
        gsk_cairo_node_get_draw_context (
          arranger->clip_start_line_node);
      gdk_cairo_set_source_rgba (
        cr, &UI_COLORS->bright_green);
      cairo_set_dash (
        cr, dashes, 1, 0);
      cairo_set_line_width (cr, line_width);
      cairo_move_to (cr, padding, 0);
      cairo_line_to (cr, padding, 800);
      cairo_stroke (cr);
      cairo_destroy (cr);
    }
  clip_start_line_node =
    arranger->clip_start_line_node;


  /* draw clip start point */
  if (x_px != 0 &&
      /* if px is inside region */
      x_px >= 0 &&
      x_px < full_width &&
      /* if loop px is visible */
      x_px >= vis_offset_x &&
      x_px < vis_offset_x + vis_width)
    {
      gtk_snapshot_save (snapshot);
      gtk_snapshot_translate (
        snapshot,
        &GRAPHENE_POINT_INIT (x_px - padding, 0));
      gtk_snapshot_push_clip (
        snapshot,
        &GRAPHENE_RECT_INIT (
          0, 0, line_width + padding * 2,
          full_height));
      gtk_snapshot_append_node (
        snapshot, clip_start_line_node);
      gtk_snapshot_pop (snapshot);
      gtk_snapshot_restore (snapshot);
    }

  /* draw loop points */
  int num_loops =
    arranger_object_get_num_loops (obj, 1);
  for (int i = 0; i < num_loops; i++)
    {
      x_px =
        (int)
        ((double)
         ((loop_end_ticks + loop_ticks * i)
          - clip_start_ticks)
         /* adjust for clip_start */
         * MW_RULER->px_per_tick);

      if (
          /* if px is vixible */
          x_px >= vis_offset_x &&
          x_px <= vis_offset_x + vis_width &&
          /* if px is inside region */
          x_px >= 0 &&
          x_px < full_width)
        {
          gtk_snapshot_save (snapshot);
          gtk_snapshot_translate (
            snapshot,
            &GRAPHENE_POINT_INIT (
              x_px - padding, 0));
          gtk_snapshot_push_clip (
            snapshot,
            &GRAPHENE_RECT_INIT (
              0, 0, line_width + padding * 2,
              full_height));
          gtk_snapshot_append_node (
            snapshot, loop_line_node);
          gtk_snapshot_pop (snapshot);
          gtk_snapshot_restore (snapshot);
        }
    }
}

/**
 * @param rect Arranger rectangle.
 */
static void
draw_midi_region (
  ZRegion *      self,
  GtkSnapshot *  snapshot,
  GdkRectangle * full_rect,
  GdkRectangle * draw_rect)
{
  ArrangerObject * obj =
    (ArrangerObject *) self;

  Track * track =
    arranger_object_get_track (obj);

  /* set color */
  GdkRGBA color = object_fill_color;
  if (track)
    {
      /* set to grey if track color is very
       *  bright */
      if (color_is_very_very_bright (&track->color))
        {
          color.red = 0.7f;
          color.green = 0.7f;
          color.blue = 0.7f;
        }
    }

  int num_loops =
    arranger_object_get_num_loops (obj, 1);
  double ticks_in_region =
    arranger_object_get_length_in_ticks (obj);
  double x_start, y_start, x_end;

  int min_val = 127, max_val = 0;
  for (int i = 0; i < self->num_midi_notes; i++)
    {
      MidiNote * mn = self->midi_notes[i];

      if (mn->val < min_val)
        min_val = mn->val;
      if (mn->val > max_val)
        max_val = mn->val;
    }
  double y_interval =
    MAX (
      (double) (max_val - min_val) + 1.0, 7.0);
  double y_note_size = 1.0 / y_interval;

  /* draw midi notes */
  double loop_end_ticks =
    obj->loop_end_pos.ticks;
  double loop_ticks =
    arranger_object_get_loop_length_in_ticks (
      obj);
  double clip_start_ticks =
    obj->clip_start_pos.ticks;

  int vis_offset_x =
    draw_rect->x - full_rect->x;
  int vis_offset_y =
    draw_rect->y - full_rect->y;
  int vis_width = draw_rect->width;
  int vis_height = draw_rect->height;
  /*int full_offset_x = full_rect->x;*/
  /*int full_offset_y = full_rect->y;*/
  int full_width = full_rect->width;
  int full_height = full_rect->height;

  bool is_looped = region_is_looped (self);

  for (int i = 0; i < self->num_midi_notes; i++)
    {
      MidiNote * mn = self->midi_notes[i];
      ArrangerObject * mn_obj =
        (ArrangerObject *) mn;

      /* note muted */
      if (arranger_object_get_muted (mn_obj))
        continue;

      /* note not playable */
      if (position_is_after_or_equal (
            &mn_obj->pos, &obj->loop_end_pos))
        continue;

      /* note not playable if looped */
      if (is_looped
          &&
          position_is_before (
            &mn_obj->pos, &obj->loop_start_pos)
          &&
          position_is_before (
            &mn_obj->pos, &obj->clip_start_pos))
        continue;

      /* get ratio (0.0 - 1.0) on x where midi note
       * starts & ends */
      double mn_start_ticks =
        position_to_ticks (&mn_obj->pos);
      double mn_end_ticks =
        position_to_ticks (&mn_obj->end_pos);
      double tmp_start_ticks, tmp_end_ticks;

      for (int j = 0; j < num_loops; j++)
        {
          /* if note started before loop start
           * only draw it once */
          if (position_is_before (
                &mn_obj->pos,
                &obj->loop_start_pos) &&
              j != 0)
            break;

          /* calculate draw endpoints */
          tmp_start_ticks =
            mn_start_ticks + loop_ticks * j;
          /* if should be clipped */
          if (position_is_after_or_equal (
                &mn_obj->end_pos,
                &obj->loop_end_pos))
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
            tmp_start_ticks /
            ticks_in_region;
          x_end =
            tmp_end_ticks /
            ticks_in_region;
          y_start =
            ((double) max_val -
             (double) mn->val) /
            y_interval;

          /* get actual values using the
           * ratios */
          x_start *=
            (double) full_width;
          x_end *=
            (double) full_width;
          y_start *=
            (double) full_height;

          /* the above values are local to the
           * region, convert to global */
          /*x_start += full_rect->x;*/
          /*x_end += full_rect->x;*/
          /*y_start += full_rect->y;*/

          /* skip if any part of the note is
           * not visible in the region's rect */
          if ((x_start >= vis_offset_x &&
               x_start <
                 vis_offset_x + vis_width) ||
              (x_end >= vis_offset_x &&
               x_end <
                 vis_offset_x + vis_width) ||
              (x_start < vis_offset_x &&
               x_end > vis_offset_x))
            {
              float draw_x =
                (float)
                MAX (x_start, vis_offset_x);
              float draw_width =
                (float)
                MIN (
                  (x_end - x_start) -
                    (draw_x - x_start),
                  (vis_offset_x + vis_width) -
                  draw_x);
              float draw_y =
                (float)
                MAX (y_start, vis_offset_y);
              float draw_height =
                (float)
                MIN (
                  (y_note_size *
                    (double) full_height) -
                      (draw_y - y_start),
                  (vis_offset_y + vis_height) -
                    draw_y);
              gtk_snapshot_append_color (
                snapshot, &color,
                &GRAPHENE_RECT_INIT (
                  draw_x, draw_y, draw_width,
                  draw_height));

            } /* endif part of note is visible */

        } /* end foreach region loop */

    } /* end foreach note */
}

/**
 * @param rect Arranger rectangle.
 */
static void
draw_chord_region (
  ZRegion *      self,
  GtkSnapshot *  snapshot,
  GdkRectangle * full_rect,
  GdkRectangle * draw_rect)
{
  ArrangerObject * obj =
    (ArrangerObject *) self;

  int num_loops =
    arranger_object_get_num_loops (obj, 1);
  double ticks_in_region =
    arranger_object_get_length_in_ticks (obj);
  double x_start, x_end;

  /*int vis_offset_x =*/
    /*draw_rect->x - full_rect->x;*/
  /*int vis_offset_y =*/
    /*draw_rect->y - full_rect->y;*/
  /*int vis_width = draw_rect->width;*/
  /*int vis_height = draw_rect->height;*/
  /*int full_offset_x = full_rect->x;*/
  /*int full_offset_y = full_rect->y;*/
  int full_width = full_rect->width;
  int full_height = full_rect->height;

  /* draw chords notes */
  double loop_end_ticks =
    obj->loop_end_pos.ticks;
  double loop_ticks =
    arranger_object_get_loop_length_in_ticks (obj);
  double clip_start_ticks =
    obj->clip_start_pos.ticks;
  ChordObject * co;
  ChordObject * next_co = NULL;
  ArrangerObject * next_co_obj = NULL;
  for (int i = 0; i < self->num_chord_objects; i++)
    {
      co = self->chord_objects[i];
      ArrangerObject * co_obj =
        (ArrangerObject *) co;
      const ChordDescriptor * descr =
        chord_object_get_chord_descriptor (co);

      /* get ratio (0.0 - 1.0) on x where chord
       * starts & ends */
      double co_start_ticks =
        co_obj->pos.ticks;
      double co_end_ticks;
      if (i < self->num_chord_objects - 1)
        {
          next_co =
            self->chord_objects[i + 1];
          next_co_obj =
            (ArrangerObject *) next_co;
          co_end_ticks =
            next_co_obj->pos.ticks;
        }
      else
        co_end_ticks =
          obj->end_pos.ticks;
      double tmp_start_ticks, tmp_end_ticks;

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
            &co_obj->pos, &obj->loop_end_pos))
        {
          for (int j = 0; j < num_loops; j++)
            {
              /* if note started before loop start
               * only draw it once */
              if (position_is_before (
                    &co_obj->pos,
                    &obj->loop_start_pos) &&
                  j != 0)
                break;

              /* calculate draw endpoints */
              tmp_start_ticks =
                co_start_ticks + loop_ticks * j;
              /* if should be clipped */
              if (next_co &&
                  position_is_after_or_equal (
                    &next_co_obj->pos,
                    &obj->loop_end_pos))
                tmp_end_ticks =
                  loop_end_ticks + loop_ticks * j;
              else
                tmp_end_ticks =
                  co_end_ticks + loop_ticks * j;

              /* adjust for clip start */
              tmp_start_ticks -= clip_start_ticks;
              tmp_end_ticks -= clip_start_ticks;

              x_start =
                tmp_start_ticks /
                ticks_in_region;
              x_end =
                tmp_end_ticks /
                ticks_in_region;

              /* skip if before the region */
              if (x_start < 0.0)
                continue;

              GdkRGBA color = { 1, 1, 1, 1 };

              /* get actual values using the
               * ratios */
              x_start *= (double) full_width;
              x_end *= (double) full_width;

              /* draw */
              gtk_snapshot_append_color (
                snapshot, &color,
                &GRAPHENE_RECT_INIT (
                  (float) x_start, 0,
                  2.f, (float) full_height));

              /* draw name */
              char desc_str[100];
              chord_descriptor_to_string (
                descr, desc_str);
              gtk_snapshot_save (snapshot);
              gtk_snapshot_translate (
                snapshot,
                &GRAPHENE_POINT_INIT (
                  (float) x_start + 4.f,
                  (float) full_height - 10.f));
              PangoLayout * layout =
                self->chords_layout;
              pango_layout_set_markup (
                layout, desc_str, -1);
              gtk_snapshot_append_layout (
                snapshot, layout, &color);
              gtk_snapshot_restore (snapshot);
            }
        }
    }
}

/**
 * @param rect Arranger rectangle.
 */
static void
draw_automation_region (
  ZRegion *      self,
  GtkSnapshot *  snapshot,
  GdkRectangle * full_rect,
  GdkRectangle * draw_rect)
{
  ArrangerObject * obj = (ArrangerObject *) self;

  GdkRGBA color = object_fill_color;

  UiDetail detail = ui_get_detail_level ();

  /* use cairo if normal detail or higher */
  bool use_cairo = false;
  if (detail < UI_DETAIL_LOW)
    {
      use_cairo = true;
    }

  int num_loops =
    arranger_object_get_num_loops (
      obj, 1);
  double ticks_in_region =
    arranger_object_get_length_in_ticks (obj);
  double x_start, y_start, x_end, y_end;

  int full_width = full_rect->width;
  int full_height = full_rect->height;

  /* draw automation */
  double loop_end_ticks =
    obj->loop_end_pos.ticks;
  double loop_ticks =
    arranger_object_get_loop_length_in_ticks (obj);
  double clip_start_ticks =
    obj->clip_start_pos.ticks;
  AutomationPoint * ap, * next_ap;
  for (int i = 0; i < self->num_aps; i++)
    {
      ap = self->aps[i];
      next_ap =
        automation_region_get_next_ap (
          self, ap, true, true);
      ArrangerObject * ap_obj =
        (ArrangerObject *) ap;
      ArrangerObject * next_ap_obj =
        (ArrangerObject *) next_ap;

      double ap_start_ticks =
        ap_obj->pos.ticks;
      double ap_end_ticks = ap_start_ticks;
      if (next_ap)
        {
          ap_end_ticks =
            next_ap_obj->pos.ticks;
        }
      double tmp_start_ticks, tmp_end_ticks;

      /* if before loop end */
      if (position_is_before (
            &ap_obj->pos, &obj->loop_end_pos))
        {
          for (int j = 0; j < num_loops; j++)
            {
              /* if ap started before loop start
               * only draw it once */
              if (position_is_before (
                    &ap_obj->pos,
                    &obj->loop_start_pos) &&
                  j != 0)
                break;

              /* calculate draw endpoints */
              tmp_start_ticks =
                ap_start_ticks +
                loop_ticks * (double) j;

              /* if should be clipped */
              if (next_ap &&
                  position_is_after_or_equal (
                    &next_ap_obj->pos,
                    &obj->loop_end_pos))
                tmp_end_ticks =
                  loop_end_ticks +
                  loop_ticks *  (double) j;
              else
                tmp_end_ticks =
                  ap_end_ticks +
                  loop_ticks *  (double) j;

              /* adjust for clip start */
              tmp_start_ticks -= clip_start_ticks;
              tmp_end_ticks -= clip_start_ticks;

              /* note: these are local to the
               * region */
              x_start =
                tmp_start_ticks / ticks_in_region;
              x_end =
                tmp_end_ticks / ticks_in_region;

              /* get ratio (0.0 - 1.0) on y where
               * ap is
               * note: these are local to the region */
              y_start =
                1.0 - (double) ap->normalized_val;
              if (next_ap)
                {
                  y_end =
                    1.0 -
                    (double) next_ap->normalized_val;
                }
              else
                {
                  y_end = y_start;
                }

              double x_start_real =
                x_start * full_width;
              /*double x_end_real =*/
                /*x_end * width;*/
              double y_start_real =
                y_start * full_height;
              double y_end_real =
                y_end * full_height;

              /* draw ap */
              if (x_start_real > 0.0 &&
                  x_start_real < full_width)
                {
                  int padding = 1;
                  gtk_snapshot_append_color (
                    snapshot, &color,
                    &GRAPHENE_RECT_INIT (
                      (float)
                      (x_start_real - padding),
                      (float)
                      y_start_real - padding,
                      2 * padding,
                      2 * padding));
                }

              /* draw curve */
              if (next_ap)
                {
                  double ac_width =
                    fabs (x_end - x_start);
                  ac_width *= full_width;

                  GskRenderNode * cr_node = NULL;
                  cairo_t * cr = NULL;
                  GdkRectangle ap_draw_rect;
                  if (use_cairo)
                    {
                      ap_draw_rect =
                        Z_GDK_RECTANGLE_INIT (
                          (int) MAX (x_start_real, 0.0),
                          0,
                          (int)
                          (x_start_real + ac_width +
                            0.1),
                          full_height);
                      if (automation_point_settings_changed (
                            ap, &ap_draw_rect, true))
                        {
                          cr_node =
                            gsk_cairo_node_new (
                              &GRAPHENE_RECT_INIT (
                                0, 0,
                                ap_draw_rect.width,
                                ap_draw_rect.height));

                          object_free_w_func_and_null (
                            gsk_render_node_unref,
                            ap->cairo_node_tl);
                        }
                      else
                        {
                          cr_node =
                            ap->cairo_node_tl;
                          gtk_snapshot_save (
                            snapshot);
                          gtk_snapshot_translate (
                            snapshot,
                            &GRAPHENE_POINT_INIT (
                              ap_draw_rect.x,
                              ap_draw_rect.y));
                          gtk_snapshot_append_node (
                            snapshot, cr_node);
                          gtk_snapshot_restore (
                            snapshot);
                          continue;
                        }

                      cr =
                        gsk_cairo_node_get_draw_context (
                          cr_node);
                      cairo_save (cr);
                      cairo_translate (
                        cr,
                        - (ap_draw_rect.x),
                        - (ap_draw_rect.y));
                      gdk_cairo_set_source_rgba (
                        cr, &color);
                      cairo_set_line_width (cr, 2.0);
                    }

                  double new_x, ap_y, new_y;
                  double ac_height =
                    fabs (y_end - y_start);
                  ac_height *= full_height;
                  for (double k =
                         MAX (x_start_real, 0.0);
                       k < (x_start_real) +
                         ac_width + 0.1;
                       k += 0.1)
                    {
                      if (math_doubles_equal (ac_width, 0.0))
                        {
                          ap_y = 0.5;
                        }
                      else
                        {
                          ap_y =
                            /* in pixels, higher values
                             * are lower */
                            1.0 -
                            automation_point_get_normalized_value_in_curve (
                              ap,
                              CLAMP (
                                (k - x_start_real) /
                                  ac_width,
                                0.0, 1.0));
                        }
                      ap_y *= ac_height;

                      new_x = k;
                      if (y_start > y_end)
                        new_y = ap_y + y_end_real;
                      else
                        new_y = ap_y + y_start_real;

                      if (new_x >= full_width)
                        break;

                      if (math_doubles_equal (
                            k, 0.0))
                        {
                          if (use_cairo)
                            {
                              cairo_move_to (
                                cr, new_x, new_y);
                            }
                          else
                            {
                              gtk_snapshot_append_color (
                                snapshot, &color,
                                &GRAPHENE_RECT_INIT (
                                  (float) new_x,
                                  (float) new_y,
                                  1, 1));
                            }
                        }

                      if (use_cairo)
                        {
                          cairo_line_to (
                            cr, new_x, new_y);
                        }
                      else
                        {
                          gtk_snapshot_append_color (
                            snapshot, &color,
                            &GRAPHENE_RECT_INIT (
                              (float) new_x,
                              (float) new_y,
                              1, 1));
                        }
                    }

                  if (use_cairo)
                    {
                      cairo_stroke (cr);
                      cairo_destroy (cr);

                      gtk_snapshot_save (snapshot);
                      gtk_snapshot_translate (
                        snapshot,
                        &GRAPHENE_POINT_INIT (
                          ap_draw_rect.x - 1,
                          ap_draw_rect.y - 1));
                      gtk_snapshot_append_node (
                        snapshot, cr_node);
                      gtk_snapshot_restore (snapshot);

                      ap->last_settings_tl.fvalue =
                        ap->fvalue;
                      ap->last_settings_tl.curve_opts =
                        ap->curve_opts;
                      ap->last_settings_tl.draw_rect =
                        ap_draw_rect;
                      ap->cairo_node_tl = cr_node;
                    }

                } /* endif have next ap */

            } /* end foreach loop */
        }
    }
}

/**
 * @param rect Arranger rectangle.
 * @param full_rect Arranger object rect.
 */
static void
draw_fade_part (
  ZRegion *     self,
  GtkSnapshot * snapshot,
  int           vis_offset_x,
  int           vis_width,
  int           full_width,
  int           height)
{
  ArrangerObject * obj =
    (ArrangerObject *) self;
  Track * track = arranger_object_get_track (obj);
  g_return_if_fail (track);

  /* set color */
  GdkRGBA color = { 0.2f, 0.2f, 0.2f, 0.6f };

  /* get fade in px */
  int fade_in_px =
    ui_pos_to_px_timeline (&obj->fade_in_pos, 0);

  /* get start px */
  int start_px = 0;

  /* get visible positions (where to draw) */
  int vis_fade_in_px =
    MIN (fade_in_px, vis_offset_x + vis_width);
  int vis_start_px =
    MAX (start_px, vis_offset_x);

  UiDetail detail = ui_get_detail_level ();

  /* use cairo if normal detail or higher */
  bool use_cairo = false;
  if (detail < UI_DETAIL_LOW)
    {
      use_cairo = true;
    }

  const int step = 1;
  if (fade_in_px - start_px != 0)
    {
      double local_px_diff =
        (double) (fade_in_px - start_px);

      /* create cairo node if necessary */
      cairo_t * cr = NULL;
      if (use_cairo)
        {
          cr =
            gtk_snapshot_append_cairo (
              snapshot,
              &GRAPHENE_RECT_INIT (
                vis_start_px, 0,
                vis_fade_in_px - vis_start_px,
                height));
          cairo_set_source_rgba (
            cr, 0.2, 0.2, 0.2, 0.6);
          /*cairo_set_line_width (cr, 3);*/
        }
      for (int i = vis_start_px;
           i <= vis_fade_in_px; i += step)
        {
          /* revert because cairo draws the other
           * way */
          double val =
            1.0 -
            fade_get_y_normalized (
              (double) (i - start_px) /
                local_px_diff,
              &obj->fade_in_opts, 1);

          float draw_y_val =
            (float) (val * height);

          if (use_cairo)
            {
              double next_val =
                1.0 -
                fade_get_y_normalized (
                  (double)
                    MIN (
                      ((i + step) - start_px),
                      local_px_diff) /
                    local_px_diff,
                  &obj->fade_in_opts, 1);

              /* if start, move only */
              if (i == vis_start_px)
                {
                  cairo_move_to (
                    cr, i, val * height);
                }

              cairo_rel_line_to (
                cr, 1,
                (next_val - val) * height);

              /* if end, draw */
              if (i == vis_fade_in_px)
                {
                  /* paint a gradient in the faded out
                   * part */
                  cairo_rel_line_to (
                    cr, 0, next_val - height);
                  cairo_rel_line_to (
                    cr, - i, 0);
                  cairo_close_path (cr);
                  cairo_fill (cr);
                }
            }
          else
            {
              gtk_snapshot_append_color (
                snapshot, &color,
                &GRAPHENE_RECT_INIT (
                  (float) i, 0,
                  (float) step, draw_y_val));
            }
        }

      if (use_cairo)
        cairo_destroy (cr);

    } /* endif fade in visible */

  /* ---- fade out ---- */

  /* get fade out px */
  int fade_out_px =
    ui_pos_to_px_timeline (&obj->fade_out_pos, 0);

  /* get end px */
  int end_px = full_width;

  /* get visible positions (where to draw) */
  int visible_fade_out_px =
    MAX (fade_out_px, vis_offset_x);
  int visible_end_px =
    MIN (end_px, vis_offset_x + vis_width);

  if (end_px - fade_out_px != 0)
    {
      double local_px_diff =
        (double) (end_px - fade_out_px);

      /* create cairo node if necessary */
      cairo_t * cr = NULL;
      if (use_cairo)
        {
          cr =
            gtk_snapshot_append_cairo (
              snapshot,
              &GRAPHENE_RECT_INIT (
                visible_fade_out_px, 0,
                visible_end_px -
                  visible_fade_out_px,
                height));
          cairo_set_source_rgba (
            cr, 0.2, 0.2, 0.2, 0.6);
          /*cairo_set_line_width (cr, 3);*/
        }

      for (int i = visible_fade_out_px;
           i <= visible_end_px; i++)
        {
          /* revert because cairo draws the other
           * way */
          double val =
            1.0 -
            fade_get_y_normalized (
              (double) (i - fade_out_px) /
                local_px_diff,
              &obj->fade_out_opts, 0);

          if (use_cairo)
            {
              double tmp =
                (double) ((i + 1) - fade_out_px);
              double next_val =
                1.0 -
                fade_get_y_normalized (
                  tmp > local_px_diff ?
                    1.0 : tmp / local_px_diff,
                  &obj->fade_out_opts, 0);

              /* if start, move only */
              if (i == visible_fade_out_px)
                {
                  cairo_move_to (
                    cr, i, val * height);
                }

              cairo_rel_line_to (
                cr, 1,
                (next_val - val) * height);

              /* if end, draw */
              if (i == visible_end_px)
                {
                  /* paint a gradient in the faded
                   * out part */
                  cairo_rel_line_to (
                    cr, 0, - height);
                  cairo_rel_line_to (
                    cr, - i, 0);
                  cairo_close_path (cr);
                  cairo_fill (cr);
                }
            }
          else
            {
              float draw_y_val =
                (float) (val * height);
              gtk_snapshot_append_color (
                snapshot, &color,
                &GRAPHENE_RECT_INIT (
                  (float) i, 0,
                  1, draw_y_val));
            }
        }

      if (use_cairo)
        cairo_destroy (cr);

    } /* endif fade out visible */
}

static void
draw_audio_part (
  ZRegion * self,
  GtkSnapshot * snapshot,
  GdkRectangle * full_rect,
  int       vis_offset_x,
  int       vis_width,
  int       full_height)
{
  g_return_if_fail (vis_width < 40000);

  AudioClip * clip = audio_region_get_clip (self);

  ArrangerObject * obj = (ArrangerObject *) self;

  double frames_per_tick =
    (double) AUDIO_ENGINE->frames_per_tick;
  if (region_get_musical_mode (self))
    {
      frames_per_tick *=
        (double) tempo_track_get_current_bpm (
           P_TEMPO_TRACK) /
        (double) clip->bpm;
    }
  double multiplier =
    frames_per_tick / MW_RULER->px_per_tick;

  UiDetail detail = ui_get_detail_level ();

  double increment = 1;
  double width = 1;
  switch (detail)
    {
    case UI_DETAIL_HIGH:
      /* snapshot does not work with midpoints */
      /*increment = 0.5;*/
      increment = 1;
      width = 1;
      break;
    case UI_DETAIL_NORMAL:
      increment = 1;
      width = 1;
      break;
    case UI_DETAIL_LOW:
      increment = 2;
      width = 2;
      break;
    case UI_DETAIL_ULTRA_LOW:
      increment = 4;
      width = 4;
      break;
    }

#if 0
  g_message ("current detail: %s",
    ui_detail_str[detail]);
#endif

  signed_frame_t loop_end_frames =
    math_round_double_to_signed_frame_t (
      obj->loop_end_pos.ticks *
      frames_per_tick);
  signed_frame_t loop_frames =
    math_round_double_to_signed_frame_t (
      arranger_object_get_loop_length_in_ticks (
        obj) *
      frames_per_tick);
  signed_frame_t clip_start_frames =
    math_round_double_to_signed_frame_t (
      obj->clip_start_pos.ticks *
      frames_per_tick);

  double local_start_x = (double) vis_offset_x;
  double local_end_x =
    local_start_x + (double) vis_width;
  signed_frame_t prev_frames =
    (signed_frame_t) (multiplier * local_start_x);
  signed_frame_t curr_frames = prev_frames;
  /*g_message ("cur frames %ld", curr_frames);*/
  /*Position tmp;*/
  /*position_from_frames (&tmp, curr_frames);*/
  /*position_print (&tmp);*/


  GdkRGBA color = object_fill_color;
  graphene_rect_t grect =
    GRAPHENE_RECT_INIT (0, 0, (float) width, 0);
  for (double i = local_start_x;
       i < (double) local_end_x; i += increment)
    {
      curr_frames = (signed_frame_t) (multiplier * i);
      /* current single channel frames */
      curr_frames += clip_start_frames;
      while (curr_frames >= loop_end_frames)
        {
          curr_frames -= loop_frames;
          if (loop_frames == 0)
            break;
        }
      float min = 0.f, max = 0.f;
      for (signed_frame_t j = prev_frames;
           j < curr_frames; j++)
        {
          for (unsigned int k = 0;
               k < clip->channels; k++)
            {
              signed_frame_t index =
                j * (signed_frame_t) clip->channels +
                (signed_frame_t) k;

              /* if outside bounds */
              if (
                index < 0 ||
                index >=
                (signed_frame_t)
                  clip->num_frames *
                  (signed_frame_t) clip->channels)
                {
                  /* skip */
                  continue;
                }
              float val = clip->frames[index];
              if (val > max)
                {
                  max = val;
                }
              if (val < min)
                {
                  min = val;
                }
            }
        }

      /* normalize */
      min = (min + 1.f) / 2.f;
      max = (max + 1.f) / 2.f;

      /* local from the full rect y */
      double local_min_y =
        MAX (
          (double) min *
            (double) full_height, 0.0);
      /* local from the full rect y */
      double local_max_y =
        MIN (
          (double) max *
            (double) full_height,
          (double) full_height);

      /* only draw if non-silent */
      if (fabs (local_min_y - local_max_y) > 0.01)
        {
          grect.origin.x = (float) i;
          grect.origin.y = (float) local_min_y;
          grect.size.height =
            (float) (local_max_y - local_min_y);
          gtk_snapshot_append_color (
            snapshot, &color, &grect);
        }

      prev_frames = curr_frames;
    }
}

/**
 * Draw audio.
 *
 * At this point, cr is translated to start at 0,0
 * in the full rect.
 *
 * @param rect Arranger rectangle.
 * @param cache_applied_rect The rectangle where
 *   the previous cache was pasted at, where
 *   0,0 is the region's top left corner. This
 *   is so that the cached part is not re-drawn.
 *   Only x and width are useful.
 * @param cache_applied_offset_x The offset from
 *   the region's top-left corner at
 *   which the cache was applied (can be negative).
 * @param cache_applied_width The width of the
 *   cache, starting from cache_applied_offset_x
 *   (even if negative).
 */
static void
draw_audio_region (
  ZRegion *      self,
  GtkSnapshot *  snapshot,
  GdkRectangle * full_rect,
  GdkRectangle * draw_rect,
  bool           cache_applied,
  int            cache_applied_offset_x,
  int            cache_applied_width)
{
  g_return_if_fail (cache_applied_width < 40000);

  /*g_message ("drawing audio region");*/
  if (self->stretching)
    {
      return;
    }

  int full_height = full_rect->height;

  int vis_offset_x =
    draw_rect->x - full_rect->x;
  int vis_width = draw_rect->width;
  draw_audio_part (
    self, snapshot,
    full_rect, vis_offset_x,
    vis_width, full_height);
  draw_fade_part (
    self, snapshot, vis_offset_x,
    vis_width, full_rect->width, full_height);
}

/**
 * @param rect Arranger rectangle.
 */
static void
draw_name (
  ZRegion *      self,
  GtkSnapshot *  snapshot,
  GdkRectangle * full_rect,
  GdkRectangle * draw_rect)
{
  g_return_if_fail (
    self && self->escaped_name);

  int full_width = full_rect->width;

  /* draw dark bg behind text */
  recreate_pango_layouts (
    self,
    MIN (full_width, 800));
  PangoLayout * layout = self->layout;
  if (DEBUGGING)
    {
      char str[200];
      strcpy (str, self->escaped_name);
      ZRegion * clip_editor_region =
        clip_editor_get_region (CLIP_EDITOR);
      if (clip_editor_region == self)
        {
          strcat (str, " (CLIP EDITOR)");
        }
      pango_layout_set_text (
        layout, str, -1);
    }
  else
    {
      pango_layout_set_text (
        layout, self->escaped_name, -1);
    }
  PangoRectangle pangorect;

  /* get extents */
  pango_layout_get_pixel_extents (
    layout, NULL, &pangorect);
  int black_box_height =
    pangorect.height + 2 * REGION_NAME_BOX_PADDING;

  /* create a rounded clip */
  /*float degrees = G_PI / 180.f;*/
  float radius = REGION_NAME_BOX_CURVINESS / 1.f;
  GskRoundedRect rounded_rect;
  gsk_rounded_rect_init (
    &rounded_rect,
    &GRAPHENE_RECT_INIT (
      0, 0,
      (pangorect.width + REGION_NAME_PADDING_R),
      black_box_height),
    &GRAPHENE_SIZE_INIT (0, 0),
    &GRAPHENE_SIZE_INIT (0, 0),
    &GRAPHENE_SIZE_INIT (radius, radius),
    &GRAPHENE_SIZE_INIT (0, 0));
  gtk_snapshot_push_rounded_clip (
    snapshot, &rounded_rect);

  /* fill bg color */
  GdkRGBA name_bg_color;
  gdk_rgba_parse (&name_bg_color, "#323232");
  name_bg_color.alpha = 0.8f;
  gtk_snapshot_append_color (
    snapshot, &name_bg_color,
    &rounded_rect.bounds);

  /* draw text */
  gtk_snapshot_save (snapshot);
  gtk_snapshot_translate (
    snapshot,
    &GRAPHENE_POINT_INIT (
      REGION_NAME_BOX_PADDING,
      REGION_NAME_BOX_PADDING));
  gtk_snapshot_append_layout (
    snapshot, layout,
    &Z_GDK_RGBA_INIT (1, 1, 1, 1));
  gtk_snapshot_restore (snapshot);

  /* pop rounded clip */
  gtk_snapshot_pop (snapshot);

  /* save rect */
  ArrangerObject * obj = (ArrangerObject *) self;
  obj->last_name_rect.x = 0;
  obj->last_name_rect.y = 0;
  obj->last_name_rect.width =
    pangorect.width + REGION_NAME_PADDING_R;
  obj->last_name_rect.height =
    pangorect.height + REGION_NAME_PADDING_R;
}

/**
 * @param rect Arranger rectangle.
 */
static void
draw_bottom_right_part (
  ZRegion *      self,
  GtkSnapshot *  snapshot,
  GdkRectangle * full_rect,
  GdkRectangle * draw_rect)
{
  /* if audio region, draw BPM */
  if (self->id.type == REGION_TYPE_AUDIO)
    {
#if 0
      cairo_save (cr);
      AudioClip * clip =
        audio_region_get_clip (self);
      char txt[50];
      sprintf (txt, "%.1f BPM", clip->bpm);
      PangoLayout * layout = self->layout;
      pango_layout_set_markup (
        layout, txt, -1);
      cairo_set_source_rgba (cr, 1, 1, 1, 1);
      PangoRectangle pangorect;
      pango_layout_get_pixel_extents (
        layout, NULL, &pangorect);
      int padding = 2;
      cairo_move_to (
        cr,
        full_rect->width -
          (pangorect.width + padding),
        full_rect->height -
          (pangorect.height + padding));
      pango_cairo_show_layout (cr, layout);
      cairo_restore (cr);
#endif
    }
}

/**
 * Draws the ZRegion in the given cairo context in
 * relative coordinates.
 *
 * @param rect Arranger rectangle.
 */
void
region_draw (
  ZRegion *      self,
  GtkSnapshot *  snapshot,
  GdkRectangle * rect)
{
  ArrangerObject * obj = (ArrangerObject *) self;

  ArrangerWidget * arranger =
    arranger_object_get_arranger (obj);

  Track * track =
    arranger_object_get_track (obj);

  /* set color */
  GdkRGBA color;
  if (track)
    color = track->color;
  else
    {
      color.red = 1;
      color.green = 0;
      color.blue = 0;
      color.alpha = 1;
    }
  ui_get_arranger_object_color (
    &color,
    arranger->hovered_object == obj,
    region_is_selected (self),
    /* FIXME */
    false,
    arranger_object_get_muted (obj) ||
      track->frozen);

  GdkRectangle draw_rect;
  GdkRectangle last_draw_rect, last_full_rect;
  GdkRectangle full_rect = obj->full_rect;
  for (int i = REGION_COUNTERPART_MAIN;
       i <= REGION_COUNTERPART_LANE; i++)
    {
      if (i == REGION_COUNTERPART_LANE)
        {
          if (!region_type_has_lane (self->id.type))
            break;

          if (!track->lanes_visible)
            break;

          TrackLane * lane =
            track->lanes[self->id.lane_pos];
          g_return_if_fail (lane);

          /* set full rectangle */
          region_get_lane_full_rect (
            self, &full_rect);
        }
      else if (i == REGION_COUNTERPART_MAIN)
        {
          /* set full rectangle */
          full_rect = obj->full_rect;
        }

      /* if full rect of current region (main or
       * lane) is not visible, skip */
      if (!ui_rectangle_overlap (
             &full_rect, rect))
        {
          continue;
        }

      /* get draw (visible only) rectangle */
      arranger_object_get_draw_rectangle (
        obj, rect, &full_rect, &draw_rect);

      /* get last rects */
      get_last_rects (
        self, i, &last_full_rect, &last_draw_rect);

      /* skip if draw rect has 0 width */
      if (draw_rect.width == 0)
        {
          continue;
        }

      g_return_if_fail (
        draw_rect.width > 0 &&
        draw_rect.width < 40000);

      /* make a rounded clip for the whole region */
      GskRoundedRect rounded_rect;
      graphene_rect_t graphene_rect =
        GRAPHENE_RECT_INIT (
          full_rect.x, full_rect.y, full_rect.width,
          full_rect.height);
      gsk_rounded_rect_init_from_rect (
        &rounded_rect, &graphene_rect, 6.f);
      gtk_snapshot_push_rounded_clip (
        snapshot, &rounded_rect);

      /* draw background */
      gtk_snapshot_append_color (
        snapshot, &color, &graphene_rect);

      /* TODO apply stretch ratio */
      if (MW_TIMELINE->action ==
            UI_OVERLAY_ACTION_STRETCHING_R)
        {
#if 0
          cairo_scale (
            cr, self->stretch_ratio, 1);
#endif
        }

      /* translate to the full rect */
      gtk_snapshot_save (snapshot);
      gtk_snapshot_translate (
        snapshot,
        &GRAPHENE_POINT_INIT (
          full_rect.x, full_rect.y));

      /* draw any remaining parts */
      switch (self->id.type)
        {
        case REGION_TYPE_MIDI:
          draw_midi_region (
            self, snapshot, &full_rect,
            &draw_rect);
          break;
        case REGION_TYPE_AUTOMATION:
          draw_automation_region (
            self, snapshot, &full_rect,
            &draw_rect);
          break;
        case REGION_TYPE_CHORD:
          draw_chord_region (
            self, snapshot, &full_rect,
            &draw_rect);
          break;
        case REGION_TYPE_AUDIO:
          draw_audio_region (
            self, snapshot, &full_rect,
            &draw_rect, false,
            (int)
            draw_rect.x - last_full_rect.x,
            (int)
            draw_rect.width);
          break;
        default:
          break;
        }

      /* ---- draw applicable icons ---- */

#define DRAW_TEXTURE(name) \
  gtk_snapshot_append_texture ( \
    snapshot, arranger->name, \
    &GRAPHENE_RECT_INIT ( \
      full_rect.width - \
        (size + paddingh) * (icons_drawn + 1),  \
    paddingv, size, size)); \
  icons_drawn++

      const int size =
        arranger->region_icon_texture_size;
      const int paddingh = 2;
      const int paddingv = 0;
      int icons_drawn = 0;

      /* draw link icon if has linked parent */
      if (self->id.link_group >= 0)
        {
          DRAW_TEXTURE (symbolic_link_texture);
        }

      /* draw musical mode icon if region is in
       * musical mode */
      if (self->id.type == REGION_TYPE_AUDIO &&
          region_get_musical_mode (self))
        {
          DRAW_TEXTURE (music_note_16th_texture);
        }

      /* if track is frozen, show frozen icon */
      if (track->frozen)
        {
          DRAW_TEXTURE (
            fork_awesome_snowflake_texture);
        }

      /* draw loop icon if region is looped */
      if (region_is_looped (self))
        {
          DRAW_TEXTURE (
            media_playlist_repeat_texture);
        }

#undef DRAW_TEXTURE

      /* remember rects */
      if (i == REGION_COUNTERPART_MAIN)
        {
          self->last_main_full_rect = full_rect;
          self->last_main_draw_rect = draw_rect;
        }
      else
        {
          self->last_lane_full_rect = full_rect;
          self->last_lane_draw_rect = draw_rect;
        }

      self->last_cache_time =
        g_get_monotonic_time ();

      draw_loop_points (
        self, snapshot, &full_rect,
        &draw_rect);
      /* TODO draw cut line */
      draw_name (
        self, snapshot, &full_rect,
        &draw_rect);

      /* draw anything on the bottom right part
       * (eg, BPM) */
      draw_bottom_right_part (
        self, snapshot, &full_rect,
        &draw_rect);

      /* restore translation */
      gtk_snapshot_restore (snapshot);

      /* draw border */
      const float border_width = 1.f;
      GdkRGBA border_color = {
        0.5f, 0.5f, 0.5f, 0.4f };
      float border_widths[] = {
        border_width, border_width, border_width,
        border_width };
      GdkRGBA border_colors[] = {
        border_color, border_color, border_color,
        border_color };
      gtk_snapshot_append_border (
        snapshot, &rounded_rect, border_widths,
        border_colors);

      /* pop rounded clip */
      gtk_snapshot_pop (snapshot);
    }

  self->last_positions_obj = *obj;
  obj->use_cache = true;
}

/**
 * Returns the lane rectangle for the region.
 */
void
region_get_lane_full_rect (
  ZRegion *       self,
  GdkRectangle * rect)
{
  ArrangerObject * obj = (ArrangerObject *) self;

  Track * track =
    arranger_object_get_track (obj);
  g_return_if_fail (track && track->lanes_visible);

  TrackLane * lane =
    track->lanes[self->id.lane_pos];
  g_return_if_fail (lane);

  *rect = obj->full_rect;
  rect->y += lane->y;
  rect->height = (int) lane->height - 1;
}
