// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "dsp/audio_region.h"
#include "dsp/automation_region.h"
#include "dsp/tempo_track.h"
#include "dsp/track.h"
#include "dsp/tracklist.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/arranger_object.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/main_notebook.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/region.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_panel.h"
#include "gui/widgets/timeline_ruler.h"
#include "project.h"
#include "utils/cairo.h"
#include "utils/color.h"
#include "utils/debug.h"
#include "utils/dsp.h"
#include "utils/gtk.h"
#include "utils/math.h"
#include "utils/objects.h"
#include "utils/ui.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n-lib.h>

static const GdkRGBA object_fill_color = { 1, 1, 1, 1 };

/** Background color for the name. */
/*static GdkRGBA name_bg_color;*/

using RegionCounterpart = enum RegionCounterpart
{
  REGION_COUNTERPART_MAIN,
  REGION_COUNTERPART_LANE,
};

/**
 * Recreates the pango layouts for drawing.
 *
 * @param width Full width of the region.
 */
static void
recreate_pango_layouts (Region * self, int width)
{
  if (G_UNLIKELY (!PANGO_IS_LAYOUT (self->layout_.get ())))
    {
      self->layout_ = PangoLayoutUniquePtr (gtk_widget_create_pango_layout (
        GTK_WIDGET (self->get_arranger ()), nullptr));
      PangoFontDescription * desc =
        pango_font_description_from_string (REGION_NAME_FONT);
      pango_layout_set_font_description (self->layout_.get (), desc);
      pango_font_description_free (desc);
      pango_layout_set_ellipsize (self->layout_.get (), PANGO_ELLIPSIZE_END);
    }
  pango_layout_set_width (
    self->layout_.get (),
    pango_units_from_double (MAX (width - REGION_NAME_PADDING_R, 0)));
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
  Region *          self,
  RegionCounterpart counterpart,
  GdkRectangle *    full_rect,
  GdkRectangle *    draw_rect)
{
  if (counterpart == REGION_COUNTERPART_MAIN)
    {
      *draw_rect = self->last_main_draw_rect_;
      *full_rect = self->last_main_full_rect_;
    }
  else
    {
      *draw_rect = self->last_lane_draw_rect_;
      *full_rect = self->last_lane_full_rect_;
    }

  z_return_if_fail ((*draw_rect).width < 40000);
}

/**
 * @param rect Arranger rectangle.
 * @param full_rect Object full rectangle.
 */
static void
draw_loop_points (
  Region *       self,
  GtkSnapshot *  snapshot,
  GdkRectangle * full_rect,
  GdkRectangle * draw_rect)
{
  double dashes[] = { 5 };
  float  dashesf[] = { 5.f };

  ArrangerWidget * arranger = self->get_arranger ();

  Position tmp;
  double   loop_start_ticks = self->loop_start_pos_.ticks_;
  double   loop_end_ticks = self->loop_end_pos_.ticks_;
  z_warn_if_fail_cmp (loop_end_ticks, >, loop_start_ticks);
  double loop_ticks = self->get_loop_length_in_ticks ();
  double clip_start_ticks = self->clip_start_pos_.ticks_;

  /* get x px for loop point */
  tmp.from_ticks (loop_start_ticks - clip_start_ticks);
  int x_px = ui_pos_to_px_timeline (tmp, 0);

  int vis_offset_x = draw_rect->x - full_rect->x;
  int vis_width = draw_rect->width;
  int full_width = full_rect->width;
  int full_height = full_rect->height;

  const int padding = 1;
  const int line_width = 1;

  /*GskRenderNode * loop_line_node = NULL;*/
  if (!arranger->loop_line_node)
    {
      graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (0, 0, 3, 800);
      arranger->loop_line_node = gsk_cairo_node_new (&tmp_r);
      cairo_t * cr = gsk_cairo_node_get_draw_context (arranger->loop_line_node);

      cairo_set_dash (cr, dashes, 1, 0);
      cairo_set_line_width (cr, line_width);
      cairo_set_source_rgba (cr, 0, 0, 0, 1.0);
      cairo_move_to (cr, padding, 0);
      cairo_line_to (cr, padding, 800);
      cairo_stroke (cr);
      cairo_destroy (cr);
    }
  /*loop_line_node = arranger->loop_line_node;*/

  GskRenderNode * clip_start_line_node = NULL;
  if (!arranger->clip_start_line_node)
    {
      graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (0, 0, 3, 800);
      arranger->clip_start_line_node = gsk_cairo_node_new (&tmp_r);

      cairo_t * cr =
        gsk_cairo_node_get_draw_context (arranger->clip_start_line_node);
      z_cairo_set_source_color (cr, UI_COLORS->bright_green);
      cairo_set_dash (cr, dashes, 1, 0);
      cairo_set_line_width (cr, line_width);
      cairo_move_to (cr, padding, 0);
      cairo_line_to (cr, padding, 800);
      cairo_stroke (cr);
      cairo_destroy (cr);
    }
  clip_start_line_node = arranger->clip_start_line_node;

  /* draw clip start point */
  if (
    x_px != 0 &&
    /* if px is inside region */
    x_px >= 0 && x_px < full_width &&
    /* if loop px is visible */
    x_px >= vis_offset_x && x_px < vis_offset_x + vis_width)
    {
      gtk_snapshot_save (snapshot);
      {
        graphene_point_t tmp_pt =
          Z_GRAPHENE_POINT_INIT ((float) x_px - (float) padding, 0.f);
        gtk_snapshot_translate (snapshot, &tmp_pt);
      }
      {
        graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (
          0.f, 0.f, (float) line_width + (float) padding * 2.f,
          (float) full_height);
        gtk_snapshot_push_clip (snapshot, &tmp_r);
      }
      gtk_snapshot_append_node (snapshot, clip_start_line_node);
      gtk_snapshot_pop (snapshot);
      gtk_snapshot_restore (snapshot);
    }

  /* draw loop points */
  int num_loops = self->get_num_loops (1);
  for (int i = 0; i < num_loops; i++)
    {
      x_px =
        (int) ((double) ((loop_end_ticks + loop_ticks * i) - clip_start_ticks)
               /* adjust for clip_start */
               * MW_RULER->px_per_tick);

      if (
        /* if px is vixible */
        x_px >= vis_offset_x && x_px <= vis_offset_x + vis_width &&
        /* if px is inside region */
        x_px >= 0 && x_px < full_width)
        {
          GskPathBuilder * builder = gsk_path_builder_new ();
          gsk_path_builder_move_to (builder, (float) x_px, 0.f);
          gsk_path_builder_line_to (builder, (float) x_px, (float) full_height);
          GskPath *   path = gsk_path_builder_free_to_path (builder);
          GskStroke * stroke = gsk_stroke_new ((float) line_width);
          gsk_stroke_set_dash (stroke, dashesf, 1);
          gtk_snapshot_push_stroke (snapshot, path, stroke);
          const GdkRGBA color = { 0, 0, 0, 1 };
          {
            graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (
              (float) x_px, 0.f, (float) line_width, (float) full_height);
            gtk_snapshot_append_color (snapshot, &color, &tmp_r);
          }
          gtk_snapshot_pop (snapshot); // stroke
        }
    }
}

