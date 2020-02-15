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
#include "utils/cairo.h"
#include "utils/math.h"
#include "utils/ui.h"

/**
 * Draws the AutomationPoint in the given cairo
 * context in absolute coordinates.
 *
 * @param cr The cairo context of the arranger.
 * @param rect Arranger rectangle.
 */
void
automation_point_draw (
  AutomationPoint * ap,
  cairo_t *         cr,
  GdkRectangle *    rect)
{
  ArrangerObject * obj =
    (ArrangerObject *) ap;
  ZRegion * region =
    arranger_object_get_region (obj);
  AutomationPoint * next_ap =
    automation_region_get_next_ap (
      region, ap);
  ArrangerObject * next_obj =
    (ArrangerObject *) next_ap;
  ArrangerWidget * arranger =
    arranger_object_get_arranger (obj);

  Track * track =
    arranger_object_get_track (
      (ArrangerObject *) ap);
  g_return_if_fail (track);

  /* get color */
  GdkRGBA color = track->color;
  ui_get_arranger_object_color (
    &color,
    arranger->hovered_object == obj,
    automation_point_is_selected (ap), 0);
  gdk_cairo_set_source_rgba (
    cr, &color);

  cairo_set_line_width (cr, 2);

  int upslope =
    next_ap && ap->fvalue < next_ap->fvalue;
  (void) upslope;
  (void) next_obj;

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
        obj->full_rect.width - AP_WIDGET_POINT_SIZE;
      double height_for_curve =
        obj->full_rect.height -
          AP_WIDGET_POINT_SIZE;

      for (double l = 0.0;
           l <= (double) width_for_curve + 0.01;
           l = l + 0.1)
        {
          /* in pixels, higher values are lower */
          automation_point_y =
            1.0 -
            automation_point_get_normalized_value_in_curve (
              ap,
              CLAMP (
                l / width_for_curve, 0.0, 1.0));
          automation_point_y *= height_for_curve;

          new_x = (l + obj->full_rect.x) - rect->x;
          new_y =
            (obj->full_rect.y +
              automation_point_y) -
            rect->y;

          if (math_doubles_equal (l, 0.0))
            {
              cairo_move_to (
                cr,
                new_x +
                  AP_WIDGET_POINT_SIZE / 2,
                new_y +
                  AP_WIDGET_POINT_SIZE / 2);
            }

          cairo_line_to (
            cr,
            new_x +
              AP_WIDGET_POINT_SIZE / 2,
            new_y +
              AP_WIDGET_POINT_SIZE / 2);
        }

      cairo_stroke (cr);
    }
  else /* no next ap */
    {
    }

  /* draw circle */
  cairo_arc (
    cr,
    (obj->full_rect.x + AP_WIDGET_POINT_SIZE / 2) -
      rect->x,
    upslope ?
      ((obj->full_rect.y + obj->full_rect.height) -
       AP_WIDGET_POINT_SIZE / 2) -
        rect->y :
      (obj->full_rect.y + AP_WIDGET_POINT_SIZE / 2) -
        rect->y,
    AP_WIDGET_POINT_SIZE / 2,
    0, 2 * G_PI);
  cairo_set_source_rgba (cr, 0, 0, 0, 1);
  cairo_fill_preserve (cr);
  gdk_cairo_set_source_rgba (cr, &color);
  cairo_stroke (cr);
}
