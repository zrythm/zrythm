/*
 * gui/widgets/midi_note.h - MIDI note
 *
 * Copyright (C) 2019 Alexandros Theodotou
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

/** \file */

#ifndef __GUI_WIDGETS_MIDI_NOTE_H__
#define __GUI_WIDGETS_MIDI_NOTE_H__

#include <audio/midi_note.h>

#include <gtk/gtk.h>

#define MIDI_NOTE_WIDGET_TYPE \
  (midi_note_widget_get_type ())
G_DECLARE_FINAL_TYPE (MidiNoteWidget,
                      midi_note_widget,
                      Z,
                      MIDI_NOTE_WIDGET,
                      GtkBox)

typedef enum MidiNoteWidgetState
{
  MNW_STATE_NONE,
  MNW_STATE_SELECTED,
  MNW_STATE_RESIZE_L,
  MNW_STATE_RESIZE_R,
  MNW_STATE_HOVER
} MidiNoteWidgetState;

typedef struct _MidiNoteWidget
{
  GtkBox                   parent_instance;
  MidiNote *               midi_note;   ///< the midi_note associated with this
  MidiNoteWidgetState      state;
} MidiNoteWidget;

/**
 * Creates a midi_note.
 */
MidiNoteWidget *
midi_note_widget_new (MidiNote * midi_note);

/**
 * Sets hover state and queues draw.
 */
void
midi_note_widget_set_state_and_queue_draw (
  MidiNoteWidget *    self,
  MidiNoteWidgetState state);

#endif