/**
 * TODO move this to arranger and show 1 line globally.
 *
 * @param rect Arranger rectangle.
 * @param full_rect Object full rectangle.
 */
static void
draw_cut_line (
  Region *         self,
  GtkSnapshot *    snapshot,
  ArrangerWidget * arranger,
  GdkRectangle *   full_rect,
  GdkRectangle *   draw_rect)
{
  GskRenderNode * cut_line_node = NULL;
  const int       padding = 1;
  const int       line_width = 1;
  if (!arranger->cut_line_node)
    {
      graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (0, 0, 3, 800);
      arranger->cut_line_node = gsk_cairo_node_new (&tmp_r);

      cairo_t * cr = gsk_cairo_node_get_draw_context (arranger->cut_line_node);
      z_cairo_set_source_color (cr, UI_COLORS->dark_text);
      double dashes[] = { 5 };
      cairo_set_dash (cr, dashes, 1, 0);
      cairo_set_line_width (cr, line_width);
      cairo_move_to (cr, padding, 0);
      cairo_line_to (cr, padding, 800);
      cairo_stroke (cr);
      cairo_destroy (cr);
    }
  cut_line_node = arranger->cut_line_node;

  int x_px = (int) arranger->hover_x - full_rect->x;

  gtk_snapshot_save (snapshot);
  {
    graphene_point_t tmp_pt =
      Z_GRAPHENE_POINT_INIT ((float) x_px - (float) padding, 0.f);
    gtk_snapshot_translate (snapshot, &tmp_pt);
  }
  {
    graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (
      0.f, 0.f, (float) line_width + (float) padding * 2.f,
      (float) full_rect->height);
    gtk_snapshot_push_clip (snapshot, &tmp_r);
  }
  gtk_snapshot_append_node (snapshot, cut_line_node);
  gtk_snapshot_pop (snapshot);
  gtk_snapshot_restore (snapshot);
}

/**
 * @param rect Arranger rectangle.
 */
static void
draw_midi_region (
  MidiRegion *   self,
  GtkSnapshot *  snapshot,
  GdkRectangle * full_rect,
  GdkRectangle * draw_rect)
{
  Track * track = self->get_track ();

  /* set color */
  GdkRGBA color = object_fill_color;
  if (track)
    {
      /* set to grey if track color is very  bright */
      if (track->color_.is_very_very_bright ())
        {
          color.red = 0.7f;
          color.green = 0.7f;
          color.blue = 0.7f;
        }
    }

  int    num_loops = self->get_num_loops (1);
  double ticks_in_region = self->get_length_in_ticks ();
  double x_start, y_start, x_end;

  int min_val = 127, max_val = 0;
  for (auto &mn : self->midi_notes_)
    {
      if (mn->val_ < min_val)
        min_val = mn->val_;
      if (mn->val_ > max_val)
        max_val = mn->val_;
    }
  double y_interval = MAX ((double) (max_val - min_val) + 1.0, 7.0);
  double y_note_size = 1.0 / y_interval;

  /* draw midi notes */
  double loop_end_ticks = self->loop_end_pos_.ticks_;
  double loop_ticks = self->get_loop_length_in_ticks ();
  double clip_start_ticks = self->clip_start_pos_.ticks_;

  int vis_offset_x = draw_rect->x - full_rect->x;
  int vis_offset_y = draw_rect->y - full_rect->y;
  int vis_width = draw_rect->width;
  int vis_height = draw_rect->height;
  /*int full_offset_x = full_rect->x;*/
  /*int full_offset_y = full_rect->y;*/
  int full_width = full_rect->width;
  int full_height = full_rect->height;

  bool is_looped = self->is_looped ();

  for (auto &mn : self->midi_notes_)
    {
      /* note muted */
      if (mn->get_muted (false))
        continue;

      /* note not playable */
      if (mn->pos_ >= self->loop_end_pos_)
        continue;

      /* note not playable if looped */
      if (
        is_looped && mn->pos_ < self->loop_start_pos_
        && mn->pos_ < self->clip_start_pos_)
        continue;

      /* get ratio (0.0 - 1.0) on x where midi note starts & ends */
      double mn_start_ticks = mn->pos_.ticks_;
      double mn_end_ticks = mn->end_pos_.ticks_;
      double tmp_start_ticks, tmp_end_ticks;

      for (int j = 0; j < num_loops; j++)
        {
          /* if note started before loop start only draw it once */
          if (mn->pos_ < self->loop_start_pos_ && j != 0)
            break;

          /* calculate draw endpoints */
          tmp_start_ticks = mn_start_ticks + loop_ticks * j;
          /* if should be clipped */
          if (mn->end_pos_ >= self->loop_end_pos_)
            tmp_end_ticks = loop_end_ticks + loop_ticks * j;
          else
            tmp_end_ticks = mn_end_ticks + loop_ticks * j;

          /* adjust for clip start */
          tmp_start_ticks -= clip_start_ticks;
          tmp_end_ticks -= clip_start_ticks;

          /* get ratios (0.0 - 1.0) of
           * where midi note is */
          x_start = tmp_start_ticks / ticks_in_region;
          x_end = tmp_end_ticks / ticks_in_region;
          y_start = ((double) max_val - (double) mn->val_) / y_interval;

          /* get actual values using the
           * ratios */
          x_start *= (double) full_width;
          x_end *= (double) full_width;
          y_start *= (double) full_height;

          /* the above values are local to the
           * region, convert to global */
          /*x_start += full_rect->x;*/
          /*x_end += full_rect->x;*/
          /*y_start += full_rect->y;*/

          /* skip if any part of the note is
           * not visible in the region's rect */
          if (
            (x_start >= vis_offset_x && x_start < vis_offset_x + vis_width)
            || (x_end >= vis_offset_x && x_end < vis_offset_x + vis_width)
            || (x_start < vis_offset_x && x_end > vis_offset_x))
            {
              float draw_x = (float) MAX (x_start, vis_offset_x);
              float draw_width = (float) MIN (
                (x_end - x_start) - (draw_x - x_start),
                (float) (vis_offset_x + vis_width) - draw_x);
              float draw_y = (float) MAX (y_start, vis_offset_y);
              float draw_height = (float) MIN (
                (y_note_size * (double) full_height) - (draw_y - y_start),
                (float) (vis_offset_y + vis_height) - draw_y);
              {
                graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (
                  draw_x, draw_y, draw_width, draw_height);
                gtk_snapshot_append_color (snapshot, &color, &tmp_r);
              }

            } /* endif part of note is visible */

        } /* end foreach region loop */

    } /* end foreach note */
}

