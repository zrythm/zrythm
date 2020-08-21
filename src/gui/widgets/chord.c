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

#include "audio/chord_descriptor.h"
#include "audio/chord_track.h"
#include "audio/midi_event.h"
#include "gui/backend/chord_editor.h"
#include "gui/backend/clip_editor.h"
#include "gui/widgets/chord.h"
#include "gui/widgets/chord_selector_window.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/resources.h"

G_DEFINE_TYPE (
  ChordWidget, chord_widget, GTK_TYPE_OVERLAY)

static ChordDescriptor *
get_chord_descriptor (
  ChordWidget * self)
{
  ChordDescriptor * descr =
    CHORD_EDITOR->chords[self->idx];
  g_return_val_if_fail (descr, NULL);

  return descr;
}

static bool
on_chord_pressed (
  GtkWidget *   widget,
  GdkEvent *    event,
  ChordWidget * self)
{
  Track * track = TRACKLIST_SELECTIONS->tracks[0];

  if (track && track->in_signal_type == TYPE_EVENT)
    {
      ChordDescriptor * descr =
        get_chord_descriptor (self);

      /* send note ons at time 0 */
      Port * port = track->processor->midi_in;
      midi_events_add_note_ons_from_chord_descr (
        port->midi_events, descr, 1,
        VELOCITY_DEFAULT, 0, F_QUEUED);
    }

  return FALSE;
}

static bool
on_chord_released (
  GtkWidget *   widget,
  GdkEvent *    event,
  ChordWidget * self)
{
  Track * track = TRACKLIST_SELECTIONS->tracks[0];

  if (track && track->in_signal_type == TYPE_EVENT)
    {
      ChordDescriptor * descr =
        get_chord_descriptor (self);

      /* send note offs at time 1 */
      Port * port = track->processor->midi_in;
      midi_events_add_note_offs_from_chord_descr (
        port->midi_events, descr, 1, 1, F_QUEUED);
    }

  return FALSE;
}

static void
on_edit_chord_pressed (
  GtkButton *   btn,
  ChordWidget * self)
{
  ChordDescriptor * descr =
    get_chord_descriptor (self);
  ChordSelectorWindowWidget * chord_selector =
    chord_selector_window_widget_new (descr);

  gtk_window_present (
    GTK_WINDOW (chord_selector));
}

/**
 * Sets the chord index on the chord widget.
 */
void
chord_widget_setup (
  ChordWidget * self,
  int           idx)
{
  self->idx = idx;

  z_gtk_container_destroy_all_children (
    GTK_CONTAINER (self));

  /* add main button */
  ChordDescriptor * descr =
    get_chord_descriptor (self);
  g_return_if_fail (descr);
  char * lbl =
    chord_descriptor_to_new_string (descr);
  self->btn =
    GTK_BUTTON (gtk_button_new_with_label (lbl));
  gtk_widget_set_visible (
    GTK_WIDGET (self->btn), true);
  g_free (lbl);
  gtk_container_add (
    GTK_CONTAINER (self), GTK_WIDGET (self->btn));
  g_signal_connect (
    self->btn, "button-press-event",
    G_CALLBACK (on_chord_pressed), self);
  g_signal_connect (
    self->btn, "button-release-event",
    G_CALLBACK (on_chord_released), self);

  /* add edit chord btn */
  self->edit_chord_btn =
    z_gtk_button_new_with_icon ("minuet-scales");
  gtk_widget_set_visible (
    GTK_WIDGET (self->edit_chord_btn), true);
  gtk_overlay_add_overlay (
    GTK_OVERLAY (self),
    GTK_WIDGET (self->edit_chord_btn));
  g_signal_connect (
    self->edit_chord_btn, "pressed",
    G_CALLBACK (on_edit_chord_pressed), self);
  gtk_widget_set_halign (
    GTK_WIDGET (self->edit_chord_btn),
    GTK_ALIGN_END);
  gtk_widget_set_valign (
    GTK_WIDGET (self->edit_chord_btn),
    GTK_ALIGN_START);
}

/**
 * Creates a chord widget for the given chord index.
 */
ChordWidget *
chord_widget_new (void)
{
  ChordWidget * self =
    g_object_new (CHORD_WIDGET_TYPE, NULL);

  gtk_widget_set_visible (GTK_WIDGET (self), true);
  gtk_widget_set_hexpand (GTK_WIDGET (self), true);
  gtk_widget_set_vexpand (GTK_WIDGET (self), true);

  return self;
}

static void
chord_widget_init (
  ChordWidget * self)
{
}

static void
chord_widget_class_init (
  ChordWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (klass, "chord");
}
