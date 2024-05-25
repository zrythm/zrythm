// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/audio_bus_track.h"
#include "dsp/automation_region.h"
#include "dsp/automation_track.h"
#include "dsp/channel.h"
#include "dsp/instrument_track.h"
#include "dsp/track.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/arranger_object.h"
#include "gui/widgets/automation_point.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/ruler.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/cairo.h"
#include "utils/gtk.h"
#include "utils/math.h"
#include "utils/objects.h"
#include "utils/ui.h"
#include "zrythm_app.h"

/**
 * Returns whether the cached render node for @ref
 * self needs to be invalidated.
 */
bool
automation_point_settings_changed (
  const AutomationPoint * self,
  const GdkRectangle *    draw_rect,
  bool                    timeline)
{
  const AutomationPointDrawSettings * last_settings =
    timeline ? &self->last_settings_tl : &self->last_settings;
  bool same =
    gdk_rectangle_equal (&last_settings->draw_rect, draw_rect)
    && curve_options_are_equal (&last_settings->curve_opts, &self->curve_opts)
    && math_floats_equal (last_settings->normalized_val, self->normalized_val);

  return !same;
}

void
automation_point_draw (
  AutomationPoint * ap,
  GtkSnapshot *     snapshot,
  GdkRectangle *    rect,
  PangoLayout *     layout)
{
  ArrangerObject *  obj = (ArrangerObject *) ap;
  ZRegion *         region = arranger_object_get_region (obj);
  AutomationPoint * next_ap =
    automation_region_get_next_ap (region, ap, true, true);
  ArrangerObject * next_obj = (ArrangerObject *) next_ap;
  ArrangerWidget * arranger = arranger_object_get_arranger (obj);

  Track * track = arranger_object_get_track ((ArrangerObject *) ap);
  g_return_if_fail (track);

  /* get color */
  GdkRGBA color = track->color;
  ui_get_arranger_object_color (
    &color, arranger->hovered_object == obj, automation_point_is_selected (ap),
    false, false);

  GdkRectangle draw_rect;
  arranger_object_get_draw_rectangle (obj, rect, &obj->full_rect, &draw_rect);

#if 0
  gtk_snapshot_append_color (
    snapshot, &Z_GDK_RGBA_INIT (1, 1, 1, 0.2),
    &Z_GRAPHENE_RECT_INIT (
      obj->full_rect.x, obj->full_rect.y,
      obj->full_rect.width,
      obj->full_rect.height));
#endif

  GskRenderNode * cr_node = NULL;
  if (automation_point_settings_changed (ap, &draw_rect, false))
    {
      cr_node = gsk_cairo_node_new (&Z_GRAPHENE_RECT_INIT (
        0.f, 0.f, (float) draw_rect.width + 3.f,
        (float) draw_rect.height + 3.f));

      object_free_w_func_and_null (gsk_render_node_unref, ap->cairo_node);
    }
  else
    {
      cr_node = ap->cairo_node;
      gtk_snapshot_save (snapshot);
      {
        graphene_point_t tmp_pt = GRAPHENE_POINT_INIT (
          (float) draw_rect.x - 1.f, (float) draw_rect.y - 1.f);
        gtk_snapshot_translate (snapshot, &tmp_pt);
      }
      gtk_snapshot_append_node (snapshot, cr_node);
      gtk_snapshot_restore (snapshot);

      return;
    }

  cairo_t * cr = gsk_cairo_node_get_draw_context (cr_node);
  cairo_save (cr);
  cairo_translate (cr, -(draw_rect.x - 1), -(draw_rect.y - 1));

  gdk_cairo_set_source_rgba (cr, &color);
  cairo_set_line_width (cr, 2);

  /* fvalue can be equal in non-float automation even though there is a curve -
   * fvalue is used during processing. use the normalized value instead */
  bool upslope = next_ap && ap->normalized_val < next_ap->normalized_val;
  (void) next_obj;

  if (next_ap)
    {
      /* TODO use cairo translate to add padding */

      /* ignore the space between the center
       * of each point and the edges (2 of them
       * so a full AP_WIDGET_POINT_SIZE) */
      double width_for_curve =
        obj->full_rect.width - (double) AP_WIDGET_POINT_SIZE;
      double height_for_curve = obj->full_rect.height - AP_WIDGET_POINT_SIZE;

      double draw_offset = draw_rect.x - obj->full_rect.x;
      g_return_if_fail (draw_offset >= 0.0);

      double step = 0.1;
      double this_y = 0;
      double draw_until =
        /* FIXME this will draw all the way
         * until the end of the automation point,
         * even if it is off-screen by many
         * pixels
         * TODO only draw up to where needed
         *
         * it is possible that cairo is smart about
         * it and doesn't exceed the surface size */
        width_for_curve - step / 2.0;
      bool has_drawing = false;
      for (double l = draw_offset; l <= draw_until; l += step)
        {
          double next_y =
            /* in pixels, higher values are lower */
            1.0
            - automation_point_get_normalized_value_in_curve (
              ap, NULL, CLAMP ((l + step) / width_for_curve, 0.0, 1.0));
          next_y *= height_for_curve;

          if (G_UNLIKELY (math_doubles_equal (l, 0.0)))
            {
              this_y =
                /* in pixels, higher values are lower */
                1.0
                - automation_point_get_normalized_value_in_curve (ap, NULL, 0.0);
              this_y *= height_for_curve;
              cairo_move_to (
                cr, (l + AP_WIDGET_POINT_SIZE / 2 + obj->full_rect.x),
                (this_y + AP_WIDGET_POINT_SIZE / 2 + obj->full_rect.y));
            }
          else
            {
              cairo_line_to (
                cr, (l + step + AP_WIDGET_POINT_SIZE / 2 + obj->full_rect.x),
                (next_y + AP_WIDGET_POINT_SIZE / 2 + obj->full_rect.y));
            }
          this_y = next_y;

          has_drawing = true;
        }

      /* nothing drawn - just a vertical line
       * needed */
      if (G_UNLIKELY (!has_drawing))
        {
          this_y =
            1.0 - automation_point_get_normalized_value_in_curve (ap, NULL, 0.0);
          this_y *= height_for_curve;
          double next_y =
            1.0 - automation_point_get_normalized_value_in_curve (ap, NULL, 1.0);
          next_y *= height_for_curve;
          cairo_move_to (
            cr, (draw_offset + AP_WIDGET_POINT_SIZE / 2 + obj->full_rect.x),
            (this_y + AP_WIDGET_POINT_SIZE / 2 + obj->full_rect.y));
          cairo_line_to (
            cr,
            (draw_offset + step + AP_WIDGET_POINT_SIZE / 2 + obj->full_rect.x),
            (next_y + AP_WIDGET_POINT_SIZE / 2 + obj->full_rect.y));
        }

      cairo_stroke (cr);
    } /* endif have next_ap */

  /* draw circle */
  cairo_arc (
    cr, obj->full_rect.x + AP_WIDGET_POINT_SIZE / 2,
    upslope
      ? ((obj->full_rect.y + obj->full_rect.height) - AP_WIDGET_POINT_SIZE / 2)
      : (obj->full_rect.y + AP_WIDGET_POINT_SIZE / 2),
    AP_WIDGET_POINT_SIZE / 2, 0, 2 * G_PI);
  cairo_set_source_rgba (cr, 0, 0, 0, 1);
  cairo_fill_preserve (cr);
  gdk_cairo_set_source_rgba (cr, &color);
  cairo_stroke (cr);

  if (DEBUGGING)
    {
      char text[500];
      sprintf (
        text, "%d/%d (%f)", ap->index, region->num_aps,
        (double) ap->normalized_val);
      if (arranger->action != UI_OVERLAY_ACTION_NONE && !obj->transient)
        {
          strcat (text, " - t");
        }
      cairo_set_source_rgba (cr, 1, 1, 1, 1);
      cairo_move_to (
        cr, (obj->full_rect.x + AP_WIDGET_POINT_SIZE / 2),
        upslope
          ? ((obj->full_rect.y + obj->full_rect.height) - AP_WIDGET_POINT_SIZE / 2)
          : (obj->full_rect.y + AP_WIDGET_POINT_SIZE / 2));
      cairo_show_text (cr, text);
    }
  else if (
    g_settings_get_boolean (S_UI, "show-automation-values")
    && !(arranger->action != UI_OVERLAY_ACTION_NONE && !obj->transient))
    {
#if ZRYTHM_TARGET_VER_MAJ > 1
      char text[50];
      sprintf (text, "%f", (double) ap->fvalue);
      cairo_set_source_rgba (cr, 1, 1, 1, 1);
      z_cairo_draw_text_full (
        cr, GTK_WIDGET (arranger), layout, text,
        (obj->full_rect.x + AP_WIDGET_POINT_SIZE / 2),
        upslope
          ? ((obj->full_rect.y + obj->full_rect.height) - AP_WIDGET_POINT_SIZE / 2)
          : (obj->full_rect.y + AP_WIDGET_POINT_SIZE / 2));
#endif
    }

  cairo_restore (cr);
  cairo_destroy (cr);

  gtk_snapshot_save (snapshot);
  {
    graphene_point_t tmp_pt = GRAPHENE_POINT_INIT (
      (float) draw_rect.x - 1.f, (float) draw_rect.y - 1.f);
    gtk_snapshot_translate (snapshot, &tmp_pt);
  }
  gtk_snapshot_append_node (snapshot, cr_node);
  gtk_snapshot_restore (snapshot);

  ap->last_settings.normalized_val = ap->normalized_val;
  ap->last_settings.curve_opts = ap->curve_opts;
  ap->last_settings.draw_rect = draw_rect;
  ap->cairo_node = cr_node;
}

