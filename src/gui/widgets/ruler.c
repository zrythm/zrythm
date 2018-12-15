/*
 * gui/widgets/ruler.c- The ruler on top of the timeline
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

#include "zrythm_app.h"
#include "project.h"
#include "settings_manager.h"
#include "audio/position.h"
#include "audio/transport.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline_ruler.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE_WITH_PRIVATE (RulerWidget,
                            ruler_widget,
                            GTK_TYPE_DRAWING_AREA)

#define Y_SPACING 5
#define FONT "Monospace"
#define FONT_SIZE 14

#define PLAYHEAD_TRIANGLE_HALF_WIDTH 6
#define PLAYHEAD_TRIANGLE_HEIGHT 8
#define START_MARKER_TRIANGLE_HEIGHT 8
#define START_MARKER_TRIANGLE_WIDTH 8
#define Q_HEIGHT 12
#define Q_WIDTH 7

#define GET_PRIVATE RULER_WIDGET_GET_PRIVATE (self)

RulerWidgetPrivate *
ruler_widget_get_private (RulerWidget * self)
{
  return ruler_widget_get_instance_private (self);
}

void
ruler_widget_px_to_pos (
  RulerWidget * self,
  Position *    pos, ///< position to fill in
  int           px) ///< pixels
{
  GET_PRIVATE;

  pos->bars = px / prv->px_per_bar + 1;
  px = px % prv->px_per_bar;
  pos->beats = px / prv->px_per_beat + 1;
  px = px % prv->px_per_beat;
  pos->quarter_beats = px / prv->px_per_quarter_beat + 1;
  px = px % prv->px_per_quarter_beat;
  pos->ticks = px / prv->px_per_tick;
  /*g_message ("%d %d %d %d",*/
             /*pos->bars,*/
             /*pos->beats,*/
             /*pos->quarter_beats,*/
             /*pos->ticks);*/
}

int
ruler_widget_pos_to_px (RulerWidget * self,
                        Position * pos)
{
  GET_PRIVATE;

  return  (pos->bars - 1) * prv->px_per_bar +
    (pos->beats - 1) * prv->px_per_beat +
    (pos->quarter_beats - 1) * prv->px_per_quarter_beat +
    pos->ticks * prv->px_per_tick;
}

