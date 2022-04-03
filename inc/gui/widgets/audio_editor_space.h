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
 * Audio editor space.
 */

#ifndef __GUI_WIDGETS_AUDIO_EDITOR_SPACE_H__
#define __GUI_WIDGETS_AUDIO_EDITOR_SPACE_H__

#include <gtk/gtk.h>

#define AUDIO_EDITOR_SPACE_WIDGET_TYPE \
  (audio_editor_space_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  AudioEditorSpaceWidget,
  audio_editor_space_widget,
  Z,
  AUDIO_EDITOR_SPACE_WIDGET,
  GtkBox)

typedef struct _ArrangerWidget ArrangerWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_AUDIO_EDITOR_SPACE \
  MW_CLIP_EDITOR_INNER->audio_editor_space

/**
 * The piano roll widget is the whole space inside
 * the clip editor tab when a AUDIO region is selected.
 */
typedef struct _AudioEditorSpaceWidget
{
  GtkBox parent_instance;

  GtkBox *            left_box;
  GtkScrolledWindow * arranger_scroll;
  GtkViewport *       arranger_viewport;
  ArrangerWidget *    arranger;
} AudioEditorSpaceWidget;

void
audio_editor_space_widget_setup (
  AudioEditorSpaceWidget * self);

/**
 * See CLIP_EDITOR_INNER_WIDGET_ADD_TO_SIZEGROUP.
 */
void
audio_editor_space_widget_update_size_group (
  AudioEditorSpaceWidget * self,
  int                      visible);

void
audio_editor_space_widget_refresh (
  AudioEditorSpaceWidget * self);

/**
 * @}
 */

#endif
