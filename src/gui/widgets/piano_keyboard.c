/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
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

#include "gui/backend/piano_roll.h"
#include "gui/widgets/piano_keyboard.h"

G_DEFINE_TYPE (
  PianoKeyboardWidget, piano_keyboard_widget,
  GTK_TYPE_DRAWING_AREA)

static gboolean
piano_keyboard_draw_cb (
  GtkWidget *           widget,
  cairo_t *             cr,
  PianoKeyboardWidget * self)
{
  GtkStyleContext *context =
    gtk_widget_get_style_context (widget);

  int width =
    gtk_widget_get_allocated_width (widget);
  int height =
    gtk_widget_get_allocated_height (widget);

  gtk_render_background (
    context, cr, 0, 0, width, height);

  int num_white_keys = 0;
  for (int i = 0; i < self->num_keys; i++)
    {
      if (!piano_roll_is_key_black (
            self->start_key + i))
        num_white_keys++;
    }

  /* draw all white keys */
  double key_width =
    (double) width / (double) num_white_keys;
  double cur_offset = 0.0;
  for (int i = 0; i < self->num_keys; i++)
    {
      bool is_black =
        piano_roll_is_key_black (self->start_key + i);
      if (is_black)
        continue;

      cairo_set_source_rgba (cr, 0, 0, 0, 1);
      cairo_rectangle (
        cr, cur_offset, 0, key_width, height);
      cairo_stroke_preserve (cr);
      cairo_set_source_rgba (cr, 1, 1, 1, 1);
      cairo_fill (cr);

      cur_offset += key_width;
    }

  /* draw all black keys */
  /*int num_black_keys = self->num_keys - num_white_keys;*/
  cur_offset = 0.0;
  for (int i = 0; i < self->num_keys; i++)
    {
      bool is_black =
        piano_roll_is_key_black (self->start_key + i);
      if (!is_black)
        {
          bool is_next_black =
            piano_roll_is_next_key_black (
              self->start_key + i);

          if (is_next_black)
            cur_offset += key_width / 2.0;
          else
            cur_offset += key_width;

          continue;
        }

      cairo_set_source_rgba (cr, 0, 0, 0, 1);
      cairo_rectangle (
        cr, cur_offset, 0, key_width, height / 1.4);
      cairo_fill (cr);

      cur_offset += key_width / 2.0;
    }

 return FALSE;
}

/**
 * Creates a piano keyboard widget.
 */
PianoKeyboardWidget *
piano_keyboard_widget_new (
  GtkOrientation orientation)
{
  PianoKeyboardWidget * self =
    g_object_new (PIANO_KEYBOARD_WIDGET_TYPE, NULL);

  g_signal_connect (
    G_OBJECT(self), "draw",
    G_CALLBACK (piano_keyboard_draw_cb),  self);

  return self;
}

static void
piano_keyboard_widget_class_init (
  PianoKeyboardWidgetClass * _klass)
{
}

static void
piano_keyboard_widget_init (
  PianoKeyboardWidget * self)
{
  gtk_widget_set_visible (GTK_WIDGET (self), true);

  self->editable = true;
  self->playable = false;
  self->scrollable = false;
  self->start_key = 0;
  self->num_keys = 36;
}