static gboolean
draw_cb (RulerWidget * self, cairo_t *cr, gpointer data)
{
  GET_PRIVATE;

  GtkStyleContext *context;

  context = gtk_widget_get_style_context (GTK_WIDGET (self));

  /*width = gtk_widget_get_allocated_width (widget);*/
  guint height = gtk_widget_get_allocated_height (GTK_WIDGET (self));

  /* get positions in px */
  int playhead_pos_in_px;
  playhead_pos_in_px =
    ruler_widget_pos_to_px (self,
                            &TRANSPORT->playhead_pos);
  int cue_pos_in_px;
  cue_pos_in_px =
    ruler_widget_pos_to_px (self,
                            &TRANSPORT->cue_pos);
  int start_marker_pos_px;
  start_marker_pos_px =
    ruler_widget_pos_to_px (self,
                            &TRANSPORT->start_marker_pos);
  int end_marker_pos_px;
  end_marker_pos_px =
    ruler_widget_pos_to_px (self,
                            &TRANSPORT->end_marker_pos);
  int loop_start_pos_px;
  loop_start_pos_px =
    ruler_widget_pos_to_px (self,
                            &TRANSPORT->loop_start_pos);
  int loop_end_pos_px;
  loop_end_pos_px =
    ruler_widget_pos_to_px (self,
                            &TRANSPORT->loop_end_pos);

  gtk_render_background (context, cr, 0, 0, prv->total_px, height);

  /* draw lines */
  int bar_count = 1;
  for (int i = 0; i < prv->total_px - SPACE_BEFORE_START; i++)
    {
      int draw_pos = i + SPACE_BEFORE_START;

      if (i % prv->px_per_bar == 0)
      {
          cairo_set_source_rgb (cr, 1, 1, 1);
          cairo_set_line_width (cr, 1);
          cairo_move_to (cr, draw_pos, 0);
          cairo_line_to (cr, draw_pos, height / 3);
          cairo_stroke (cr);
          cairo_set_source_rgb (cr, 0.8, 0.8, 0.8);
          cairo_select_font_face(cr, FONT,
              CAIRO_FONT_SLANT_NORMAL,
              CAIRO_FONT_WEIGHT_NORMAL);
          cairo_set_font_size(cr, FONT_SIZE);
          gchar * label = g_strdup_printf ("%d", bar_count);
          static cairo_text_extents_t extents;
          cairo_text_extents(cr, label, &extents);
          cairo_move_to (cr,
                         (draw_pos ) - extents.width / 2,
                         (height / 2) + Y_SPACING);
          cairo_show_text(cr, label);
          bar_count++;
      }
      else if (i % prv->px_per_beat == 0)
      {
          cairo_set_source_rgb (cr, 0.7, 0.7, 0.7);
          cairo_set_line_width (cr, 0.5);
          cairo_move_to (cr, draw_pos, 0);
          cairo_line_to (cr, draw_pos, height / 4);
          cairo_stroke (cr);
      }
      else if (0)
        {

        }
    if (i == playhead_pos_in_px)
      {
          cairo_set_source_rgb (cr, 0.7, 0.7, 0.7);
          cairo_set_line_width (cr, 2);
          cairo_move_to (cr,
                         draw_pos - PLAYHEAD_TRIANGLE_HALF_WIDTH,
                         height - PLAYHEAD_TRIANGLE_HEIGHT);
          cairo_line_to (cr, draw_pos, height);
          cairo_line_to (cr,
                         draw_pos + PLAYHEAD_TRIANGLE_HALF_WIDTH,
                         height - PLAYHEAD_TRIANGLE_HEIGHT);
          cairo_fill (cr);
      }
    if (i == cue_pos_in_px)
      {
          cairo_set_source_rgb (cr, 0, 0.6, 0.9);
          cairo_set_line_width (cr, 2);
          cairo_move_to (
            cr,
            draw_pos,
            ((height - PLAYHEAD_TRIANGLE_HEIGHT) - Q_HEIGHT) - 1);
          cairo_line_to (
            cr,
            draw_pos + Q_WIDTH,
            ((height - PLAYHEAD_TRIANGLE_HEIGHT) - Q_HEIGHT / 2) - 1);
          cairo_line_to (
            cr,
            draw_pos,
            (height - PLAYHEAD_TRIANGLE_HEIGHT) - 1);
          cairo_fill (cr);
      }
    if (i == start_marker_pos_px)
      {
          cairo_set_source_rgb (cr, 1, 0, 0);
          cairo_set_line_width (cr, 2);
          cairo_move_to (cr, draw_pos, 0);
          cairo_line_to (cr, draw_pos + START_MARKER_TRIANGLE_WIDTH,
                         0);
          cairo_line_to (cr, draw_pos,
                         START_MARKER_TRIANGLE_HEIGHT);
          cairo_fill (cr);
      }
    if (i == end_marker_pos_px)
      {
          cairo_set_source_rgb (cr, 1, 0, 0);
          cairo_set_line_width (cr, 2);
          cairo_move_to (cr, draw_pos - START_MARKER_TRIANGLE_WIDTH, 0);
          cairo_line_to (cr, draw_pos, 0);
          cairo_line_to (cr, draw_pos, START_MARKER_TRIANGLE_HEIGHT);
          cairo_fill (cr);
      }
    if (i == loop_start_pos_px)
      {
          cairo_set_source_rgb (cr, 0, 0.9, 0.7);
          cairo_set_line_width (cr, 2);
          cairo_move_to (cr, draw_pos, START_MARKER_TRIANGLE_HEIGHT + 1);
          cairo_line_to (cr, draw_pos, START_MARKER_TRIANGLE_HEIGHT * 2 + 1);
          cairo_line_to (cr, draw_pos + START_MARKER_TRIANGLE_WIDTH,
                         START_MARKER_TRIANGLE_HEIGHT + 1);
          cairo_fill (cr);
      }
    if (i == loop_end_pos_px)
      {
          cairo_set_source_rgb (cr, 0, 0.9, 0.7);
          cairo_set_line_width (cr, 2);
          cairo_move_to (cr, draw_pos, START_MARKER_TRIANGLE_HEIGHT + 1);
          cairo_line_to (cr, draw_pos - START_MARKER_TRIANGLE_WIDTH,
                         START_MARKER_TRIANGLE_HEIGHT + 1);
          cairo_line_to (cr, draw_pos,
                         START_MARKER_TRIANGLE_HEIGHT * 2 + 1);
          cairo_fill (cr);
      }
  }

 return FALSE;
}

