/*
 * gui/widgets/channel.c - A channel widget
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
#include "gui/widgets/channel.h"
#include "gui/widgets/fader.h"
#include "gui/widgets/knob.h"
#include "gui/widget_manager.h"

#include <gtk/gtk.h>

/**
 * Draws the color picker.
 */
static int
draw_color_picker_cb (GtkWidget * widget, cairo_t * cr, void* data)
{
  guint width, height;
  GtkStyleContext *context;
  context = gtk_widget_get_style_context (widget);

  width = gtk_widget_get_allocated_width (widget);
  height = gtk_widget_get_allocated_height (widget);

  gtk_render_background (context, cr, 0, 0, width, height);

  /* a custom shape that could be wrapped in a function */
  double x         = 0,        /* parameters like cairo_rectangle */
         y         = 0,
         aspect        = 1.0,     /* aspect ratio */
         corner_radius = height / 8.0;   /* and corner curvature radius */

  double radius = corner_radius / aspect;
  double degrees = G_PI / 180.0;

  cairo_new_sub_path (cr);
  cairo_arc (cr, x + width - radius, y + radius, radius, -90 * degrees, 0 * degrees);
  cairo_arc (cr, x + width - radius, y + height - radius, radius, 0 * degrees, 90 * degrees);
  cairo_arc (cr, x + radius, y + height - radius, radius, 90 * degrees, 180 * degrees);
  cairo_arc (cr, x + radius, y + radius, radius, 180 * degrees, 270 * degrees);
  cairo_close_path (cr);

  cairo_set_source_rgb (cr, 0.2, 0.6, 0.45);
  cairo_fill_preserve (cr);
}

static void
setup_channel_color ()
{
  GtkWidget * da = GET_WIDGET ("gdrawingarea-master-color");

  gtk_widget_set_size_request (da, -1, 10);
  g_signal_connect (G_OBJECT (da), "draw",
                    G_CALLBACK (draw_color_picker_cb), NULL);

}

void
setup_master_channel_gui ()

{
  knob_set (GET_WIDGET ("gdrawingarea-phase-knob"));

  setup_channel_color ();

  fader_set (GET_WIDGET ("gdrawingarea-fader"));

}
