/*
 * gui/widgets/automation_curve.c- AutomationCurve
 *
 * Copyright (C) 2018 Alexandros Theodotou
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "audio/automation_track.h"
#include "audio/bus_track.h"
#include "audio/channel.h"
#include "audio/instrument_track.h"
#include "audio/track.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/automation_curve.h"
#include "gui/widgets/ruler.h"
#include "utils/ui.h"

G_DEFINE_TYPE (AutomationCurveWidget, automation_curve_widget, GTK_TYPE_DRAWING_AREA)

static double
curve_val_from_real (AutomationCurveWidget * self)
{
  double real = self->ac->curviness;
  return (real - AP_MIN_CURVINESS) / AP_CURVINESS_RANGE;
}

static double
real_val_from_curve (AutomationCurveWidget * self, double curve)
{
  return curve * AP_CURVINESS_RANGE + AP_MIN_CURVINESS;
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
  int use_y = abs(offset_y - self->last_y) > abs(offset_x - self->last_x);
  /*double multiplier = 0.005;*/
  double diff = use_y ? offset_y - self->last_y : offset_x - self->last_x;
  double height = gtk_widget_get_allocated_height (GTK_WIDGET (self));
  double adjusted_diff = diff / height;
  double new_curve_val = clamp (curve_val_from_real (self) + adjusted_diff,
                                1.0,
                                0.0);
  automation_curve_set_curviness (self->ac,
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

/* TODO cache because when the playhead covers it
 * it has to get redrawn */
static gboolean
automation_curve_draw_cb (
  AutomationCurveWidget * self,
  cairo_t *cr,
  gpointer data)
{
  /* not cached, redraw */
  if (!self->cache)
    {
      guint width, height;
      GtkStyleContext *context;

      width =
        gtk_widget_get_allocated_width (GTK_WIDGET (self));
      height =
        gtk_widget_get_allocated_height (GTK_WIDGET (self));
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
      Track * track = self->ac->at->track;
      /*ChannelTrack * ct = (ChannelTrack *) track;*/
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

      AutomationPoint * next_ap = automation_track_get_ap_after_curve (self->ac->at,
                                                                  self->ac);
      AutomationPoint * prev_ap = automation_track_get_ap_before_curve (self->ac->at,
                                                                  self->ac);

      /*gtk_widget_translate_coordinates(*/
                /*GTK_WIDGET (ap->widget),*/
                /*GTK_WIDGET (MW_TRACKLIST),*/
                /*0,*/
                /*0,*/
                /*&wx,*/
                /*&wy);*/
      /*wx = arranger_get_x_pos_in_px (&ap->pos);*/

      /*gint prev_wx, prev_wy;*/
      /*AutomationPoint * prev_ap =*/
        /*automation_track_get_ap_before_curve (at, self->ac);*/
      /*gtk_widget_translate_coordinates(*/
                /*GTK_WIDGET (prev_ap->widget),*/
                /*GTK_WIDGET (MW_TRACKLIST),*/
                /*0,*/
                /*0,*/
                /*&prev_wx,*/
                /*&prev_wy);*/
      /*prev_wx = arranger_get_x_pos_in_px (&prev_ap->pos);*/
      /*int ww = wx - prev_wx;*/
      double automation_point_y;
      int prev_y_px = automation_point_get_y_in_px (prev_ap);
      int next_y_px = automation_point_get_y_in_px (next_ap);
      int prev_higher = prev_y_px < next_y_px;
      double new_x = AC_Y_HALF_PADDING;
      double new_y = prev_higher ? AC_Y_HALF_PADDING : height - AC_Y_HALF_PADDING;
      cairo_move_to (self->cached_cr,
                     new_x,
                     new_y);
      /*int height = prev_y_px > curr_y_px ?*/
        /*prev_y_px - curr_y_px :*/
        /*curr_y_px - prev_y_px;*/
      /*cairo_set_line_width (self->cached_cr, 1);*/
      /*for (int l = 0; l < ww; l++)*/
      for (double l = AC_Y_HALF_PADDING;
           l <= ((double) width - AC_Y_HALF_PADDING);
           l = l + 0.1)
        {
          automation_point_y =
            automation_curve_get_y_px (
              self->ac,
              l - AC_Y_HALF_PADDING, // this should come to 0
              width - AC_Y_PADDING,
              height - AC_Y_PADDING);
          new_x = l;
          new_y = prev_higher ? automation_point_y : height + automation_point_y;

          /* this is because height is 1 smaller than the actual height, so start drawing from 0.5 to actual height - 0.5 */
          if (prev_higher)
            new_y += AC_Y_HALF_PADDING;
          else
            new_y -= AC_Y_HALF_PADDING;
          /*g_message ("new x %f , new y %f",*/
                     /*new_x,*/
                     /*new_y);*/
          cairo_line_to (self->cached_cr,
                         new_x,
                         new_y);
        }

      cairo_stroke (self->cached_cr);

      self->cache = 1;
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

      bot_bar_change_status (
        "Automation Curve - Click and drag to  "
        "change curviness");
      self->cache = 0;
    }
  else if (event->type == GDK_LEAVE_NOTIFY)
    {
      ui_set_cursor_from_name (widget, "default");
      gtk_widget_unset_state_flags (
        GTK_WIDGET (self),
        GTK_STATE_FLAG_PRELIGHT);

      bot_bar_change_status ("");
      self->cache = 0;
    }
  g_idle_add ((GSourceFunc) gtk_widget_queue_draw, GTK_WIDGET (self));
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
automation_curve_widget_init (AutomationCurveWidget * self)
{
}
