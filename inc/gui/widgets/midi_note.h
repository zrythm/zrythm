/*
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

/** \file */

#ifndef __GUI_WIDGETS_MIDI_NOTE_H__
#define __GUI_WIDGETS_MIDI_NOTE_H__

#include <audio/midi_note.h>

#include <gtk/gtk.h>

#define MIDI_NOTE_WIDGET_TYPE                  (midi_note_widget_get_type ())
#define MIDI_NOTE_WIDGET(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MIDI_NOTE_WIDGET_TYPE, MidiNoteWidget))
#define MIDI_NOTE_WIDGET_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST  ((klass), MIDI_NOTE_WIDGET, MidiNoteWidgetClass))
#define IS_MIDI_NOTE_WIDGET(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MIDI_NOTE_WIDGET_TYPE))
#define IS_MIDI_NOTE_WIDGET_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE  ((klass), MIDI_NOTE_WIDGET_TYPE))
#define MIDI_NOTE_WIDGET_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS  ((obj), MIDI_NOTE_WIDGET_TYPE, MidiNoteWidgetClass))

typedef enum MidiNoteHoverState
{
  MIDI_NOTE_HOVER_STATE_NONE,
  MIDI_NOTE_HOVER_STATE_EDGE_L,
  MIDI_NOTE_HOVER_STATE_EDGE_R,
  MIDI_NOTE_HOVER_STATE_MIDDLE
} MidiNoteHoverState;

typedef struct MidiNoteWidget
{
  GtkBox                   parent_instance;
  MidiNote                   * midi_note;   ///< the midi_note associated with this
  MidiNoteHoverState         hover_state;
} MidiNoteWidget;

typedef struct MidiNoteWidgetClass
{
  GtkBoxClass       parent_class;
} MidiNoteWidgetClass;

/**
 * Creates a midi_note.
 */
MidiNoteWidget *
midi_note_widget_new (MidiNote * midi_note);

GType midi_note_widget_get_type(void);

#endif


