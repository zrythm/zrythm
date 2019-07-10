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

/**
 * \file
 *
 * MidiNote widget API.
 */

#ifndef __GUI_WIDGETS_MIDI_NOTE_H__
#define __GUI_WIDGETS_MIDI_NOTE_H__

#include "audio/midi_note.h"
#include "gui/widgets/arranger_object.h"
#include "utils/ui.h"

#include <gtk/gtk.h>

#define MIDI_NOTE_WIDGET_TYPE \
  (midi_note_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  MidiNoteWidget,
  midi_note_widget,
  Z, MIDI_NOTE_WIDGET,
  GtkBox)

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * GUI widget for a MidiNote.
 */
typedef struct _MidiNoteWidget
{
  GtkBox                   parent_instance;
  GtkDrawingArea *         drawing_area;
  MidiNote *               midi_note; ///< the midi_note associated with this
  GtkWindow *            tooltip_win;
  GtkLabel *             tooltip_label;

  /** If cursor is at resizing L. */
  int                      resize_l;

  /** If cursor is at resizing R. */
  int                      resize_r;
} MidiNoteWidget;

/**
 * Creates a midi_note.
 */
MidiNoteWidget *
midi_note_widget_new (MidiNote * midi_note);

/**
 * Destroys the widget completely.
 */
void
midi_note_widget_destroy (MidiNoteWidget *self);

DECLARE_ARRANGER_OBJECT_WIDGET_SELECT (
  MidiNote, midi_note);

void
midi_note_widget_update_tooltip (
  MidiNoteWidget * self,
  int              show);

/**
 * Returns if the current position is for resizing
 * L.
 */
int
midi_note_widget_is_resize_l (
  MidiNoteWidget * self,
  int              x);

/**
 * Returns if the current position is for resizing
 * L.
 */
int
midi_note_widget_is_resize_r (
  MidiNoteWidget * self,
  int              x);

void
midi_note_widget_update_tooltip (
  MidiNoteWidget * self,
  int              show);

/**
 * @}
 */

#endif
