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

#include "audio/transport.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/arranger_bg.h"
#include "gui/widgets/editor_ruler.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_ruler.h"
#include "gui/widgets/timeline_bg.h"
#include "gui/widgets/tracklist.h"
#include "project.h"
#include "settings/settings.h"
#include "zrythm.h"
#include "utils/cairo.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE_WITH_PRIVATE (ArrangerBgWidget,
                            arranger_bg_widget,
                            GTK_TYPE_DRAWING_AREA)

/**
 * Minimum number of pixels between beat lines.
 */
#define PX_TO_HIDE_BEATS 40.0
/*#define PX_TO_HIDE_BEATS 30.0*/

static gboolean
arranger_bg_draw_cb (
  GtkWidget *widget,
  cairo_t *cr,
  ArrangerBgWidget * self)
{
  ARRANGER_BG_WIDGET_GET_PRIVATE (self);

  RULER_WIDGET_GET_PRIVATE (ab_prv->ruler);
  if (rw_prv->px_per_bar < 2.0)
    return FALSE;

  GdkRectangle rect;
  gdk_cairo_get_clip_rectangle (
    cr, &rect);

  if (ab_prv->redraw ||
      !gdk_rectangle_equal (
         &rect, &ab_prv->last_rect))
    {
      ab_prv->last_rect = rect;
      int i = 0;

      GtkStyleContext *context =
        gtk_widget_get_style_context (widget);

      /*int width =*/
        /*gtk_widget_get_allocated_width (widget);*/
      /*int height =*/
        /*gtk_widget_get_allocated_height (widget);*/

      ab_prv->cached_surface =
        cairo_surface_create_similar (
          cairo_get_target (cr),
          CAIRO_CONTENT_COLOR_ALPHA,
          rect.width, rect.height);
      ab_prv->cached_cr =
        cairo_create (ab_prv->cached_surface);

      gtk_render_background (
        context, ab_prv->cached_cr, 0, 0,
        rect.width, rect.height);

      /* draw loop background */
      if (TRANSPORT->loop)
        {
          double start_px = 0, end_px = 0;
          if (ab_prv->arranger ==
                (ArrangerWidget *) MW_TIMELINE ||
              ab_prv->arranger ==
                (ArrangerWidget *) MW_PINNED_TIMELINE)
            {
              start_px =
                ui_pos_to_px_timeline (
                  &TRANSPORT->loop_start_pos, 1);
              end_px =
                ui_pos_to_px_timeline (
                  &TRANSPORT->loop_end_pos, 1);
            }
          else
            {
              start_px =
                ui_pos_to_px_editor (
                  &TRANSPORT->loop_start_pos, 1);
              end_px =
                ui_pos_to_px_editor (
                  &TRANSPORT->loop_end_pos, 1);
            }
          cairo_set_source_rgba (
            ab_prv->cached_cr, 0, 0.9, 0.7, 0.08);
          cairo_set_line_width (
            ab_prv->cached_cr, 2);

          /* if transport loop start is within the
           * screen */
          if (start_px > rect.x &&
              start_px <= rect.x + rect.width)
            {
              /* draw the loop start line */
              double x =
                (start_px - rect.x) + 1.0;
              cairo_move_to (
                ab_prv->cached_cr, x, 0);
              cairo_line_to (
                ab_prv->cached_cr, x, rect.height);
              cairo_stroke (ab_prv->cached_cr);
            }
          /* if transport loop end is within the
           * screen */
          if (end_px > rect.x &&
              end_px < rect.x + rect.width)
            {
              double x =
                (end_px - rect.x) - 1.0;
              cairo_move_to (
                ab_prv->cached_cr, x, 0);
              cairo_line_to (
                ab_prv->cached_cr, x, rect.height);
              cairo_stroke (ab_prv->cached_cr);
            }

          /* draw transport loop area */
          cairo_set_source_rgba (
            ab_prv->cached_cr, 0, 0.9, 0.7, 0.02);
          double loop_start_local_x =
            MAX (0, start_px - rect.x);
          cairo_rectangle (
            ab_prv->cached_cr,
            loop_start_local_x, 0,
            end_px - MAX (rect.x, start_px),
            rect.height);
          cairo_fill (ab_prv->cached_cr);
        }

      /* handle vertical drawing */

      /* get sixteenth interval */
      int sixteenth_interval =
        ruler_widget_get_sixteenth_interval (
          (RulerWidget *) (ab_prv->ruler));

      /* get the beat interval */
      int beat_interval =
        ruler_widget_get_beat_interval (
          (RulerWidget *) (ab_prv->ruler));

      /* get the interval for bars */
      int bar_interval =
        (int)
        MAX ((PX_TO_HIDE_BEATS) /
             (double) rw_prv->px_per_bar, 1.0);

      i = 0;
      double curr_px;
      while (
        (curr_px =
           rw_prv->px_per_bar * (i += bar_interval) +
             SPACE_BEFORE_START) <
         rect.x + rect.width)
        {
          if (curr_px < rect.x)
            continue;

          cairo_set_source_rgb (
            ab_prv->cached_cr, 0.3, 0.3, 0.3);
          double x = curr_px - rect.x;
          cairo_set_line_width (ab_prv->cached_cr, 1);
          cairo_move_to (
            ab_prv->cached_cr, x, 0);
          cairo_line_to (
            ab_prv->cached_cr, x, rect.height);
          cairo_stroke (ab_prv->cached_cr);
        }
      i = 0;
      if (beat_interval > 0)
        {
          while ((curr_px =
                  rw_prv->px_per_beat *
                    (i += beat_interval) +
                  SPACE_BEFORE_START) <
                 rect.x + rect.width)
            {
              if (curr_px < rect.x)
                continue;

              cairo_set_source_rgb (
                ab_prv->cached_cr, 0.25, 0.25, 0.25);
              cairo_set_line_width (
                ab_prv->cached_cr, 0.6);
              double x = curr_px - rect.x;
              cairo_move_to (
                ab_prv->cached_cr, x, 0);
              cairo_line_to (
                ab_prv->cached_cr, x, rect.height);
              cairo_stroke (ab_prv->cached_cr);
            }
        }
      i = 0;
      if (sixteenth_interval > 0)
        {
          while ((curr_px =
                  rw_prv->px_per_sixteenth *
                    (i += sixteenth_interval) +
                  SPACE_BEFORE_START) <
                 rect.x + rect.width)
            {
              if (curr_px < rect.x)
                continue;

              cairo_set_source_rgb (
                ab_prv->cached_cr, 0.2, 0.2, 0.2);
              cairo_set_line_width (
                ab_prv->cached_cr, 0.5);
              double x = curr_px - rect.x;
              cairo_move_to (
                ab_prv->cached_cr, x, 0);
              cairo_line_to (
                ab_prv->cached_cr, x, rect.height);
              cairo_stroke (ab_prv->cached_cr);
            }
        }

      /* draw selections */
      arranger_bg_widget_draw_selections (
        ab_prv->arranger, ab_prv->cached_cr, &rect);

      /* draw range */
      if (PROJECT->has_range)
        {
          /* in order they appear */
          Position * range_first_pos, * range_second_pos;
          if (position_is_before_or_equal (
                &PROJECT->range_1, &PROJECT->range_2))
            {
              range_first_pos = &PROJECT->range_1;
              range_second_pos = &PROJECT->range_2;
            }
          else
            {
              range_first_pos = &PROJECT->range_2;
              range_second_pos = &PROJECT->range_1;
            }

          int range_first_px, range_second_px;
          range_first_px =
            ui_pos_to_px_timeline (
              range_first_pos, 1);
          range_second_px =
            ui_pos_to_px_timeline (
              range_second_pos, 1);
          cairo_set_source_rgba (
            ab_prv->cached_cr, 0.3, 0.3, 0.3, 0.3);
          cairo_rectangle (
            ab_prv->cached_cr,
            MAX (0, range_first_px - rect.x), 0,
            range_second_px -
              MAX (rect.x, range_first_px),
            rect.height);
          cairo_fill (ab_prv->cached_cr);
          cairo_set_source_rgba (
            ab_prv->cached_cr, 0.8, 0.8, 0.8, 0.4);
          cairo_set_line_width (ab_prv->cached_cr, 2);

          /* if start is within the screen */
          if (range_first_px > rect.x &&
              range_first_px <= rect.x + rect.width)
            {
              /* draw the start line */
              double x =
                (range_first_px - rect.x) + 1.0;
              cairo_move_to (
                ab_prv->cached_cr, x, 0);
              cairo_line_to (
                ab_prv->cached_cr, x, rect.height);
              cairo_stroke (ab_prv->cached_cr);
            }
          /* if end is within the screen */
          if (range_second_px > rect.x &&
              range_second_px < rect.x + rect.width)
            {
              double x =
                (range_second_px - rect.x) - 1.0;
              cairo_move_to (
                ab_prv->cached_cr, x, 0);
              cairo_line_to (
                ab_prv->cached_cr, x, rect.height);
              cairo_stroke (ab_prv->cached_cr);
            }
        }
      ab_prv->redraw = 0;
    }

  cairo_set_source_surface (
    cr, ab_prv->cached_surface, rect.x, rect.y);
  cairo_paint (cr);

  return FALSE;
}

