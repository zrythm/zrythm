/*
 * gui/widgets/timeline.c - The timeline containing regions
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

#include "project.h"
#include "audio/timeline.h"

#include <gtk/gtk.h>

#define DEFAULT_PX_PER_BEAT 60.0f

static int px_per_beat;
static int px_per_bar;
static int total_px;
static GtkWidget * multi_paned;

static void
draw_borders (GtkWidget * widget,
              gpointer data)
{
  static int y_offset = 0;

  cairo_t *cr = (cairo_t *) data;
  guint height = gtk_widget_get_allocated_height (widget);
  g_message ("%d", height);

  cairo_set_source_rgb (cr, 0.7, 0.7, 0.7);
  cairo_set_line_width (cr, 0.5);
  cairo_move_to (cr, 0, y_offset + height);
  cairo_line_to (cr, 300, y_offset + height);
  cairo_stroke (cr);

  y_offset += height;
}

static gboolean
draw_callback (GtkWidget *widget, cairo_t *cr, gpointer data)
{
  guint width, height;
  GdkRGBA color;
  GtkStyleContext *context;

  context = gtk_widget_get_style_context (widget);

  width = gtk_widget_get_allocated_width (widget);
  height = gtk_widget_get_allocated_height (widget);

  static int playhead_pos_in_px;
  playhead_pos_in_px =
    project->timeline->playhead_pos.bar * px_per_bar;

  gtk_render_background (context, cr, 0, 0, total_px, height);

  /* used for ruler numbers */
  int count = 0;

  /* handle vertical drawing */
  for (int i = 0; i < total_px; i++)
  {
    if (i == playhead_pos_in_px)
      {
          cairo_set_source_rgb (cr, 1, 0, 0);
          cairo_set_line_width (cr, 2);
          cairo_move_to (cr, i, 0);
          cairo_line_to (cr, i, height);
          cairo_stroke (cr);
      }
      if (i % px_per_bar == 0)
      {
          cairo_set_source_rgb (cr, 0.3, 0.3, 0.3);
          cairo_set_line_width (cr, 1);
          cairo_move_to (cr, i, 0);
          cairo_line_to (cr, i, height);
          cairo_stroke (cr);
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
  }

  /* handle horizontal drawing */
  gtk_container_forall (
    GTK_CONTAINER (multi_paned),
    draw_borders,
    cr);


 return FALSE;
}

/**
 * adds callbacks to the drawing area given
 */
void
set_timeline (GtkWidget * _multi_paned,
              GtkWidget * overlay,
              GtkWidget * drawing_area)
{
  multi_paned = _multi_paned;
  int default_px_per_beat =
    g_variant_get_int32 (
      g_settings_get_value (project->settings,
                            "pixels-per-beat"));

  /* adjust for zoom level */
  px_per_beat = (int) ((float) default_px_per_beat *
                           project->timeline->zoom_level);

  px_per_bar = px_per_beat *
                  project->timeline->time_sig_denominator;

  total_px = px_per_bar *
    project->timeline->total_bars;


  // set the size
  gtk_widget_set_size_request (
    GTK_WIDGET (overlay),
    total_px,
    gtk_widget_get_allocated_height (multi_paned));

  g_signal_connect (G_OBJECT (drawing_area), "draw",
                    G_CALLBACK (draw_callback), NULL);
}
