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

#ifndef __GUI_WIDGETS_PIANO_ROLL_H__
#define __GUI_WIDGETS_PIANO_ROLL_H__

#include <gtk/gtk.h>

#define CLIP_EDITOR_INNER_WIDGET_TYPE \
  (clip_editor_inner_widget_get_type ())
G_DECLARE_FINAL_TYPE (ClipEditorInnerWidget,
                      clip_editor_inner_widget,
                      Z,
                      CLIP_EDITOR_INNER_WIDGET,
                      GtkBox)

typedef struct _EditorRulerWidget EditorRulerWidget;
typedef struct _ColorAreaWidget ColorAreaWidget;
typedef struct _MidiEditorSpaceWidget
  MidiEditorSpaceWidget;
typedef struct _AudioEditorSpaceWidget
  AudioEditorSpaceWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_CLIP_EDITOR_INNER MW_CLIP_EDITOR->clip_editor_inner

/**
 * The piano roll widget is the whole space inside
 * the clip editor tab when a MIDI region is selected.
 */
typedef struct _ClipEditorInnerWidget
{
  GtkBox               parent_instance;

  ColorAreaWidget *    color_bar;
  GtkToolbar *         bot_of_arranger_toolbar;
  GtkLabel *           track_name_label;
  GtkBox *             left_of_ruler_box;
  GtkScrolledWindow *  ruler_scroll;
  GtkViewport *        ruler_viewport;
  EditorRulerWidget *  ruler;
  GtkStack *           editor_stack;
  GtkSizeGroup *       left_of_ruler_size_group;

  /* ==== Piano Roll (Midi Editor) ==== */

  /** Toggle between drum mode and normal mode. */
  GtkButton *          toggle_notation;

  MidiEditorSpaceWidget * midi_editor_space;

  /* ==== End Piano Roll (Midi Editor) ==== */

  /* ==== Automation Editor ==== */

  /* ==== End Automation Editor ==== */

  /* ==== Chord Editor ==== */

  /* ==== End Chord Editor ==== */

  /* ==== Audio Editor ==== */

  AudioEditorSpaceWidget * audio_editor_space;

  /* ==== End Audio Editor ==== */

} ClipEditorInnerWidget;

void
clip_editor_inner_widget_setup (
  ClipEditorInnerWidget * self);

void
clip_editor_inner_widget_refresh (
  ClipEditorInnerWidget * self);

/**
 * @}
 */

#endif
