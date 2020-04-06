/*
 * Copyright (C) 2018-2020 Alexandros Theodotou <alex at zrythm dot org>
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

#include <math.h>

#include "audio/audio_bus_track.h"
#include "audio/automation_region.h"
#include "audio/channel.h"
#include "audio/fade.h"
#include "audio/instrument_track.h"
#include "audio/track.h"
#include "config.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/arranger_object.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/region.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_panel.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/cairo.h"
#include "utils/flags.h"
#include "utils/math.h"
#include "utils/ui.h"

#include <glib/gi18n-lib.h>

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

  if (!PANGO_IS_LAYOUT (self->layout))
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
 * @param rect Arranger rectangle.
 * @param full_rect Object full rectangle.
 */
static void
draw_background (
  ZRegion *       self,
  cairo_t *      cr,
  GdkRectangle * rect,
  GdkRectangle * full_rect,
  GdkRectangle * draw_rect,
  RegionCounterpart counterpart)
{
  ArrangerObject * obj =
    (ArrangerObject *) self;

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
    arranger_object_get_muted (obj));
  gdk_cairo_set_source_rgba (
    cr, &color);

  /* if there are still region parts outside the
   * rect, add some padding so that the region
   * doesn't curve when it's not its edge */
  int draw_x = draw_rect->x - rect->x;
  int draw_x_has_padding = 0;
  if (draw_rect->x > full_rect->x)
    {
      draw_x -=
        MIN (draw_rect->x - full_rect->x, 4);
      draw_x_has_padding = 1;
    }
  int draw_width = draw_rect->width;
  if (draw_rect->x + draw_rect->width <
      full_rect->x + full_rect->width)
    {
      draw_width +=
        MAX (
          (draw_rect->x + draw_rect->width) -
            (full_rect->x + full_rect->width), 4);
    }
  if (draw_x_has_padding)
    {
      draw_width += 4;
    }

  z_cairo_rounded_rectangle (
    cr, draw_x, full_rect->y - rect->y,
    draw_width, full_rect->height,
    1.0, 4.0);

  /* clip this path so all drawing afterwards will
   * be confined inside it. preserve so we can
   * fill after this call. each region is drawn
   * wrapped inside cairo_save() and
   * cairo_restore() to remove the clip when done */
  cairo_clip_preserve (cr);

  cairo_fill (cr);

  /* draw link icon if has linked parent */
  if (self->has_link)
    {
      const int size = 16;
      const int paddingh = 2;
      const int paddingv = 0;

      cairo_surface_t * surface =
        z_cairo_get_surface_from_icon_name (
          "z-emblem-symbolic-link", size, 1);

      /* add main icon */
      double end_region_global =
        full_rect->x + full_rect->width;
      cairo_set_source_surface (
        cr, surface,
        (end_region_global - rect->x) -
          (size + paddingh),
        (full_rect->y - rect->y) + paddingv);
      cairo_paint (cr);
    }
}

/**
 * @param rect Arranger rectangle.
 * @param full_rect Object full rectangle.
 */
