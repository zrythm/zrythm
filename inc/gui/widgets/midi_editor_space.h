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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Piano roll widget.
 */

#ifndef __GUI_WIDGETS_MIDI_EDITOR_SPACE_H__
#define __GUI_WIDGETS_MIDI_EDITOR_SPACE_H__

#include <gtk/gtk.h>

#define MIDI_EDITOR_SPACE_WIDGET_TYPE \
  (midi_editor_space_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  MidiEditorSpaceWidget,
  midi_editor_space_widget,
  Z, MIDI_EDITOR_SPACE_WIDGET,
  GtkBox)

typedef struct _MidiArrangerWidget MidiArrangerWidget;
typedef struct _MidiModifierArrangerWidget
  MidiModifierArrangerWidget;
typedef struct _PianoRollKeyLabelWidget
  PianoRollKeyLabelWidget;
typedef struct _PianoRollKeyWidget
  PianoRollKeyWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_MIDI_EDITOR_SPACE \
  MW_CLIP_EDITOR_INNER->midi_editor_space

/**
 * The piano roll widget is the whole space inside
 * the clip editor tab when a MIDI region is selected.
 */
typedef struct _MidiEditorSpaceWidget
{
  GtkBox               parent_instance;

  GtkPaned *           midi_arranger_velocity_paned;

  GtkScrolledWindow *  piano_roll_keys_scroll;
  GtkViewport *        piano_roll_keys_viewport;

  GtkBox *             midi_notes_box;

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

  GtkBox *             midi_vel_chooser_box;
  GtkComboBoxText *    midi_modifier_chooser;

  /**
   * Note pressed.
   *
   * Used for note presses (see MidiEditorSpaceKeyWidget).
   */
  int                  note_pressed;

  /**
   * Note released.
   *
   * Used for note presses (see MidiEditorSpaceKeyWidget).
   */
  int                  note_released;

  /** Pixel height of each key, determined by the
   * zoom level. */
  int                  px_per_key;

  /** Pixel height of all keys combined. */
  int                  total_key_px;
} MidiEditorSpaceWidget;

void
midi_editor_space_widget_setup (
  MidiEditorSpaceWidget * self);

/**
 * See CLIP_EDITOR_INNER_WIDGET_ADD_TO_SIZEGROUP.
 */
void
midi_editor_space_widget_update_size_group (
  MidiEditorSpaceWidget * self,
  int                     visible);

/**
 * Refresh the labels only (for highlighting).
 *
 * @param hard_refresh Removes and radds the labels,
 *   otherwise just calls refresh on them.
 */
void
midi_editor_space_widget_refresh_labels (
  MidiEditorSpaceWidget * self,
  int               hard_refresh);

void
midi_editor_space_widget_refresh (
  MidiEditorSpaceWidget * self);

/**
 * @}
 */

#endif