/**
 * @param rect Arranger rectangle.
 */
static void
draw_chord_region (
  ChordRegion *  self,
  GtkSnapshot *  snapshot,
  GdkRectangle * full_rect,
  GdkRectangle * draw_rect)
{
  int    num_loops = self->get_num_loops (1);
  double ticks_in_region = self->get_length_in_ticks ();
  double x_start;

  int full_width = full_rect->width;
  int full_height = full_rect->height;

  /* draw chords notes */
  /*double loop_end_ticks = obj->loop_end_pos_.ticks;*/
  double loop_ticks = self->get_loop_length_in_ticks ();
  double clip_start_ticks = self->clip_start_pos_.ticks_;
  for (auto &co : self->chord_objects_)
    {
      const auto descr = co->get_chord_descriptor ();

      /* get ratio (0.0 - 1.0) on x where chord starts & ends */
      double co_start_ticks = co->pos_.ticks_;
      double tmp_start_ticks;

      /* if before loop end */
      if (co->pos_ < self->loop_end_pos_)
        {
          for (int j = 0; j < num_loops; j++)
            {
              /* if note started before loop start
               * only draw it once */
              if (co->pos_ < self->loop_start_pos_ && j != 0)
                break;

              /* calculate draw endpoints */
              tmp_start_ticks = co_start_ticks + loop_ticks * j;

              /* adjust for clip start */
              tmp_start_ticks -= clip_start_ticks;
              /*tmp_end_ticks -= clip_start_ticks;*/

              x_start = tmp_start_ticks / ticks_in_region;
              /*x_end = tmp_end_ticks / ticks_in_region;*/

              /* skip if before the region */
              if (x_start < 0.0)
                continue;

              GdkRGBA color = { 1, 1, 1, 1 };

              /* get actual values using the
               * ratios */
              x_start *= (double) full_width;
              /*x_end *= (double) full_width;*/

              /* draw */
              {
                graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (
                  (float) x_start, 0, 2.f, (float) full_height);
                gtk_snapshot_append_color (snapshot, &color, &tmp_r);
              }

              /* draw name */
              auto descr_str = descr->to_string ();
              gtk_snapshot_save (snapshot);
              {
                graphene_point_t tmp_pt = Z_GRAPHENE_POINT_INIT (
                  (float) x_start + 4.f, (float) full_height - 10.f);
                gtk_snapshot_translate (snapshot, &tmp_pt);
              }
              PangoLayout * layout = self->chords_layout_.get ();
              pango_layout_set_markup (layout, descr_str.c_str (), -1);
              gtk_snapshot_append_layout (snapshot, layout, &color);
              gtk_snapshot_restore (snapshot);
            }
        }
    }
}

/**
 * @param full_rect Region rectangle with absolute coordinates.
 *
 * @return Whether to break.
 */
