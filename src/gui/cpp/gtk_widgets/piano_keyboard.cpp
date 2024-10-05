// SPDX-FileCopyrightText: © 2020-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cmath>

#include "common/dsp/chord_descriptor.h"
#include "common/utils/ui.h"
#include "gui/cpp/backend/chord_editor.h"
#include "gui/cpp/backend/clip_editor.h"
#include "gui/cpp/backend/piano_roll.h"
#include "gui/cpp/backend/project.h"
#include "gui/cpp/backend/zrythm.h"
#include "gui/cpp/gtk_widgets/piano_keyboard.h"
#include "gui/cpp/gtk_widgets/zrythm_app.h"

G_DEFINE_TYPE (PianoKeyboardWidget, piano_keyboard_widget, GTK_TYPE_DRAWING_AREA)

/**
 * Draws an orange circle if the note is enabled.
 */
static void
draw_orange_circle (
  PianoKeyboardWidget * self,
  cairo_t *             cr,
  double                key_width,
  double                cur_offset,
  int                   i)
{
  double height = (double) gtk_widget_get_height (GTK_WIDGET (self));
  if (self->for_chord)
    {
      auto &descr = CHORD_EDITOR->chords_[self->chord_idx];
      if (descr.notes_[self->start_key + i])
        {
          double circle_radius = key_width / 3.0;
          bool   is_black = PIANO_ROLL->is_key_black (self->start_key + i);
          auto   color = UI_COLORS->dark_orange.to_gdk_rgba ();
          gdk_cairo_set_source_rgba (cr, &color);
          cairo_set_source_rgba (cr, 1, 0, 0, 1);
          cairo_arc (
            cr, cur_offset + key_width / 2.0,
            is_black ? height / 3.0 : height / 1.2, circle_radius, 0, 2 * M_PI);
          cairo_fill (cr);
        }
    }
}

static void
piano_keyboard_draw_cb (
  GtkDrawingArea * drawing_area,
  cairo_t *        cr,
  int              width,
  int              height,
  gpointer         user_data)
{
  PianoKeyboardWidget * self = Z_PIANO_KEYBOARD_WIDGET (user_data);

  int num_white_keys = 0;
  for (int i = 0; i < self->num_keys; i++)
    {
      if (!PIANO_ROLL->is_key_black (self->start_key + i))
        num_white_keys++;
    }

  /* draw all white keys */
  double key_width = (double) width / (double) num_white_keys;
  double cur_offset = 0.0;
  for (int i = 0; i < self->num_keys; i++)
    {
      bool is_black = PIANO_ROLL->is_key_black (self->start_key + i);
      if (is_black)
        continue;

      cairo_set_source_rgba (cr, 0, 0, 0, 1);
      cairo_rectangle (cr, cur_offset, 0, key_width, height);
      cairo_stroke_preserve (cr);
      cairo_set_source_rgba (cr, 1, 1, 1, 1);
      cairo_fill (cr);

      /* draw orange circle if part of chord */
      draw_orange_circle (self, cr, key_width, cur_offset, i);

      cur_offset += key_width;
    }

  /* draw all black keys */
  /*int num_black_keys = self->num_keys - num_white_keys;*/
  cur_offset = 0.0;
  for (int i = 0; i < self->num_keys; i++)
    {
      bool is_black = PIANO_ROLL->is_key_black (self->start_key + i);
      if (!is_black)
        {
          bool is_next_black =
            PIANO_ROLL->is_next_key_black (self->start_key + i);

          if (is_next_black)
            cur_offset += key_width / 2.0;
          else
            cur_offset += key_width;

          continue;
        }

      cairo_set_source_rgba (cr, 0, 0, 0, 1);
      cairo_rectangle (cr, cur_offset, 0, key_width, (double) height / 1.4);
      cairo_fill (cr);

      /* draw orange circle if part of chord */
      draw_orange_circle (self, cr, key_width, cur_offset, i);

      cur_offset += key_width / 2.0;
    }
}

void
piano_keyboard_widget_refresh (PianoKeyboardWidget * self)
{
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

/**
 * Creates a piano keyboard widget.
 */
PianoKeyboardWidget *
piano_keyboard_widget_new_for_chord_key (const int chord_idx)
{
  PianoKeyboardWidget * self =
    piano_keyboard_widget_new (GTK_ORIENTATION_HORIZONTAL);

  self->chord_idx = chord_idx;
  self->for_chord = true;
  self->editable = true;
  self->playable = false;
  self->scrollable = false;
  self->start_key = 0;
  self->num_keys = 48;

  return self;
}

/**
 * Creates a piano keyboard widget.
 */
PianoKeyboardWidget *
piano_keyboard_widget_new (GtkOrientation orientation)
{
  PianoKeyboardWidget * self = static_cast<PianoKeyboardWidget *> (
    g_object_new (PIANO_KEYBOARD_WIDGET_TYPE, nullptr));

  gtk_drawing_area_set_draw_func (
    GTK_DRAWING_AREA (self), piano_keyboard_draw_cb, self, nullptr);

  return self;
}

static void
piano_keyboard_widget_class_init (PianoKeyboardWidgetClass * _klass)
{
}

static void
piano_keyboard_widget_init (PianoKeyboardWidget * self)
{
  gtk_widget_set_visible (GTK_WIDGET (self), true);

  self->editable = true;
  self->playable = false;
  self->scrollable = false;
  self->start_key = 0;
  self->num_keys = 36;
}
