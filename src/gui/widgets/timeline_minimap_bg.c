/*
 * gui/widgets/timeline_minimap_bg.h - Minimap
 *   bg
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

#include "gui/widgets/timeline_minimap_bg.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (TimelineMinimapBgWidget,
               timeline_minimap_bg_widget,
               GTK_TYPE_DRAWING_AREA)

static gboolean
draw_cb (GtkWidget *widget, cairo_t *cr, gpointer data)
{
  GtkStyleContext * context =
    gtk_widget_get_style_context (widget);

  guint width, height;
  width = gtk_widget_get_allocated_width (widget);
  height = gtk_widget_get_allocated_height (widget);

  gtk_render_background (context, cr, 0, 0, width, height);

  return FALSE;
}

TimelineMinimapBgWidget *
timeline_minimap_bg_widget_new ()
{
  TimelineMinimapBgWidget * self =
    g_object_new (TIMELINE_MINIMAP_BG_WIDGET_TYPE,
                  NULL);

  return self;
}

static void
timeline_minimap_bg_widget_class_init (
  TimelineMinimapBgWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (klass,
                                 "timeline-minimap-bg");
}

static void
timeline_minimap_bg_widget_init (
  TimelineMinimapBgWidget * self)
{
  gtk_widget_set_visible (GTK_WIDGET (self),
                          1);

  g_signal_connect (G_OBJECT (self), "draw",
                    G_CALLBACK (draw_cb), NULL);
}
