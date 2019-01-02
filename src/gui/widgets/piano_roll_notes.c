/*
 * gui/widgets/midi_note_notes.c- The midi
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

#include "audio/engine.h"
#include "audio/midi.h"
#include "audio/port.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/piano_roll.h"
#include "gui/widgets/piano_roll_labels.h"
#include "gui/widgets/piano_roll_notes.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (PianoRollNotesWidget,
               piano_roll_notes_widget,
               GTK_TYPE_DRAWING_AREA)

#define LABELS_WIDGET PIANO_ROLL->piano_roll_labels

/* 1 = black */
static int notes[12] = {
    0,
    1,
    0,
    1,
    0,
    0,
    1,
    0,
    1,
    0,
    1,
    0 };

static gboolean
draw_cb (PianoRollNotesWidget * self, cairo_t *cr, gpointer data)
{
  guint width, height;
  GtkStyleContext *context;

  context = gtk_widget_get_style_context (GTK_WIDGET (self));

  width = gtk_widget_get_allocated_width (GTK_WIDGET (self));
  height = gtk_widget_get_allocated_height (GTK_WIDGET (self));

  gtk_render_background (context, cr, 0, 0, LABELS_WIDGET->total_px, height);

  /* draw note notes with bot lines */
  for (int i = 0; i < 128; i++)
    {
      int top_line_px = LABELS_WIDGET->total_px - LABELS_WIDGET->px_per_note * (i + 1);
      int black_note = notes [i % 12] == 1;

      if (black_note)
        {
          cairo_set_source_rgb (cr, 0, 0, 0);
        }
      else
        {
          cairo_set_source_rgb (cr, 1, 1, 1);
        }
      cairo_rectangle (cr, 0, top_line_px,
                       width, LABELS_WIDGET->px_per_note);
      cairo_fill (cr);

      /* add shade if currently pressed note */
      if (i == self->note)
        {
          if (black_note)
            cairo_set_source_rgba (cr, 1, 1, 1, 0.1);
          else
            cairo_set_source_rgba (cr, 0, 0, 0, 0.3);
          cairo_rectangle (cr, 0, top_line_px,
                           width, LABELS_WIDGET->px_per_note);
          cairo_fill (cr);
        }
    }

  /* draw dividing lines */
  for (int i = 0; i < 128; i++)
    {
      int bot_line_px = LABELS_WIDGET->total_px - LABELS_WIDGET->px_per_note * i;
      cairo_set_source_rgba (cr, 0, 0, 0, 0.8);
      cairo_set_line_width (cr, 1.0);
      cairo_move_to (cr, 0, bot_line_px);
      cairo_line_to (cr, width, bot_line_px);
      cairo_stroke (cr);
    }

 return FALSE;
}

static void
multipress_pressed (GtkGestureMultiPress *gesture,
               gint                  n_press,
               gdouble               x,
               gdouble               y,
               gpointer              user_data)
{
}

static void
drag_begin (GtkGestureDrag * gesture,
               gdouble         start_x,
               gdouble         start_y,
               gpointer        user_data)
{
  PianoRollNotesWidget * self = (PianoRollNotesWidget *) user_data;

  /* add midi event to engine midi_editor_manual_press port */

  jack_midi_event_t * ev = &MANUAL_PRESS_QUEUE->jack_midi_events[0];
  ev->time = 0;
  ev->size = 3;
  start_y = PIANO_ROLL->piano_roll_labels->total_px - start_y;
  self->note = start_y / PIANO_ROLL->piano_roll_labels->px_per_note;
  int vel = 90;
  if (!ev->buffer)
    ev->buffer = calloc (3, sizeof (jack_midi_data_t));
  ev->buffer[0] = MIDI_CH1_NOTE_ON; /* status byte */
  ev->buffer[1] = self->note; /* note number 0-127 */
  ev->buffer[2] = vel; /* velocity 0-127 */
  MANUAL_PRESS_QUEUE->num_events = 1;

  self->start_y = start_y;
  gtk_widget_queue_draw (GTK_WIDGET (self));

}

