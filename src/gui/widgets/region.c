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

#include <math.h>

#include "audio/audio_bus_track.h"
#include "audio/automation_region.h"
#include "audio/channel.h"
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
  Region * self,
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
  Region *       self,
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
    0);
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
    cr,
    draw_x,
    full_rect->y - rect->y,
    draw_width,
    full_rect->height,
    1.0, 4.0);
  cairo_fill (cr);
}

/**
 * @param rect Arranger rectangle.
 * @param full_rect Object full rectangle.
 */
static void
draw_loop_points (
  Region *       self,
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
  long loop_start_ticks =
    obj->loop_start_pos.total_ticks;
  long loop_end_ticks =
    obj->loop_end_pos.total_ticks;
  g_warn_if_fail (
    loop_end_ticks > loop_start_ticks);
  long loop_ticks =
    arranger_object_get_loop_length_in_ticks (
      obj);
  long clip_start_ticks =
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
  Region *       self,
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
  long ticks_in_region =
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
  long loop_end_ticks =
    obj->loop_end_pos.total_ticks;
  long loop_ticks =
    arranger_object_get_loop_length_in_ticks (
      obj);
  long clip_start_ticks =
    obj->clip_start_pos.total_ticks;

  for (int i = 0; i < self->num_midi_notes; i++)
    {
      MidiNote * mn = self->midi_notes[i];
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
                (double) tmp_start_ticks /
                (double) ticks_in_region;
              x_end =
                (double) tmp_end_ticks /
                (double) ticks_in_region;
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
draw_automation_region (
  Region *       self,
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
  long ticks_in_region =
    arranger_object_get_length_in_ticks (obj);
  double x_start, y_start, x_end, y_end;

  /* draw automation */
  long loop_end_ticks =
    obj->loop_end_pos.total_ticks;
  long loop_ticks =
    arranger_object_get_loop_length_in_ticks (obj);
  long clip_start_ticks =
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

      long ap_start_ticks =
        ap_obj->pos.total_ticks;
      long ap_end_ticks = ap_start_ticks;
      if (next_ap)
        {
          ap_end_ticks =
            next_ap_obj->pos.total_ticks;
        }
      long tmp_start_ticks, tmp_end_ticks;

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
                loop_ticks * (long) j;

              /* if should be clipped */
              if (next_ap &&
                  position_is_after_or_equal (
                    &next_ap_obj->pos,
                    &obj->loop_end_pos))
                tmp_end_ticks =
                  loop_end_ticks +
                  loop_ticks *  (long) j;
              else
                tmp_end_ticks =
                  ap_end_ticks +
                  loop_ticks *  (long) j;

              /* adjust for clip start */
              tmp_start_ticks -=
                clip_start_ticks;
              tmp_end_ticks -=
                clip_start_ticks;

              /* note: these are local to the
               * region */
              x_start =
                (double) tmp_start_ticks /
                (double) ticks_in_region;
              x_end =
                (double) tmp_end_ticks /
                (double) ticks_in_region;

              /* get ratio (0.0 - 1.0) on y where
               * ap is
               * note: these are local to the region */
              y_start =
                1.0 - (double) ap->normalized_val;
              if (next_ap)
                {
                  y_end =
                    1.0 - (double) next_ap->normalized_val;
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
              int padding = 1;
              cairo_rectangle (
                cr,
                x_start_real - padding,
                y_start_real - padding,
                2 * padding,
                2 * padding);
              cairo_fill (cr);

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
                  for (double k = x_start_real;
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

                      if (math_doubles_equal (
                            k, 0.0, 0.001))
                        {
                          cairo_move_to (
                            cr, new_x, new_y);
                        }

                      cairo_line_to (
                        cr, new_x, new_y);
                    }
                  cairo_stroke (cr);
                }
            }
        }
    }
}

/**
 * @param rect Arranger rectangle.
 */
