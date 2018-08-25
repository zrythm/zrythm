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

#include "zrythm_app.h"
#include "project.h"
#include "settings_manager.h"
#include "audio/timeline.h"
#include "gui/widgets/timeline.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (TimelineWidget, timeline_widget, GTK_TYPE_DRAWING_AREA)

/**
 * pixels to draw between each beat,
 * before being adjusted for zoom.
 * used by the ruler and timeline
 */
#define PX_PER_BEAT 20

static int px_per_beat;
static int px_per_bar;
static int total_px;
static GtkWidget * gpaned_instruments;

static void
draw_borders (GtkWidget * widget,
              cairo_t * cr,
              int y_offset)
{
  guint height = gtk_widget_get_allocated_height (widget);

  cairo_set_source_rgb (cr, 0.7, 0.7, 0.7);
  cairo_set_line_width (cr, 0.5);
  cairo_move_to (cr, 0, y_offset);
  cairo_line_to (cr, total_px, y_offset);
  cairo_stroke (cr);
}

static gboolean
draw_cb (GtkWidget *widget, cairo_t *cr, gpointer data)
{
  guint width, height;
  GdkRGBA color;
  GtkStyleContext *context;

  context = gtk_widget_get_style_context (widget);

  width = gtk_widget_get_allocated_width (widget);
  height = gtk_widget_get_allocated_height (widget);

  static int playhead_pos_in_px;
  playhead_pos_in_px =
    AUDIO_TIMELINE->playhead_pos.bar * px_per_bar;

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
          cairo_set_source_rgb (cr, 0.25, 0.25, 0.25);
          cairo_set_line_width (cr, 0.5);
          cairo_move_to (cr, i, 0);
          cairo_line_to (cr, i, height);
          cairo_stroke (cr);
      }
      else if (0)
        {

        }
  }

  /* handle horizontal drawing */
  GtkWidget * inner_paned = gpaned_instruments;
  int y_offset = 0;
  do
    {
      GtkWidget * flowbox = gtk_paned_get_child1 (
              GTK_PANED (inner_paned));

      gint wx, wy;
      gtk_widget_translate_coordinates(
                flowbox,
                widget,
                0,
                0,
                &wx,
                &wy);
      draw_borders (flowbox, cr, wy);
      inner_paned = gtk_paned_get_child2 (
            GTK_PANED (inner_paned));
  } while (GTK_IS_PANED (inner_paned));

  gint wx, wy;
  gtk_widget_translate_coordinates(
            inner_paned,
            widget,
            0,
            0,
            &wx,
            &wy);
  draw_borders (inner_paned, cr, wy);

  return 0;
}

TimelineWidget *
timeline_widget_new (GtkWidget * _paned,
                     GtkWidget * overlay)
{
  TimelineWidget * self = g_object_new (TIMELINE_WIDGET_TYPE, NULL);

  gpaned_instruments = _paned;
  int default_px_per_beat = PX_PER_BEAT;

  /* adjust for zoom level */
  px_per_beat = (int) ((float) default_px_per_beat *
                           AUDIO_TIMELINE->zoom_level);

  px_per_bar = px_per_beat *
                  AUDIO_TIMELINE->time_sig_denominator;

  total_px = px_per_bar *
    AUDIO_TIMELINE->total_bars;


  // set the size
  int ww, hh;
  gtk_widget_get_size_request (
    GTK_WIDGET (gpaned_instruments),
    &ww,
    &hh);
  gtk_widget_set_size_request (
    GTK_WIDGET (overlay),
    total_px,
    hh);

  gtk_container_add (GTK_CONTAINER (overlay),
                     GTK_WIDGET (self));

  g_signal_connect (G_OBJECT (self), "draw",
                    G_CALLBACK (draw_cb), NULL);

  return self;
}

static void
timeline_widget_class_init (TimelineWidgetClass * klass)
{
}

static void
timeline_widget_init (TimelineWidget *self )
{
  gtk_widget_show (GTK_WIDGET (self));
}
