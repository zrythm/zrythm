/*
 * gui/widgets/automation_curve.c- AutomationCurve
 *
 * Copyright (C) 2018 Alexandros Theodotou
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

#include "audio/automation_region.h"
#include "audio/automation_track.h"
#include "audio/audio_bus_track.h"
#include "audio/channel.h"
#include "audio/instrument_track.h"
#include "audio/track.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/automation_curve.h"
#include "gui/widgets/ruler.h"
#include "utils/math.h"
#include "utils/ui.h"

G_DEFINE_TYPE (
  AutomationCurveWidget,
  automation_curve_widget,
  GTK_TYPE_DRAWING_AREA)

static double
curve_val_from_real (
  AutomationCurveWidget * self)
{
  double real = self->ac->curviness;
  return
    (real - AP_MIN_CURVINESS) /
     AP_CURVINESS_RANGE;
}

static double
real_val_from_curve (
  AutomationCurveWidget * self, double curve)
{
  return
    curve * AP_CURVINESS_RANGE +
    AP_MIN_CURVINESS;
}

static double clamp
(double x, double upper, double lower)
{
    return MIN(upper, MAX(x, lower));
}

static void
drag_update (GtkGestureDrag * gesture,
               gdouble         offset_x,
               gdouble         offset_y,
               gpointer        user_data)
{
  AutomationCurveWidget * self = (AutomationCurveWidget *) user_data;
  offset_y = - offset_y;
  /*int use_y = fabs (offset_y - self->last_y) >*/
    /*fabs (offset_x - self->last_x);*/
  int use_y = 1;
  /*double multiplier = 0.005;*/
  double diff = use_y ? offset_y - self->last_y : offset_x - self->last_x;
  /*double height = gtk_widget_get_allocated_height (GTK_WIDGET (self));*/
  double adjusted_diff = diff / 120.0;
  double new_curve_val =
    clamp (
      curve_val_from_real (self) + adjusted_diff,
      1.0, 0.0);
  automation_curve_set_curviness (
    self->ac,
    real_val_from_curve (self, new_curve_val));
  self->last_x = offset_x;
  self->last_y = offset_y;
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

static void
drag_end (GtkGestureDrag *gesture,
               gdouble         offset_x,
               gdouble         offset_y,
               gpointer        user_data)
{
  AutomationCurveWidget * self = (AutomationCurveWidget *) user_data;
  self->last_x = 0;
  self->last_y = 0;
}