static void
draw_name (
  Region *          self,
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
 * Draws the Region in the given cairo context in
 * relative coordinates.
 *
 * @param cr The cairo context in the region's
 *   drawable coordinates.
 * @param rect Arranger rectangle.
 */
void
region_draw (
  Region *       self,
  cairo_t *      cr,
  GdkRectangle * rect)
{
  ArrangerObject * obj = (ArrangerObject *) self;

  GdkRectangle draw_rect;
  GdkRectangle full_rect = obj->full_rect;
  for (int i = REGION_COUNTERPART_MAIN;
       i <= REGION_COUNTERPART_LANE; i++)
    {
      if (i == REGION_COUNTERPART_LANE)
        {
          if (!region_type_has_lane (self->type))
            break;

          Track * track =
            arranger_object_get_track (obj);
          if (!track->lanes_visible)
            break;

          TrackLane * lane = self->lane;
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

      /* get draw (visible only) rectangle */
      arranger_object_get_draw_rectangle (
        obj, rect, &full_rect, &draw_rect);

      draw_background (
        self, cr, rect, &full_rect, &draw_rect, i);

      switch (self->type)
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
        default:
          break;
        }

      draw_loop_points (
        self, cr, rect, &full_rect, &draw_rect, i);
      draw_name (
        self, cr, rect, &full_rect, &draw_rect, i);

      /* TODO draw cut line? */
    }
}

/**
 * Returns the lane rectangle for the region.
 */
void
region_get_lane_full_rect (
  Region *       self,
  GdkRectangle * rect)
{
  ArrangerObject * obj = (ArrangerObject *) self;

  Track * track =
    arranger_object_get_track (obj);
  g_return_if_fail (track && track->lanes_visible);

  TrackLane * lane = self->lane;
  g_return_if_fail (lane);

  *rect = obj->full_rect;
  rect->y += lane->y;
  rect->height = lane->height - 1;
}

#if 0

/**
 * Draws the loop points (dashes).
 */
void
region_widget_draw_loop_points (
  RegionWidget * self,
  GtkWidget *    widget,
  cairo_t *      cr,
  GdkRectangle * rect)
{
  REGION_WIDGET_GET_PRIVATE (self);
  Region * r = rw_prv->region;

  int height =
    gtk_widget_get_allocated_height (widget);

  double dashes[] = { 5 };
  cairo_set_dash (
    cr, dashes, 1, 0);
  cairo_set_line_width (cr, 1);
  cairo_set_source_rgba (
    cr, 0, 0, 0, 1.0);

  ArrangerObject * r_obj =
    (ArrangerObject *) r;
  Position tmp;
  long loop_start_ticks =
    r_obj->loop_start_pos.total_ticks;
  long loop_end_ticks =
    r_obj->loop_end_pos.total_ticks;
  g_warn_if_fail (
    loop_end_ticks > loop_start_ticks);
  long loop_ticks =
    arranger_object_get_loop_length_in_ticks (
      r_obj);
  long clip_start_ticks =
    r_obj->clip_start_pos.total_ticks;

  position_from_ticks (
    &tmp, loop_start_ticks - clip_start_ticks);
  int px =
    ui_pos_to_px_timeline (&tmp, 0);
  if (px != 0 &&
      px >= rect->x && px < rect->x + rect->width)
    {
      cairo_set_source_rgba (
        cr, 0, 1, 0, 1.0);
      cairo_move_to (cr, px, 0);
      cairo_line_to (
        cr, px, height);
      cairo_stroke (cr);
    }

  int num_loops =
    arranger_object_get_num_loops (r_obj, 1);
  for (int i = 0; i < num_loops; i++)
    {
      position_from_ticks (
        &tmp, loop_end_ticks + loop_ticks * i);

      /* adjust for clip_start */
      position_add_ticks (
        &tmp, - clip_start_ticks);

      px = ui_pos_to_px_timeline (&tmp, 0);

      if (px >= rect->x && px < rect->width)
        {
          cairo_set_source_rgba (
            cr, 0, 0, 0, 1.0);
          cairo_move_to (
            cr, px, 0);
          cairo_line_to (
            cr, px, height);
          cairo_stroke (
            cr);
        }
    }

  cairo_set_dash (
    cr, NULL, 0, 0);
}

/**
 * Draws the name of the Region.
 *
 * To be called during a cairo draw callback.
 */
void
region_widget_draw_name (
  RegionWidget * self,
  cairo_t *      cr,
  GdkRectangle * rect,
  GdkRectangle * full_rect,
  GdkRectangle * draw_rect,
  RegionCounterpart counter)
{
  REGION_WIDGET_GET_PRIVATE (self);

  /* no need to draw if the start of the region is
   * not visible */
  if (rect->x > 800)
    return;

  Region * region = rw_prv->region;
  g_return_if_fail (
    region &&
    region->name);

  char str[200];
  strcpy (str, region->name);

  /* draw dark bg behind text */
  PangoLayout * layout = rw_prv->layout;
  pango_layout_set_text (
    layout, str, -1);
  PangoRectangle pangorect;
  /* get extents */
  pango_layout_get_pixel_extents (
    layout, NULL, &pangorect);
  gdk_cairo_set_source_rgba (
    cr, &name_bg_color);
  double radius = NAME_BOX_CURVINESS / 1.0;
  double degrees = G_PI / 180.0;
  cairo_new_sub_path (cr);
  cairo_move_to (
    cr, pangorect.width + NAME_PADDING_R, 0);
  cairo_arc (
    cr, (pangorect.width + NAME_PADDING_R) - radius,
    NAME_BOX_HEIGHT - radius, radius,
    0 * degrees, 90 * degrees);
  cairo_line_to (cr, 0, NAME_BOX_HEIGHT);
  cairo_line_to (cr, 0, 0);
  cairo_close_path (cr);
  cairo_fill (cr);

  /* draw text */
  cairo_set_source_rgba (
    cr, 1, 1, 1, 1);
  cairo_move_to (cr, 2, 2);
  pango_cairo_show_layout (cr, layout);
}

static void
region_widget_init (
  RegionWidget * self)
{
  gtk_widget_set_visible (GTK_WIDGET (self), 1);
  g_object_ref (self);

  /* this should really be a singleton but ok for
   * now */
  gdk_rgba_parse (&name_bg_color, "#323232");
  name_bg_color.alpha = 0.8;

  g_signal_connect (
    G_OBJECT (self), "screen-changed",
    G_CALLBACK (on_screen_changed),  self);
  g_signal_connect (
    G_OBJECT (self), "size-allocate",
    G_CALLBACK (on_size_allocate),  self);
}
#endif