static void
draw_loop_points (
  ZRegion *       self,
  cairo_t *      cr,
  GdkRectangle * rect,
  GdkRectangle * full_rect,
  GdkRectangle * draw_rect,
  RegionCounterpart counterpart)
{
  double dashes[] = { 5 };
  cairo_set_dash (
    cr, dashes, 1, 0);
  cairo_set_line_width (cr, 1);

  ArrangerObject * obj =
    (ArrangerObject *) self;
  Position tmp;
  double loop_start_ticks =
    obj->loop_start_pos.total_ticks;
  double loop_end_ticks =
    obj->loop_end_pos.total_ticks;
  g_warn_if_fail (
    loop_end_ticks > loop_start_ticks);
  double loop_ticks =
    arranger_object_get_loop_length_in_ticks (
      obj);
  double clip_start_ticks =
    obj->clip_start_pos.total_ticks;

  /* get x px for loop point */
  position_from_ticks (
    &tmp, loop_start_ticks - clip_start_ticks);
  int x_px =
    ui_pos_to_px_timeline (&tmp, 0);

  /* convert x_px to global */
  x_px += full_rect->x;

  if (x_px != full_rect->x &&
      /* if px is inside region */
      x_px >= full_rect->x &&
      x_px < full_rect->x + full_rect->width &&
      /* if loop px is visible */
      x_px >= rect->x &&
      x_px < rect->x + rect->width)
    {
      gdk_cairo_set_source_rgba (
        cr, &UI_COLORS->bright_green);
      cairo_move_to (
        cr, x_px - rect->x,
        draw_rect->y - rect->y);
      cairo_line_to (
        cr, x_px - rect->x,
        (draw_rect->y + draw_rect->height) -
        rect->y);
      cairo_stroke (cr);
    }

  int num_loops =
    arranger_object_get_num_loops (obj, 1);
  for (int i = 0; i < num_loops; i++)
    {
      position_from_ticks (
        &tmp, loop_end_ticks + loop_ticks * i);

      /* adjust for clip_start */
      position_add_ticks (
        &tmp, - clip_start_ticks);

      /* note: this is relative to the region */
      x_px = ui_pos_to_px_timeline (&tmp, 0);

      /* make px global */
      x_px += full_rect->x;

      if (
          /* if px is vixible */
          x_px >= rect->x &&
          x_px <= rect->x + rect->width &&
          /* if px is inside region */
          x_px >= full_rect->x &&
          x_px < full_rect->x + full_rect->width)
        {
          cairo_set_source_rgba (
            cr, 0, 0, 0, 1.0);
          cairo_move_to (
            cr, x_px - rect->x,
            draw_rect->y - rect->y);
          cairo_line_to (
            cr, x_px - rect->x,
            (draw_rect->y + draw_rect->height) -
              rect->y);
          cairo_stroke (cr);
        }
    }

  cairo_set_dash (
    cr, NULL, 0, 0);
}

/**
 * @param rect Arranger rectangle.
 */
static void
draw_midi_region (
  ZRegion *       self,
  cairo_t *      cr,
  GdkRectangle * rect,
  GdkRectangle * full_rect,
  GdkRectangle * draw_rect,
  RegionCounterpart counterpart)
{
  ArrangerObject * obj =
    (ArrangerObject *) self;

  cairo_set_source_rgba (
    cr, 1, 1, 1, 1);
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
    obj->loop_end_pos.total_ticks;
  double loop_ticks =
    arranger_object_get_loop_length_in_ticks (
      obj);
  double clip_start_ticks =
    obj->clip_start_pos.total_ticks;

  for (int i = 0; i < self->num_midi_notes; i++)
    {
      MidiNote * mn = self->midi_notes[i];
      ArrangerObject * mn_obj =
        (ArrangerObject *) mn;
      if (arranger_object_get_muted (mn_obj))
        continue;

      /* get ratio (0.0 - 1.0) on x where midi note
       * starts & ends */
      double mn_start_ticks =
        position_to_ticks (&mn_obj->pos);
      double mn_end_ticks =
        position_to_ticks (&mn_obj->end_pos);
      double tmp_start_ticks, tmp_end_ticks;

      /* if before loop end */
      if (position_is_before (
            &mn_obj->pos,
            &obj->loop_end_pos))
        {
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
                (double) full_rect->width;
              x_end *=
                (double) full_rect->width;
              y_start *=
                (double) full_rect->height;

              /* the above values are local to the
               * region, convert to global */
              x_start += full_rect->x;
              x_end += full_rect->x;
              y_start += full_rect->y;

              /* skip if any part of the note is
               * not visible in the region's rect */
              if ((x_start >= draw_rect->x &&
                   x_start <
                     draw_rect->x +
                     draw_rect->width) ||
                  (x_end >= draw_rect->x &&
                   x_end <
                     draw_rect->x +
                     draw_rect->width) ||
                  (x_start < draw_rect->x &&
                   x_end > draw_rect->x))
                {
                  double draw_x =
                    MAX (x_start, draw_rect->x);
                  double draw_width =
                    MIN (
                      (x_end - x_start) -
                        (draw_x - x_start),
                      (draw_rect->x +
                        draw_rect->width) - draw_x);
                  double draw_y =
                    MAX (y_start, draw_rect->y);
                  double draw_height =
                    MIN (
                      (y_note_size *
                        (double) full_rect->height) -
                          (draw_y - y_start),
                      (draw_rect->y +
                        draw_rect->height) - draw_y);
                  cairo_rectangle (
                    cr,
                    draw_x - rect->x,
                    draw_y - rect->y,
                    draw_width,
                    draw_height);
                  cairo_fill (cr);
                }
            }
        }
    }
}

