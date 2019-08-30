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
#define PX_TO_HIDE_BEATS 30.0

static gboolean
arranger_bg_draw_cb (
  GtkWidget *widget,
  cairo_t *cr,
  ArrangerBgWidget * self)
{
  ARRANGER_BG_WIDGET_GET_PRIVATE (self);

  int i = 0;

  GdkRectangle rect;
  gdk_cairo_get_clip_rectangle (cr,
	                        &rect);

  RULER_WIDGET_GET_PRIVATE (ab_prv->ruler);
  if (rw_prv->px_per_bar < 2.0)
    return FALSE;

  GtkStyleContext *context;

  context = gtk_widget_get_style_context (widget);

  gtk_render_background (context, cr,
                         rect.x, rect.y,
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
      cairo_set_source_rgba (cr, 0, 0.9, 0.7, 0.08);
      cairo_set_line_width (cr, 2);
      if (start_px > rect.x)
        {
          cairo_move_to (
            cr, start_px + 1.0, rect.y);
          cairo_line_to (
            cr, start_px + 1.0, rect.height);
          cairo_stroke (cr);
        }
      if (end_px < rect.x + rect.width)
        {
          cairo_move_to (
            cr, end_px - 1.0, rect.y);
          cairo_line_to (
            cr, end_px - 1.0, rect.height);
          cairo_stroke (cr);
        }
      cairo_set_source_rgba (cr, 0, 0.9, 0.7, 0.02);
      cairo_rectangle (
        cr, MAX (rect.x, start_px), rect.y,
        /* FIXME use rect properly, this exceeds */
        end_px - MAX (rect.x, start_px),
        rect.height);
      cairo_fill (cr);
    }

  /* in order they appear */
  Position * range_first_pos, * range_second_pos;
  if (position_compare (&PROJECT->range_1,
                        &PROJECT->range_2) <= 0)
    {
      range_first_pos = &PROJECT->range_1;
      range_second_pos = &PROJECT->range_2;
    }
  else
    {
      range_first_pos = &PROJECT->range_2;
      range_second_pos = &PROJECT->range_1;
    }

  /* handle vertical drawing */

  /* gather divisors of the number of beats per
   * bar */
#define beats_per_bar TRANSPORT->beats_per_bar
  int divisors[16];
  int num_divisors = 0;
  for (i = 1; i <= beats_per_bar; i++)
    {
      if (beats_per_bar % i == 0)
        divisors[num_divisors++] = i;
    }

  /* decide the raw interval to keep between beats */
  int _beat_interval =
    MAX (PX_TO_HIDE_BEATS /
         rw_prv->px_per_beat, 1);

  /* round the interval to the divisors */
  int beat_interval = -1;
  for (i = 0; i < num_divisors; i++)
    {
      if (_beat_interval <= divisors[i])
        {
          if (divisors[i] != beats_per_bar)
            beat_interval = divisors[i];
          break;
        }
    }

  /* get the interval for bars */
  int bar_interval =
    MAX ((PX_TO_HIDE_BEATS) /
         rw_prv->px_per_bar, 1);

  i = 0;
  double curr_px;
  while ((curr_px =
          rw_prv->px_per_bar * (i += bar_interval) +
          SPACE_BEFORE_START) <
         rect.x + rect.width)
    {
      if (curr_px < rect.x)
        continue;

      cairo_set_source_rgb (cr, 0.3, 0.3, 0.3);
      cairo_set_line_width (cr, 1);
      cairo_move_to (cr, curr_px, rect.y);
      cairo_line_to (cr,
                     curr_px, rect.y + rect.height);
      cairo_stroke (cr);
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
            cr, 0.25, 0.25, 0.25);
          cairo_set_line_width (cr, 0.5);
          cairo_move_to (cr, curr_px, rect.y);
          cairo_line_to (
            cr,
            curr_px, rect.y + rect.height);
          cairo_stroke (cr);
        }
    }

  /* draw selections */
  arranger_bg_widget_draw_selections (
    ab_prv->arranger,
    cr);

  /* draw range */
  if (PROJECT->has_range)
    {
      int range_first_px, range_second_px;
      range_first_px =
        ui_pos_to_px_timeline (range_first_pos, 1);
      range_second_px =
        ui_pos_to_px_timeline (range_second_pos, 1);
      cairo_set_source_rgba (
        cr, 0.3, 0.3, 0.3, 0.3);
      cairo_rectangle (
        cr,
        range_first_px > rect.x ?
          range_first_px :
          rect.x,
        rect.y,
        range_second_px < rect.x + rect.width ?
          range_second_px - range_first_px :
          (rect.x + rect.width) - range_first_px,
        rect.height);
      cairo_fill (cr);
      cairo_set_source_rgba (cr, 0.8, 0.8, 0.8, 0.4);
      cairo_set_line_width (cr, 2);
      z_cairo_draw_vertical_line (
        cr,
        range_first_px > rect.x ?
          range_first_px :
          rect.x,
        rect.y,
        rect.y + rect.height);
      z_cairo_draw_vertical_line (
        cr,
        range_second_px < rect.x + rect.width ?
          range_second_px :
          rect.x + rect.width,
        rect.y,
        rect.y + rect.height);
    }

  return 0;
}

static gboolean
on_motion (GtkWidget *widget,
               GdkEventMotion  *event,
               gpointer   user_data)
{
  if (event->type == GDK_ENTER_NOTIFY)
    {
      bot_bar_change_status (
        "Arranger - Double click inside a track "
        "lane to create objects "
        "(hold Shift to disable snapping) - Click "
        "on the top half of a track lane and drag "
        "to select objects - Click on the bottom "
        "half of a track lane and drag to select a "
        "range");
    }
  else if (event->type == GDK_LEAVE_NOTIFY)
    bot_bar_change_status ("");

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
  cairo_t *        cr)
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
        cr, ar_prv->start_x, ar_prv->start_y,
        offset_x, offset_y);
      break;
    case UI_OVERLAY_ACTION_RAMPING:
      cairo_set_source_rgba (
        cr, 0.9, 0.9, 0.9, 1.0);
      cairo_set_line_width (cr, 2.0);
      cairo_move_to (
        cr, ar_prv->start_x, ar_prv->start_y);
      cairo_line_to (
        cr,
        ar_prv->start_x + ar_prv->last_offset_x,
        ar_prv->start_y + ar_prv->last_offset_y);
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
arranger_bg_widget_get_private (ArrangerBgWidget * self)
{
  return arranger_bg_widget_get_instance_private (self);
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
