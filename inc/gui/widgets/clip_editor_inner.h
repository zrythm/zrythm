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
 * Piano roll widget.
 */

#ifndef __GUI_WIDGETS_PIANO_ROLL_H__
#define __GUI_WIDGETS_PIANO_ROLL_H__

#include <gtk/gtk.h>

#define CLIP_EDITOR_INNER_WIDGET_TYPE \
  (clip_editor_inner_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ClipEditorInnerWidget,
  clip_editor_inner_widget,
  Z,
  CLIP_EDITOR_INNER_WIDGET,
  GtkBox)

typedef struct _RulerWidget     RulerWidget;
typedef struct _ColorAreaWidget ColorAreaWidget;
typedef struct _MidiEditorSpaceWidget
  MidiEditorSpaceWidget;
typedef struct _AudioEditorSpaceWidget
  AudioEditorSpaceWidget;
typedef struct _ChordEditorSpaceWidget
  ChordEditorSpaceWidget;
typedef struct _AutomationEditorSpaceWidget
  AutomationEditorSpaceWidget;
typedef struct _ArrangerWidget ArrangerWidget;
typedef struct _RotatedLabelWidget
  RotatedLabelWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_CLIP_EDITOR_INNER \
  MW_CLIP_EDITOR->clip_editor_inner

/**
 * Adds or remove the widget from the
 * "left of ruler" size group.
 */
void
clip_editor_inner_widget_add_to_left_of_ruler_sizegroup (
  ClipEditorInnerWidget * self,
  GtkWidget *             widget,
  bool                    add);

/**
 * The piano roll widget is the whole space inside
 * the clip editor tab when a MIDI region is selected.
 */
typedef struct _ClipEditorInnerWidget
{
  GtkBox parent_instance;

  ColorAreaWidget *    color_bar;
  GtkBox *             bot_of_arranger_toolbar;
  RotatedLabelWidget * track_name_rotated_label;
  GtkBox *             left_of_ruler_box;
  GtkScrolledWindow *  ruler_scroll;
  GtkViewport *        ruler_viewport;
  RulerWidget *        ruler;
  GtkStack *           editor_stack;
  GtkSizeGroup *       left_of_ruler_size_group;

  /* ==== Piano Roll (Midi Editor) ==== */

  /** Toggle between drum mode and normal mode. */
  GtkToggleButton * toggle_notation;
  GtkToggleButton * toggle_listen_notes;
  GtkToggleButton * show_automation_values;

  MidiEditorSpaceWidget * midi_editor_space;

  /* ==== End Piano Roll (Midi Editor) ==== */

  /* ==== Automation Editor ==== */

  AutomationEditorSpaceWidget *
    automation_editor_space;

  /* ==== End Automation Editor ==== */

  /* ==== Chord Editor ==== */

  ChordEditorSpaceWidget * chord_editor_space;

  /* ==== End Chord Editor ==== */

  /* ==== Audio Editor ==== */

  AudioEditorSpaceWidget * audio_editor_space;

  /* ==== End Audio Editor ==== */

  /** Size group for keeping the whole ruler and
   * each timeline the same width. */
  GtkSizeGroup * ruler_arranger_hsize_group;

} ClipEditorInnerWidget;

void
clip_editor_inner_widget_setup (
  ClipEditorInnerWidget * self);

void
clip_editor_inner_widget_refresh (
  ClipEditorInnerWidget * self);

ArrangerWidget *
clip_editor_inner_widget_get_visible_arranger (
  ClipEditorInnerWidget * self);

/**
 * @}
 */

#endif