static gboolean
on_motion (GtkWidget *widget,
               GdkEventMotion  *event,
               gpointer   user_data)
{
  if (event->type == GDK_ENTER_NOTIFY)
    {
    }
  else if (event->type == GDK_LEAVE_NOTIFY)
    {
    }

  return FALSE;
}

/**
 * Draws the selection in its background.
 *
 * Should only be called by the bg widgets when drawing.
 */
void
arranger_bg_widget_draw_selections (
  ArrangerWidget * self,
  cairo_t *        cr,
  GdkRectangle *   rect)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  double offset_x, offset_y;
  offset_x =
    ar_prv->start_x + ar_prv->last_offset_x > 0 ?
    ar_prv->last_offset_x :
    1 - ar_prv->start_x;
  offset_y =
    ar_prv->start_y + ar_prv->last_offset_y > 0 ?
    ar_prv->last_offset_y :
    1 - ar_prv->start_y;

  /* if action is selecting and not selecting range
   * (in the case of timeline */
  switch (ar_prv->action)
    {
    case UI_OVERLAY_ACTION_SELECTING:
    case UI_OVERLAY_ACTION_DELETE_SELECTING:
      z_cairo_draw_selection (
        cr, ar_prv->start_x - rect->x,
        ar_prv->start_y - rect->y,
        offset_x, offset_y);
      break;
    case UI_OVERLAY_ACTION_RAMPING:
      cairo_set_source_rgba (
        cr, 0.9, 0.9, 0.9, 1.0);
      cairo_set_line_width (cr, 2.0);
      cairo_move_to (
        cr, ar_prv->start_x -  rect->x,
        ar_prv->start_y - rect->y);
      cairo_line_to (
        cr,
        (ar_prv->start_x + ar_prv->last_offset_x) -
          rect->x,
        (ar_prv->start_y + ar_prv->last_offset_y) -
          rect->y);
      cairo_stroke (cr);
      break;
    default:
      break;
    }
}

