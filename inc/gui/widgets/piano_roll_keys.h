/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Piano roll keys canvas.
 */

#ifndef __GUI_WIDGETS_PIANO_ROLL_KEYS_H__
#define __GUI_WIDGETS_PIANO_ROLL_KEYS_H__

#include <gtk/gtk.h>

#define PIANO_ROLL_KEYS_WIDGET_TYPE \
  (piano_roll_keys_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  PianoRollKeysWidget,
  piano_roll_keys_widget,
  Z,
  PIANO_ROLL_KEYS_WIDGET,
  GtkWidget)

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_PIANO_ROLL_KEYS \
  MW_MIDI_EDITOR_SPACE->piano_roll_keys

/**
 * The piano roll widget is the whole space inside
 * the clip editor tab when a MIDI region is selected.
 */
typedef struct _PianoRollKeysWidget
{
  GtkWidget parent_instance;

  /** Start key pressed. */
  int start_key;

  /** Last key hovered during a press. */
  int last_key;

  /** Last hovered key. */
  int last_hovered_key;

  /**
   * Note in the middle of the arranger (0-127).
   *
   * This will be used to scroll to each refresh.
   */
  int last_mid_note;

  /**
   * Note pressed.
   *
   * Used for note presses (see
   * MidiEditorSpaceKeyWidget).
   */
  int note_pressed;

  /**
   * Note released.
   *
   * Used for note presses (see
   * MidiEditorSpaceKeyWidget).
   */
  int note_released;

  /** Pixel height of each key, determined by the
   * zoom level. */
  double px_per_key;

  /** Pixel height of all keys combined. */
  double total_key_px;

  GtkGestureClick * multipress;

  /** Cache layout for drawing the name. */
  PangoLayout * layout;
} PianoRollKeysWidget;

/**
 * Returns the appropriate font size based on the
 * current pixels (height) per key.
 */
int
piano_roll_keys_widget_get_font_size (
  PianoRollKeysWidget * self);

void
piano_roll_keys_widget_refresh (PianoRollKeysWidget * self);

void
piano_roll_keys_widget_redraw_note (
  PianoRollKeysWidget * self,
  int                   note);

void
piano_roll_keys_widget_redraw_full (
  PianoRollKeysWidget * self);

void
piano_roll_keys_widget_setup (PianoRollKeysWidget * self);

int
piano_roll_keys_widget_get_key_from_y (
  PianoRollKeysWidget * self,
  double                y);

/**
 * @}
 */

#endif
