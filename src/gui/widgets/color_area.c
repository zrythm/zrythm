/*
 * Copyright (C) 2018-2021 Alexandros Theodotou <alex at zrythm dot org>
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
static void
color_area_draw_cb (
  GtkDrawingArea * drawing_area,
  cairo_t *        cr,
  int              width,
  int              height,
  gpointer         user_data)
{
  ColorAreaWidget * self =
    Z_COLOR_AREA_WIDGET (user_data);
  GtkWidget * widget = GTK_WIDGET (drawing_area);

  if (self->redraw)
    {
      GtkStyleContext * context =
        gtk_widget_get_style_context (widget);

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
        {
          Track * track = self->track;
          if (track_is_enabled (track))
            {
              color = self->track->color;
            }
          else
            {
              color.red = 0.5;
              color.green = 0.5;
              color.blue = 0.5;
            }
        }
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
          Track * track = self->track;

          /* draw each parent */
          GPtrArray * parents =
            g_ptr_array_sized_new (8);
          track_add_folder_parents (
            track, parents, false);

          size_t len = parents->len + 1;
          for (size_t i = 0; i < parents->len; i++)
            {
              Track * parent_track =
                g_ptr_array_index (parents, i);

              double start_y =
                ((double) i /
                 (double) len) *
                (double) height;
              double h =
                (double) height /
                (double) len;

              cairo_rectangle (
                self->cached_cr,
                0, start_y, width, h);
              color = parent_track->color;
              if (self->hovered)
                color_brighten_default (&color);
              gdk_cairo_set_source_rgba (
                self->cached_cr, &color);
              cairo_fill (self->cached_cr);
            }
          g_ptr_array_unref (parents);

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
}

static void
multipress_pressed (
  GtkGestureClick * gesture,
  gint                   n_press,
  gdouble                x,
  gdouble                y,
  ColorAreaWidget *      self)
{
  if (n_press == 1 && self->track)
    {
      object_color_chooser_dialog_widget_run (
        GTK_WINDOW (MAIN_WINDOW),
        self->track, NULL, NULL);
    }
}

static void
color_area_widget_on_resize (
  GtkDrawingArea * drawing_area,
  gint             width,
  gint             height,
  gpointer         user_data)
{
  ColorAreaWidget * self =
    Z_COLOR_AREA_WIDGET (user_data);
  self->redraw = true;
}

static void
on_enter (
  GtkEventControllerMotion * motion_controller,
  gdouble                    x,
  gdouble                    y,
  gpointer                   user_data)
{
  ColorAreaWidget * self =
    Z_COLOR_AREA_WIDGET (user_data);
  self->hovered = true;
  gtk_widget_queue_draw (GTK_WIDGET (self));
  self->redraw = true;
}

static void
on_leave (
  GtkEventControllerMotion * motion_controller,
  gpointer                   user_data)
{
  ColorAreaWidget * self =
    Z_COLOR_AREA_WIDGET (user_data);
  self->hovered = false;
  gtk_widget_queue_draw (GTK_WIDGET (self));
  self->redraw = true;
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
  self->color = *color;
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
  gtk_drawing_area_set_draw_func (
    GTK_DRAWING_AREA (self), color_area_draw_cb,
    self, NULL);

  GtkGestureClick * mp =
    GTK_GESTURE_CLICK (
      gtk_gesture_click_new ());
  g_signal_connect (
    G_OBJECT (mp), "pressed",
    G_CALLBACK (multipress_pressed), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self),
    GTK_EVENT_CONTROLLER (mp));

  g_signal_connect (
    G_OBJECT (self), "resize",
    G_CALLBACK (color_area_widget_on_resize),
    self);

  GtkEventControllerMotion * motion_controller =
    GTK_EVENT_CONTROLLER_MOTION (
      gtk_event_controller_motion_new ());
  g_signal_connect (
    G_OBJECT (motion_controller), "enter",
    G_CALLBACK (on_enter),  self);
  g_signal_connect (
    G_OBJECT (motion_controller), "leave",
    G_CALLBACK (on_leave),  self);
  gtk_widget_add_controller (
    GTK_WIDGET (self),
    GTK_EVENT_CONTROLLER (motion_controller));

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
