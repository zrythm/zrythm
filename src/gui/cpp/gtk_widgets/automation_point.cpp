// SPDX-FileCopyrightText: © 2018-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/dsp/automation_region.h"
#include "common/dsp/track.h"
#include "common/utils/gtk.h"
#include "common/utils/math.h"
#include "common/utils/objects.h"
#include "common/utils/ui.h"
#include "gui/cpp/backend/project.h"
#include "gui/cpp/backend/settings/g_settings_manager.h"
#include "gui/cpp/gtk_widgets/arranger.h"
#include "gui/cpp/gtk_widgets/arranger_object.h"
#include "gui/cpp/gtk_widgets/automation_point.h"
#include "gui/cpp/gtk_widgets/bot_bar.h"
#include "gui/cpp/gtk_widgets/main_window.h"
#include "gui/cpp/gtk_widgets/zrythm_app.h"

bool
automation_point_settings_changed (
  const AutomationPoint * self,
  const GdkRectangle *    draw_rect,
  bool                    timeline)
{
  const auto &last_settings =
    timeline ? self->last_settings_tl_ : self->last_settings_;
  return !gdk_rectangle_equal (&last_settings.draw_rect, draw_rect)
         || last_settings.curve_opts != self->curve_opts_
         || !math_floats_equal (
           last_settings.normalized_val, self->normalized_val_);
}

