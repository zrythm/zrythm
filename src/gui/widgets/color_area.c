/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "audio/track.h"
#include "gui/widgets/dialogs/object_color_chooser_dialog.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/track.h"
#include "gui/widgets/track_top_grid.h"
#include "utils/cairo.h"
#include "utils/color.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

G_DEFINE_TYPE (
  ColorAreaWidget, color_area_widget,
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
  if (self->redraw)
    {
      GtkStyleContext * context =
        gtk_widget_get_style_context (widget);

      int width =
        gtk_widget_get_allocated_width (widget);
      int height =
        gtk_widget_get_allocated_height (widget);

      self->cached_cr =
        cairo_create (self->cached_surface);
      z_cairo_reset_caches (
        &self->cached_cr,
        &self->cached_surface, width,
        height, cr);

      gtk_render_background (
        context, self->cached_cr, 0, 0,
        width, height);

      GdkRGBA color;
      if (self->type == COLOR_AREA_TYPE_TRACK)
        color = self->track->color;
      else
        color = self->color;

      if (self->hovered)
        {
          color_brighten_default (&color);
        }

      cairo_rectangle (
        self->cached_cr, 0, 0, width, height);
      gdk_cairo_set_source_rgba (
        self->cached_cr, &color);
      cairo_fill (self->cached_cr);

      /* show track icon */
      if (self->type == COLOR_AREA_TYPE_TRACK)
        {
          /*TRACK_WIDGET_GET_PRIVATE (*/
            /*self->track->widget);*/

          /*if (tw_prv->icon)*/
            /*{*/
              /*cairo_surface_t * surface =*/
                /*gdk_cairo_surface_create_from_pixbuf (*/
                  /*tw_prv->icon, 0, NULL);*/

              /*GdkRGBA c2, c3;*/
              /*ui_get_contrast_color (*/
                /*color, &c2);*/
              /*ui_get_contrast_color (*/
                /*&c2, &c3);*/

              /*[> add shadow in the back <]*/
              /*cairo_set_source_rgba (*/
                /*self->cached_cr, c3.red,*/
                /*c3.green, c3.blue, 0.4);*/
              /*cairo_mask_surface(*/
                /*self->cached_cr, surface, 2, 2);*/
              /*cairo_fill(self->cached_cr);*/

              /*[> add main icon <]*/
              /*cairo_set_source_rgba (*/
                /*self->cached_cr, c2.red, c2.green, c2.blue, 1);*/
              /*[>cairo_set_source_surface (<]*/
                /*[>self->cached_cr, surface, 1, 1);<]*/
              /*cairo_mask_surface(*/
                /*self->cached_cr, surface, 1, 1);*/
              /*cairo_fill (self->cached_cr);*/
            /*}*/
        }
      self->redraw = 0;
    }

  cairo_set_source_surface (
    cr, self->cached_surface, 0, 0);
  cairo_paint (cr);

  return FALSE;
}

static void
multipress_pressed (
  GtkGestureMultiPress * gesture,
  gint                   n_press,
  gdouble                x,
  gdouble                y,
  ColorAreaWidget *      self)
{
  if (n_press == 1 && self->track)
    {
      ObjectColorChooserDialogWidget * color_chooser =
        object_color_chooser_dialog_widget_new_for_track (
          self->track);
      object_color_chooser_dialog_widget_run (
        color_chooser);
    }
}

static void
on_size_allocate (
  GtkWidget    *widget,
  GdkRectangle *allocation,
  ColorAreaWidget * self)
{
  self->redraw = true;
}

static gboolean
on_motion (
  GtkWidget *       widget,
  GdkEventMotion *  event,
  ColorAreaWidget * self)
{
  if (event->type == GDK_ENTER_NOTIFY)
    {
      self->hovered = true;
      gtk_widget_queue_draw (GTK_WIDGET (self));
      self->redraw = true;
    }
  else if (event->type == GDK_LEAVE_NOTIFY)
    {
      self->hovered = false;
      gtk_widget_queue_draw (GTK_WIDGET (self));
      self->redraw = true;
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
  self->redraw = true;

  g_message (
    "setting up track %s for %p", track->name, self);
}

/**
 * Changes the color.
 *
 * Track types don't need to do this since the
 * color is read directly from the Track.
 */
void
color_area_widget_set_color (
  ColorAreaWidget * self,
  GdkRGBA * color)
{
  self->color = *color;

  gtk_widget_queue_draw (GTK_WIDGET (self));
  self->redraw = true;
}

static void
color_area_widget_init (ColorAreaWidget * self)
{
  /* make able to notify */
  gtk_widget_add_events (
    GTK_WIDGET (self),
    GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK |
    GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);


  GtkGestureMultiPress * mp =
    GTK_GESTURE_MULTI_PRESS (
      gtk_gesture_multi_press_new (
        GTK_WIDGET (self)));
  g_signal_connect (
    G_OBJECT (mp), "pressed",
    G_CALLBACK (multipress_pressed), self);
  g_signal_connect (
    G_OBJECT (self), "size-allocate",
    G_CALLBACK (on_size_allocate), self);
  g_signal_connect (
    G_OBJECT (self), "draw",
    G_CALLBACK (color_area_draw_cb), self);
  g_signal_connect (
    G_OBJECT (self), "enter-notify-event",
    G_CALLBACK (on_motion),  self);
  g_signal_connect (
    G_OBJECT (self), "leave-notify-event",
    G_CALLBACK (on_motion),  self);
  self->redraw = true;
}

static void
color_area_widget_class_init (
  ColorAreaWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (
    klass, "color-area");
}
