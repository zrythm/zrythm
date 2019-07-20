/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/chord_descriptor.h"
#include "audio/chord_object.h"
#include "audio/chord_track.h"
#include "audio/midi.h"
#include "gui/backend/clip_editor.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/chord_selector_window.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_editor_space.h"
#include "gui/widgets/chord_key.h"
#include "project.h"
#include "utils/cairo.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (ChordKeyWidget,
               chord_key_widget,
               GTK_TYPE_DRAWING_AREA)

static gboolean
chord_key_draw_cb (
  GtkWidget * widget,
  cairo_t *cr,
  ChordKeyWidget * self)
{
  guint width, height;
  GtkStyleContext *context;

  context =
    gtk_widget_get_style_context (widget);

  width =
    gtk_widget_get_allocated_width (widget);
  height =
    gtk_widget_get_allocated_height (widget);

  gtk_render_background (
    context, cr, 0, 0, width, height);

  char * str =
    chord_descriptor_to_string (self->descr);
  cairo_set_source_rgba (
    cr, 1,1,1,1);
  z_cairo_draw_text (cr, str);
  g_free (str);

 return FALSE;
}

static void
on_pressed (
  GtkGestureMultiPress *gesture,
  gint                  n_press,
  gdouble               x,
  gdouble               y,
  ChordKeyWidget *  self)
{
  if (n_press == 2)
    {
      ChordSelectorWindowWidget * chord_selector =
        chord_selector_window_widget_new (
          self->descr);

      gtk_window_present (
        GTK_WINDOW (chord_selector));
    }
}

/**
 * Creates a ChordKeyWidget for the given
 * MIDI note descriptor.
 */
ChordKeyWidget *
chord_key_widget_new (
  ChordDescriptor * descr)
{
  ChordKeyWidget * self =
    g_object_new (CHORD_KEY_WIDGET_TYPE,
                  NULL);

  self->descr = descr;

  gtk_widget_set_visible (
    GTK_WIDGET (self), 1);

  gtk_widget_set_size_request (
    GTK_WIDGET (self), 98,
    MW_MIDI_EDITOR_SPACE->px_per_key);

  return self;
}

static void
chord_key_widget_class_init (
  ChordKeyWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (
    klass, "chord-key");
}

static void
chord_key_widget_init (
  ChordKeyWidget * self)
{
  /* make it able to notify */
  gtk_widget_add_events (
    GTK_WIDGET (self),
    GDK_ALL_EVENTS_MASK);

  self->multipress =
    GTK_GESTURE_MULTI_PRESS (
      gtk_gesture_multi_press_new (
        GTK_WIDGET (self)));

  g_signal_connect (
    G_OBJECT (self), "draw",
    G_CALLBACK (chord_key_draw_cb), self);
  g_signal_connect (
    G_OBJECT(self->multipress), "pressed",
    G_CALLBACK (on_pressed),  self);
}

