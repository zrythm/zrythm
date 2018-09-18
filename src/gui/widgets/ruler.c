/*
 * gui/widgets/ruler.cpp - The ruler on top of the timeline
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
#include "gui/widgets/ruler.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (RulerWidget, ruler_widget, GTK_TYPE_DRAWING_AREA)

/**
 * pixels to draw between each beat,
 * before being adjusted for zoom.
 * used by the ruler and timeline
 */
#define DEFAULT_PX_PER_TICK 0.03f
#define Y_SPACING 5
#define FONT "Monospace"
#define FONT_SIZE 14

#define PLAYHEAD_TRIANGLE_HALF_WIDTH 6
#define PLAYHEAD_TRIANGLE_HEIGHT 8
#define START_MARKER_TRIANGLE_HEIGHT 8
#define START_MARKER_TRIANGLE_WIDTH 8
#define Q_HEIGHT 12
#define Q_WIDTH 7

static int px_per_beat;
static int px_per_bar;
static int px_per_quarter_beat;
static float px_per_tick;
static int total_px;

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
  playhead_pos_in_px =
    (TRANSPORT->playhead_pos.bars - 1) * px_per_bar +
    (TRANSPORT->playhead_pos.beats - 1) * px_per_beat +
    (TRANSPORT->playhead_pos.quarter_beats - 1) * px_per_quarter_beat +
    TRANSPORT->playhead_pos.ticks * px_per_tick;
  static int q_pos_in_px;
  q_pos_in_px =
    (TRANSPORT->q_pos.bars - 1) * px_per_bar +
    (TRANSPORT->q_pos.beats - 1) * px_per_beat +
    (TRANSPORT->q_pos.quarter_beats - 1) * px_per_quarter_beat +
    TRANSPORT->q_pos.ticks * px_per_tick;
  static int start_marker_pos_px;
  start_marker_pos_px =
    (TRANSPORT->start_marker_pos.bars - 1) * px_per_bar;
  static int end_marker_pos_px;
  end_marker_pos_px =
    (TRANSPORT->end_marker_pos.bars - 1) * px_per_bar;
  static int loop_start_pos_px;
  loop_start_pos_px =
    (TRANSPORT->loop_start_pos.bars - 1 ) * px_per_bar;
  static int loop_end_pos_px;
  loop_end_pos_px =
    (TRANSPORT->loop_end_pos.bars - 1) * px_per_bar;

  gtk_render_background (context, cr, 0, 0, total_px, height);

  /* draw lines */
  int bar_count = 1;
  for (int i = 0; i < total_px; i++)
  {
      if (i % px_per_bar == 0)
      {
          cairo_set_source_rgb (cr, 1, 1, 1);
          cairo_set_line_width (cr, 1);
          cairo_move_to (cr, i, 0);
          cairo_line_to (cr, i, height / 3);
          cairo_stroke (cr);
          cairo_set_source_rgb (cr, 0.8, 0.8, 0.8);
          cairo_select_font_face(cr, FONT,
              CAIRO_FONT_SLANT_NORMAL,
              CAIRO_FONT_WEIGHT_NORMAL);
          cairo_set_font_size(cr, FONT_SIZE);
          gchar * label = g_strdup_printf ("%d", bar_count);
          static cairo_text_extents_t extents;
          cairo_text_extents(cr, label, &extents);
          cairo_move_to (cr, i - extents.width / 2,
                         (height / 2) + Y_SPACING);
          cairo_show_text(cr, label);
          bar_count++;
      }
      else if (i % px_per_beat == 0)
      {
          cairo_set_source_rgb (cr, 0.7, 0.7, 0.7);
          cairo_set_line_width (cr, 0.5);
          cairo_move_to (cr, i, 0);
          cairo_line_to (cr, i, height / 4);
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
                         i - PLAYHEAD_TRIANGLE_HALF_WIDTH,
                         height - PLAYHEAD_TRIANGLE_HEIGHT);
          cairo_line_to (cr, i, height);
          cairo_line_to (cr,
                         i + PLAYHEAD_TRIANGLE_HALF_WIDTH,
                         height - PLAYHEAD_TRIANGLE_HEIGHT);
          cairo_fill (cr);
      }
    if (i == q_pos_in_px)
      {
          cairo_set_source_rgb (cr, 0, 0.6, 0.9);
          cairo_set_line_width (cr, 2);
          cairo_move_to (
            cr,
            i,
            ((height - PLAYHEAD_TRIANGLE_HEIGHT) - Q_HEIGHT) - 1);
          cairo_line_to (
            cr,
            i + Q_WIDTH,
            ((height - PLAYHEAD_TRIANGLE_HEIGHT) - Q_HEIGHT / 2) - 1);
          cairo_line_to (
            cr,
            i,
            (height - PLAYHEAD_TRIANGLE_HEIGHT) - 1);
          cairo_fill (cr);
      }
    if (i == start_marker_pos_px)
      {
          cairo_set_source_rgb (cr, 1, 0, 0);
          cairo_set_line_width (cr, 2);
          cairo_move_to (cr, i, 0);
          cairo_line_to (cr, i + START_MARKER_TRIANGLE_WIDTH,
                         0);
          cairo_line_to (cr, i,
                         START_MARKER_TRIANGLE_HEIGHT);
          cairo_fill (cr);
      }
    if (i == end_marker_pos_px)
      {
          cairo_set_source_rgb (cr, 1, 0, 0);
          cairo_set_line_width (cr, 2);
          cairo_move_to (cr, i - START_MARKER_TRIANGLE_WIDTH, 0);
          cairo_line_to (cr, i, 0);
          cairo_line_to (cr, i, START_MARKER_TRIANGLE_HEIGHT);
          cairo_fill (cr);
      }
    if (i == loop_start_pos_px)
      {
          cairo_set_source_rgb (cr, 0, 0.9, 0.7);
          cairo_set_line_width (cr, 2);
          cairo_move_to (cr, i, START_MARKER_TRIANGLE_HEIGHT + 1);
          cairo_line_to (cr, i, START_MARKER_TRIANGLE_HEIGHT * 2 + 1);
          cairo_line_to (cr, i + START_MARKER_TRIANGLE_WIDTH,
                         START_MARKER_TRIANGLE_HEIGHT + 1);
          cairo_fill (cr);
      }
    if (i == loop_end_pos_px)
      {
          cairo_set_source_rgb (cr, 0, 0.9, 0.7);
          cairo_set_line_width (cr, 2);
          cairo_move_to (cr, i, START_MARKER_TRIANGLE_HEIGHT + 1);
          cairo_line_to (cr, i - START_MARKER_TRIANGLE_WIDTH,
                         START_MARKER_TRIANGLE_HEIGHT + 1);
          cairo_line_to (cr, i,
                         START_MARKER_TRIANGLE_HEIGHT * 2 + 1);
          cairo_fill (cr);
      }

  }

 return FALSE;
}