static void
drag_update (GtkGestureDrag * gesture,
               gdouble         offset_x,
               gdouble         offset_y,
               gpointer        user_data)
{
  PianoRollNotesWidget * self = (PianoRollNotesWidget *) user_data;

  int prev_note = self->note;
  self->note = (self->start_y - offset_y) / PIANO_ROLL->piano_roll_labels->px_per_note;
  int vel = 90;

  /* if note changed */
  if (prev_note != self->note)
    {
      jack_midi_event_t * ev;

      /* add note on event for new note */
      ev = &MANUAL_PRESS_QUEUE->jack_midi_events[MANUAL_PRESS_QUEUE->num_events];
      ev->time = 0;
      ev->size = 3;
      if (!ev->buffer)
        ev->buffer = calloc (3, sizeof (jack_midi_data_t));
      ev->buffer[0] = MIDI_CH1_NOTE_ON; /* status byte */
      ev->buffer[1] = self->note; /* note number 0-127 */
      ev->buffer[2] = vel; /* velocity 0-127 */
      MANUAL_PRESS_QUEUE->num_events++;

      /* add note off event for prev note */
      ev = &MANUAL_PRESS_QUEUE->jack_midi_events[MANUAL_PRESS_QUEUE->num_events];
      ev->time = 0;
      ev->size = 3;
      if (!ev->buffer)
        ev->buffer = calloc (3, sizeof (jack_midi_data_t));
      ev->buffer[0] = MIDI_CH1_NOTE_OFF; /* status byte */
      ev->buffer[1] = prev_note; /* note number 0-127 */
      ev->buffer[2] = vel; /* velocity 0-127 */
      MANUAL_PRESS_QUEUE->num_events++;
    }

  gtk_widget_queue_draw (GTK_WIDGET (self));

}

static void
drag_end (GtkGestureDrag *gesture,
               gdouble         offset_x,
               gdouble         offset_y,
               gpointer        user_data)
{
  PianoRollNotesWidget * self = (PianoRollNotesWidget *) user_data;
  jack_midi_event_t * ev;

  /* add note off event for prev note */
  ev = &MANUAL_PRESS_QUEUE->jack_midi_events[MANUAL_PRESS_QUEUE->num_events];
  ev->time = 0;
  ev->size = 3;
  if (!ev->buffer)
    ev->buffer = calloc (3, sizeof (jack_midi_data_t));
  ev->buffer[0] = MIDI_CH1_NOTE_OFF; /* status byte */
  ev->buffer[1] = self->note; /* note number 0-127 */
  ev->buffer[2] = 0; /* velocity 0-127 */
  MANUAL_PRESS_QUEUE->num_events++;

  self->start_y = 0;
  self->note = -1;

  gtk_widget_queue_draw (GTK_WIDGET (self));
}

static void
piano_roll_notes_widget_class_init (PianoRollNotesWidgetClass * klass)
{
}

static void
piano_roll_notes_widget_init (PianoRollNotesWidget * self)
{
  /* make it able to notify */
  gtk_widget_add_events (GTK_WIDGET (self),
                         GDK_ALL_EVENTS_MASK);

  self->drag = GTK_GESTURE_DRAG (
                gtk_gesture_drag_new (GTK_WIDGET (self)));
  self->multipress = GTK_GESTURE_MULTI_PRESS (
                gtk_gesture_multi_press_new (GTK_WIDGET (self)));

  g_signal_connect (G_OBJECT (self), "draw",
                    G_CALLBACK (draw_cb), NULL);
  /*g_signal_connect (G_OBJECT(self), "button_press_event",*/
                    /*G_CALLBACK (button_press_cb),  self);*/
  g_signal_connect (G_OBJECT(self->drag), "drag-begin",
                    G_CALLBACK (drag_begin),  self);
  g_signal_connect (G_OBJECT(self->drag), "drag-update",
                    G_CALLBACK (drag_update),  self);
  g_signal_connect (G_OBJECT(self->drag), "drag-end",
                    G_CALLBACK (drag_end),  self);
  g_signal_connect (G_OBJECT (self->multipress), "pressed",
                    G_CALLBACK (multipress_pressed), self);
}
