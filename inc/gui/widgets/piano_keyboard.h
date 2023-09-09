/*
 * Copyright (C) 2020-2022 Alexandros Theodotou <alex at zrythm dot org>
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
 * Piano keyboard widget.
 */

#ifndef __GUI_WIDGETS_PIANO_KEYBOARD_H__
#define __GUI_WIDGETS_PIANO_KEYBOARD_H__

#include <stdbool.h>

#include <gtk/gtk.h>

#define PIANO_KEYBOARD_WIDGET_TYPE (piano_keyboard_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  PianoKeyboardWidget,
  piano_keyboard_widget,
  Z,
  PIANO_KEYBOARD_WIDGET,
  GtkDrawingArea)

typedef struct ChordDescriptor ChordDescriptor;

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * Piano Keyboard widget.
 */
typedef struct _PianoKeyboardWidget
{
  GtkDrawingArea parent_instance;

  /** Number of visible keys (1-128). */
  int num_keys;

  /** 0-127. */
  int start_key;

  int pressed_keys[128];
  int num_pressed_keys;

  /** When true, start key can change and when false
   * the keys are fixed. */
  bool scrollable;

  /** Whether clicking on keys plays them. */
  bool playable;

  /** Whether clicking on keys "enables" them. */
  bool editable;

  /** Horizontal/vertical. */
  GtkOrientation orientation;

  /** Chord index, if this widget is for
   * a chord key. */
  int chord_idx;

  bool for_chord;

} PianoKeyboardWidget;

/**
 * Creates a piano keyboard widget.
 */
PianoKeyboardWidget *
piano_keyboard_widget_new (GtkOrientation orientation);

void
piano_keyboard_widget_refresh (PianoKeyboardWidget * self);

/**
 * Creates a piano keyboard widget.
 */
PianoKeyboardWidget *
piano_keyboard_widget_new_for_chord_key (const int chord_idx);

/**
 * @}
 */

#endif
