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

#include "audio/scale_object.h"
#include "audio/chord_track.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/scale_object.h"
#include "gui/widgets/scale_selector_window.h"
#include "project.h"
#include "utils/cairo.h"
#include "utils/ui.h"

G_DEFINE_TYPE (
  ScaleObjectWidget,
  scale_object_widget,
  ARRANGER_OBJECT_WIDGET_TYPE)

static gboolean
scale_draw_cb (
  GtkWidget * widget,
  cairo_t *   cr,
  ScaleObjectWidget * self)
{
  ARRANGER_OBJECT_WIDGET_GET_PRIVATE (self);

  if (ao_prv->redraw)
    {
      GtkStyleContext *context =
        gtk_widget_get_style_context (widget);

      int width =
        gtk_widget_get_allocated_width (widget);
      int height =
        gtk_widget_get_allocated_height (widget);

      ao_prv->cached_surface =
        cairo_surface_create_similar (
          cairo_get_target (cr),
          CAIRO_CONTENT_COLOR_ALPHA,
          width, height);
      ao_prv->cached_cr =
        cairo_create (ao_prv->cached_surface);

      gtk_render_background (
        context, ao_prv->cached_cr,
        0, 0, width, height);

      /* set color */
      GdkRGBA color = P_CHORD_TRACK->color;
      ui_get_arranger_object_color (
        &color,
        gtk_widget_get_state_flags (
          GTK_WIDGET (self)) &
          GTK_STATE_FLAG_PRELIGHT,
        scale_object_is_selected (
          self->scale_object),
        scale_object_is_transient (
          self->scale_object));
      gdk_cairo_set_source_rgba (
        ao_prv->cached_cr, &color);

      cairo_rectangle (
        ao_prv->cached_cr, 0, 0,
        width - SCALE_OBJECT_WIDGET_TRIANGLE_W,
        height);
      cairo_fill (ao_prv->cached_cr);

      cairo_move_to (
        ao_prv->cached_cr,
        width - SCALE_OBJECT_WIDGET_TRIANGLE_W, 0);
      cairo_line_to (
        ao_prv->cached_cr, width, height);
      cairo_line_to (
        ao_prv->cached_cr,
        width - SCALE_OBJECT_WIDGET_TRIANGLE_W,
        height);
      cairo_line_to (
        ao_prv->cached_cr,
        width - SCALE_OBJECT_WIDGET_TRIANGLE_W, 0);
      cairo_close_path (ao_prv->cached_cr);
      cairo_fill (ao_prv->cached_cr);

      char * str =
        musical_scale_to_string (
          self->scale_object->scale);
      if (DEBUGGING &&
          scale_object_is_transient (
            self->scale_object))
        {
          char * tmp =
            g_strdup_printf (
              "%s [t]", str);
          g_free (str);
          str = tmp;
        }

      GdkRGBA c2;
      ui_get_contrast_color (
        &color, &c2);
      gdk_cairo_set_source_rgba (
        ao_prv->cached_cr, &c2);
      PangoLayout * layout =
        z_cairo_create_default_pango_layout (
          widget);
      z_cairo_draw_text (
        ao_prv->cached_cr, widget, layout, str);
      g_object_unref (layout);
      g_free (str);
      ao_prv->redraw = 0;
    }

  cairo_set_source_surface (
    cr, ao_prv->cached_surface, 0, 0);
  cairo_paint (cr);

 return FALSE;
}

static void
on_press (
  GtkGestureMultiPress *gesture,
  gint                  n_press,
  gdouble               x,
  gdouble               y,
  ScaleObjectWidget *   self)
{
  if (n_press == 2)
    {
      ScaleSelectorWindowWidget * scale_selector =
        scale_selector_window_widget_new (
          self);

      gtk_window_present (
        GTK_WINDOW (scale_selector));
    }
}

ScaleObjectWidget *
scale_object_widget_new (ScaleObject * scale)
{
  ScaleObjectWidget * self =
    g_object_new (SCALE_OBJECT_WIDGET_TYPE, NULL);

  arranger_object_widget_setup (
    Z_ARRANGER_OBJECT_WIDGET (self),
    (ArrangerObject *) scale);

  self->scale_object = scale;

  char scale_str[100];
  musical_scale_strcpy (
    scale->scale, scale_str);
  PangoLayout * layout =
    z_cairo_create_default_pango_layout (
      (GtkWidget *) self);
  z_cairo_get_text_extents_for_widget (
    (GtkWidget *) self, layout,
    scale_str, &self->textw, &self->texth);
  g_object_unref (layout);

  return self;
}

static void
scale_object_widget_class_init (
  ScaleObjectWidgetClass * _klass)
{
  /*GtkWidgetClass * klass =*/
    /*GTK_WIDGET_CLASS (_klass);*/
}

static void
scale_object_widget_init (
  ScaleObjectWidget * self)
{
  ARRANGER_OBJECT_WIDGET_GET_PRIVATE (self);

  self->mp =
    GTK_GESTURE_MULTI_PRESS (
      gtk_gesture_multi_press_new (
        GTK_WIDGET (ao_prv->drawing_area)));

  g_signal_connect (
    G_OBJECT (ao_prv->drawing_area), "draw",
    G_CALLBACK (scale_draw_cb), self);
  g_signal_connect (
    G_OBJECT (self->mp), "pressed",
    G_CALLBACK (on_press), self);
}
