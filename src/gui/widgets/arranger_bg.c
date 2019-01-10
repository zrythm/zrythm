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

#include "zrythm.h"
#include "project.h"
#include "settings.h"
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
#include "gui/widgets/tracklist.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE_WITH_PRIVATE (ArrangerBgWidget,
                            arranger_bg_widget,
                            GTK_TYPE_DRAWING_AREA)

static gboolean
draw_cb (GtkWidget *widget, cairo_t *cr, gpointer data)
{
  ArrangerBgWidget * self = Z_ARRANGER_BG_WIDGET (widget);
  ARRANGER_BG_WIDGET_GET_PRIVATE (self);

  RULER_WIDGET_GET_PRIVATE (ab_prv->ruler);

  /*GdkRGBA color;*/
  GtkStyleContext *context;

  context = gtk_widget_get_style_context (widget);

  /*guint width = gtk_widget_get_allocated_width (widget);*/
  guint height = gtk_widget_get_allocated_height (widget);

  /* get positions in px */
  static int playhead_pos_in_px;
  playhead_pos_in_px =
    (TRANSPORT->playhead_pos.bars - 1) * prv->px_per_bar +
    (TRANSPORT->playhead_pos.beats - 1) * prv->px_per_beat +
    (TRANSPORT->playhead_pos.sixteenths - 1) * prv->px_per_sixteenth +
    TRANSPORT->playhead_pos.ticks * prv->px_per_tick;

  gtk_render_background (context, cr, 0, 0, prv->total_px, height);

  /* handle vertical drawing */
  for (int i = SPACE_BEFORE_START; i < prv->total_px; i++)
  {
    int actual_pos = i - SPACE_BEFORE_START;
    if (actual_pos == playhead_pos_in_px)
      {
          cairo_set_source_rgb (cr, 1, 0, 0);
          cairo_set_line_width (cr, 2);
          cairo_move_to (cr, i, 0);
          cairo_line_to (cr, i, height);
          cairo_stroke (cr);
      }
      if (actual_pos % prv->px_per_bar == 0)
      {
          cairo_set_source_rgb (cr, 0.3, 0.3, 0.3);
          cairo_set_line_width (cr, 1);
          cairo_move_to (cr, i, 0);
          cairo_line_to (cr, i, height);
          cairo_stroke (cr);
      }
      else if (actual_pos % prv->px_per_beat == 0)
      {
          cairo_set_source_rgb (cr, 0.25, 0.25, 0.25);
          cairo_set_line_width (cr, 0.5);
          cairo_move_to (cr, i, 0);
          cairo_line_to (cr, i, height);
          cairo_stroke (cr);
      }
  }

  /* draw selections */
  arranger_bg_draw_selections (
    ab_prv->arranger,
    cr);

  return 0;
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
  RULER_WIDGET_GET_PRIVATE (ab_prv->ruler);
  prv->start_x = 0;
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

  // set the size FIXME uncomment
  int ww, hh;
  PianoRollLabelsWidget * piano_roll_labels =
    PIANO_ROLL->piano_roll_labels;
  RULER_WIDGET_GET_PRIVATE (MW_RULER);
  gtk_widget_get_size_request (
    GTK_WIDGET (piano_roll_labels),
    &ww,
    &hh);
  gtk_widget_set_size_request (
    GTK_WIDGET (self),
    prv->total_px,
    hh);

  gtk_widget_set_can_focus (GTK_WIDGET (self),
                           1);
  gtk_widget_set_focus_on_click (GTK_WIDGET (self),
                                 1);

  g_signal_connect (G_OBJECT (self), "draw",
                    G_CALLBACK (draw_cb), NULL);
  g_signal_connect (G_OBJECT(ab_prv->drag), "drag-begin",
                    G_CALLBACK (drag_begin),  self);
  g_signal_connect (G_OBJECT(ab_prv->drag), "drag-update",
                    G_CALLBACK (drag_update),  self);
  g_signal_connect (G_OBJECT(ab_prv->drag), "drag-end",
                    G_CALLBACK (drag_end),  self);
  g_signal_connect (G_OBJECT (ab_prv->multipress), "pressed",
                    G_CALLBACK (multipress_pressed), self);
}