/**
 * @param rect Arranger rectangle.
 */
static void
draw_chord_region (
  ZRegion *       self,
  cairo_t *      cr,
  GdkRectangle * rect,
  GdkRectangle * full_rect,
  GdkRectangle * draw_rect,
  RegionCounterpart counterpart)
{
  ArrangerObject * obj =
    (ArrangerObject *) self;

  int num_loops =
    arranger_object_get_num_loops (obj, 1);
  double ticks_in_region =
    arranger_object_get_length_in_ticks (obj);
  double x_start, x_end;

  /* draw chords notes */
  double loop_end_ticks =
    obj->loop_end_pos.total_ticks;
  double loop_ticks =
    arranger_object_get_loop_length_in_ticks (obj);
  double clip_start_ticks =
    obj->clip_start_pos.total_ticks;
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
        co_obj->pos.total_ticks;
      double co_end_ticks;
      if (i < self->num_chord_objects - 1)
        {
          next_co =
            self->chord_objects[i + 1];
          next_co_obj =
            (ArrangerObject *) next_co;
          co_end_ticks =
            next_co_obj->pos.total_ticks;
        }
      else
        co_end_ticks =
          obj->end_pos.total_ticks;
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

              cairo_set_source_rgba (
                cr, 1, 1, 1, 0.3);

              /* get actual values using the
               * ratios */
              x_start *=
                (double) obj->full_rect.width;
              x_end *=
                (double) obj->full_rect.width;

              /* the above values are local to the
               * region, convert to global */
              x_start += obj->full_rect.x;
              x_end += obj->full_rect.x;

              /* draw */
              cairo_rectangle (
                cr,
                x_start - rect->x,
                obj->full_rect.y - rect->y,
                12.0,
                obj->full_rect.height);
              cairo_fill (cr);

              cairo_set_source_rgba (
                cr, 0, 0, 0, 1);

              /* draw name */
              char descr_str[100];
              chord_descriptor_to_string (
                descr, descr_str);
              ArrangerWidget * arranger =
                arranger_object_get_arranger (obj);
              z_cairo_draw_text_full (
                cr, GTK_WIDGET (arranger),
                self->chords_layout,
                descr_str,
                (int)
                ((double) x_start *
                  (double) obj->full_rect.width + 2.0),
                2);
            }
        }
    }
}

/**
 * @param rect Arranger rectangle.
 */
static void
draw_automation_region (
  ZRegion *       self,
  cairo_t *      cr,
  GdkRectangle * rect,
  GdkRectangle * full_rect,
  GdkRectangle * draw_rect,
  RegionCounterpart counterpart)
{
  ArrangerObject * obj = (ArrangerObject *) self;
  cairo_set_source_rgba (cr, 1, 1, 1, 1);

  int num_loops =
    arranger_object_get_num_loops (
      obj, 1);
  double ticks_in_region =
    arranger_object_get_length_in_ticks (obj);
  double x_start, y_start, x_end, y_end;

  /* draw automation */
  double loop_end_ticks =
    obj->loop_end_pos.total_ticks;
  double loop_ticks =
    arranger_object_get_loop_length_in_ticks (obj);
  double clip_start_ticks =
    obj->clip_start_pos.total_ticks;
  AutomationPoint * ap, * next_ap;
  for (int i = 0; i < self->num_aps; i++)
    {
      ap = self->aps[i];
      next_ap =
        automation_region_get_next_ap (self, ap);
      ArrangerObject * ap_obj =
        (ArrangerObject *) ap;
      ArrangerObject * next_ap_obj =
        (ArrangerObject *) next_ap;

      double ap_start_ticks =
        ap_obj->pos.total_ticks;
      double ap_end_ticks = ap_start_ticks;
      if (next_ap)
        {
          ap_end_ticks =
            next_ap_obj->pos.total_ticks;
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
                x_start * full_rect->width;
              /*double x_end_real =*/
                /*x_end * width;*/
              double y_start_real =
                y_start * full_rect->height;
              double y_end_real =
                y_end * full_rect->height;

              /* draw ap */
              if (x_start_real > 0.0 &&
                  x_start_real < full_rect->width)
                {
                  int padding = 1;
                  cairo_rectangle (
                    cr,
                    (x_start_real +
                       obj->full_rect.x) -
                      (padding + rect->x),
                    (y_start_real +
                       obj->full_rect.y) -
                      (padding + rect->y),
                    2 * padding,
                    2 * padding);
                  cairo_fill (cr);
                }

              /* draw curve */
              if (next_ap)
                {
                  double new_x, ap_y, new_y;
                  double ac_height =
                    fabs (y_end - y_start);
                  ac_height *= full_rect->height;
                  double ac_width =
                    fabs (x_end - x_start);
                  ac_width *= full_rect->width;
                    cairo_set_line_width (
                      cr, 2.0);
                  for (double k =
                         MAX (x_start_real, 0.0);
                       k < (x_start_real) +
                         ac_width + 0.1;
                       k += 0.1)
                    {
                      /* in pixels, higher values are lower */
                      ap_y =
                        1.0 -
                        automation_point_get_normalized_value_in_curve (
                          ap,
                          CLAMP (
                            (k - x_start_real) /
                              ac_width,
                            0.0, 1.0));
                      ap_y *= ac_height;

                      new_x = k;
                      if (y_start > y_end)
                        new_y = ap_y + y_end_real;
                      else
                        new_y = ap_y + y_start_real;

                      if (new_x >=
                            obj->full_rect.width)
                        break;

                      if (math_doubles_equal (
                            k, 0.0))
                        {
                          cairo_move_to (
                            cr,
                            (new_x +
                              obj->full_rect.x) -
                              rect->x,
                            (new_y +
                              obj->full_rect.y) -
                              rect->y);
                        }

                      cairo_line_to (
                        cr,
                        (new_x +
                          obj->full_rect.x) -
                          rect->x,
                        (new_y +
                          obj->full_rect.y) -
                          rect->y);
                    }
                  cairo_stroke (cr);
                }
            }
        }
    }
}

