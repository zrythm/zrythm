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
  ARRANGER_OBJECT_WIDGET_TYPE)

static gboolean
chord_draw_cb (
  GtkWidget * widget,
  cairo_t *   cr,
  ChordObjectWidget * self)
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

      z_cairo_reset_caches (
        &ao_prv->cached_cr,
        &ao_prv->cached_surface, width,
        height, cr);

      gtk_render_background (
        context, ao_prv->cached_cr,
        0, 0, width, height);

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
        ao_prv->cached_cr, &color);
      cairo_rectangle (
        ao_prv->cached_cr, 0, 0,
        width - CHORD_OBJECT_WIDGET_TRIANGLE_W,
        height);
      cairo_fill (ao_prv->cached_cr);

      cairo_move_to (
        ao_prv->cached_cr,
        width - CHORD_OBJECT_WIDGET_TRIANGLE_W, 0);
      cairo_line_to (
        ao_prv->cached_cr, width, height);
      cairo_line_to (
        ao_prv->cached_cr,
        width - CHORD_OBJECT_WIDGET_TRIANGLE_W,
        height);
      cairo_line_to (
        ao_prv->cached_cr,
        width - CHORD_OBJECT_WIDGET_TRIANGLE_W, 0);
      cairo_close_path (ao_prv->cached_cr);
      cairo_fill (ao_prv->cached_cr);

      char str[100];
      chord_descriptor_to_string (descr, str);
      if (DEBUGGING &&
          chord_object_is_transient (
            chord_object))
        {
          strcat (str, " [t]");
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

ChordObjectWidget *
chord_object_widget_new (ChordObject * chord)
{
  ChordObjectWidget * self =
    g_object_new (CHORD_OBJECT_WIDGET_TYPE, NULL);

  arranger_object_widget_setup (
    Z_ARRANGER_OBJECT_WIDGET (self),
    (ArrangerObject *) chord);

  self->chord_object = chord;

  return self;
}

static void
finalize (
  ChordObjectWidget * self)
{
  if (self->mp)
    g_object_unref (self->mp);

  G_OBJECT_CLASS (
    chord_object_widget_parent_class)->
      finalize (G_OBJECT (self));
}

static void
chord_object_widget_class_init (
  ChordObjectWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);
  GObjectClass * oklass = G_OBJECT_CLASS (klass);
  oklass->finalize =
    (GObjectFinalizeFunc) finalize;
}

static void
chord_object_widget_init (
  ChordObjectWidget * self)
{
  ARRANGER_OBJECT_WIDGET_GET_PRIVATE (self);

  self->mp =
    GTK_GESTURE_MULTI_PRESS (
      gtk_gesture_multi_press_new (
        GTK_WIDGET (ao_prv->drawing_area)));

  g_signal_connect (
    G_OBJECT (ao_prv->drawing_area), "draw",
    G_CALLBACK (chord_draw_cb), self);
  g_signal_connect (
    G_OBJECT (self->mp), "pressed",
    G_CALLBACK (on_press), self);
}