static inline bool
handle_loop (
  AutomationRegion * self,
  GtkSnapshot *      snapshot,
  GdkRectangle *     full_rect,
  GdkRectangle *     draw_rect,
  int                cur_loop,
  AutomationPoint *  ap,
  AutomationPoint *  next_ap,
  double             ticks_in_region)
{
  double loop_ticks = self->get_loop_length_in_ticks ();

  double ap_end_ticks = ap->pos_.ticks_;
  if (next_ap)
    {
      ap_end_ticks = next_ap->pos_.ticks_;
    }

  /* if both ap and next ap are before loop start don't
   * draw at all */
  if (
    ap->pos_ < self->loop_start_pos_ && next_ap
    && next_ap->pos_ < self->loop_start_pos_ && cur_loop != 0)
    {
      return true;
    }

  /* calculate draw endpoints */
  z_return_val_if_fail_cmp (loop_ticks, >, 0, true);
  z_return_val_if_fail_cmp (cur_loop, >=, 0, true);
  double global_start_ticks_after_loops =
    ap->pos_.ticks_ + loop_ticks * (double) cur_loop;

  /* if ap started before loop start only draw from the loop
   * start point */
  if (ap->pos_ < self->loop_start_pos_ && cur_loop != 0)
    {
      global_start_ticks_after_loops +=
        self->loop_start_pos_.ticks_ - ap->pos_.ticks_;
    }

  /* if should be clipped */
  double global_end_ticks_after_loops =
    ap_end_ticks + loop_ticks * (double) cur_loop;
  double global_end_ticks_after_loops_with_clipoff; /* same as above but the part
                                                       outside the loop/region
                                                       is clipped off */
  if (next_ap && next_ap->pos_ >= self->loop_end_pos_)
    {
      global_end_ticks_after_loops_with_clipoff =
        self->loop_end_pos_.ticks_ + loop_ticks * (double) cur_loop;
    }
  else if (next_ap && next_ap->pos_ >= self->end_pos_)
    {
      global_end_ticks_after_loops_with_clipoff =
        self->end_pos_.ticks_ + loop_ticks * (double) cur_loop;
    }
  else
    {
      global_end_ticks_after_loops_with_clipoff = global_end_ticks_after_loops;
    }

  /* adjust for clip start */
  global_start_ticks_after_loops -= self->clip_start_pos_.ticks_;
  global_end_ticks_after_loops -= self->clip_start_pos_.ticks_;
  global_end_ticks_after_loops_with_clipoff -= self->clip_start_pos_.ticks_;

#if 0
  z_debug (
    "loop %d:, abs start ticks after loops {:f} | abs end ticks after loops {:f} | abs end ticks after loops w clipof {:f}",
    cur_loop, abs_start_ticks_after_loops,
    global_end_ticks_after_loops,
    global_end_ticks_after_loops_with_clipoff);
#endif

  double x_start_ratio_in_region =
    global_start_ticks_after_loops / ticks_in_region;
  double x_end_ratio_in_region = global_end_ticks_after_loops / ticks_in_region;
  double x_end_ratio_in_region_with_clipoff =
    global_end_ticks_after_loops_with_clipoff / ticks_in_region;

  /* get ratio (0.0 - 1.0) on y where ap is */
  double y_start_ratio, y_end_ratio;
  y_start_ratio = 1.0 - (double) ap->normalized_val_;
  if (next_ap)
    {
      y_end_ratio = 1.0 - (double) next_ap->normalized_val_;
    }
  else
    {
      y_end_ratio = y_start_ratio;
    }

  double x_start_in_region = x_start_ratio_in_region * full_rect->width;
  double x_end_in_region = x_end_ratio_in_region * full_rect->width;
  double x_end_in_region_with_clipoff =
    x_end_ratio_in_region_with_clipoff * full_rect->width;
  double y_start = y_start_ratio * full_rect->height;
  double y_end = y_end_ratio * full_rect->height;

#if 0
  z_debug (
    "x start ratio in region {:f} full rect width %d x start in region {:f} | x end ratio in region {:f} x end in region {:f}",
    x_start_ratio_in_region, full_rect->width,
    x_start_in_region, x_end_ratio_in_region, x_end_in_region);
#endif

  /*  -- OK UP TO HERE --  */

  GdkRGBA color = object_fill_color;

  /* draw ap */
  if (x_start_in_region > 0.0 && x_start_in_region < full_rect->width)
    {
      int padding = 1;
      {
        graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (
          (float) (x_start_in_region - padding), (float) (y_start - padding),
          2.f * (float) padding, 2.f * (float) padding);
        gtk_snapshot_append_color (snapshot, &color, &tmp_r);
      }
    }

  bool ret_val = false;

  /* draw curve */
  if (next_ap)
    {
      const int line_width = 2;
      double    curve_width = fabs (x_end_in_region - x_start_in_region);

      ArrangerWidget * arranger = self->get_arranger ();
      const auto settings = arranger_widget_get_editor_setting_values (arranger);

      /* rectangle where origin is the top left of the
       * screen */
      GdkRectangle vis_rect = Z_GDK_RECTANGLE_INIT (
        settings->scroll_start_x_ - full_rect->x, -full_rect->y,
        gtk_widget_get_width (GTK_WIDGET (arranger)), full_rect->height);

      /* rectangle for the loop part, where the region start
       * is (0,0) */
      GdkRectangle ap_loop_part_rect = Z_GDK_RECTANGLE_INIT (
        (int) x_start_in_region, 0,
        /* add the line width otherwise the end of the
         * curve gets cut off */
        (int) (curve_width) + line_width, full_rect->height);

      /* adjust if automation point starts before the region */
      if (x_start_in_region < 0)
        {
          ap_loop_part_rect.x = 0;
          ap_loop_part_rect.width += (int) x_start_in_region;
        }

      /* adjust if automation point ends after the loop/region end */
      if (x_end_in_region > x_end_in_region_with_clipoff)
        {
          double clipoff = x_end_in_region - x_end_in_region_with_clipoff;
          /*z_debug ("clipped off {:f}", clipoff);*/
          ap_loop_part_rect.width -= (int) clipoff;
        }

      GskPathBuilder * builder = gsk_path_builder_new ();

      double ac_height = fabs (y_end_ratio - y_start_ratio) * full_rect->height;
      double step = 0.5;
      double start_from = MAX (vis_rect.x, ap_loop_part_rect.x);
      start_from = MAX (start_from, 0.0);
      double until = ap_loop_part_rect.x + ap_loop_part_rect.width;
      until = MIN (vis_rect.x + vis_rect.width, until);
      bool first_call = true;
      for (double k = start_from; k < until + step; k += step)
        {
          double ap_y =
            math_doubles_equal (curve_width, 0.0)
              ? 0.5
              :
              /* in pixels, higher values
               * are lower */
              1.0
                - ap->get_normalized_value_in_curve (
                  self, std::clamp<double> (
                          (k - x_start_in_region) / curve_width, 0.0, 1.0));
          /*z_debug ("start from {:f} k {:f} x start in region {:f} ratio {:f},
           * ac width
           * {:f}, ap y {:f}", start_from, k, x_start_in_region, CLAMP ((k -
           * x_start_in_region) / curve_width, 0.0, 1.0), curve_width, ap_y);*/
          ap_y *= ac_height;

          double new_x = k;
          double new_y;
          if (y_start_ratio > y_end_ratio)
            new_y = ap_y + y_end;
          else
            new_y = ap_y + y_start;

          if (new_x >= full_rect->width)
            {
              ret_val = true;
              break;
            }

          if (G_UNLIKELY (first_call))
            {
              gsk_path_builder_move_to (builder, (float) new_x, (float) new_y);
              first_call = false;
            }

          gsk_path_builder_line_to (builder, (float) new_x, (float) new_y);
        } /* end foreach draw step */

      GskPath *   path = gsk_path_builder_free_to_path (builder);
      GskStroke * stroke = gsk_stroke_new ((float) line_width);
      gtk_snapshot_push_stroke (snapshot, path, stroke);
      {
        graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (
          (float) ap_loop_part_rect.x, (float) ap_loop_part_rect.y,
          (float) ap_loop_part_rect.width, (float) ap_loop_part_rect.height);
        gtk_snapshot_append_color (snapshot, &color, &tmp_r);
      }
      gtk_snapshot_pop (snapshot); // stroke

      ap->last_settings_tl_.normalized_val = ap->normalized_val_;
      ap->last_settings_tl_.curve_opts = ap->curve_opts_;
      ap->last_settings_tl_.draw_rect = ap_loop_part_rect;

    } /* endif have next ap */

  return ret_val;
}

/**
 * @param full_rect Region rectangle with absolute coordinates.
 */
static void
draw_automation_region (
  AutomationRegion * self,
  GtkSnapshot *      snapshot,
  GdkRectangle *     full_rect,
  GdkRectangle *     draw_rect)
{
  int    num_loops = self->get_num_loops (true);
  double ticks_in_region = self->get_length_in_ticks ();

  /* draw automation */
  for (auto &ap : self->aps_)
    {
      AutomationPoint * next_ap = self->get_next_ap (*ap, true, true);

      /* if before loop end */
      if (ap->pos_ < self->loop_end_pos_)
        {
          for (int j = 0; j < num_loops; j++)
            {
              bool need_break = handle_loop (
                self, snapshot, full_rect, draw_rect, j, ap.get (), next_ap,
                ticks_in_region);
              if (need_break)
                break;
            } /* end foreach loop */

        } /* endif before loop end */
    }
}

