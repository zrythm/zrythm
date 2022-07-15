/*
 * Copyright (C) 2018-2019, 2021-2022 Alexandros Theodotou <alex at zrythm dot org>
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
  Z,
  MIDI_EDITOR_SPACE_WIDGET,
  GtkWidget)

typedef struct _ArrangerWidget      ArrangerWidget;
typedef struct _PianoRollKeysWidget PianoRollKeysWidget;

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
  GtkWidget parent_instance;

  GtkPaned * midi_arranger_velocity_paned;

  GtkScrolledWindow * piano_roll_keys_scroll;
  GtkViewport *       piano_roll_keys_viewport;

  GtkBox * midi_notes_box;

  PianoRollKeysWidget * piano_roll_keys;

  /** Piano roll. */
  GtkBox *            midi_arranger_box;
  GtkScrolledWindow * arranger_scroll;
  GtkViewport *       arranger_viewport;
  ArrangerWidget *    arranger;
  GtkScrolledWindow * modifier_arranger_scroll;
  GtkViewport *       modifier_arranger_viewport;
  ArrangerWidget *    modifier_arranger;

  GtkScrollbar * arranger_hscrollbar;
  GtkScrollbar * arranger_vscrollbar;

  GtkBox *          midi_vel_chooser_box;
  GtkComboBoxText * midi_modifier_chooser;

  /** Vertical size goup for the keys and the
   * arranger. */
  GtkSizeGroup * arranger_and_keys_vsize_group;
} MidiEditorSpaceWidget;

void
midi_editor_space_widget_setup (MidiEditorSpaceWidget * self);

/**
 * See CLIP_EDITOR_INNER_WIDGET_ADD_TO_SIZEGROUP.
 */
void
midi_editor_space_widget_update_size_group (
  MidiEditorSpaceWidget * self,
  int                     visible);

void
midi_editor_space_widget_refresh (
  MidiEditorSpaceWidget * self);

/**
 * @}
 */

#endif
