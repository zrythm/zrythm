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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Piano roll widget.
 */

#ifndef __GUI_WIDGETS_PIANO_ROLL_H__
#define __GUI_WIDGETS_PIANO_ROLL_H__

#include <gtk/gtk.h>

#define PIANO_ROLL_WIDGET_TYPE \
  (piano_roll_widget_get_type ())
G_DECLARE_FINAL_TYPE (PianoRollWidget,
                      piano_roll_widget,
                      Z,
                      PIANO_ROLL_WIDGET,
                      GtkBox)

typedef struct _PianoRollKeyLabelWidget PianoRollKeyLabelWidget;
typedef struct _PianoRollKeyWidget PianoRollKeyWidget;
typedef struct _MidiArrangerWidget MidiArrangerWidget;
typedef struct _MidiRulerWidget MidiRulerWidget;
typedef struct _ColorAreaWidget ColorAreaWidget;
typedef struct _MidiModifierArrangerWidget
  MidiModifierArrangerWidget;
typedef struct PianoRoll PianoRoll;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_PIANO_ROLL MW_CLIP_EDITOR->piano_roll

/**
 * The piano roll widget is the whole space inside
 * the clip editor tab when a MIDI region is selected.
 */
typedef struct _PianoRollWidget
{
  GtkBox               parent_instance;

  ColorAreaWidget *    color_bar;
  GtkToolbar *         midi_bot_toolbar;
  GtkLabel *           midi_name_label;
  GtkBox *             midi_controls_above_notes_box;
  GtkScrolledWindow *  midi_ruler_scroll;
  GtkViewport *        midi_ruler_viewport;
  MidiRulerWidget *    ruler;
  GtkPaned *           midi_arranger_velocity_paned;
  GtkScrolledWindow *  piano_roll_keys_scroll;
  GtkViewport *        piano_roll_keys_viewport;

  /**
   * Box to add piano roll keys.
   *
   * It should contain boxes that have
   * PianoRollKeyLabelWidget on the left and
   * PianoRollKeyWidget on the right.
   *
   * In drum mode, PianoRollKeyWidget will be set to
   * invisible.
   */
  GtkBox *             piano_roll_keys_box;

  PianoRollKeyWidget * piano_roll_keys[128];
  PianoRollKeyLabelWidget * piano_roll_key_labels[128];

  /** Piano roll. */
  GtkBox *             midi_arranger_box;
  GtkScrolledWindow *  arranger_scroll;
  GtkViewport *        arranger_viewport;
  MidiArrangerWidget * arranger;
  GtkScrolledWindow *  modifier_arranger_scroll;
  GtkViewport *        modifier_arranger_viewport;
  MidiModifierArrangerWidget * modifier_arranger;
  GtkButton *          toggle_notation;

  /** Backend. */
  PianoRoll *          piano_roll;

  /**
   * Note pressed.
   *
   * Used for note presses (see PianoRollKeyWidget).
   */
  int                  note_pressed;

  /**
   * Note released.
   *
   * Used for note presses (see PianoRollKeyWidget).
   */
  int                  note_released;

  /** Pixel height of each key, determined by the
   * zoom level. */
  int                  px_per_key;

  /** Pixel height of all keys combined. */
  int                  total_key_px;
} MidiEditorWidget;

void
piano_roll_widget_setup (
  PianoRollWidget * self,
  PianoRoll *       pr);

/**
 * Refresh the labels only (for highlighting).
 *
 * @param hard_refresh Removes and radds the labels,
 *   otherwise just calls refresh on them.
 */
void
piano_roll_widget_refresh_labels (
  PianoRollWidget * self,
  int               hard_refresh);

void
piano_roll_widget_refresh (
  PianoRollWidget * self);

/**
 * @}
 */

#endif
