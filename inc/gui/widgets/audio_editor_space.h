// SPDX-FileCopyrightText: © 2019, 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

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

  GtkBox *         left_box;
  ArrangerWidget * arranger;
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
