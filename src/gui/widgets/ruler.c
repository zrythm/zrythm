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
#include "gui/widgets/main_window.h"
#include "gui/widgets/ruler.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (RulerWidget, ruler_widget, GTK_TYPE_DRAWING_AREA)

#define Y_SPACING 5
#define FONT "Monospace"
#define FONT_SIZE 14

#define PLAYHEAD_TRIANGLE_HALF_WIDTH 6
#define PLAYHEAD_TRIANGLE_HEIGHT 8
#define START_MARKER_TRIANGLE_HEIGHT 8
#define START_MARKER_TRIANGLE_WIDTH 8
#define Q_HEIGHT 12
#define Q_WIDTH 7

void
ruler_widget_px_to_pos (
           Position * pos, ///< position to fill in
           int      px) ///< pixels
{
  /*g_message ("%d", px);*/
  RulerWidget * self = MAIN_WINDOW->ruler;
  pos->bars = px / self->px_per_bar + 1;
  px = px % self->px_per_bar;
  pos->beats = px / self->px_per_beat + 1;
  px = px % self->px_per_beat;
  pos->quarter_beats = px / self->px_per_quarter_beat + 1;
  px = px % self->px_per_quarter_beat;
  pos->ticks = px / self->px_per_tick;
  /*g_message ("%d %d %d %d",*/
             /*pos->bars,*/
             /*pos->beats,*/
             /*pos->quarter_beats,*/
             /*pos->ticks);*/
}

int
ruler_widget_pos_to_px (Position * pos)
{
  RulerWidget * self = MAIN_WINDOW->ruler;
  return  (pos->bars - 1) * self->px_per_bar +
    (pos->beats - 1) * self->px_per_beat +
    (pos->quarter_beats - 1) * self->px_per_quarter_beat +
    pos->ticks * self->px_per_tick;
}

static gboolean
draw_cb (RulerWidget * self, cairo_t *cr, gpointer data)
{
  guint width, height;
  GdkRGBA color;
  GtkStyleContext *context;

  context = gtk_widget_get_style_context (GTK_WIDGET (self));

  /*width = gtk_widget_get_allocated_width (widget);*/
  height = gtk_widget_get_allocated_height (GTK_WIDGET (self));

  /* get positions in px */
  static int playhead_pos_in_px;
  playhead_pos_in_px = ruler_widget_pos_to_px (&TRANSPORT->playhead_pos);
  static int cue_pos_in_px;
  cue_pos_in_px = ruler_widget_pos_to_px (&TRANSPORT->cue_pos);
  static int start_marker_pos_px;
  start_marker_pos_px = ruler_widget_pos_to_px (&TRANSPORT->start_marker_pos);
  static int end_marker_pos_px;
  end_marker_pos_px = ruler_widget_pos_to_px (&TRANSPORT->end_marker_pos);
  static int loop_start_pos_px;
  loop_start_pos_px = ruler_widget_pos_to_px (&TRANSPORT->loop_start_pos);
  static int loop_end_pos_px;
  loop_end_pos_px = ruler_widget_pos_to_px (&TRANSPORT->loop_end_pos);

  gtk_render_background (context, cr, 0, 0, self->total_px, height);

  /* draw lines */
  int bar_count = 1;
  for (int i = 0; i < self->total_px - SPACE_BEFORE_START; i++)
    {
      int draw_pos = i + SPACE_BEFORE_START;

      if (i % self->px_per_bar == 0)
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
      else if (i % self->px_per_beat == 0)
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
  if (n_press == 2)
    {
      Position pos;
      ruler_widget_px_to_pos (&pos,
                 x - SPACE_BEFORE_START);
      position_set_to_pos (&TRANSPORT->cue_pos,
                           &pos);
    }
}

static void
drag_begin (GtkGestureDrag * gesture,
               gdouble         start_x,
               gdouble         start_y,
               gpointer        user_data)
{
  RulerWidget * self = (RulerWidget *) user_data;
  self->start_x = start_x;
  Position pos;
  ruler_widget_px_to_pos (&pos,
             start_x - SPACE_BEFORE_START);
  transport_move_playhead (&pos, 1);
}

static void
drag_update (GtkGestureDrag * gesture,
               gdouble         offset_x,
               gdouble         offset_y,
               gpointer        user_data)
{
  RulerWidget * self = (RulerWidget *) user_data;
  Position pos;
  ruler_widget_px_to_pos (&pos,
             (self->start_x + offset_x) - SPACE_BEFORE_START);
  transport_move_playhead (&pos, 1);
}

static void
drag_end (GtkGestureDrag *gesture,
               gdouble         offset_x,
               gdouble         offset_y,
               gpointer        user_data)
{
  RulerWidget * self = (RulerWidget *) user_data;
  self->start_x = 0;
}

RulerWidget *
ruler_widget_new ()
{
  g_message ("Creating ruler...");
  RulerWidget * self = g_object_new (RULER_WIDGET_TYPE, NULL);

  /* adjust for zoom level */
  self->px_per_tick = (DEFAULT_PX_PER_TICK * TRANSPORT->zoom_level);
  self->px_per_quarter_beat = (int) (self->px_per_tick * TICKS_PER_QUARTER_BEAT);
  self->px_per_beat = (int) (self->px_per_tick * TICKS_PER_BEAT);
  self->px_per_bar = self->px_per_beat * TRANSPORT->beats_per_bar;

  self->total_px = self->px_per_bar *
    (TRANSPORT->total_bars);


  // set the size
  gtk_widget_set_size_request (
    GTK_WIDGET (self),
    self->total_px,
    -1);

  /* FIXME drags */
  g_signal_connect (G_OBJECT (self), "draw",
                    G_CALLBACK (draw_cb), NULL);
  /*g_signal_connect (G_OBJECT(self), "button_press_event",*/
                    /*G_CALLBACK (button_press_cb),  self);*/
  g_signal_connect (G_OBJECT(self->drag), "drag-begin",
                    G_CALLBACK (drag_begin),  self);
  g_signal_connect (G_OBJECT(self->drag), "drag-update",
                    G_CALLBACK (drag_update),  self);
  g_signal_connect (G_OBJECT(self->drag), "drag-end",
                    G_CALLBACK (drag_end),  self);
  g_signal_connect (G_OBJECT (self->multipress), "pressed",
                    G_CALLBACK (multipress_pressed), self);

  return self;
}

static void
ruler_widget_class_init (RulerWidgetClass * klass)
{
}

static void
ruler_widget_init (RulerWidget * self)
{
  /* make it able to notify */
  gtk_widget_add_events (GTK_WIDGET (self),
                         GDK_ALL_EVENTS_MASK);

  self->drag = GTK_GESTURE_DRAG (
                gtk_gesture_drag_new (GTK_WIDGET (self)));
  self->multipress = GTK_GESTURE_MULTI_PRESS (
                gtk_gesture_multi_press_new (GTK_WIDGET (self)));
}
