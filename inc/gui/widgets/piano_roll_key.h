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

/**
 * \file
 */

#ifndef __GUI_WIDGETS_PIANO_ROLL_KEY_H__
#define __GUI_WIDGETS_PIANO_ROLL_KEY_H__

#include <gtk/gtk.h>

#define PIANO_ROLL_KEY_WIDGET_TYPE \
  (piano_roll_key_widget_get_type ())
G_DECLARE_FINAL_TYPE (PianoRollKeyWidget,
                      piano_roll_key_widget,
                      Z,
                      PIANO_ROLL_KEY_WIDGET,
                      GtkDrawingArea)

/**
* Piano roll note widget to be shown on the left
* side of the piano roll (128 of these).
*/
typedef struct _PianoRollKeyWidget
{
  GtkDrawingArea         parent_instance;

  /** The note this widget is for. */
  MidiNoteDescriptor *   descr;

  GtkGestureMultiPress * multipress;
} PianoRollKeyWidget;

/**
 * Creates a PianoRollKeyWidget for the given
 * MIDI note descriptor.
 */
PianoRollKeyWidget *
piano_roll_key_widget_new (
  MidiNoteDescriptor * descr);

#endif