/**
 * @param rect Arranger rectangle.
 * @param full_rect Arranger object rect.
 */
static void
draw_fade_part (
  AudioRegion * self,
  GtkSnapshot * snapshot,
  int           vis_offset_x,
  int           vis_width,
  int           full_width,
  int           height)
{
  Track * track = self->get_track ();
  z_return_if_fail (track);

  const float top_line_width = 4.f;

  /* set color */
  GdkRGBA       color = { 0.2f, 0.2f, 0.2f, 0.6f };
  const GdkRGBA white = Z_GDK_RGBA_INIT (1, 1, 1, 1);

  /* get fade in px */
  int fade_in_px = ui_pos_to_px_timeline (self->fade_in_pos_, 0);

  /* get start px */
  int start_px = 0;

  /* get visible positions (where to draw) */
  int vis_fade_in_px = MIN (fade_in_px, vis_offset_x + vis_width);
  int vis_start_px = MAX (start_px, vis_offset_x);

  UiDetail detail = ui_get_detail_level ();

  /* use cairo if normal detail or higher */
  bool use_cairo = false;
  if (detail < UiDetail::Low)
    {
      use_cairo = true;
    }

  const int step = 1;
  if (vis_fade_in_px - vis_start_px > 0)
    {
      double local_px_diff = (double) (fade_in_px - start_px);

      /* draw a white line on top of the fade area */
      if (arranger_object_is_hovered_or_start_object (self, nullptr))
        {
          {
            graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (
              (float) vis_start_px, 0.f,
              (float) vis_fade_in_px - (float) vis_start_px, top_line_width);
            gtk_snapshot_append_color (snapshot, &white, &tmp_r);
          }
        }

      /* create cairo node if necessary */
      cairo_t * cr = NULL;
      if (use_cairo)
        {
          graphene_rect_t grect = Z_GRAPHENE_RECT_INIT (
            (float) vis_start_px, 0.f,
            (float) vis_fade_in_px - (float) vis_start_px, (float) height);
          /*z_graphene_rect_print (&grect);*/
          cr = gtk_snapshot_append_cairo (snapshot, &grect);
          cairo_set_source_rgba (cr, 0.2, 0.2, 0.2, 0.6);
          /*cairo_set_line_width (cr, 3);*/
        }
      for (int i = vis_start_px; i <= vis_fade_in_px; i += step)
        {
          /* revert because cairo draws the other
           * way */
          double val =
            1.0
            - fade_get_y_normalized (
              self->fade_in_opts_, (double) (i - start_px) / local_px_diff, true);

          float draw_y_val = (float) (val * height);

          if (use_cairo)
            {
              double next_val =
                1.0
                - fade_get_y_normalized (
                  self->fade_in_opts_,
                  (double) MIN (((i + step) - start_px), local_px_diff)
                    / local_px_diff,
                  true);

              /* if start, move only */
              if (i == vis_start_px)
                {
                  cairo_move_to (cr, i, val * height);
                }

              cairo_rel_line_to (cr, 1, (next_val - val) * height);

              /* if end, draw */
              if (i == vis_fade_in_px)
                {
                  /* paint a gradient in the faded out
                   * part */
                  cairo_rel_line_to (cr, 0, next_val - height);
                  cairo_rel_line_to (cr, -i, 0);
                  cairo_close_path (cr);
                  cairo_fill (cr);
                }
            }
          else
            {
              graphene_rect_t grect =
                Z_GRAPHENE_RECT_INIT ((float) i, 0, (float) step, draw_y_val);
              /*z_graphene_rect_print (&grect);*/
              gtk_snapshot_append_color (snapshot, &color, &grect);
            }
        }

      if (use_cairo)
        cairo_destroy (cr);

    } /* endif fade in visible */

  /* ---- fade out ---- */

  /* get fade out px */
  int fade_out_px = ui_pos_to_px_timeline (self->fade_out_pos_, 0);

  /* get end px */
  int end_px = full_width;

  /* get visible positions (where to draw) */
  int visible_fade_out_px = MAX (fade_out_px, vis_offset_x);
  int visible_end_px = MIN (end_px, vis_offset_x + vis_width);

  if (visible_end_px - visible_fade_out_px > 0)
    {
      double local_px_diff = (double) (end_px - fade_out_px);

      /* draw a white line on top of the fade area */
      if (arranger_object_is_hovered_or_start_object (self, nullptr))
        {
          {
            graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (
              (float) visible_fade_out_px, 0.f,
              (float) visible_end_px - (float) visible_fade_out_px,
              top_line_width);
            gtk_snapshot_append_color (snapshot, &white, &tmp_r);
          }
        }

      /* create cairo node if necessary */
      cairo_t * cr = NULL;
      if (use_cairo)
        {
          graphene_rect_t grect = Z_GRAPHENE_RECT_INIT (
            (float) visible_fade_out_px, 0.f,
            (float) visible_end_px - (float) visible_fade_out_px,
            (float) height);
          /*z_graphene_rect_print (&grect);*/
          cr = gtk_snapshot_append_cairo (snapshot, &grect);
          cairo_set_source_rgba (cr, 0.2, 0.2, 0.2, 0.6);
          /*cairo_set_line_width (cr, 3);*/
        }

      for (int i = visible_fade_out_px; i <= visible_end_px; i++)
        {
          /* revert because cairo draws the other way */
          double val =
            1.0
            - fade_get_y_normalized (
              self->fade_out_opts_, (double) (i - fade_out_px) / local_px_diff,
              false);

          if (use_cairo)
            {
              double tmp = (double) ((i + 1) - fade_out_px);
              double next_val =
                1.0
                - fade_get_y_normalized (
                  self->fade_out_opts_,
                  tmp > local_px_diff ? 1.0 : tmp / local_px_diff, false);

              /* if start, move only */
              if (i == visible_fade_out_px)
                {
                  cairo_move_to (cr, i, val * height);
                }

              cairo_rel_line_to (cr, 1, (next_val - val) * height);

              /* if end, draw */
              if (i == visible_end_px)
                {
                  /* paint a gradient in the faded
                   * out part */
                  cairo_rel_line_to (cr, 0, -height);
                  cairo_rel_line_to (cr, -i, 0);
                  cairo_close_path (cr);
                  cairo_fill (cr);
                }
            }
          else
            {
              float           draw_y_val = (float) (val * height);
              graphene_rect_t grect =
                Z_GRAPHENE_RECT_INIT ((float) i, 0, 1, draw_y_val);
              /*z_graphene_rect_print (&grect);*/
              gtk_snapshot_append_color (snapshot, &color, &grect);
            }
        }

      if (use_cairo)
        cairo_destroy (cr);

    } /* endif fade out visible */
}

