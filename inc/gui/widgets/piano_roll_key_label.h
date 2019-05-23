/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

/**
 * \file
 */

#ifndef __GUI_WIDGETS_PIANO_ROLL_KEY_LABEL_H__
#define __GUI_WIDGETS_PIANO_ROLL_KEY_LABEL_H__

#include <gtk/gtk.h>

#define PIANO_ROLL_KEY_LABEL_WIDGET_TYPE \
  (piano_roll_key_label_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  PianoRollKeyLabelWidget,
  piano_roll_key_label_widget,
  Z, PIANO_ROLL_KEY_LABEL_WIDGET,
  GtkStack)

typedef struct _EditableLabelWidget
  EditableLabelWidget;

/**
* Piano roll label widget to be shown next to the
* piano roll notes.
*
* In drum mode editable_lbl will be used with
* the custom name. In
* normal mode lbl will be used with the note
* name.
*/
typedef struct _PianoRollKeyLabelWidget
{
  GtkStack               parent_instance;

  /** Editable label - if drum mode. */
  EditableLabelWidget *  editable_lbl;

  /** Normal label - piano roll mode. */
  GtkLabel *             lbl;

  /** The note this widget is for. */
  MidiNoteDescriptor *   descr;
} PianoRollKeyLabelWidget;

/**
 * Refreshes the widget.
 */
void
piano_roll_key_label_widget_refresh (
  PianoRollKeyLabelWidget * self);

/**
 * Creates a PianoRollKeyLabelWidget for the given
 * MIDI note descriptor.
 */
PianoRollKeyLabelWidget *
piano_roll_key_label_widget_new (
  MidiNoteDescriptor * descr);

#endif
