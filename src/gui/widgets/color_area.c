/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/track.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/track.h"
#include "gui/widgets/track_top_grid.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (ColorAreaWidget,
               color_area_widget,
               GTK_TYPE_DRAWING_AREA)

/**
 * Draws the color picker.
 */
static int
color_area_draw_cb (
  GtkWidget *       widget,
  cairo_t *         cr,
  ColorAreaWidget * self)
{
  GtkStyleContext * context =
    gtk_widget_get_style_context (widget);

  guint width =
    gtk_widget_get_allocated_width (widget);
  guint height =
    gtk_widget_get_allocated_height (widget);

  gtk_render_background (
    context, cr, 0, 0, width, height);

  GdkRGBA * color;
  if (self->type == COLOR_AREA_TYPE_TRACK)
    color = &self->track->color;
  else
    color = self->color;

  cairo_rectangle (cr, 0, 0, width, height);
  gdk_cairo_set_source_rgba (cr, color);
  cairo_fill (cr);

  /* show track icon */
  if (self->type == COLOR_AREA_TYPE_TRACK)
    {
      TRACK_WIDGET_GET_PRIVATE (self->track->widget);

      if (tw_prv->icon)
        {
          cairo_surface_t * surface =
            gdk_cairo_surface_create_from_pixbuf (
              tw_prv->icon,
              0,
              NULL);

          /* add shadow in the back */
          cairo_set_source_rgba (
            cr, 0, 0, 0, 0.4);
          cairo_mask_surface(
            cr, surface, 2, 2);
          cairo_fill(cr);

          /* add main icon */
          cairo_set_source_rgba (
            cr, 0.8, 0.8, 0.8, 1);
          /*cairo_set_source_surface (*/
            /*cr, surface, 1, 1);*/
          cairo_mask_surface(
            cr, surface, 1, 1);
          cairo_fill (cr);
        }
    }

  return FALSE;
}

/**
 * Creates a generic color widget using the given
 * color pointer.
 */
void
color_area_widget_setup_generic (
  ColorAreaWidget * self,
  GdkRGBA * color)
{
  /* TODO */

}

/**
 * Creates a ColorAreaWidget for use inside
 * TrackWidget implementations.
 */
void
color_area_widget_setup_track (
  ColorAreaWidget * self,
  Track *           track)
{
  self->track = track;
  self->type = COLOR_AREA_TYPE_TRACK;
}

/**
 * Changes the color.
 *
 * Track types don't need to do this since the
 * color is read directly from the Track.
 */
void
color_area_widget_set_color (
  ColorAreaWidget * widget,
  GdkRGBA * color)
{
  widget->color = color;

  gtk_widget_queue_draw (GTK_WIDGET (widget));
}

static void
color_area_widget_init (ColorAreaWidget * self)
{
  g_signal_connect (
    G_OBJECT (self), "draw",
    G_CALLBACK (color_area_draw_cb), self);
}

static void
color_area_widget_class_init (
  ColorAreaWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (
    klass, "color-area");
}