/**
 * Returns if the automation point (circle) is hit.
 *
 * This function assumes that the point is already
 * inside the full rect of the automation point.
 *
 * @param x X, or -1 to not check x.
 * @param y Y, or -1 to not check y.
 *
 * @note the transient is also checked.
 */
bool
automation_point_is_point_hit (AutomationPoint * self, double x, double y)
{
  ArrangerObject * obj = (ArrangerObject *) self;

  bool x_ok = (obj->full_rect.x - x) < AP_WIDGET_POINT_SIZE;

  if (y < 0)
    {
      return x_ok;
    }
  else
    {
      bool curves_up = automation_point_curves_up (self);
      if (
        x_ok && curves_up
          ? (obj->full_rect.y + obj->full_rect.height) - y < AP_WIDGET_POINT_SIZE
          : y - obj->full_rect.y < AP_WIDGET_POINT_SIZE)
        return true;
    }

  return false;
}

/**
 * Returns if the automation curve is hit.
 *
 * This function assumes that the point is already
 * inside the full rect of the automation point.
 *
 * @param x X in global coordinates.
 * @param y Y in global coordinates.
 * @param delta_from_curve Allowed distance from the
 *   curve.
 *
 * @note the transient is also checked.
 */
bool
automation_point_is_curve_hit (
  AutomationPoint * self,
  double            x,
  double            y,
  double            delta_from_curve)
{
  ArrangerObject * obj = (ArrangerObject *) self;

  /* subtract from 1 because cairo draws in the
   * opposite direction */
  double curve_val =
    1.0
    - automation_point_get_normalized_value_in_curve (
      self, NULL, (x - obj->full_rect.x) / obj->full_rect.width);
  curve_val = obj->full_rect.y + curve_val * obj->full_rect.height;

  if (fabs (curve_val - y) <= delta_from_curve)
    return true;

  return false;
}