static void
multipress_pressed (GtkGestureMultiPress *gesture,
               gint                  n_press,
               gdouble               x,
               gdouble               y,
               gpointer              user_data)
{
  ArrangerBgWidget * self =
    Z_ARRANGER_BG_WIDGET (user_data);
  gtk_widget_grab_focus (GTK_WIDGET (self));
}

static void
drag_begin (GtkGestureDrag * gesture,
               gdouble         start_x,
               gdouble         start_y,
               gpointer        user_data)
{
  ArrangerBgWidget * self =
    Z_ARRANGER_BG_WIDGET (user_data);
  ARRANGER_BG_WIDGET_GET_PRIVATE (self);
  ab_prv->start_x = start_x;
}

static void
drag_update (GtkGestureDrag * gesture,
               gdouble         offset_x,
               gdouble         offset_y,
               gpointer        user_data)
{
}

static void
drag_end (GtkGestureDrag *gesture,
          gdouble         offset_x,
          gdouble         offset_y,
          gpointer        user_data)
{
  ArrangerBgWidget * self =
    Z_ARRANGER_BG_WIDGET (user_data);
  ARRANGER_BG_WIDGET_GET_PRIVATE (self);
  ab_prv->start_x = 0;
}

void
arranger_bg_widget_refresh (ArrangerBgWidget * self)
{
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

ArrangerBgWidgetPrivate *
arranger_bg_widget_get_private (
  ArrangerBgWidget * self)
{
  return
    arranger_bg_widget_get_instance_private (self);
}

static void
arranger_bg_widget_class_init (
  ArrangerBgWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (klass,
                                 "arranger-bg");
}

static void
arranger_bg_widget_init (ArrangerBgWidget *self )
{
  ARRANGER_BG_WIDGET_GET_PRIVATE (self);
  ab_prv->drag =
    GTK_GESTURE_DRAG (
      gtk_gesture_drag_new (GTK_WIDGET (self)));
  ab_prv->multipress =
    GTK_GESTURE_MULTI_PRESS (
      gtk_gesture_multi_press_new (GTK_WIDGET (self)));

  gtk_widget_set_can_focus (GTK_WIDGET (self),
                           1);
  gtk_widget_set_focus_on_click (GTK_WIDGET (self),
                                 1);

  g_signal_connect (
    G_OBJECT (self), "draw",
    G_CALLBACK (arranger_bg_draw_cb), self);
  g_signal_connect (
    G_OBJECT(ab_prv->drag), "drag-begin",
    G_CALLBACK (drag_begin),  self);
  g_signal_connect (
    G_OBJECT(ab_prv->drag), "drag-update",
    G_CALLBACK (drag_update),  self);
  g_signal_connect (
    G_OBJECT(ab_prv->drag), "drag-end",
    G_CALLBACK (drag_end),  self);
  g_signal_connect (
    G_OBJECT (ab_prv->multipress), "pressed",
    G_CALLBACK (multipress_pressed), self);
  g_signal_connect (
    G_OBJECT (self), "motion-notify-event",
    G_CALLBACK (on_motion), self);
  g_signal_connect (
    G_OBJECT (self), "leave-notify-event",
    G_CALLBACK (on_motion), self);
  g_signal_connect (
    G_OBJECT (self), "enter-notify-event",
    G_CALLBACK (on_motion), self);
}