/**
 * TODO: for updating the global static variables
 * when needed
 */
void
reset_ruler ()
{

}

static gboolean
multipress_pressed (GtkGestureMultiPress *gesture,
               gint                  n_press,
               gdouble               x,
               gdouble               y,
               gpointer              user_data)
{
  RulerWidget * self = RULER_WIDGET (user_data);
  if (n_press == 2)
    {
      Position pos;
      ruler_widget_px_to_pos (
        self,
        &pos,
        x - SPACE_BEFORE_START);
      position_set_to_pos (&TRANSPORT->cue_pos,
                           &pos);
    }
  return FALSE;
}

static void
drag_begin (GtkGestureDrag * gesture,
               gdouble         start_x,
               gdouble         start_y,
               gpointer        user_data)
{
  RulerWidget * self = RULER_WIDGET (user_data);
  GET_PRIVATE;

  prv->start_x = start_x;
  Position pos;
  ruler_widget_px_to_pos (
    self,
    &pos,
    start_x - SPACE_BEFORE_START);
  transport_move_playhead (&pos, 1);
}

static void
drag_update (GtkGestureDrag * gesture,
               gdouble         offset_x,
               gdouble         offset_y,
               gpointer        user_data)
{
  RulerWidget * self = RULER_WIDGET (user_data);
  GET_PRIVATE;

  Position pos;
  ruler_widget_px_to_pos (
    self,
    &pos,
    (prv->start_x + offset_x) - SPACE_BEFORE_START);
  transport_move_playhead (&pos, 1);
}

static void
drag_end (GtkGestureDrag *gesture,
               gdouble         offset_x,
               gdouble         offset_y,
               gpointer        user_data)
{
  RulerWidget * self = (RulerWidget *) user_data;
  GET_PRIVATE;

  prv->start_x = 0;
}

/* FIXME delete */
RulerWidget *
ruler_widget_new ()
{
  return NULL;
}

static void
ruler_widget_class_init (RulerWidgetClass * klass)
{
}

static void
ruler_widget_init (RulerWidget * self)
{
  g_message ("initing ruler...");
  GET_PRIVATE;

  /* make it able to notify */
  gtk_widget_add_events (GTK_WIDGET (self),
                         GDK_ALL_EVENTS_MASK);

  prv->drag = GTK_GESTURE_DRAG (
                gtk_gesture_drag_new (GTK_WIDGET (self)));
  prv->multipress = GTK_GESTURE_MULTI_PRESS (
                gtk_gesture_multi_press_new (GTK_WIDGET (self)));

  /*adjust for zoom level*/
  float px_per_tick =
    DEFAULT_PX_PER_TICK * TRANSPORT->zoom_level;
  unsigned int px_per_quarter_beat =
    px_per_tick * TICKS_PER_QUARTER_BEAT;
  unsigned int px_per_beat =
    px_per_tick * TICKS_PER_BEAT;
  unsigned int px_per_bar =
    px_per_beat * TRANSPORT->beats_per_bar;
  prv->px_per_tick = px_per_tick;
  GTK_WIDGET (self);
  prv->px_per_quarter_beat = px_per_quarter_beat;
  GTK_WIDGET (self);
  prv->px_per_beat = px_per_beat;
  GTK_WIDGET (self);
  prv->px_per_bar = px_per_bar;
  GTK_WIDGET (self);

  prv->total_px = px_per_bar * TRANSPORT->total_bars;
  GTK_WIDGET (self);

  // set the size
  gtk_widget_set_size_request (
    GTK_WIDGET (self),
    prv->total_px,
    -1);

  /* FIXME drags */
  g_signal_connect (G_OBJECT (self), "draw",
                    G_CALLBACK (draw_cb), NULL);
  /*g_signal_connect (G_OBJECT(self), "button_press_event",*/
                    /*G_CALLBACK (button_press_cb),  self);*/
  g_signal_connect (G_OBJECT(prv->drag), "drag-begin",
                    G_CALLBACK (drag_begin),  self);
  g_signal_connect (G_OBJECT(prv->drag), "drag-update",
                    G_CALLBACK (drag_update),  self);
  g_signal_connect (G_OBJECT(prv->drag), "drag-end",
                    G_CALLBACK (drag_end),  self);
  g_signal_connect (G_OBJECT (prv->multipress), "pressed",
                    G_CALLBACK (multipress_pressed), self);
}