static void
draw_audio_part (
  AudioRegion *  self,
  GtkSnapshot *  snapshot,
  GdkRectangle * full_rect,
  int            vis_offset_x,
  int            vis_width,
  int            full_height)
{
  z_return_if_fail (vis_width < 40000);

  auto * clip = self->get_clip ();

  auto frames_per_tick = (double) AUDIO_ENGINE->frames_per_tick_;
  if (self->get_musical_mode ())
    {
      frames_per_tick *=
        (double) P_TEMPO_TRACK->get_current_bpm () / (double) clip->bpm_;
    }
  double multiplier = frames_per_tick / MW_RULER->px_per_tick;

  UiDetail detail = ui_get_detail_level ();

  double increment = 1;
  double width = 1;
  switch (detail)
    {
    case UiDetail::High:
      /* snapshot does not work with midpoints */
      /*increment = 0.5;*/
      increment = 0.2;
      width = 1;
      break;
    case UiDetail::Normal:
      increment = 1;
      width = 1;
      break;
    case UiDetail::Low:
      increment = 2;
      width = 2;
      break;
    case UiDetail::UltraLow:
      increment = 4;
      width = 4;
      break;
    }

#if 0
  z_info ("current detail: {}",
    ui_detail_str[detail]);
#endif

  signed_frame_t loop_end_frames = math_round_double_to_signed_frame_t (
    self->loop_end_pos_.ticks_ * frames_per_tick);
  signed_frame_t loop_frames = math_round_double_to_signed_frame_t (
    self->get_loop_length_in_ticks () * frames_per_tick);
  signed_frame_t clip_start_frames = math_round_double_to_signed_frame_t (
    self->clip_start_pos_.ticks_ * frames_per_tick);

  double         local_start_x = (double) vis_offset_x;
  double         local_end_x = local_start_x + (double) vis_width;
  signed_frame_t prev_frames = (signed_frame_t) (multiplier * local_start_x);
  signed_frame_t curr_frames = prev_frames;

  float * prev_min = object_new_n (clip->channels_, float);
  float * prev_max = object_new_n (clip->channels_, float);
  dsp_fill (prev_min, 0.5f, clip->channels_);
  dsp_fill (prev_max, 0.5f, clip->channels_);
  float * ch_min = object_new_n (clip->channels_, float);
  float * ch_max = object_new_n (clip->channels_, float);
  dsp_fill (ch_min, 0.5f, clip->channels_);
  dsp_fill (ch_max, 0.5f, clip->channels_);
  const double full_height_per_ch =
    (double) full_height / (double) clip->channels_;
  for (double i = local_start_x; i < (double) local_end_x; i += increment)
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

      if (prev_frames >= (signed_frame_t) clip->num_frames_)
        {
          prev_frames = curr_frames;
          continue;
        }

      z_return_if_fail_cmp (curr_frames, >=, 0);
      signed_frame_t from = (MAX (0, prev_frames));
      signed_frame_t to =
        (MIN ((signed_frame_t) clip->num_frames_, curr_frames));
      signed_frame_t frames_to_check = to - from;
      if (from + frames_to_check > (signed_frame_t) clip->num_frames_)
        {
          frames_to_check = (signed_frame_t) clip->num_frames_ - from;
        }
      z_return_if_fail_cmp (from, <, (signed_frame_t) clip->num_frames_);
      z_return_if_fail_cmp (
        from + frames_to_check, <=, (signed_frame_t) clip->num_frames_);
      if (frames_to_check > 0)
        {
          size_t frames_to_check_unsigned = (size_t) frames_to_check;
          for (unsigned int k = 0; k < clip->channels_; k++)
            {
              ch_min[k] = dsp_min (
                &clip->ch_frames_.getReadPointer (k)[from],
                (size_t) frames_to_check_unsigned);
              ch_max[k] = dsp_max (
                &clip->ch_frames_.getReadPointer (k)[from],
                (size_t) frames_to_check_unsigned);

              /* normalize */
              ch_min[k] = (ch_min[k] + 1.f) / 2.f;
              ch_max[k] = (ch_max[k] + 1.f) / 2.f;
            }
        }
      else
        {
          dsp_copy (prev_min, ch_min, clip->channels_);
          dsp_copy (prev_max, ch_max, clip->channels_);
        }

      for (unsigned int k = 0; k < clip->channels_; k++)
        {
          /* adjust to draw from the middle so it draws bars instead of single
           * points when zoomed */
          if (ch_max[k] > 0.5f && ch_min[k] > 0.5f)
            {
              ch_min[k] = 0.5f;
            }
          if (ch_min[k] < 0.5f && ch_max[k] < 0.5f)
            {
              ch_max[k] = 0.5f;
            }
          /* local from the full rect y */
          double local_min_y =
            MAX ((double) ch_min[k] * (double) full_height_per_ch, 0.0);
          /* local from the full rect y */
          double local_max_y = MIN (
            (double) ch_max[k] * (double) full_height_per_ch,
            (double) full_height_per_ch);

          /* only draw if non-silent */
          float       avg = (ch_max[k] + ch_min[k]) / 2.f;
          const float epsilon = 0.001f;
          if (
            !math_floats_equal_epsilon (avg, 0.5f, epsilon)
            || ch_max[k] - ch_min[k] > epsilon)
            {
              gtk_snapshot_save (snapshot);
              {
                graphene_point_t tmp_pt = Z_GRAPHENE_POINT_INIT (
                  0, (float) k * (float) full_height_per_ch);
                gtk_snapshot_translate (snapshot, &tmp_pt);
              }
              {
                graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (
                  (float) i, (float) local_min_y, (float) MAX (width, 1),
                  (float) MAX (local_max_y - local_min_y, 1));
                gtk_snapshot_append_color (snapshot, &object_fill_color, &tmp_r);
              }
              gtk_snapshot_restore (snapshot);
            }
          prev_min[k] = ch_min[k];
          prev_max[k] = ch_max[k];
        } /* endforeach channel k */

      prev_frames = curr_frames;
    }
  free (ch_min);
  free (ch_max);
  free (prev_min);
  free (prev_max);
}

