/*
 * SPDX-FileCopyrightText: © 2020-2022 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

/**
 * \file
 *
 * Piano keyboard widget.
 */

#ifndef __GUI_WIDGETS_PIANO_KEYBOARD_H__
#define __GUI_WIDGETS_PIANO_KEYBOARD_H__

#include "gtk_wrapper.h"

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