/**
 * @param rect Arranger rectangle.
 * @param full_rect Arranger object rect.
 */
static void
draw_fades (
  ZRegion *      self,
  cairo_t *      cr,
  GdkRectangle * rect,
  GdkRectangle * full_rect,
  GdkRectangle * draw_rect,
  RegionCounterpart counterpart)
{
  ArrangerObject * obj =
    (ArrangerObject *) self;
  Track * track = arranger_object_get_track (obj);
  g_return_if_fail (track);

  /* set color */
  cairo_set_source_rgba (
    cr, 0.2, 0.2, 0.2, 0.6);
  cairo_set_line_width (cr, 3);

  /* get fade in px as global */
  int fade_in_px =
    full_rect->x +
    ui_pos_to_px_timeline (&obj->fade_in_pos, 0);

  /* get start px as global */
  int start_px = full_rect->x;

  /* get visible positions (where to draw) */
  int visible_fade_in_px =
    MIN (fade_in_px, rect->x + rect->width);
  int visible_start_px =
    MAX (start_px, rect->x);

  for (int i = visible_start_px;
       i <= visible_fade_in_px; i++)
    {
      /* revert because cairo draws the other way */
      double val =
        1.0 -
        fade_get_y_normalized (
          (double) (i - start_px) /
            (double) (fade_in_px - start_px),
          &obj->fade_in_opts, 1);
      double next_val =
        1.0 -
        fade_get_y_normalized (
          (double) ((i + 1) - start_px) /
            (double) (fade_in_px - start_px),
          &obj->fade_in_opts, 1);

      /* if start */
      if (i == visible_start_px)
        {
          cairo_move_to (
            cr, i - rect->x,
            (full_rect->y +
             val * full_rect->height) -
              rect->y);
        }

      cairo_rel_line_to (
        cr, 1,
        (next_val - val) * full_rect->height);

      /* if end */
      if (i == visible_fade_in_px)
        {
          /* paint a gradient in the faded out
           * part */
          cairo_rel_line_to (
            cr, 0, next_val - full_rect->height);
          cairo_rel_line_to (
            cr, - i, 0);
          cairo_close_path (cr);
          cairo_fill (cr);
        }
    }

  /* ---- fade out ---- */

  /* get fade out px as global */
  int fade_out_px =
    full_rect->x +
    ui_pos_to_px_timeline (&obj->fade_out_pos, 0);

  /* get end px as global */
  int end_px = full_rect->x + full_rect->width;

  /* get visible positions (where to draw) */
  int visible_fade_out_px =
    MAX (fade_out_px, rect->x);
  int visible_end_px =
    MIN (end_px, rect->x + rect->width);

  for (int i = visible_fade_out_px;
       i <= visible_end_px; i++)
    {
      /* revert because cairo draws the other way */
      double val =
        1.0 -
        fade_get_y_normalized (
          (double) (i - fade_out_px) /
            (double) (end_px - fade_out_px),
          &obj->fade_out_opts, 0);
      double next_val =
        1.0 -
        fade_get_y_normalized (
          (double) ((i + 1) - fade_out_px) /
            (double) (end_px - fade_out_px),
          &obj->fade_out_opts, 0);

      /* if start, move only */
      if (i == visible_fade_out_px)
        {
          cairo_move_to (
            cr, i - rect->x,
            (full_rect->y +
             val * full_rect->height) -
              rect->y);
        }

      cairo_rel_line_to (
        cr, 1,
        (next_val - val) * full_rect->height);

      /* if end, draw */
      if (i == visible_end_px)
        {
          /* paint a gradient in the faded out
           * part */
          cairo_rel_line_to (
            cr, 0, - full_rect->height);
          cairo_rel_line_to (
            cr, - i, 0);
          cairo_close_path (cr);
          cairo_fill (cr);
        }
    }
}