void
automation_point_draw (
  AutomationPoint * ap,
  GtkSnapshot *     snapshot,
  GdkRectangle *    rect,
  PangoLayout *     layout)
{
  const auto * region = ap->get_region ();
  const auto * next_ap = region->get_next_ap (*ap, true, true);
  auto *       arranger = region->get_arranger ();

  auto * track = ap->get_track ();
  z_return_if_fail (track);

  /* get color */
  auto color = Color::get_arranger_object_color (
    track->color_, ap->is_hovered (), ap->is_selected (), false, false);

  GdkRectangle draw_rect;
  arranger_object_get_draw_rectangle (ap, rect, &ap->full_rect_, &draw_rect);

  GskRenderNode * cr_node = nullptr;
  if (automation_point_settings_changed (ap, &draw_rect, false))
    {
      graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (
        0.f, 0.f, (float) draw_rect.width + 3.f, (float) draw_rect.height + 3.f);
      cr_node = gsk_cairo_node_new (&tmp_r);

      object_free_w_func_and_null (gsk_render_node_unref, ap->cairo_node_);
    }
  else
    {
      cr_node = ap->cairo_node_;
      gtk_snapshot_save (snapshot);
      {
        graphene_point_t tmp_pt = Z_GRAPHENE_POINT_INIT (
          (float) draw_rect.x - 1.f, (float) draw_rect.y - 1.f);
        gtk_snapshot_translate (snapshot, &tmp_pt);
      }
      gtk_snapshot_append_node (snapshot, cr_node);
      gtk_snapshot_restore (snapshot);

      return;
    }

  auto * cr = gsk_cairo_node_get_draw_context (cr_node);
  cairo_save (cr);
  cairo_translate (cr, -(draw_rect.x - 1), -(draw_rect.y - 1));

  auto color_rgba = color.to_gdk_rgba ();
  gdk_cairo_set_source_rgba (cr, &color_rgba);
  cairo_set_line_width (cr, 2);

  /* fvalue can be equal in non-float automation even though there is a curve -
   * fvalue is used during processing. use the normalized value instead */
  bool upslope = next_ap && ap->normalized_val_ < next_ap->normalized_val_;

  if (next_ap)
    {
      /* TODO use cairo translate to add padding */

      /* ignore the space between the center
       * of each point and the edges (2 of them
       * so a full AP_WIDGET_POINT_SIZE) */
      double width_for_curve =
        ap->full_rect_.width - (double) AP_WIDGET_POINT_SIZE;
      double height_for_curve = ap->full_rect_.height - AP_WIDGET_POINT_SIZE;

      double draw_offset = draw_rect.x - ap->full_rect_.x;
      z_return_if_fail (draw_offset >= 0);

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
            - ap->get_normalized_value_in_curve (
              nullptr,
              std::clamp<double> ((l + step) / width_for_curve, 0.0, 1.0));
          next_y *= height_for_curve;

          if (math_doubles_equal (l, 0.0))
            {
              this_y =
                /* in pixels, higher values are lower */
                1.0 - ap->get_normalized_value_in_curve (nullptr, 0.0);
              this_y *= height_for_curve;
              cairo_move_to (
                cr, (l + AP_WIDGET_POINT_SIZE / 2 + ap->full_rect_.x),
                (this_y + AP_WIDGET_POINT_SIZE / 2 + ap->full_rect_.y));
            }
          else
            {
              cairo_line_to (
                cr, (l + step + AP_WIDGET_POINT_SIZE / 2 + ap->full_rect_.x),
                (next_y + AP_WIDGET_POINT_SIZE / 2 + ap->full_rect_.y));
            }
          this_y = next_y;

          has_drawing = true;
        }

      /* nothing drawn - just a vertical line
       * needed */
      if (G_UNLIKELY (!has_drawing))
        {
          this_y = 1.0 - ap->get_normalized_value_in_curve (nullptr, 0.0);
          this_y *= height_for_curve;
          double next_y = 1.0 - ap->get_normalized_value_in_curve (nullptr, 1.0);
          next_y *= height_for_curve;
          cairo_move_to (
            cr, (draw_offset + AP_WIDGET_POINT_SIZE / 2 + ap->full_rect_.x),
            (this_y + AP_WIDGET_POINT_SIZE / 2 + ap->full_rect_.y));
          cairo_line_to (
            cr,
            (draw_offset + step + AP_WIDGET_POINT_SIZE / 2 + ap->full_rect_.x),
            (next_y + AP_WIDGET_POINT_SIZE / 2 + ap->full_rect_.y));
        }

      cairo_stroke (cr);
    } /* endif have next_ap */

  /* draw circle */
  cairo_arc (
    cr, ap->full_rect_.x + AP_WIDGET_POINT_SIZE / 2,
    upslope
      ? ((ap->full_rect_.y + ap->full_rect_.height) - AP_WIDGET_POINT_SIZE / 2)
      : (ap->full_rect_.y + AP_WIDGET_POINT_SIZE / 2),
    AP_WIDGET_POINT_SIZE / 2, 0, 2 * G_PI);
  cairo_set_source_rgba (cr, 0, 0, 0, 1);
  cairo_fill_preserve (cr);
  color_rgba = color.to_gdk_rgba ();
  gdk_cairo_set_source_rgba (cr, &color_rgba);
  cairo_stroke (cr);

  if (DEBUGGING)
    {
      std::string text = fmt::format (
        "{}/{} ({})", ap->index_, region->aps_.size (), ap->normalized_val_);
      if (arranger->action != UiOverlayAction::None && !ap->transient_)
        {
          text += " - t";
        }
      cairo_set_source_rgba (cr, 1, 1, 1, 1);
      cairo_move_to (
        cr, (ap->full_rect_.x + AP_WIDGET_POINT_SIZE / 2),
        upslope
          ? ((ap->full_rect_.y + ap->full_rect_.height) - AP_WIDGET_POINT_SIZE / 2)
          : (ap->full_rect_.y + AP_WIDGET_POINT_SIZE / 2));
      cairo_show_text (cr, text.c_str ());
    }
  else if (
    g_settings_get_boolean (S_UI, "show-automation-values")
    && !(arranger->action != UiOverlayAction::None && !ap->transient_))
    {
#if ZRYTHM_TARGET_VER_MAJ > 1
      std::string text = fmt::format ("{}", ap.fvalue_);
      cairo_set_source_rgba (cr, 1, 1, 1, 1);
      z_cairo_draw_text_full (
        cr, GTK_WIDGET (arranger), layout, text.c_str (),
        (obj.full_rect_.x + AP_WIDGET_POINT_SIZE / 2),
        upslope
          ? ((obj.full_rect_.y + obj.full_rect_.height) - AP_WIDGET_POINT_SIZE / 2)
          : (obj.full_rect_.y + AP_WIDGET_POINT_SIZE / 2));
#endif
    }

  cairo_restore (cr);
  cairo_destroy (cr);

  gtk_snapshot_save (snapshot);
  {
    graphene_point_t tmp_pt = Z_GRAPHENE_POINT_INIT (
      (float) draw_rect.x - 1.f, (float) draw_rect.y - 1.f);
    gtk_snapshot_translate (snapshot, &tmp_pt);
  }
  gtk_snapshot_append_node (snapshot, cr_node);
  gtk_snapshot_restore (snapshot);

  ap->last_settings_.normalized_val = ap->normalized_val_;
  ap->last_settings_.curve_opts = ap->curve_opts_;
  ap->last_settings_.draw_rect = draw_rect;
  ap->cairo_node_ = cr_node;
}

bool
automation_point_is_point_hit (const AutomationPoint &self, double x, double y)
{
  const auto &obj = static_cast<const ArrangerObject &> (self);

  bool x_ok = (obj.full_rect_.x - x) < AP_WIDGET_POINT_SIZE;

  if (y < 0)
    {
      return x_ok;
    }
  else
    {
      bool curves_up = self.curves_up ();
      if (
        x_ok && curves_up
          ? (obj.full_rect_.y + obj.full_rect_.height) - y < AP_WIDGET_POINT_SIZE
          : y - obj.full_rect_.y < AP_WIDGET_POINT_SIZE)
        return true;
    }

  return false;
}

bool
automation_point_is_curve_hit (
  const AutomationPoint &self,
  double                 x,
  double                 y,
  double                 delta_from_curve)
{
  const auto &obj = static_cast<const ArrangerObject &> (self);

  double curve_val =
    1.0
    - self.get_normalized_value_in_curve (
      nullptr, (x - obj.full_rect_.x) / obj.full_rect_.width);
  curve_val = obj.full_rect_.y + curve_val * obj.full_rect_.height;

  return std::abs (curve_val - y) <= delta_from_curve;
}
