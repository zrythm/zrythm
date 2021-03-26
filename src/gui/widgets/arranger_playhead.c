/*
 * gui/widgets/arranger_playhead.c - Arranger playhead
 *
 * Copyright (C) 2019 Alexandros Theodotou
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/** \file
 */

#include "gui/widgets/arranger_playhead.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (ArrangerPlayheadWidget,
               arranger_playhead_widget,
               GTK_TYPE_DRAWING_AREA)

static gboolean
arranger_playhead_draw_cb (
  GtkWidget *widget,
  cairo_t *cr,
  gpointer data)
{
  GtkStyleContext *context;

  context = gtk_widget_get_style_context (widget);

  int width =
    gtk_widget_get_allocated_width (widget);
  int height =
    gtk_widget_get_allocated_height (widget);

  gtk_render_background (
    context, cr, 0, 0, width, height);

  return 0;
}

ArrangerPlayheadWidget *
arranger_playhead_widget_new ()
{
  ArrangerPlayheadWidget * self =
    g_object_new (ARRANGER_PLAYHEAD_WIDGET_TYPE,
                  NULL);

  return self;
}

/**
 * GTK boilerplate.
 */
static void
arranger_playhead_widget_init (
  ArrangerPlayheadWidget * self)
{
  /* connect signal */
  g_signal_connect (
    GTK_WIDGET (self), "draw",
    G_CALLBACK (arranger_playhead_draw_cb), NULL);

  /* show */
  gtk_widget_set_visible (GTK_WIDGET (self),
                          1);
}

static void
arranger_playhead_widget_class_init (
  ArrangerPlayheadWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (klass,
                                 "arranger-playhead");
}
