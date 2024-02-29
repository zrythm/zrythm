// SPDX-FileCopyrightText: © 2018-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include <math.h>

#include "dsp/audio_bus_track.h"
#include "dsp/audio_region.h"
#include "dsp/automation_region.h"
#include "dsp/channel.h"
#include "dsp/fade.h"
#include "dsp/instrument_track.h"
#include "dsp/tempo_track.h"
#include "dsp/track.h"
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
#include "utils/dsp.h"
#include "utils/flags.h"
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
recreate_pango_layouts (ZRegion * self, int width)
{
  ArrangerObject * obj = (ArrangerObject *) self;

  if (G_UNLIKELY (!PANGO_IS_LAYOUT (self->layout)))
    {
      PangoFontDescription * desc;
      self->layout = gtk_widget_create_pango_layout (
        GTK_WIDGET (arranger_object_get_arranger (obj)), NULL);
      desc = pango_font_description_from_string (REGION_NAME_FONT);
      pango_layout_set_font_description (self->layout, desc);
      pango_font_description_free (desc);
      pango_layout_set_ellipsize (self->layout, PANGO_ELLIPSIZE_END);
    }
  pango_layout_set_width (
    self->layout,
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
  float  dashesf[] = { 5.f };

  ArrangerObject * obj = (ArrangerObject *) self;

  ArrangerWidget * arranger = arranger_object_get_arranger (obj);

  Position tmp;
  double   loop_start_ticks = obj->loop_start_pos.ticks;
  double   loop_end_ticks = obj->loop_end_pos.ticks;
  z_warn_if_fail_cmp (loop_end_ticks, >, loop_start_ticks);
  double loop_ticks = arranger_object_get_loop_length_in_ticks (obj);
  double clip_start_ticks = obj->clip_start_pos.ticks;

  /* get x px for loop point */
  position_from_ticks (&tmp, loop_start_ticks - clip_start_ticks);
  int x_px = ui_pos_to_px_timeline (&tmp, 0);

  int vis_offset_x = draw_rect->x - full_rect->x;
  int vis_width = draw_rect->width;
  int full_width = full_rect->width;
  int full_height = full_rect->height;

  const int padding = 1;
  const int line_width = 1;

  /*GskRenderNode * loop_line_node = NULL;*/
  if (!arranger->loop_line_node)
    {
      arranger->loop_line_node =
        gsk_cairo_node_new (&GRAPHENE_RECT_INIT (0, 0, 3, 800));
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
      arranger->clip_start_line_node =
        gsk_cairo_node_new (&GRAPHENE_RECT_INIT (0, 0, 3, 800));

      cairo_t * cr =
        gsk_cairo_node_get_draw_context (arranger->clip_start_line_node);
      gdk_cairo_set_source_rgba (cr, &UI_COLORS->bright_green);
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
      gtk_snapshot_translate (
        snapshot, &GRAPHENE_POINT_INIT ((float) x_px - (float) padding, 0.f));
      gtk_snapshot_push_clip (
        snapshot,
        &GRAPHENE_RECT_INIT (
          0.f, 0.f, (float) line_width + (float) padding * 2.f,
          (float) full_height));
      gtk_snapshot_append_node (snapshot, clip_start_line_node);
      gtk_snapshot_pop (snapshot);
      gtk_snapshot_restore (snapshot);
    }

  /* draw loop points */
  int num_loops = arranger_object_get_num_loops (obj, 1);
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
          gtk_snapshot_append_color (
            snapshot, &color,
            &GRAPHENE_RECT_INIT (
              (float) x_px, 0.f, (float) line_width, (float) full_height));
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
  ZRegion *        self,
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
      arranger->cut_line_node =
        gsk_cairo_node_new (&GRAPHENE_RECT_INIT (0, 0, 3, 800));

      cairo_t * cr = gsk_cairo_node_get_draw_context (arranger->cut_line_node);
      gdk_cairo_set_source_rgba (cr, &UI_COLORS->dark_text);
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
  gtk_snapshot_translate (
    snapshot, &GRAPHENE_POINT_INIT ((float) x_px - (float) padding, 0.f));
  gtk_snapshot_push_clip (
    snapshot,
    &GRAPHENE_RECT_INIT (
      0.f, 0.f, (float) line_width + (float) padding * 2.f,
      (float) full_rect->height));
  gtk_snapshot_append_node (snapshot, cut_line_node);
  gtk_snapshot_pop (snapshot);
  gtk_snapshot_restore (snapshot);
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
  ArrangerObject * obj = (ArrangerObject *) self;

  Track * track = arranger_object_get_track (obj);

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

  int    num_loops = arranger_object_get_num_loops (obj, 1);
  double ticks_in_region = arranger_object_get_length_in_ticks (obj);
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
  double y_interval = MAX ((double) (max_val - min_val) + 1.0, 7.0);
  double y_note_size = 1.0 / y_interval;

  /* draw midi notes */
  double loop_end_ticks = obj->loop_end_pos.ticks;
  double loop_ticks = arranger_object_get_loop_length_in_ticks (obj);
  double clip_start_ticks = obj->clip_start_pos.ticks;

  int vis_offset_x = draw_rect->x - full_rect->x;
  int vis_offset_y = draw_rect->y - full_rect->y;
  int vis_width = draw_rect->width;
  int vis_height = draw_rect->height;
  /*int full_offset_x = full_rect->x;*/
  /*int full_offset_y = full_rect->y;*/
  int full_width = full_rect->width;
  int full_height = full_rect->height;

  bool is_looped = region_is_looped (self);

  for (int i = 0; i < self->num_midi_notes; i++)
    {
      MidiNote *       mn = self->midi_notes[i];
      ArrangerObject * mn_obj = (ArrangerObject *) mn;

      /* note muted */
      if (arranger_object_get_muted (mn_obj, false))
        continue;

      /* note not playable */
      if (position_is_after_or_equal (&mn_obj->pos, &obj->loop_end_pos))
        continue;

      /* note not playable if looped */
      if (
        is_looped && position_is_before (&mn_obj->pos, &obj->loop_start_pos)
        && position_is_before (&mn_obj->pos, &obj->clip_start_pos))
        continue;

      /* get ratio (0.0 - 1.0) on x where midi note
       * starts & ends */
      double mn_start_ticks = position_to_ticks (&mn_obj->pos);
      double mn_end_ticks = position_to_ticks (&mn_obj->end_pos);
      double tmp_start_ticks, tmp_end_ticks;

      for (int j = 0; j < num_loops; j++)
        {
          /* if note started before loop start
           * only draw it once */
          if (position_is_before (&mn_obj->pos, &obj->loop_start_pos) && j != 0)
            break;

          /* calculate draw endpoints */
          tmp_start_ticks = mn_start_ticks + loop_ticks * j;
          /* if should be clipped */
          if (position_is_after_or_equal (&mn_obj->end_pos, &obj->loop_end_pos))
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
          y_start = ((double) max_val - (double) mn->val) / y_interval;

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
              gtk_snapshot_append_color (
                snapshot, &color,
                &GRAPHENE_RECT_INIT (draw_x, draw_y, draw_width, draw_height));

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
  ArrangerObject * obj = (ArrangerObject *) self;

  int    num_loops = arranger_object_get_num_loops (obj, 1);
  double ticks_in_region = arranger_object_get_length_in_ticks (obj);
  double x_start;

  int full_width = full_rect->width;
  int full_height = full_rect->height;

  /* draw chords notes */
  /*double loop_end_ticks = obj->loop_end_pos.ticks;*/
  double        loop_ticks = arranger_object_get_loop_length_in_ticks (obj);
  double        clip_start_ticks = obj->clip_start_pos.ticks;
  ChordObject * co;
  for (int i = 0; i < self->num_chord_objects; i++)
    {
      co = self->chord_objects[i];
      ArrangerObject *        co_obj = (ArrangerObject *) co;
      const ChordDescriptor * descr = chord_object_get_chord_descriptor (co);

      /* get ratio (0.0 - 1.0) on x where chord
       * starts & ends */
      double co_start_ticks = co_obj->pos.ticks;
      double tmp_start_ticks;

      /* if before loop end */
      if (position_is_before (&co_obj->pos, &obj->loop_end_pos))
        {
          for (int j = 0; j < num_loops; j++)
            {
              /* if note started before loop start
               * only draw it once */
              if (
                position_is_before (&co_obj->pos, &obj->loop_start_pos)
                && j != 0)
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
              gtk_snapshot_append_color (
                snapshot, &color,
                &GRAPHENE_RECT_INIT (
                  (float) x_start, 0, 2.f, (float) full_height));

              /* draw name */
              char desc_str[100];
              chord_descriptor_to_string (descr, desc_str);
              gtk_snapshot_save (snapshot);
              gtk_snapshot_translate (
                snapshot,
                &GRAPHENE_POINT_INIT (
                  (float) x_start + 4.f, (float) full_height - 10.f));
              PangoLayout * layout = self->chords_layout;
              pango_layout_set_markup (layout, desc_str, -1);
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
  ZRegion *         self,
  GtkSnapshot *     snapshot,
  GdkRectangle *    full_rect,
  GdkRectangle *    draw_rect,
  int               cur_loop,
  AutomationPoint * ap,
  AutomationPoint * next_ap,
  double            ticks_in_region)
{
  ArrangerObject * obj = (ArrangerObject *) self;
  ArrangerObject * ap_obj = (ArrangerObject *) ap;
  ArrangerObject * next_ap_obj = (ArrangerObject *) next_ap;

  double loop_ticks = arranger_object_get_loop_length_in_ticks (obj);

  double ap_end_ticks = ap_obj->pos.ticks;
  if (next_ap)
    {
      ap_end_ticks = next_ap_obj->pos.ticks;
    }

  /* if both ap and next ap are before loop start don't
   * draw at all */
  if (
    position_is_before (&ap_obj->pos, &obj->loop_start_pos) && next_ap
    && position_is_before (&next_ap_obj->pos, &obj->loop_start_pos)
    && cur_loop != 0)
    {
      return true;
    }

  /* calculate draw endpoints */
  z_return_val_if_fail_cmp (loop_ticks, >, 0, true);
  z_return_val_if_fail_cmp (cur_loop, >=, 0, true);
  double global_start_ticks_after_loops =
    ap_obj->pos.ticks + loop_ticks * (double) cur_loop;

  /* if ap started before loop start only draw from the loop
   * start point */
  if (position_is_before (&ap_obj->pos, &obj->loop_start_pos) && cur_loop != 0)
    {
      global_start_ticks_after_loops +=
        obj->loop_start_pos.ticks - ap_obj->pos.ticks;
    }

  /* if should be clipped */
  double global_end_ticks_after_loops =
    ap_end_ticks + loop_ticks * (double) cur_loop;
  double global_end_ticks_after_loops_with_clipoff; /* same as above but the part
                                                       outside the loop/region
                                                       is clipped off */
  if (
    next_ap
    && position_is_after_or_equal (&next_ap_obj->pos, &obj->loop_end_pos))
    {
      global_end_ticks_after_loops_with_clipoff =
        obj->loop_end_pos.ticks + loop_ticks * (double) cur_loop;
    }
  else if (
    next_ap && position_is_after_or_equal (&next_ap_obj->pos, &obj->end_pos))
    {
      global_end_ticks_after_loops_with_clipoff =
        obj->end_pos.ticks + loop_ticks * (double) cur_loop;
    }
  else
    {
      global_end_ticks_after_loops_with_clipoff = global_end_ticks_after_loops;
    }

  /* adjust for clip start */
  global_start_ticks_after_loops -= obj->clip_start_pos.ticks;
  global_end_ticks_after_loops -= obj->clip_start_pos.ticks;
  global_end_ticks_after_loops_with_clipoff -= obj->clip_start_pos.ticks;

#if 0
  g_debug (
    "loop %d:, abs start ticks after loops %f | abs end ticks after loops %f | abs end ticks after loops w clipof %f",
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
  y_start_ratio = 1.0 - (double) ap->normalized_val;
  if (next_ap)
    {
      y_end_ratio = 1.0 - (double) next_ap->normalized_val;
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
  g_debug (
    "x start ratio in region %f full rect width %d x start in region %f | x end ratio in region %f x end in region %f",
    x_start_ratio_in_region, full_rect->width,
    x_start_in_region, x_end_ratio_in_region, x_end_in_region);
#endif

  /*  -- OK UP TO HERE --  */

  GdkRGBA color = object_fill_color;

  /* draw ap */
  if (x_start_in_region > 0.0 && x_start_in_region < full_rect->width)
    {
      int padding = 1;
      gtk_snapshot_append_color (
        snapshot, &color,
        &GRAPHENE_RECT_INIT (
          (float) (x_start_in_region - padding), (float) (y_start - padding),
          2.f * (float) padding, 2.f * (float) padding));
    }

  bool ret_val = false;

  /* draw curve */
  if (next_ap)
    {
      const int line_width = 2;
      double    curve_width = fabs (x_end_in_region - x_start_in_region);

      ArrangerWidget *     arranger = arranger_object_get_arranger (obj);
      const EditorSettings settings =
        arranger_widget_get_editor_setting_values (arranger);

      /* rectangle where origin is the top left of the
       * screen */
      GdkRectangle vis_rect = Z_GDK_RECTANGLE_INIT (
        settings.scroll_start_x - full_rect->x, -full_rect->y,
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
          /*g_debug ("clipped off %f", clipoff);*/
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
                - automation_point_get_normalized_value_in_curve (
                  ap, self,
                  CLAMP ((k - x_start_in_region) / curve_width, 0.0, 1.0));
          /*g_debug ("start from %f k %f x start in region %f ratio %f, ac width
           * %f, ap y %f", start_from, k, x_start_in_region, CLAMP ((k -
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
      gtk_snapshot_append_color (
        snapshot, &color,
        &GRAPHENE_RECT_INIT (
          (float) ap_loop_part_rect.x, (float) ap_loop_part_rect.y,
          (float) ap_loop_part_rect.width, (float) ap_loop_part_rect.height));
      gtk_snapshot_pop (snapshot); // stroke

      ap->last_settings_tl.normalized_val = ap->normalized_val;
      ap->last_settings_tl.curve_opts = ap->curve_opts;
      ap->last_settings_tl.draw_rect = ap_loop_part_rect;

    } /* endif have next ap */

  return ret_val;
}

/**
 * @param full_rect Region rectangle with absolute coordinates.
 */
static void
draw_automation_region (
  ZRegion *      self,
  GtkSnapshot *  snapshot,
  GdkRectangle * full_rect,
  GdkRectangle * draw_rect)
{
  ArrangerObject * obj = (ArrangerObject *) self;

  int    num_loops = arranger_object_get_num_loops (obj, 1);
  double ticks_in_region = arranger_object_get_length_in_ticks (obj);

  /* draw automation */
  for (int i = 0; i < self->num_aps; i++)
    {
      AutomationPoint * ap = self->aps[i];
      ArrangerObject *  ap_obj = (ArrangerObject *) ap;
      AutomationPoint * next_ap =
        automation_region_get_next_ap (self, ap, true, true);

      /* if before loop end */
      if (position_is_before (&ap_obj->pos, &obj->loop_end_pos))
        {
          for (int j = 0; j < num_loops; j++)
            {
              bool need_break = handle_loop (
                self, snapshot, full_rect, draw_rect, j, ap, next_ap,
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
  ZRegion *     self,
  GtkSnapshot * snapshot,
  int           vis_offset_x,
  int           vis_width,
  int           full_width,
  int           height)
{
  ArrangerObject * obj = (ArrangerObject *) self;
  Track *          track = arranger_object_get_track (obj);
  g_return_if_fail (track);

  /* set color */
  GdkRGBA color = { 0.2f, 0.2f, 0.2f, 0.6f };

  /* get fade in px */
  int fade_in_px = ui_pos_to_px_timeline (&obj->fade_in_pos, 0);

  /* get start px */
  int start_px = 0;

  /* get visible positions (where to draw) */
  int vis_fade_in_px = MIN (fade_in_px, vis_offset_x + vis_width);
  int vis_start_px = MAX (start_px, vis_offset_x);

  UiDetail detail = ui_get_detail_level ();

  /* use cairo if normal detail or higher */
  bool use_cairo = false;
  if (detail < UI_DETAIL_LOW)
    {
      use_cairo = true;
    }

  const int step = 1;
  if (vis_fade_in_px - vis_start_px > 0)
    {
      double local_px_diff = (double) (fade_in_px - start_px);

      /* create cairo node if necessary */
      cairo_t * cr = NULL;
      if (use_cairo)
        {
          graphene_rect_t grect = GRAPHENE_RECT_INIT (
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
              (double) (i - start_px) / local_px_diff, &obj->fade_in_opts, 1);

          float draw_y_val = (float) (val * height);

          if (use_cairo)
            {
              double next_val =
                1.0
                - fade_get_y_normalized (
                  (double) MIN (((i + step) - start_px), local_px_diff)
                    / local_px_diff,
                  &obj->fade_in_opts, 1);

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
                GRAPHENE_RECT_INIT ((float) i, 0, (float) step, draw_y_val);
              /*z_graphene_rect_print (&grect);*/
              gtk_snapshot_append_color (snapshot, &color, &grect);
            }
        }

      if (use_cairo)
        cairo_destroy (cr);

    } /* endif fade in visible */

  /* ---- fade out ---- */

  /* get fade out px */
  int fade_out_px = ui_pos_to_px_timeline (&obj->fade_out_pos, 0);

  /* get end px */
  int end_px = full_width;

  /* get visible positions (where to draw) */
  int visible_fade_out_px = MAX (fade_out_px, vis_offset_x);
  int visible_end_px = MIN (end_px, vis_offset_x + vis_width);

  if (visible_end_px - visible_fade_out_px > 0)
    {
      double local_px_diff = (double) (end_px - fade_out_px);

      /* create cairo node if necessary */
      cairo_t * cr = NULL;
      if (use_cairo)
        {
          graphene_rect_t grect = GRAPHENE_RECT_INIT (
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
          /* revert because cairo draws the other
           * way */
          double val =
            1.0
            - fade_get_y_normalized (
              (double) (i - fade_out_px) / local_px_diff, &obj->fade_out_opts, 0);

          if (use_cairo)
            {
              double tmp = (double) ((i + 1) - fade_out_px);
              double next_val =
                1.0
                - fade_get_y_normalized (
                  tmp > local_px_diff ? 1.0 : tmp / local_px_diff,
                  &obj->fade_out_opts, 0);

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
                GRAPHENE_RECT_INIT ((float) i, 0, 1, draw_y_val);
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
  ZRegion *      self,
  GtkSnapshot *  snapshot,
  GdkRectangle * full_rect,
  int            vis_offset_x,
  int            vis_width,
  int            full_height)
{
  g_return_if_fail (vis_width < 40000);

  AudioClip * clip = audio_region_get_clip (self);

  ArrangerObject * obj = (ArrangerObject *) self;

  double frames_per_tick = (double) AUDIO_ENGINE->frames_per_tick;
  if (region_get_musical_mode (self))
    {
      frames_per_tick *=
        (double) tempo_track_get_current_bpm (P_TEMPO_TRACK)
        / (double) clip->bpm;
    }
  double multiplier = frames_per_tick / MW_RULER->px_per_tick;

  UiDetail detail = ui_get_detail_level ();

  double increment = 1;
  double width = 1;
  switch (detail)
    {
    case UI_DETAIL_HIGH:
      /* snapshot does not work with midpoints */
      /*increment = 0.5;*/
      increment = 0.2;
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

  signed_frame_t loop_end_frames = math_round_double_to_signed_frame_t (
    obj->loop_end_pos.ticks * frames_per_tick);
  signed_frame_t loop_frames = math_round_double_to_signed_frame_t (
    arranger_object_get_loop_length_in_ticks (obj) * frames_per_tick);
  signed_frame_t clip_start_frames = math_round_double_to_signed_frame_t (
    obj->clip_start_pos.ticks * frames_per_tick);

  double         local_start_x = (double) vis_offset_x;
  double         local_end_x = local_start_x + (double) vis_width;
  signed_frame_t prev_frames = (signed_frame_t) (multiplier * local_start_x);
  signed_frame_t curr_frames = prev_frames;

  float * prev_min = object_new_n (clip->channels, float);
  float * prev_max = object_new_n (clip->channels, float);
  dsp_fill (prev_min, 0.5f, clip->channels);
  dsp_fill (prev_max, 0.5f, clip->channels);
  float * ch_min = object_new_n (clip->channels, float);
  float * ch_max = object_new_n (clip->channels, float);
  dsp_fill (ch_min, 0.5f, clip->channels);
  dsp_fill (ch_max, 0.5f, clip->channels);
  const double full_height_per_ch =
    (double) full_height / (double) clip->channels;
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

      if (prev_frames >= (signed_frame_t) clip->num_frames)
        {
          prev_frames = curr_frames;
          continue;
        }

      z_return_if_fail_cmp (curr_frames, >=, 0);
      signed_frame_t from = (MAX (0, prev_frames));
      signed_frame_t to = (MIN ((signed_frame_t) clip->num_frames, curr_frames));
      signed_frame_t frames_to_check = to - from;
      if (from + frames_to_check > (signed_frame_t) clip->num_frames)
        {
          frames_to_check = (signed_frame_t) clip->num_frames - from;
        }
      z_return_if_fail_cmp (from, <, (signed_frame_t) clip->num_frames);
      z_return_if_fail_cmp (
        from + frames_to_check, <=, (signed_frame_t) clip->num_frames);
      if (frames_to_check > 0)
        {
          size_t frames_to_check_unsigned = (size_t) frames_to_check;
          for (unsigned int k = 0; k < clip->channels; k++)
            {
              ch_min[k] = dsp_min (
                &clip->ch_frames[k][from], (size_t) frames_to_check_unsigned);
              ch_max[k] = dsp_max (
                &clip->ch_frames[k][from], (size_t) frames_to_check_unsigned);

              /* normalize */
              ch_min[k] = (ch_min[k] + 1.f) / 2.f;
              ch_max[k] = (ch_max[k] + 1.f) / 2.f;
            }
        }
      else
        {
          dsp_copy (prev_min, ch_min, clip->channels);
          dsp_copy (prev_max, ch_max, clip->channels);
        }

      for (unsigned int k = 0; k < clip->channels; k++)
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
              gtk_snapshot_translate (
                snapshot,
                &GRAPHENE_POINT_INIT (0, (float) k * (float) full_height_per_ch));
              gtk_snapshot_append_color (
                snapshot, &object_fill_color,
                &GRAPHENE_RECT_INIT (
                  (float) i, (float) local_min_y, (float) MAX (width, 1),
                  (float) MAX (local_max_y - local_min_y, 1)));
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
  ZRegion *      self,
  GtkSnapshot *  snapshot,
  GdkRectangle * full_rect,
  GdkRectangle * draw_rect)
{
  g_return_if_fail (self && self->escaped_name);

  int full_width = full_rect->width;

  /* draw dark bg behind text */
  recreate_pango_layouts (self, MIN (full_width, 800));
  PangoLayout * layout = self->layout;
  if (DEBUGGING)
    {
      char str[200];
      strcpy (str, self->escaped_name);
      ZRegion * clip_editor_region = clip_editor_get_region (CLIP_EDITOR);
      if (clip_editor_region == self)
        {
          strcat (str, " (CLIP EDITOR)");
        }
      pango_layout_set_text (layout, str, -1);
    }
  else
    {
      pango_layout_set_text (layout, self->escaped_name, -1);
    }
  PangoRectangle pangorect;

  /* get extents */
  pango_layout_get_pixel_extents (layout, NULL, &pangorect);
  int black_box_height = pangorect.height + 2 * REGION_NAME_BOX_PADDING;

  /* create a rounded clip */
  /*float degrees = G_PI / 180.f;*/
  float          radius = REGION_NAME_BOX_CURVINESS / 1.f;
  GskRoundedRect rounded_rect;
  gsk_rounded_rect_init (
    &rounded_rect,
    &GRAPHENE_RECT_INIT (
      0.f, 0.f, (float) (pangorect.width + REGION_NAME_PADDING_R),
      (float) black_box_height),
    &GRAPHENE_SIZE_INIT (0, 0), &GRAPHENE_SIZE_INIT (0, 0),
    &GRAPHENE_SIZE_INIT (radius, radius), &GRAPHENE_SIZE_INIT (0, 0));
  gtk_snapshot_push_rounded_clip (snapshot, &rounded_rect);

  /* fill bg color */
  GdkRGBA name_bg_color;
  gdk_rgba_parse (&name_bg_color, "#323232");
  name_bg_color.alpha = 0.8f;
  gtk_snapshot_append_color (snapshot, &name_bg_color, &rounded_rect.bounds);

  /* draw text */
  gtk_snapshot_save (snapshot);
  gtk_snapshot_translate (
    snapshot,
    &GRAPHENE_POINT_INIT (REGION_NAME_BOX_PADDING, REGION_NAME_BOX_PADDING));
  gtk_snapshot_append_layout (snapshot, layout, &Z_GDK_RGBA_INIT (1, 1, 1, 1));
  gtk_snapshot_restore (snapshot);

  /* pop rounded clip */
  gtk_snapshot_pop (snapshot);

  /* save rect */
  ArrangerObject * obj = (ArrangerObject *) self;
  obj->last_name_rect.x = 0;
  obj->last_name_rect.y = 0;
  obj->last_name_rect.width = pangorect.width + REGION_NAME_PADDING_R;
  obj->last_name_rect.height = pangorect.height + REGION_NAME_PADDING_R;
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
region_draw (ZRegion * self, GtkSnapshot * snapshot, GdkRectangle * rect)
{
  ArrangerObject * obj = (ArrangerObject *) self;

  ArrangerWidget * arranger = arranger_object_get_arranger (obj);

  Track * track = arranger_object_get_track (obj);
  g_return_if_fail (IS_TRACK_AND_NONNULL (track));

  /* set color */
  GdkRGBA color;
  if (self->use_color)
    {
      color = self->color;
    }
  else if (track)
    color = track->color;
  else
    {
      color.red = 1;
      color.green = 0;
      color.blue = 0;
      color.alpha = 1;
    }
  ui_get_arranger_object_color (
    &color, arranger->hovered_object == obj, region_is_selected (self),
    /* FIXME */
    false, arranger_object_get_muted (obj, true) || track->frozen);

  GdkRectangle draw_rect;
  GdkRectangle last_draw_rect, last_full_rect;
  GdkRectangle full_rect = obj->full_rect;
  for (int i = REGION_COUNTERPART_MAIN; i <= REGION_COUNTERPART_LANE; i++)
    {
      if (i == REGION_COUNTERPART_LANE)
        {
          if (!region_type_has_lane (self->id.type))
            break;

          if (!track->lanes_visible)
            break;

          TrackLane * lane = track->lanes[self->id.lane_pos];
          g_return_if_fail (lane);

          /* set full rectangle */
          region_get_lane_full_rect (self, &full_rect);
        }
      else if (i == REGION_COUNTERPART_MAIN)
        {
          /* set full rectangle */
          full_rect = obj->full_rect;
        }

      /* if full rect of current region (main or
       * lane) is not visible, skip */
      if (!ui_rectangle_overlap (&full_rect, rect))
        {
          continue;
        }

      /* get draw (visible only) rectangle */
      arranger_object_get_draw_rectangle (obj, rect, &full_rect, &draw_rect);

      /* get last rects */
      get_last_rects (self, i, &last_full_rect, &last_draw_rect);

      /* skip if draw rect has 0 width */
      if (draw_rect.width == 0)
        {
          continue;
        }

      g_return_if_fail (draw_rect.width > 0 && draw_rect.width < 40000);

      /* make a rounded clip for the whole region */
      GskRoundedRect  rounded_rect;
      graphene_rect_t graphene_rect = GRAPHENE_RECT_INIT (
        (float) full_rect.x, (float) full_rect.y, (float) full_rect.width,
        (float) full_rect.height);
      gsk_rounded_rect_init_from_rect (&rounded_rect, &graphene_rect, 6.f);
      gtk_snapshot_push_rounded_clip (snapshot, &rounded_rect);

      /* draw background */
      gtk_snapshot_append_color (snapshot, &color, &graphene_rect);

      /* TODO apply stretch ratio */
      if (MW_TIMELINE->action == UI_OVERLAY_ACTION_STRETCHING_R)
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
        &GRAPHENE_POINT_INIT ((float) full_rect.x, (float) full_rect.y));

      /* draw any remaining parts */
      switch (self->id.type)
        {
        case REGION_TYPE_MIDI:
          draw_midi_region (self, snapshot, &full_rect, &draw_rect);
          break;
        case REGION_TYPE_AUTOMATION:
          draw_automation_region (self, snapshot, &full_rect, &draw_rect);
          break;
        case REGION_TYPE_CHORD:
          draw_chord_region (self, snapshot, &full_rect, &draw_rect);
          break;
        case REGION_TYPE_AUDIO:
          draw_audio_region (
            self, snapshot, &full_rect, &draw_rect, false,
            (int) draw_rect.x - last_full_rect.x, (int) draw_rect.width);
          break;
        default:
          break;
        }

        /* ---- draw applicable icons ---- */

#define DRAW_TEXTURE(name) \
  gtk_snapshot_append_texture ( \
    snapshot, arranger->name, \
    &GRAPHENE_RECT_INIT ( \
      (float) (full_rect.width - (size + paddingh) * (icons_drawn + 1)), \
      (float) paddingv, (float) size, (float) size)); \
  icons_drawn++

      const int size = arranger->region_icon_texture_size;
      const int paddingh = 2;
      const int paddingv = 0;
      int       icons_drawn = 0;

      /* draw link icon if has linked parent */
      if (self->id.link_group >= 0)
        {
          DRAW_TEXTURE (symbolic_link_texture);
        }

      /* draw musical mode icon if region is in
       * musical mode */
      if (self->id.type == REGION_TYPE_AUDIO && region_get_musical_mode (self))
        {
          DRAW_TEXTURE (music_note_16th_texture);
        }

      /* if track is frozen, show frozen icon */
      if (track->frozen)
        {
          DRAW_TEXTURE (fork_awesome_snowflake_texture);
        }

      /* draw loop icon if region is looped */
      if (region_is_looped (self))
        {
          DRAW_TEXTURE (media_playlist_repeat_texture);
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

      self->last_cache_time = g_get_monotonic_time ();

      draw_loop_points (self, snapshot, &full_rect, &draw_rect);

      if (arranger_object_should_show_cut_lines (obj, arranger->alt_held))
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

  self->last_positions_obj = *obj;
  obj->use_cache = true;
}

/**
 * Returns the lane rectangle for the region.
 */
void
region_get_lane_full_rect (ZRegion * self, GdkRectangle * rect)
{
  ArrangerObject * obj = (ArrangerObject *) self;

  Track * track = arranger_object_get_track (obj);
  g_return_if_fail (track && track->lanes_visible);

  TrackLane * lane = track->lanes[self->id.lane_pos];
  g_return_if_fail (lane);

  *rect = obj->full_rect;
  rect->y += lane->y;
  rect->height = (int) lane->height - 1;
}