/**
 * @param rect Arranger rectangle.
 */
static void
draw_audio_region (
  ZRegion *       self,
  cairo_t *      cr,
  GdkRectangle * rect,
  GdkRectangle * full_rect,
  GdkRectangle * draw_rect,
  RegionCounterpart counterpart)
{
  ArrangerObject * obj = (ArrangerObject *) self;
  if (self->stretching)
    {
      arranger_object_queue_redraw (obj);
      return;
    }

  cairo_set_source_rgba (cr, 1, 1, 1, 1);

  AudioClip * clip =
    AUDIO_POOL->clips[self->pool_id];

  long loop_end_frames =
    obj->loop_end_pos.frames;
  long loop_frames =
    arranger_object_get_loop_length_in_frames (
      obj);
  long clip_start_frames =
    obj->clip_start_pos.frames;

  double local_start_x =
    (double) draw_rect->x - (double) full_rect->x;
  double local_end_x =
    local_start_x +
    (double) draw_rect->width;
  long prev_frames =
    ui_px_to_frames_timeline (local_start_x, 0);
  for (double i = local_start_x;
       i < (double) local_end_x; i += 1.0)
    {
      /* current single channel frames */
      long curr_frames =
        ui_px_to_frames_timeline (i, 0);
      curr_frames += clip_start_frames;
      while (curr_frames >= loop_end_frames)
        {
          curr_frames -= loop_frames;
        }
      float min = 0.f, max = 0.f;
      for (long j = prev_frames;
           j < curr_frames; j++)
        {
          for (unsigned int k = 0;
               k < clip->channels; k++)
            {
              long index =
                j * (long) clip->channels +
                (long) k;
              g_warn_if_fail (
                index >= 0 &&
                index <
                (long)
                  self->num_frames *
                  (long) clip->channels);
              float val =
                self->frames[index];
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
        cr,
        /* x */
        (i - 0.5 + full_rect->x) - rect->x,
        /* from y */
        (full_rect->y +
          MAX (
            (double) min *
              (double) full_rect->height, 0.0)) - rect->y,
        /* to y */
        (full_rect->y +
          MIN (
            (double) max *
              (double) full_rect->height,
            (double) full_rect->height)) - rect->y);

      prev_frames = curr_frames;
    }
}

/**
 * @param rect Arranger rectangle.
 */
static void
draw_name (
  ZRegion *          self,
  cairo_t *         cr,
  GdkRectangle *    rect,
  GdkRectangle *    full_rect,
  GdkRectangle *    draw_rect,
  RegionCounterpart counterpart)
{
  /* no need to draw if the start of the region is
   * not visible */
  if (rect->x - full_rect->x > 800)
    return;

  g_return_if_fail (
    self && self->name);

  char str[200];
  strcpy (str, self->name);

  if (DEBUGGING)
    {
      ZRegion * clip_editor_region =
        clip_editor_get_region (CLIP_EDITOR);
      if (clip_editor_region == self)
        {
          strcat (str, " (CLIP EDITOR)");
        }
    }

  /* draw dark bg behind text */
  recreate_pango_layouts (
    self,
    MIN (full_rect->width, 800));
  PangoLayout * layout = self->layout;
  pango_layout_set_text (
    layout, str, -1);
  PangoRectangle pangorect;
  /* get extents */
  pango_layout_get_pixel_extents (
    layout, NULL, &pangorect);
  GdkRGBA name_bg_color;
  gdk_rgba_parse (&name_bg_color, "#323232");
  name_bg_color.alpha = 0.8;
  gdk_cairo_set_source_rgba (
    cr, &name_bg_color);
  double radius = REGION_NAME_BOX_CURVINESS / 1.0;
  double degrees = G_PI / 180.0;
  cairo_new_sub_path (cr);
  cairo_move_to (
    cr,
    (pangorect.width + REGION_NAME_PADDING_R) +
      (full_rect->x - rect->x),
    full_rect->y - rect->y);
  int black_box_height =
    pangorect.height + 2 * REGION_NAME_BOX_PADDING;
  cairo_arc (
    cr,
    ((pangorect.width + REGION_NAME_PADDING_R) -
      radius) +
      (full_rect->x - rect->x),
    (black_box_height - radius) +
     (full_rect->y - rect->y),
    radius,
    0 * degrees, 90 * degrees);
  cairo_line_to (
    cr, full_rect->x - rect->x,
    black_box_height +
      (full_rect->y - rect->y));
  cairo_line_to (
    cr, full_rect->x - rect->x,
    full_rect->y - rect->y);
  cairo_close_path (cr);
  cairo_fill (cr);

  /* draw text */
  cairo_set_source_rgba (
    cr, 1, 1, 1, 1);
  cairo_move_to (
    cr,
    REGION_NAME_BOX_PADDING +
      (full_rect->x - rect->x),
    REGION_NAME_BOX_PADDING +
      (full_rect->y - rect->y));
  pango_cairo_show_layout (cr, layout);
}

/**
 * Draws the ZRegion in the given cairo context in
 * relative coordinates.
 *
 * @param cr The cairo context in the region's
 *   drawable coordinates.
 * @param rect Arranger rectangle.
 */
void
region_draw (
  ZRegion *       self,
  cairo_t *      cr,
  GdkRectangle * rect)
{
  ArrangerObject * obj = (ArrangerObject *) self;

  GdkRectangle draw_rect;
  GdkRectangle full_rect = obj->full_rect;
  for (int i = REGION_COUNTERPART_MAIN;
       i <= REGION_COUNTERPART_LANE; i++)
    {
      cairo_save (cr);
      if (i == REGION_COUNTERPART_LANE)
        {
          if (!region_type_has_lane (self->id.type))
            break;

          Track * track =
            arranger_object_get_track (obj);
          if (!track->lanes_visible)
            break;

          TrackLane * lane =
            track->lanes[self->id.lane_pos];
          g_return_if_fail (lane);

          /* set full rectangle */
          region_get_lane_full_rect (
            self, &full_rect);

          /*g_message ("drawing lane %s",*/
            /*self->name);*/
        }
      else if (i == REGION_COUNTERPART_MAIN)
        {
          /* set full rectangle */
          full_rect = obj->full_rect;
        }

      /* get draw (visible only) rectangle */
      arranger_object_get_draw_rectangle (
        obj, rect, &full_rect, &draw_rect);

      draw_background (
        self, cr, rect, &full_rect, &draw_rect, i);

      switch (self->id.type)
        {
        case REGION_TYPE_MIDI:
          draw_midi_region (
            self, cr, rect, &full_rect,
            &draw_rect, i);
          break;
        case REGION_TYPE_AUTOMATION:
          draw_automation_region (
            self, cr, rect, &full_rect,
            &draw_rect, i);
          break;
        case REGION_TYPE_CHORD:
          draw_chord_region (
            self, cr, rect, &full_rect,
            &draw_rect, i);
          break;
        case REGION_TYPE_AUDIO:
          draw_audio_region (
            self, cr, rect, &full_rect,
            &draw_rect, i);
          draw_fades (
            self, cr, rect, &full_rect,
            &draw_rect, i);
          break;
        default:
          break;
        }

      draw_loop_points (
        self, cr, rect, &full_rect, &draw_rect, i);
      draw_name (
        self, cr, rect, &full_rect, &draw_rect, i);

      /* TODO draw cut line? */

      cairo_restore (cr);
    }
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
  rect->height = lane->height - 1;
}
