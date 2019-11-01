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

#include "audio/automation_region.h"
#include "audio/automation_track.h"
#include "audio/audio_bus_track.h"
#include "audio/channel.h"
#include "audio/instrument_track.h"
#include "audio/track.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/automation_point.h"
#include "gui/widgets/ruler.h"
#include "project.h"
#include "utils/math.h"
#include "utils/ui.h"

G_DEFINE_TYPE (
  AutomationPointWidget,
  automation_point_widget,
  ARRANGER_OBJECT_WIDGET_TYPE)

static gboolean
draw_cb (
  GtkWidget * widget,
  cairo_t *   cr,
  AutomationPointWidget * self)
{
  ARRANGER_OBJECT_WIDGET_GET_PRIVATE (self);

  if (ao_prv->redraw)
    {
      GtkStyleContext *context;

      context =
        gtk_widget_get_style_context (widget);

      int width =
        gtk_widget_get_allocated_width (widget);
      int height =
        gtk_widget_get_allocated_height (widget);
      ao_prv->cached_surface =
        cairo_surface_create_similar (
          cairo_get_target (cr),
          CAIRO_CONTENT_COLOR_ALPHA,
          width, height);
      ao_prv->cached_cr =
        cairo_create (ao_prv->cached_surface);

      gtk_render_background (
        context, ao_prv->cached_cr, 0, 0,
        width, height);

      AutomationPoint * ap = self->ap;
      AutomationPoint * next_ap =
        automation_region_get_next_ap (
          ap->region, ap);

      Track * track =
        arranger_object_get_track (
          (ArrangerObject *) ap);
      g_return_val_if_fail (track, FALSE);

      /* get color */
      GdkRGBA color = track->color;
      ui_get_arranger_object_color (
        &color,
        gtk_widget_get_state_flags (
          GTK_WIDGET (self)) &
          GTK_STATE_FLAG_PRELIGHT,
        automation_point_is_selected (
          self->ap),
        automation_point_is_transient (
          self->ap));
      gdk_cairo_set_source_rgba (
        ao_prv->cached_cr, &color);

      int upslope =
        next_ap && ap->fvalue < next_ap->fvalue;

      /* draw circle */
      cairo_arc (
        ao_prv->cached_cr,
        AP_WIDGET_POINT_SIZE / 2,
        (upslope ?
           height -
           AP_WIDGET_POINT_SIZE / 2 :
           AP_WIDGET_POINT_SIZE / 2),
        AP_WIDGET_POINT_SIZE / 2,
        0, 2 * G_PI);
      cairo_stroke_preserve(ao_prv->cached_cr);
      cairo_fill(ao_prv->cached_cr);

      if (next_ap)
        {
          /* draw the curve */
          double automation_point_y;
          double new_x = 0;

          /* TODO use cairo translate to add padding */

          /* set starting point */
          double new_y;

          /* ignore the space between the center
           * of each point and the edges (2 of them
           * so a full AP_WIDGET_POINT_SIZE) */
          double width_for_curve =
            width - AP_WIDGET_POINT_SIZE;
          double height_for_curve =
            height - AP_WIDGET_POINT_SIZE;

          for (double l = 0.0;
               l <= (double) width_for_curve;
               l = l + 0.1)
            {
              /* in pixels, higher values are lower */
              automation_point_y =
                1.0 -
                automation_point_get_normalized_value_in_curve (
                  self->ap,
                  l / width_for_curve);
              automation_point_y *= height_for_curve;

              new_x = l;
              new_y = automation_point_y;

              if (math_doubles_equal (l, 0.0, 0.01))
                {
                  cairo_move_to (
                    ao_prv->cached_cr,
                    new_x +
                      AP_WIDGET_POINT_SIZE / 2,
                    new_y +
                      AP_WIDGET_POINT_SIZE / 2);
                }

              cairo_line_to (
                ao_prv->cached_cr,
                new_x +
                  AP_WIDGET_POINT_SIZE / 2,
                new_y +
                  AP_WIDGET_POINT_SIZE / 2);
            }

          cairo_stroke (ao_prv->cached_cr);
        }
      else /* no next ap */
        {
        }

      ao_prv->redraw = 0;
    }

  cairo_set_source_surface (
    cr, ao_prv->cached_surface, 0, 0);
  cairo_paint (cr);

 return FALSE;
}

AutomationPointWidget *
automation_point_widget_new (
  AutomationPoint * ap)
{
  AutomationPointWidget * self =
    g_object_new (
      AUTOMATION_POINT_WIDGET_TYPE,
      "visible", 1,
      NULL);

  arranger_object_widget_setup (
    Z_ARRANGER_OBJECT_WIDGET (self),
    (ArrangerObject *) ap);

  self->ap = ap;

  return self;
}

static void
automation_point_widget_class_init (
  AutomationPointWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (
    klass, "automation-point");
}

static void
automation_point_widget_init (
  AutomationPointWidget * self)
{
  ARRANGER_OBJECT_WIDGET_GET_PRIVATE (self);

  /* connect signals */
  g_signal_connect (
    G_OBJECT (ao_prv->drawing_area), "draw",
    G_CALLBACK (draw_cb), self);
}
