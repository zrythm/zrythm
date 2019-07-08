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

#ifndef __GUI_WIDGETS_AUDIO_CLIP_EDITOR_H__
#define __GUI_WIDGETS_AUDIO_CLIP_EDITOR_H__

#include <gtk/gtk.h>

#define AUDIO_CLIP_EDITOR_WIDGET_TYPE \
  (audio_clip_editor_widget_get_type ())
G_DECLARE_FINAL_TYPE (AudioClipEditorWidget,
                      audio_clip_editor_widget,
                      Z,
                      AUDIO_CLIP_EDITOR_WIDGET,
                      GtkGrid)

#define MW_AUDIO_CLIP_EDITOR \
  MW_CLIP_EDITOR->audio_clip_editor

typedef struct _AudioRulerWidget AudioRulerWidget;
typedef struct _AudioArrangerWidget
  AudioArrangerWidget;
typedef struct _ColorAreaWidget ColorAreaWidget;
typedef struct AudioClipEditor AudioClipEditor;

typedef struct _AudioClipEditorWidget
{
  GtkGrid                 parent_instance;
  GtkLabel *              track_name;
  ColorAreaWidget *       color_bar;
  GtkScrolledWindow *     ruler_scroll;
  GtkViewport *           ruler_viewport;
  AudioRulerWidget *      ruler;
  GtkScrolledWindow *     arranger_scroll;
  GtkViewport *           arranger_viewport;
  AudioArrangerWidget *   arranger;

  /** Backend. */
  AudioClipEditor *       audio_clip_editor;
} AudioClipEditorWidget;

void
audio_clip_editor_widget_setup (
  AudioClipEditorWidget * self,
  AudioClipEditor *       ace);

#endif
