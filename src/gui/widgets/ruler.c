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

#include "zrythm_system.h"
#include "project.h"
#include "settings_manager.h"
#include "audio/timeline.h"
#include "gui/widget_manager.h"

#include <gtk/gtk.h>

#define Y_SPACING 9
#define FONT "Monospace"
#define FONT_SIZE 16

static int px_per_beat;
static int px_per_bar;
static int total_px;

static gboolean
draw_callback (GtkWidget *widget, cairo_t *cr, gpointer data)
{
  guint width, height;
  GdkRGBA color;
  GtkStyleContext *context;

  context = gtk_widget_get_style_context (widget);

  /*width = gtk_widget_get_allocated_width (widget);*/
  height = gtk_widget_get_allocated_height (widget);

  static int playhead_pos_in_px;
  playhead_pos_in_px =
    AUDIO_TIMELINE->playhead_pos.bar * px_per_bar;

  static int start_marker_pos_px;
  start_marker_pos_px =
    AUDIO_TIMELINE->start_marker_pos.bar * px_per_bar;
  static int end_marker_pos_px;
  end_marker_pos_px =
    AUDIO_TIMELINE->end_marker_pos.bar * px_per_bar;
  static int loop_start_pos_px;
  loop_start_pos_px =
    AUDIO_TIMELINE->loop_start_pos.bar * px_per_bar;
  static int loop_end_pos_px;
  loop_end_pos_px =
    AUDIO_TIMELINE->loop_end_pos.bar * px_per_bar;

  gtk_render_background (context, cr, 0, 0, total_px, height);

  /* used for ruler numbers */
  int bar_count = 0;
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
          cairo_set_source_rgb (cr, 1, 0, 0);
          cairo_set_line_width (cr, 2);
          cairo_move_to (cr, i, 0);
          cairo_arc (cr, i, 3.0, 3.0, 0, 2*G_PI);
          cairo_fill (cr);
          cairo_move_to (cr, i, 0);
          cairo_line_to (cr, i, height);
          cairo_stroke (cr);
      }
    if (i == start_marker_pos_px ||
        i == end_marker_pos_px)
      {
          cairo_set_source_rgb (cr, 1, 0, 0);
          cairo_set_line_width (cr, 14);
          cairo_move_to (cr, i, 0);
          cairo_line_to (cr, i, 8);
          cairo_stroke (cr);
      }
    if (i == loop_start_pos_px ||
        i == loop_end_pos_px)
      {
          cairo_set_source_rgb (cr, 0, 1, 0);
          cairo_set_line_width (cr, 7);
          cairo_move_to (cr, i, 0);
          cairo_line_to (cr, i, 5);
          cairo_stroke (cr);
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

/**
 * adds callbacks to the drawing area given
 */
void
set_ruler (GtkWidget * drawing_area)
{
  int default_px_per_beat = PX_PER_BEAT;

  /* adjust for zoom level */
  px_per_beat = (int) ((float) default_px_per_beat *
                           AUDIO_TIMELINE->zoom_level);

  px_per_bar = px_per_beat *
                  AUDIO_TIMELINE->time_sig_denominator;

  total_px = px_per_bar *
    AUDIO_TIMELINE->total_bars;


  // set the size
  gtk_widget_set_size_request (
    GTK_WIDGET (drawing_area),
    total_px,
    -1);

  g_signal_connect (G_OBJECT (drawing_area), "draw",
                    G_CALLBACK (draw_callback), NULL);
}