/**
 * Draw audio.
 *
 * At this point, cr is translated to start at 0,0 in the full rect.
 *
 * @param rect Arranger rectangle.
 * @param cache_applied_rect The rectangle where the previous cache was
 *   pasted at, where 0,0 is the region's top left corner. This is so that
 *   the cached part is not re-drawn.
 *   Only x and width are useful.
 * @param cache_applied_offset_x The offset from the region's top-left corner
 *   at which the cache was applied (can be negative).
 * @param cache_applied_width The width of the cache, starting from
 *   cache_applied_offset_x (even if negative).
 */
static void
draw_audio_region (
  AudioRegion *  self,
  GtkSnapshot *  snapshot,
  GdkRectangle * full_rect,
  GdkRectangle * draw_rect,
  bool           cache_applied,
  int            cache_applied_offset_x,
  int            cache_applied_width)
{
  z_return_if_fail (cache_applied_width < 40000);

  /*z_info ("drawing audio region");*/
  if (self->stretching_)
    {
      return;
    }

  int full_height = full_rect->height;

  int vis_offset_x = draw_rect->x - full_rect->x;
  int vis_width = draw_rect->width;
  draw_audio_part (
    self, snapshot, full_rect, vis_offset_x, vis_width, full_height);
  draw_fade_part (
    self, snapshot, vis_offset_x, vis_width, full_rect->width, full_height);
}

/**
 * @param rect Arranger rectangle.
 */
static void
draw_name (
  Region *       self,
  GtkSnapshot *  snapshot,
  GdkRectangle * full_rect,
  GdkRectangle * draw_rect)
{
  z_return_if_fail (self && !self->escaped_name_.empty ());

  int full_width = full_rect->width;

  std::string escaped_name;
  if (self->id_.link_group_ >= 0)
    {
      escaped_name =
        fmt::format ("%s #%d", self->escaped_name_, self->id_.link_group_);
    }
  else
    {
      escaped_name = self->escaped_name_;
    }

  /* draw dark bg behind text */
  recreate_pango_layouts (self, MIN (full_width, 800));
  PangoLayout * layout = self->layout_.get ();
  if (DEBUGGING)
    {
      std::string str = escaped_name;
      Region *    clip_editor_region = CLIP_EDITOR->get_region ();
      if (clip_editor_region == self)
        {
          str += " (CLIP EDITOR)";
        }
      pango_layout_set_markup (layout, str.c_str (), -1);
    }
  else
    {
      pango_layout_set_markup (layout, escaped_name.c_str (), -1);
    }
  PangoRectangle pangorect;

  /* get extents */
  pango_layout_get_pixel_extents (layout, nullptr, &pangorect);
  int black_box_height = pangorect.height + 2 * REGION_NAME_BOX_PADDING;

  /* create a rounded clip */
  /*float degrees = G_PI / 180.f;*/
  float           radius = REGION_NAME_BOX_CURVINESS / 1.f;
  GskRoundedRect  rounded_rect;
  graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (
    0.f, 0.f, (float) (pangorect.width + REGION_NAME_PADDING_R),
    (float) black_box_height);
  graphene_size_t tmp_sz = GRAPHENE_SIZE_INIT (0, 0);
  graphene_size_t tmp_sz2 = GRAPHENE_SIZE_INIT (radius, radius);
  gsk_rounded_rect_init (
    &rounded_rect, &tmp_r, &tmp_sz, &tmp_sz, &tmp_sz2, &tmp_sz2);
  gtk_snapshot_push_rounded_clip (snapshot, &rounded_rect);

  /* fill bg color */
  GdkRGBA name_bg_color;
  gdk_rgba_parse (&name_bg_color, "#323232");
  name_bg_color.alpha = 0.8f;
  gtk_snapshot_append_color (snapshot, &name_bg_color, &rounded_rect.bounds);

  /* draw text */
  gtk_snapshot_save (snapshot);
  {
    graphene_point_t tmp_pt =
      Z_GRAPHENE_POINT_INIT (REGION_NAME_BOX_PADDING, REGION_NAME_BOX_PADDING);
    gtk_snapshot_translate (snapshot, &tmp_pt);
  }
  const GdkRGBA white = Z_GDK_RGBA_INIT (1, 1, 1, 1);
  gtk_snapshot_append_layout (snapshot, layout, &white);
  gtk_snapshot_restore (snapshot);

  /* pop rounded clip */
  gtk_snapshot_pop (snapshot);

  /* save rect */
  self->last_name_rect_.x = 0;
  self->last_name_rect_.y = 0;
  self->last_name_rect_.width = pangorect.width + REGION_NAME_PADDING_R;
  self->last_name_rect_.height = pangorect.height + REGION_NAME_PADDING_R;
}

/**
 * @param rect Arranger rectangle.
 */
static void
draw_bottom_right_part (
  Region *       self,
  GtkSnapshot *  snapshot,
  GdkRectangle * full_rect,
  GdkRectangle * draw_rect)
{
  /* if audio region, draw BPM */
  if (self->is_audio ())
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
        layout, nullptr, &pangorect);
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
 * Draws the Region in the given cairo context in
 * relative coordinates.
 *
 * @param rect Arranger rectangle.
 */