static gboolean
automation_curve_draw_cb (
  AutomationCurveWidget * self,
  cairo_t *cr,
  gpointer data)
{
  if (self->redraw)
    {
      gint width, height;
      GtkStyleContext *context;

      width =
        gtk_widget_get_allocated_width (
          GTK_WIDGET (self));
      height =
        gtk_widget_get_allocated_height (
          GTK_WIDGET (self));
      self->cached_surface =
        cairo_surface_create_similar (
          cairo_get_target (cr),
          CAIRO_CONTENT_COLOR_ALPHA,
          width,
          height);
      self->cached_cr =
        cairo_create (self->cached_surface);

      context =
        gtk_widget_get_style_context (GTK_WIDGET (self));

      gtk_render_background (context, self->cached_cr, 0, 0, width, height);

      /*cairo_rectangle (self->cached_cr, 0, 0, width, height);*/
      /*cairo_fill (self->cached_cr);*/

      GdkRGBA * color;
      Track * track = self->ac->region->at->track;
      color = &track->color;
      /*if (self->hover)*/
        /*{*/
          /*cairo_set_source_rgba (self->cached_cr,*/
                                 /*color->red + 0.2,*/
                                 /*color->green + 0.2,*/
                                 /*color->blue + 0.2,*/
                                 /*0.8);*/
        /*}*/
      /*else if (self->selected)*/
        /*{*/
          /*cairo_set_source_rgba (self->cached_cr,*/
                                 /*color->red + 0.4,*/
                                 /*color->green + 0.4,*/
                                 /*color->blue + 0.4,*/
                                 /*0.8);*/
        /*}*/
      /*else*/
        /*{*/
          cairo_set_source_rgba (self->cached_cr, color->red, color->green, color->blue, 0.7);
        /*}*/

      /*AutomationPoint * next_ap =*/
        /*automation_region_get_ap_after_curve (*/
          /*self->ac->region, self->ac);*/
      /*AutomationPoint * prev_ap =*/
        /*automation_region_get_ap_before_curve (*/
          /*self->ac->region, self->ac);*/

      double automation_point_y;
      double new_x = 0;

      /* TODO use cairo translate to add padding */

      /* set starting point */
      double new_y;

      for (double l = 0.0;
           l <= ((double) width);
           l = l + 0.1)
        {
          /* in pixels, higher values are lower */
          automation_point_y =
            1.0 -
            automation_curve_get_normalized_value (
              self->ac,
              l / width);
          automation_point_y *= height;

          new_x = l;
          new_y = automation_point_y;

          if (math_doubles_equal (l, 0.0, 0.01))
            {
              cairo_move_to (self->cached_cr,
                             new_x,
                             new_y);
            }

          cairo_line_to (self->cached_cr,
                         new_x,
                         new_y);
        }

      cairo_stroke (self->cached_cr);

      self->redraw = 0;
    }

  cairo_set_source_surface (
    cr, self->cached_surface, 0, 0);
  cairo_paint (cr);

  return FALSE;
}

static void
on_motion (GtkWidget * widget, GdkEventMotion *event)
{
  AutomationCurveWidget * self =
    Z_AUTOMATION_CURVE_WIDGET (widget);

  GtkAllocation allocation;
  gtk_widget_get_allocation (widget,
                             &allocation);

  if (event->type == GDK_ENTER_NOTIFY)
    {
      gtk_widget_set_state_flags (
        GTK_WIDGET (self),
        GTK_STATE_FLAG_PRELIGHT,
        0);

      self->redraw = 1;
    }
  else if (event->type == GDK_LEAVE_NOTIFY)
    {
      ui_set_cursor_from_name (widget, "default");
      gtk_widget_unset_state_flags (
        GTK_WIDGET (self),
        GTK_STATE_FLAG_PRELIGHT);

      self->redraw = 1;
    }
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

AutomationCurveWidget *
automation_curve_widget_new (AutomationCurve * ac)
{
  g_message ("Creating automation_curve widget...");
  AutomationCurveWidget * self =
    g_object_new (
      AUTOMATION_CURVE_WIDGET_TYPE,
      "visible", 1,
      NULL);

  self->ac = ac;

  gtk_widget_add_events (GTK_WIDGET (self), GDK_ALL_EVENTS_MASK);
  self->drag = GTK_GESTURE_DRAG (gtk_gesture_drag_new (GTK_WIDGET (self)));

  /* connect signals */
  g_signal_connect (
    G_OBJECT (self), "draw",
    G_CALLBACK (automation_curve_draw_cb), self);
  g_signal_connect (
    G_OBJECT (self), "enter-notify-event",
    G_CALLBACK (on_motion),  self);
  g_signal_connect (
    G_OBJECT(self), "leave-notify-event",
    G_CALLBACK (on_motion),  self);
  g_signal_connect (
    G_OBJECT(self), "motion-notify-event",
    G_CALLBACK (on_motion),  self);
  g_signal_connect (
    G_OBJECT(self->drag), "drag-update",
    G_CALLBACK (drag_update),  self);
  g_signal_connect (
    G_OBJECT(self->drag), "drag-end",
    G_CALLBACK (drag_end),  self);

  return self;
}

static void
automation_curve_widget_class_init (
  AutomationCurveWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (
    klass, "automation-curve");
}

static void
automation_curve_widget_init (
  AutomationCurveWidget * self)
{
  g_object_ref (self);
}
