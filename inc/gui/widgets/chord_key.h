/*
 * Copyright (C) 2019-2022 Alexandros Theodotou <alex at zrythm dot org>
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

#ifndef __GUI_WIDGETS_CHORD_KEY_H__
#define __GUI_WIDGETS_CHORD_KEY_H__

#include <gtk/gtk.h>

#define CHORD_KEY_WIDGET_TYPE (chord_key_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ChordKeyWidget,
  chord_key_widget,
  Z,
  CHORD_KEY_WIDGET,
  GtkGrid)

typedef struct _PianoKeyboardWidget PianoKeyboardWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
* Piano roll note widget to be shown on the left
* side of the piano roll (128 of these).
*/
typedef struct _ChordKeyWidget
{
  GtkGrid parent_instance;

  /** The chord this widget is for. */
  int chord_idx;

  GtkLabel *            chord_lbl;
  GtkBox *              piano_box;
  PianoKeyboardWidget * piano;
  GtkBox *              btn_box;
  GtkButton *           choose_chord_btn;
  GtkButton *           invert_prev_btn;
  GtkButton *           invert_next_btn;

  GtkGestureClick * multipress;
} ChordKeyWidget;

void
chord_key_widget_refresh (ChordKeyWidget * self);

/**
 * Creates a ChordKeyWidget for the given
 * MIDI note descriptor.
 */
ChordKeyWidget *
chord_key_widget_new (int idx);

/**
 * @}
 */

#endif