void
region_draw (Region * self, GtkSnapshot * snapshot, GdkRectangle * rect)
{
  ArrangerWidget * arranger = self->get_arranger ();

  Track * track = self->get_track ();
  z_return_if_fail (IS_TRACK_AND_NONNULL (track));

  /* set color */
  Color color;
  if (self->use_color_)
    {
      color = self->color_;
    }
  else if (track)
    color = track->color_;
  else
    {
      color = Color{ 1, 0, 0, 1 };
    }
  color = Color::get_arranger_object_color (
    color, self->is_hovered (), self->is_selected (),
    /* FIXME */
    false, self->get_muted (true) || track->frozen_);

  GdkRectangle draw_rect;
  GdkRectangle last_draw_rect, last_full_rect;
  GdkRectangle full_rect = self->full_rect_;
  for (int i = REGION_COUNTERPART_MAIN; i <= REGION_COUNTERPART_LANE; i++)
    {
      if (i == REGION_COUNTERPART_LANE)
        {
          if (!self->has_lane ())
            break;

          auto laned_track = dynamic_cast<LanedTrack *> (track);
          if (!laned_track->lanes_visible_)
            break;

          /* TODO: reenable this check */
          // TrackLane * lane = track->lanes[self->id.lane_pos];
          // z_return_if_fail (lane);

          /* set full rectangle */
          region_get_lane_full_rect (self, &full_rect);
        }
      else if (i == REGION_COUNTERPART_MAIN)
        {
          /* set full rectangle */
          full_rect = self->full_rect_;
        }

      /* if full rect of current region (main or
       * lane) is not visible, skip */
      if (!ui_rectangle_overlap (&full_rect, rect))
        {
          continue;
        }

      /* get draw (visible only) rectangle */
      arranger_object_get_draw_rectangle (self, rect, &full_rect, &draw_rect);

      /* get last rects */
      get_last_rects (
        self, ENUM_INT_TO_VALUE (RegionCounterpart, i), &last_full_rect,
        &last_draw_rect);

      /* skip if draw rect has 0 width */
      if (draw_rect.width == 0)
        {
          continue;
        }

      z_return_if_fail (draw_rect.width > 0 && draw_rect.width < 40000);

      /* make a rounded clip for the whole region */
      GskRoundedRect  rounded_rect;
      graphene_rect_t graphene_rect = Z_GRAPHENE_RECT_INIT (
        (float) full_rect.x, (float) full_rect.y, (float) full_rect.width,
        (float) full_rect.height);
      gsk_rounded_rect_init_from_rect (&rounded_rect, &graphene_rect, 6.f);
      gtk_snapshot_push_rounded_clip (snapshot, &rounded_rect);

      /* draw background */
      z_gtk_snapshot_append_color (snapshot, color, &graphene_rect);

      /* TODO apply stretch ratio */
      if (MW_TIMELINE->action == UiOverlayAction::STRETCHING_R)
        {
#if 0
          cairo_scale (
            cr, self->stretch_ratio, 1);
#endif
        }

      /* translate to the full rect */
      gtk_snapshot_save (snapshot);
      {
        graphene_point_t tmp_pt =
          Z_GRAPHENE_POINT_INIT ((float) full_rect.x, (float) full_rect.y);
        gtk_snapshot_translate (snapshot, &tmp_pt);
      }

      /* draw any remaining parts */
      std::visit (
        [&] (auto &&region) {
          using T = std::decay_t<decltype (region)>;
          if constexpr (std::is_same_v<T, MidiRegion>)
            draw_midi_region (region, snapshot, &full_rect, &draw_rect);
          else if constexpr (std::is_same_v<T, AutomationRegion>)
            draw_automation_region (region, snapshot, &full_rect, &draw_rect);
          else if constexpr (std::is_same_v<T, ChordRegion>)
            draw_chord_region (region, snapshot, &full_rect, &draw_rect);
          else if constexpr (std::is_same_v<T, AudioRegion>)
            draw_audio_region (
              region, snapshot, &full_rect, &draw_rect, false,
              (int) draw_rect.x - last_full_rect.x, (int) draw_rect.width);
        },
        convert_to_variant<RegionPtrVariant> (self));

      /* ---- draw applicable icons ---- */

#define DRAW_TEXTURE(name) \
  { \
    graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT ( \
      (float) (full_rect.width - (size + paddingh) * (icons_drawn + 1)), \
      (float) paddingv, (float) size, (float) size); \
    gtk_snapshot_append_texture (snapshot, arranger->name, &tmp_r); \
  } \
  icons_drawn++

      const int size = arranger->region_icon_texture_size;
      const int paddingh = 2;
      const int paddingv = 0;
      int       icons_drawn = 0;

#if 0
      /* draw link icon if has linked parent */
      if (self->id.link_group >= 0)
        {
          DRAW_TEXTURE (symbolic_link_texture);
        }
#endif

      /* draw musical mode icon if region is in
       * musical mode */
      if (self->is_audio ())
        {
          auto ar = dynamic_cast<AudioRegion *> (self);
          if (ar->get_musical_mode ())
            {
              DRAW_TEXTURE (music_note_16th_texture);
            }
        }

      /* if track is frozen, show frozen icon */
      if (track->frozen_)
        {
          DRAW_TEXTURE (fork_awesome_snowflake_texture);
        }

      /* draw loop icon if region is looped */
      if (self->is_looped ())
        {
          DRAW_TEXTURE (media_playlist_repeat_texture);
        }

#undef DRAW_TEXTURE

      /* remember rects */
      if (i == REGION_COUNTERPART_MAIN)
        {
          self->last_main_full_rect_ = full_rect;
          self->last_main_draw_rect_ = draw_rect;
        }
      else
        {
          self->last_lane_full_rect_ = full_rect;
          self->last_lane_draw_rect_ = draw_rect;
        }

      self->last_cache_time_ = g_get_monotonic_time ();

      draw_loop_points (self, snapshot, &full_rect, &draw_rect);

      if (arranger_object_should_show_cut_lines (self, arranger->alt_held))
        {
          draw_cut_line (self, snapshot, arranger, &full_rect, &draw_rect);
        }

      draw_name (self, snapshot, &full_rect, &draw_rect);

      /* draw anything on the bottom right part
       * (eg, BPM) */
      draw_bottom_right_part (self, snapshot, &full_rect, &draw_rect);

      /* restore translation */
      gtk_snapshot_restore (snapshot);

      /* draw border */
      const float border_width = 1.f;
      GdkRGBA     border_color = { 0.5f, 0.5f, 0.5f, 0.4f };
      float       border_widths[] = {
        border_width, border_width, border_width, border_width
      };
      GdkRGBA border_colors[] = {
        border_color, border_color, border_color, border_color
      };
      gtk_snapshot_append_border (
        snapshot, &rounded_rect, border_widths, border_colors);

      /* pop rounded clip */
      gtk_snapshot_pop (snapshot);
    }
}

/**
 * Returns the lane rectangle for the region.
 */
void
region_get_lane_full_rect (Region * self, GdkRectangle * rect)
{
  auto track = dynamic_cast<LanedTrack *> (self->get_track ());
  z_return_if_fail (track && track->lanes_visible_);

  std::visit (
    [&] (auto &&track) {
      auto &lane = track->lanes_[self->id_.lane_pos_];

      *rect = self->full_rect_;
      rect->y += lane->y_;
      rect->height = (int) lane->height_ - 1;
    },
    convert_to_variant<LanedTrackPtrVariant> (track));
}
