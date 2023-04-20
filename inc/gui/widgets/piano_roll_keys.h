// SPDX-FileCopyrightText: © 2019-2021, 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

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
   * Used for note presses.
   */
  int note_pressed;

  /**
   * Note released.
   *
   * Used for note presses.
   */
  int note_released;

  /** Pixel height of each key, determined by the
   * zoom level. */
  double px_per_key;

  /** Pixel height of all keys combined. */
  double total_key_px;

  GtkGestureClick * click;

  /** Cache layout for drawing the name. */
  PangoLayout * layout;

  /** Popover to be reused for context menus. */
  GtkPopoverMenu * popover_menu;
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
