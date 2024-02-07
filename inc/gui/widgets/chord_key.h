/*
 * SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

/**
 * \file
 */

#ifndef __GUI_WIDGETS_CHORD_KEY_H__
#define __GUI_WIDGETS_CHORD_KEY_H__

#include <gtk/gtk.h>

#define CHORD_KEY_WIDGET_TYPE (chord_key_widget_get_type ())
G_DECLARE_FINAL_TYPE (ChordKeyWidget, chord_key_widget, Z, CHORD_KEY_WIDGET, GtkGrid)

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
