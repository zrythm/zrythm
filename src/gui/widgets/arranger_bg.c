/*
 * gui/widgets/arranger_bg.c - Arranger background
 *
 * Copyright (C) 2019 Alexandros Theodotou
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

#include "audio/transport.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/arranger_bg.h"
#include "gui/widgets/midi_ruler.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/piano_roll.h"
#include "gui/widgets/piano_roll_labels.h"
#include "gui/widgets/ruler.h"
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

static gboolean
arranger_bg_draw_cb (GtkWidget *widget, cairo_t *cr, gpointer data)
{
  ArrangerBgWidget * self = (ArrangerBgWidget *) widget;
  ARRANGER_BG_WIDGET_GET_PRIVATE (self);

  /* if not cached, draw */
  if (!ab_prv->cache)
    {
      guint height =
        gtk_widget_get_allocated_height (widget);
      guint width =
        gtk_widget_get_allocated_width (widget);

      ab_prv->cached_surface =
        cairo_surface_create_similar (
          cairo_get_target (cr),
          CAIRO_CONTENT_COLOR_ALPHA,
          width,
          height);
      ab_prv->cached_cr =
        cairo_create (ab_prv->cached_surface);

      RULER_WIDGET_GET_PRIVATE (ab_prv->ruler);

      /*GdkRGBA color;*/
      GtkStyleContext *context;

      context = gtk_widget_get_style_context (widget);

      /*guint width = gtk_widget_get_allocated_width (widget);*/
      /*guint height = gtk_widget_get_allocated_height (widget);*/

      gtk_render_background (context, ab_prv->cached_cr,
                             0, 0,
                             width, height);

      /* handle vertical drawing */
      for (int i = SPACE_BEFORE_START;
           i < width;
           i++)
        {
          int actual_pos = i - SPACE_BEFORE_START;
          if (actual_pos % rw_prv->px_per_bar == 0)
            {
              cairo_set_source_rgb (ab_prv->cached_cr, 0.3, 0.3, 0.3);
              cairo_set_line_width (ab_prv->cached_cr, 1);
              cairo_move_to (ab_prv->cached_cr, i, 0);
              cairo_line_to (ab_prv->cached_cr, i, height);
              cairo_stroke (ab_prv->cached_cr);
            }
          else if (actual_pos % rw_prv->px_per_beat == 0)
            {
              cairo_set_source_rgb (ab_prv->cached_cr, 0.25, 0.25, 0.25);
              cairo_set_line_width (ab_prv->cached_cr, 0.5);
              cairo_move_to (ab_prv->cached_cr, i, 0);
              cairo_line_to (ab_prv->cached_cr, i, height);
              cairo_stroke (ab_prv->cached_cr);
            }
        }

      ab_prv->cache = 1;
    }

  cairo_set_source_surface (cr, ab_prv->cached_surface, 0, 0);
  cairo_paint (cr);

      /* draw selections */
      arranger_bg_widget_draw_selections (
        ab_prv->arranger,
        cr);

  return 0;
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
  offset_x = ar_prv->start_x + ar_prv->last_offset_x > 0 ?
    ar_prv->last_offset_x :
    1 - ar_prv->start_x;
  offset_y = ar_prv->start_y + ar_prv->last_offset_y > 0 ?
    ar_prv->last_offset_y :
    1 - ar_prv->start_y;
  if (ar_prv->action == ARRANGER_ACTION_SELECTING)
    {
      z_cairo_draw_selection (cr,
                              ar_prv->start_x,
                              ar_prv->start_y,
                              offset_x,
                              offset_y);
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
  ARRANGER_BG_WIDGET_GET_PRIVATE (self);

  ab_prv->cache = 0;
  if (Z_IS_TIMELINE_BG_WIDGET (self))
    Z_TIMELINE_BG_WIDGET (self)->cache = 0;

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

  g_signal_connect (G_OBJECT (self), "draw",
                    G_CALLBACK (arranger_bg_draw_cb), NULL);
  g_signal_connect (G_OBJECT(ab_prv->drag), "drag-begin",
                    G_CALLBACK (drag_begin),  self);
  g_signal_connect (G_OBJECT(ab_prv->drag), "drag-update",
                    G_CALLBACK (drag_update),  self);
  g_signal_connect (G_OBJECT(ab_prv->drag), "drag-end",
                    G_CALLBACK (drag_end),  self);
  g_signal_connect (G_OBJECT (ab_prv->multipress), "pressed",
                    G_CALLBACK (multipress_pressed), self);
}