static gboolean
tick_callback (GtkWidget * widget, GdkFrameClock * frame_clock,
               gpointer user_data)
{
  gtk_widget_queue_draw (widget);
  return G_SOURCE_CONTINUE;
}

/**
 * TODO: for updating the global static variables
 * when needed
 */
void
reset_ruler ()
{

}

RulerWidget *
ruler_widget_new ()
{
  g_message ("Creating ruler...");
  RulerWidget * self = g_object_new (RULER_WIDGET_TYPE, NULL);

  /* adjust for zoom level */
  px_per_tick = (DEFAULT_PX_PER_TICK * TRANSPORT->zoom_level);
  px_per_quarter_beat = (int) (px_per_tick * TICKS_PER_QUARTER_BEAT);
  px_per_beat = (int) (px_per_tick * TICKS_PER_BEAT);
  px_per_bar = px_per_beat * TRANSPORT->beats_per_bar;

  total_px = px_per_bar *
    (TRANSPORT->total_bars);


  // set the size
  gtk_widget_set_size_request (
    GTK_WIDGET (self),
    total_px,
    -1);

  g_signal_connect (G_OBJECT (self), "draw",
                    G_CALLBACK (draw_cb), NULL);

  gtk_widget_add_tick_callback (GTK_WIDGET (self),
                                tick_callback,
                                NULL,
                                NULL);

  return self;
}

static void
ruler_widget_class_init (RulerWidgetClass * klass)
{
}

static void
ruler_widget_init (RulerWidget * self)
{
  gtk_widget_show (GTK_WIDGET (self));
}
