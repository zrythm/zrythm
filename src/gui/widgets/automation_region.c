// SPDX-FileCopyrightText: © 2018-2019 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <math.h>

#include "dsp/automation_region.h"
#include "dsp/channel.h"
#include "dsp/instrument_track.h"
#include "dsp/midi_note.h"
#include "dsp/track.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/automation_region.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/region.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline_arranger.h"
#include "utils/cairo.h"
#include "utils/math.h"

G_DEFINE_TYPE (
  AutomationRegionWidget,
  automation_region_widget,
  REGION_WIDGET_TYPE)

#define Y_PADDING 6.f
#define Y_HALF_PADDING 3.f

static gboolean
automation_region_draw_cb (
  GtkWidget *              widget,
  cairo_t *                cr,
  AutomationRegionWidget * self)
{
  ARRANGER_OBJECT_WIDGET_GET_PRIVATE (self);

  GdkRectangle rect;
  gdk_cairo_get_clip_rectangle (cr, &rect);

  /* not cached, redraw */
  if (arranger_object_widget_should_redraw (
        (ArrangerObjectWidget *) self, &rect))
    {
      REGION_WIDGET_GET_PRIVATE (self);

      int i, j;

      GtkStyleContext * context =
        gtk_widget_get_style_context (widget);

      int width = gtk_widget_get_width (widget);
      int height = gtk_widget_get_height (widget);

      z_cairo_reset_caches (
        &ao_prv->cached_cr, &ao_prv->cached_surface, width,
        height, cr);

      gtk_render_background (
        context, ao_prv->cached_cr, 0, 0, width, height);

      /* draw background rectangle */
      region_widget_draw_background (
        (RegionWidget *) self, widget, ao_prv->cached_cr,
        &rect);

      /* draw loop dashes */
      region_widget_draw_loop_points (
        (RegionWidget *) self, widget, ao_prv->cached_cr,
        &rect);

      cairo_set_source_rgba (ao_prv->cached_cr, 1, 1, 1, 1);

      ZRegion * r = rw_prv->region;
      ZRegion * main_region = region_get_main (r);
      int       num_loops = arranger_object_get_num_loops (
        (ArrangerObject *) r, 1);
      long ticks_in_region =
        arranger_object_get_length_in_ticks (
          (ArrangerObject *) r);
      double x_start, y_start, x_end, y_end;

      /* draw automation */
      long loop_end_ticks =
        ((ArrangerObject *) r)->loop_end_pos.total_ticks;
      long loop_ticks =
        arranger_object_get_loop_length_in_ticks (
          (ArrangerObject *) r);
      long clip_start_ticks =
        ((ArrangerObject *) r)->clip_start_pos.total_ticks;
      AutomationPoint *ap, *next_ap;
      for (i = 0; i < main_region->num_aps; i++)
        {
          ap = main_region->aps[i];
          next_ap =
            automation_region_get_next_ap (main_region, ap);
          ArrangerObject * r_obj = (ArrangerObject *) r;
          ArrangerObject * ap_obj = (ArrangerObject *) ap;
          ArrangerObject * next_ap_obj =
            (ArrangerObject *) next_ap;

          ap = (AutomationPoint *)
            arranger_object_get_visible_counterpart (ap_obj);

          /* get ratio (0.0 - 1.0) on x where midi
           * note starts & ends */
          long ap_start_ticks = ap_obj->pos.total_ticks;
          long ap_end_ticks = ap_start_ticks;
          if (next_ap)
            {
              ap_end_ticks = next_ap_obj->pos.total_ticks;
            }
          long tmp_start_ticks, tmp_end_ticks;

          /* if before loop end */
          if (position_is_before (
                &ap_obj->pos, &r_obj->loop_end_pos))
            {
              for (j = 0; j < num_loops; j++)
                {
                  /* if ap started before loop start
                   * only draw it once */
                  if (
                    position_is_before (
                      &ap_obj->pos, &r_obj->loop_start_pos)
                    && j != 0)
                    break;

                  /* calculate draw endpoints */
                  tmp_start_ticks =
                    ap_start_ticks + loop_ticks * (long) j;

                  /* if should be clipped */
                  if (
                    next_ap
                    && position_is_after_or_equal (
                      &next_ap_obj->pos, &r_obj->loop_end_pos))
                    tmp_end_ticks =
                      loop_end_ticks + loop_ticks * (long) j;
                  else
                    tmp_end_ticks =
                      ap_end_ticks + loop_ticks * (long) j;

                  /* adjust for clip start */
                  tmp_start_ticks -= clip_start_ticks;
                  tmp_end_ticks -= clip_start_ticks;

                  x_start =
                    (double) tmp_start_ticks
                    / (double) ticks_in_region;
                  x_end =
                    (double) tmp_end_ticks
                    / (double) ticks_in_region;

                  /* get ratio (0.0 - 1.0) on y where
                   * midi note is */
                  y_start = 1.0 - (double) ap->normalized_val;
                  if (next_ap)
                    y_end =
                      1.0 - (double) next_ap->normalized_val;
                  else
                    y_end = y_start;

                  double x_start_real = x_start * width;
                  /*double x_end_real =*/
                  /*x_end * width;*/
                  double y_start_real = y_start * height;
                  double y_end_real = y_end * height;

                  /* draw ap */
                  int padding = 1;
                  cairo_rectangle (
                    ao_prv->cached_cr, x_start_real - padding,
                    y_start_real - padding, 2 * padding,
                    2 * padding);
                  cairo_fill (ao_prv->cached_cr);

                  /* draw curve */
                  if (next_ap)
                    {
                      double new_x, ap_y, new_y;
                      double ac_height =
                        fabs (y_end - y_start);
                      ac_height *= height;
                      double ac_width = fabs (x_end - x_start);
                      ac_width *= width;
                      cairo_set_line_width (
                        ao_prv->cached_cr, 2.0);
                      for (double k = x_start_real;
                           k < (x_start_real) + ac_width + 0.1;
                           k += 0.1)
                        {
                          /* in pixels, higher values are lower */
                          ap_y =
                            1.0
                            - automation_point_get_normalized_value_in_curve (
                              ap,
                              CLAMP (
                                (k - x_start_real) / ac_width,
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
                                ao_prv->cached_cr, new_x,
                                new_y);
                            }

                          cairo_line_to (
                            ao_prv->cached_cr, new_x, new_y);
                        }
                      cairo_stroke (ao_prv->cached_cr);
                    }
                }
            }
        }

      region_widget_draw_name (
        Z_REGION_WIDGET (self), ao_prv->cached_cr, &rect);

      arranger_object_widget_draw_cut_line (
        Z_ARRANGER_OBJECT_WIDGET (self), ao_prv->cached_cr,
        &rect);

      arranger_object_widget_post_redraw (
        (ArrangerObjectWidget *) self, &rect);
    }

  cairo_set_source_surface (cr, ao_prv->cached_surface, 0, 0);
  cairo_paint (cr);

  return FALSE;
}

AutomationRegionWidget *
automation_region_widget_new (ZRegion * region)
{
  AutomationRegionWidget * self =
    g_object_new (AUTOMATION_REGION_WIDGET_TYPE, NULL);

  region_widget_setup (Z_REGION_WIDGET (self), region);
  ARRANGER_OBJECT_WIDGET_GET_PRIVATE (self);

  /* connect signals */
  g_signal_connect (
    G_OBJECT (ao_prv->drawing_area), "draw",
    G_CALLBACK (automation_region_draw_cb), self);

  return self;
}

static void
automation_region_widget_class_init (
  AutomationRegionWidgetClass * klass)
{
}

static void
automation_region_widget_init (AutomationRegionWidget * self)
{
}
