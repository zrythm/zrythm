/*
 * gui/widgets/ruler_playhead.c - Ruler playhead
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

/** \file
 */

#include "gui/widgets/ruler_playhead.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (RulerPlayheadWidget,
               ruler_playhead_widget,
               GTK_TYPE_DRAWING_AREA)

static gboolean
draw_cb (GtkWidget *widget, cairo_t *cr, gpointer data)
{
  GtkStyleContext *context;

  context = gtk_widget_get_style_context (widget);

  guint width = gtk_widget_get_allocated_width (widget);
  guint height = gtk_widget_get_allocated_height (widget);

  gtk_render_background (context, cr, 0, 0, width, height);

  cairo_set_source_rgb (cr, 0.7, 0.7, 0.7);
  cairo_set_line_width (cr, 2);
  cairo_move_to (cr, 0, 0);
  cairo_line_to (cr, width / 2, height);
  cairo_line_to (cr,
                 width,
                 0);
  cairo_fill (cr);

  return 0;
}

RulerPlayheadWidget *
ruler_playhead_widget_new ()
{
  RulerPlayheadWidget * self =
    g_object_new (RULER_PLAYHEAD_WIDGET_TYPE,
                  NULL);

  return self;
}

/**
 * GTK boilerplate.
 */
static void
ruler_playhead_widget_init (RulerPlayheadWidget * self)
{
  /* connect signal */
  g_signal_connect (GTK_WIDGET (self),
                    "draw",
                    G_CALLBACK (draw_cb), NULL);

  /* show */
  gtk_widget_set_visible (GTK_WIDGET (self),
                          1);
}

static void
ruler_playhead_widget_class_init (
  RulerPlayheadWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (klass,
                                 "ruler-playhead");
}

