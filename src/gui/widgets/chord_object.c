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

#include "audio/chord_object.h"
#include "audio/chord_track.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/arranger_object.h"
#include "gui/widgets/chord_object.h"
#include "gui/widgets/chord_selector_window.h"
#include "project.h"
#include "utils/cairo.h"
#include "utils/ui.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  ChordObjectWidget,
  chord_object_widget,
  GTK_TYPE_BOX)

static gboolean
chord_draw_cb (
  GtkWidget * widget,
  cairo_t *   cr,
  ChordObjectWidget * self)
{
  if (self->redraw)
    {
      GtkStyleContext *context =
        gtk_widget_get_style_context (widget);

      int width =
        gtk_widget_get_allocated_width (widget);
      int height =
        gtk_widget_get_allocated_height (widget);

      self->cached_surface =
        cairo_surface_create_similar (
          cairo_get_target (cr),
          CAIRO_CONTENT_COLOR_ALPHA,
          width, height);
      self->cached_cr =
        cairo_create (self->cached_surface);

      gtk_render_background (
        context, self->cached_cr, 0, 0, width, height);

      /* get color */
      GdkRGBA color = P_CHORD_TRACK->color;
      ChordObject * chord_object =
        self->chord_object;
      ui_get_arranger_object_color (
        &color,
        gtk_widget_get_state_flags (
          GTK_WIDGET (self)) &
          GTK_STATE_FLAG_PRELIGHT,
        chord_object_is_selected (
          self->chord_object),
        chord_object_is_transient (
          self->chord_object));
      ChordDescriptor * descr =
        CHORD_EDITOR->chords[chord_object->index];
      gdk_cairo_set_source_rgba (
        self->cached_cr, &color);
      cairo_rectangle (
        self->cached_cr, 0, 0,
        width - CHORD_OBJECT_WIDGET_TRIANGLE_W,
        height);
      cairo_fill(self->cached_cr);

      cairo_move_to (
        self->cached_cr,
        width - CHORD_OBJECT_WIDGET_TRIANGLE_W, 0);
      cairo_line_to (
        self->cached_cr, width, height);
      cairo_line_to (
        self->cached_cr,
        width - CHORD_OBJECT_WIDGET_TRIANGLE_W,
        height);
      cairo_line_to (
        self->cached_cr,
        width - CHORD_OBJECT_WIDGET_TRIANGLE_W, 0);
      cairo_close_path (self->cached_cr);
      cairo_fill(self->cached_cr);

      char * str =
        chord_descriptor_to_string (descr);
      if (DEBUGGING &&
          chord_object_is_transient (
            chord_object))
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
        self->cached_cr, &c2);
      PangoLayout * layout =
        z_cairo_create_default_pango_layout (
          widget);
      z_cairo_draw_text (
        self->cached_cr, widget, layout, str);
      g_object_unref (layout);
      g_free (str);

      self->redraw = 0;
    }

  cairo_set_source_surface (
    cr, self->cached_surface, 0, 0);
  cairo_paint (cr);

 return FALSE;
}

static void
on_press (
  GtkGestureMultiPress *gesture,
  gint                  n_press,
  gdouble               x,
  gdouble               y,
  ChordObjectWidget *   self)
{
  if (n_press == 2)
    {
      ChordSelectorWindowWidget * chord_selector =
        chord_selector_window_widget_new (
          chord_object_get_chord_descriptor (
            self->chord_object));

      gtk_window_present (
        GTK_WINDOW (chord_selector));
    }
}

void
chord_object_widget_force_redraw (
  ChordObjectWidget * self)
{
  self->redraw = 1;
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

static gboolean
on_motion (
  GtkWidget *      widget,
  GdkEventMotion * event,
  ChordObjectWidget * self)
{
  if (event->type == GDK_ENTER_NOTIFY)
    {
      gtk_widget_set_state_flags (
        GTK_WIDGET (self),
        GTK_STATE_FLAG_PRELIGHT,
        0);
    }
  else if (event->type == GDK_LEAVE_NOTIFY)
    {
      gtk_widget_unset_state_flags (
        GTK_WIDGET (self),
        GTK_STATE_FLAG_PRELIGHT);
    }
  chord_object_widget_force_redraw (self);

  return FALSE;
}

ChordObjectWidget *
chord_object_widget_new (ChordObject * chord)
{
  ChordObjectWidget * self =
    g_object_new (CHORD_OBJECT_WIDGET_TYPE, NULL);

  self->chord_object = chord;

  return self;
}

static void
chord_object_widget_class_init (
  ChordObjectWidgetClass * _klass)
{
  /*GtkWidgetClass * klass =*/
    /*GTK_WIDGET_CLASS (_klass);*/
}

static void
chord_object_widget_init (
  ChordObjectWidget * self)
{
  self->drawing_area =
    GTK_DRAWING_AREA (gtk_drawing_area_new ());
  gtk_widget_set_visible (
    GTK_WIDGET (self->drawing_area), 1);
  gtk_widget_set_hexpand (
    GTK_WIDGET (self->drawing_area), 1);
  gtk_container_add (
    GTK_CONTAINER (self),
    GTK_WIDGET (self->drawing_area));

  /* GDK_ALL_EVENTS_MASK is needed, otherwise the
   * grab gets broken */
  gtk_widget_add_events (
    GTK_WIDGET (self->drawing_area),
    GDK_ALL_EVENTS_MASK);

  self->mp =
    GTK_GESTURE_MULTI_PRESS (
      gtk_gesture_multi_press_new (
        GTK_WIDGET (self->drawing_area)));

  g_signal_connect (
    G_OBJECT (self->drawing_area), "draw",
    G_CALLBACK (chord_draw_cb), self);
  g_signal_connect (
    G_OBJECT (self->drawing_area),
    "enter-notify-event",
    G_CALLBACK (on_motion),  self);
  g_signal_connect (
    G_OBJECT(self->drawing_area),
    "leave-notify-event",
    G_CALLBACK (on_motion),  self);
  g_signal_connect (
    G_OBJECT(self->drawing_area),
    "motion-notify-event",
    G_CALLBACK (on_motion),  self);
  g_signal_connect (
    G_OBJECT (self->mp), "pressed",
    G_CALLBACK (on_press), self);

  self->redraw = 1;

  g_object_ref (self);
}
