/*
 * Copyright (C) 2019 Alexandros Theodotou
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

#ifndef __GUI_WIDGETS_CLIP_EDITOR_H__
#define __GUI_WIDGETS_CLIP_EDITOR_H__

#include <gtk/gtk.h>

#define CLIP_EDITOR_WIDGET_TYPE \
  (clip_editor_widget_get_type ())
G_DECLARE_FINAL_TYPE (ClipEditorWidget,
                      clip_editor_widget,
                      Z,
                      CLIP_EDITOR_WIDGET,
                      GtkStack)

#define MW_CLIP_EDITOR MW_BOT_DOCK_EDGE->clip_editor
#define MW_PIANO_ROLL_BOX \
  MW_CLIP_EDITOR->piano_roll_box

typedef struct _PianoRollWidget PianoRollWidget;
typedef struct _PianoRollSelectionInfoWidget
  PianoRollSelectionInfoWidget;
typedef struct _PianoRollToolbarWidget
  PianoRollToolbarWidget;
typedef struct _AudioClipEditorWidget
  AudioClipEditorWidget;
typedef struct ClipEditor ClipEditor;

/**
 * The ClipEditorWidget shows in the Clip Editor /
 * Piano Roll tab of the bottom panel, and is a stack
 * of PianoRollWidget for the piano roll and
 * AudioClipEditorWidget for audio regions.
 */
typedef struct _ClipEditorWidget
{
  GtkStack                 parent_instance;
  GtkBox *                 piano_roll_box;
  PianoRollSelectionInfoWidget * piano_roll_selections;
  PianoRollToolbarWidget * piano_roll_toolbar;
  PianoRollWidget *        piano_roll;
  AudioClipEditorWidget *  audio_clip_editor;
  GtkLabel *               no_selection_label;

  /** Backend. */
  ClipEditor *             clip_editor;
} ClipEditorWidget;

void
clip_editor_widget_setup (
  ClipEditorWidget * self,
  ClipEditor *       clip_editor);

#endif
